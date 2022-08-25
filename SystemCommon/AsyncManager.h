#pragma once
#include "SystemCommonRely.h"
#include "AsyncAgent.h"
#include "Exceptions.h"
#include "LoopBase.h"
#include "MiniLogger.h"
#include "PromiseTask.h"
#include "common/IntrusiveDoubleLinkList.hpp"
#include "common/TimeUtil.hpp"
#include <atomic>

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common
{
namespace asyexe
{

struct StackSize
{
    constexpr static uint32_t Default = 0, Tiny = 4096, Small = 65536, Big = 512 * 1024, Large = 1024 * 1024, Huge = 4 * 1024 * 1024;
};

class SYSCOMMONAPI AsyncTaskException : public BaseException
{
public:
    enum class Reasons : uint8_t { Terminated, Timeout, Cancelled };
private:
    COMMON_EXCEPTION_PREPARE(AsyncTaskException, BaseException,
        const Reasons Reason;
        ExceptionInfo(const std::u16string_view msg, const Reasons reason)
            : TPInfo(TYPENAME, msg), Reason(reason)
        { }
    );
    AsyncTaskException(const Reasons reason, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, reason)
    { }
};

namespace detail
{

class SYSCOMMONAPI AsyncTaskPromiseProvider : public common::BasicPromiseProvider
{
    friend struct AsyncTaskNodeBase;
protected:
    uint64_t ElapseTime = 0;
public:
    ~AsyncTaskPromiseProvider() override;
    [[nodiscard]] uint64_t ElapseNs() noexcept override;
    [[nodiscard]] uint64_t E2EElapseNs() noexcept;
};

template<typename T>
class AsyncTaskResult : public ::common::detail::BasicResult_<T, AsyncTaskPromiseProvider>
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
public:
    //forceinline AsyncTaskPromiseProvider& GetAsyncProvider() noexcept { return this->Promise; }
    AsyncTaskResult() { }
    ~AsyncTaskResult() override { }
};


enum class AsyncTaskStatus : uint8_t
{
    New = 0, Ready = 1, Yield = 128, Wait = 129, Error = 250, Finished = 251
};


struct SYSCOMMONAPI AsyncTaskNodeBase : public common::container::IntrusiveDoubleLinkListNodeBase<AsyncTaskNodeBase>
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
private:
    void* const PtrContext;
    std::u16string Name;
    ::common::PmsCore Promise; // current waiting promise
    static void ReleaseNode(AsyncTaskNodeBase* node);
    virtual void OnExecute(const AsyncAgent& agent) = 0;
    virtual void OnException(std::exception_ptr e) noexcept = 0;
protected:
    common::SimpleTimer TaskTimer; // execution timer
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    static std::pair<void*, std::byte*> CreateSpace(size_t align, size_t size) noexcept;
    AsyncTaskNodeBase(void* ptrCtx, std::u16string& name, uint32_t tuid, uint32_t stackSize) noexcept;
    void Execute(const AsyncAgent& agent);
    void SumPartialTime() noexcept;
    void FinishTask(const AsyncTaskStatus status, AsyncTaskPromiseProvider& taskTime) noexcept;
private:
    uint64_t ElapseTime = 0; // execution time
    const uint32_t TaskUid;
    const uint32_t StackSize;
public:
    virtual ~AsyncTaskNodeBase();
};

template<typename RetType, bool AcceptAgent>
struct AsyncTaskNode : public AsyncTaskNodeBase
{
    friend class ::common::asyexe::AsyncManager;
private:
    using FuncType = std::conditional_t<AcceptAgent, std::function<RetType(const AsyncAgent&)>, std::function<RetType()>>;
    FuncType Func;
    BasicPromise<RetType, AsyncTaskResult<RetType>> InnerPms;
    template<typename F>
    AsyncTaskNode(void* ptrCtx, std::u16string& name, uint32_t tuid, uint32_t stackSize, F&& func) :
        AsyncTaskNodeBase(ptrCtx, name, tuid, stackSize), Func(std::forward<F>(func))
    { }
    forceinline AsyncTaskPromiseProvider& GetAsyncProvider() noexcept { return InnerPms.GetRawPromise()->GetPromise(); }
    forceinline RetType InnerCall([[maybe_unused]] const AsyncAgent& agent)
    {
        if constexpr (AcceptAgent)
            return Func(agent);
        else
            return Func();
    }
    virtual void OnExecute([[maybe_unused]]const AsyncAgent& agent) override
    {
        if constexpr (std::is_same_v<RetType, void>)
        {
            InnerCall(agent);
            FinishTask(detail::AsyncTaskStatus::Finished, GetAsyncProvider());
            InnerPms.SetData();
        }
        else
        {
            auto ret = InnerCall(agent);
            FinishTask(detail::AsyncTaskStatus::Finished, GetAsyncProvider());
            InnerPms.SetData(std::move(ret));
        }
    }
    virtual void OnException(std::exception_ptr e) noexcept override
    { 
        FinishTask(detail::AsyncTaskStatus::Error, GetAsyncProvider());
        InnerPms.SetException(e);
    }
    template<typename F>
    static AsyncTaskNode* Create(std::u16string& name, uint32_t tuid, uint32_t stackSize, F&& func)
    {
        const auto [base, ptr] = AsyncTaskNodeBase::CreateSpace(alignof(AsyncTaskNode), sizeof(AsyncTaskNode));
        return new (ptr)AsyncTaskNode(base, name, tuid, stackSize, std::forward<F>(func));
    }
public:
    virtual ~AsyncTaskNode() override {}
};


template<typename F, bool AcceptAgent>
struct RetTypeGetter;
template<typename F>
struct RetTypeGetter<F, true>
{
    using Type = std::invoke_result_t<F, const AsyncAgent&>;
};
template<typename F>
struct RetTypeGetter<F, false>
{
    using Type = std::invoke_result_t<F>;
};

}



class AsyncManager final : private common::loop::LoopBase
{
    friend class AsyncAgent;
private:
    using Injector = std::function<void(void)>;
    struct FiberContext;
    common::container::IntrusiveDoubleLinkList<detail::AsyncTaskNodeBase> TaskList;
    std::unique_ptr<FiberContext> Context;
    Injector ExitCallback = nullptr;
    detail::AsyncTaskNodeBase*Current = nullptr;
    const std::u16string Name;
    const AsyncAgent Agent;
    common::mlog::MiniLogger<false> Logger;
    uint32_t TimeYieldSleep, TimeSensitive;
    std::atomic_uint32_t TaskUid{ 0 };
    bool AllowStopAdd;

    SYSCOMMONAPI uint32_t PreCheckTask(std::u16string& taskName);
    SYSCOMMONAPI bool AddNode(detail::AsyncTaskNodeBase* node);

    void Resume(detail::AsyncTaskStatus status);
    LoopAction OnLoop() override;
    bool OnStart(std::any& cookie) noexcept override;
    void OnStop() noexcept override;
    bool SleepCheck() noexcept override; // double check if shoul sleep
public:
    SYSCOMMONAPI AsyncManager(const bool isthreaded, const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20, const bool allowStopAdd = false);
    SYSCOMMONAPI AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20, const bool allowStopAdd = false);
    SYSCOMMONAPI ~AsyncManager() override;
    SYSCOMMONAPI bool Start(Injector initer = {}, Injector exiter = {});
    SYSCOMMONAPI bool Stop();
    SYSCOMMONAPI bool RequestStop();
    SYSCOMMONAPI common::loop::LoopExecutor& GetHost();

    template<typename Func>
    forceinline auto AddTask(Func&& task, std::u16string taskName = u"", uint32_t stackSize = 0)
    {
        constexpr bool AcceptAgent = std::is_invocable_v<Func, const AsyncAgent&>;
        static_assert(AcceptAgent || std::is_invocable_v<Func>, "Unsupported Task Func Type");
        using Ret = typename detail::RetTypeGetter<Func, AcceptAgent>::Type;
        const auto tuid = PreCheckTask(taskName);
        const auto node = detail::AsyncTaskNode<Ret, AcceptAgent>::Create(taskName, tuid, stackSize, std::forward<Func>(task));
        AddNode(node);
        return node->InnerPms.GetPromiseResult();
    }
};


}
}


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif