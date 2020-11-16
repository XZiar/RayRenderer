#pragma once
#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include <atomic>
#define BOOST_CONTEXT_STATIC_LINK 1
#define BOOST_CONTEXT_NO_LIB 1
#include <boost/context/continuation.hpp>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common
{
namespace asyexe
{

namespace detail
{

class ASYEXEAPI AsyncTaskPromiseProvider : public common::BasicPromiseProvider
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


struct ASYEXEAPI AsyncTaskNodeBase : public common::container::IntrusiveDoubleLinkListNodeBase<AsyncTaskNodeBase>
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
private:
    boost::context::continuation Context;
    std::u16string Name;
    ::common::PmsCore Promise = nullptr; // current waiting promise
    virtual void operator()(const AsyncAgent& agent) = 0;
    virtual void OnException(std::exception_ptr e) = 0;
protected:
    common::SimpleTimer TaskTimer; // execution timer
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    AsyncTaskNodeBase(const std::u16string name, uint32_t tuid, uint32_t stackSize);
    void BeginTask() noexcept;
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
    AsyncTaskNode(const std::u16string name, uint32_t tuid, uint32_t stackSize, F&& func) :
        AsyncTaskNodeBase(name, tuid, stackSize), Func(std::forward<F>(func))
    { }
    forceinline AsyncTaskPromiseProvider& GetAsyncProvider() noexcept { return InnerPms.GetRawPromise()->GetPromise(); }
    forceinline RetType InnerCall([[maybe_unused]] const AsyncAgent& agent)
    {
        if constexpr (AcceptAgent)
            return Func(agent);
        else
            return Func();
    }
    virtual void operator()([[maybe_unused]]const AsyncAgent& agent) override
    {
        BeginTask();
        try
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
        catch (const boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
        {
            std::rethrow_exception(std::current_exception());
        }
        catch (...)
        {
            OnException(std::current_exception());
        }
    }
    virtual void OnException(std::exception_ptr e) override 
    { 
        FinishTask(detail::AsyncTaskStatus::Error, GetAsyncProvider());
        InnerPms.SetException(e);
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



class ASYEXEAPI AsyncManager final : private common::loop::LoopBase
{
    friend class AsyncAgent;
private:
    using Injector = std::function<void(void)>;
    common::container::IntrusiveDoubleLinkList<detail::AsyncTaskNodeBase> TaskList;
    boost::context::continuation Context;
    Injector ExitCallback = nullptr;
    detail::AsyncTaskNodeBase*Current = nullptr;
    const std::u16string Name;
    const AsyncAgent Agent;
    common::mlog::MiniLogger<false> Logger;
    uint32_t TimeYieldSleep, TimeSensitive;
    std::atomic_uint32_t TaskUid{ 0 };
    bool AllowStopAdd;

    uint32_t PreCheckTask(std::u16string& taskName);
    bool AddNode(detail::AsyncTaskNodeBase* node);

    void Resume(detail::AsyncTaskStatus status);
    virtual LoopAction OnLoop() override;
    virtual bool OnStart(std::any cookie) noexcept override;
    virtual void OnStop() noexcept override;
    virtual bool SleepCheck() noexcept override; // double check if shoul sleep
public:
    AsyncManager(const bool isthreaded, const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20, const bool allowStopAdd = false);
    AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20, const bool allowStopAdd = false)
        : AsyncManager(true, name, timeYieldSleep, timeSensitive, allowStopAdd) { }
    virtual ~AsyncManager() override;
    bool Start(Injector initer = {}, Injector exiter = {});
    bool Stop();
    bool RequestStop();
    common::loop::LoopExecutor& GetHost();

    template<typename Func>
    forceinline auto AddTask(Func&& task, std::u16string taskName = u"", uint32_t stackSize = 0)
    {
        constexpr bool AcceptAgent = std::is_invocable_v<Func, const AsyncAgent&>;
        static_assert(AcceptAgent || std::is_invocable_v<Func>, "Unsupported Task Func Type");
        using Ret = typename detail::RetTypeGetter<Func, AcceptAgent>::Type;
        const auto tuid = PreCheckTask(taskName);
        const auto node = new detail::AsyncTaskNode<Ret, AcceptAgent>(taskName, tuid, stackSize, std::forward<Func>(task));
        AddNode(node);
        return node->InnerPms.GetPromiseResult();
    }
};


}
}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif