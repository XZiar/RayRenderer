#pragma once
#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "common/PromiseTaskSTD.hpp"
#include "common/IntToString.hpp"
#include <atomic>
#define BOOST_CONTEXT_STATIC_LINK 1
#define BOOST_CONTEXT_NO_LIB 1
#include <boost/context/continuation.hpp>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace common
{
namespace asyexe
{

namespace detail
{

struct AsyncTaskTime
{
    friend struct AsyncTaskNodeBase;
protected:
    uint64_t ElapseTime = 0;
};

template<typename T>
class AsyncTaskResult : public PromiseResultSTD<T>, public AsyncTaskTime
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
public:
    AsyncTaskResult(std::promise<T>& pms) : PromiseResultSTD<T>(pms) { }
    virtual ~AsyncTaskResult() override { }
    virtual uint64_t ElapseNs() override { return ElapseTime; }
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
    PmsCore Promise = nullptr; // current waiting promise
    virtual void operator()(const AsyncAgent& agent) = 0;
    virtual void OnException(std::exception_ptr e) = 0;
protected:
    common::SimpleTimer TaskTimer; // execution timer
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    AsyncTaskNodeBase(const std::u16string name, uint32_t stackSize) : Name(name),
        StackSize(stackSize == 0 ? static_cast<uint32_t>(boost::context::fixedsize_stack::traits_type::default_size()) : stackSize) { }
    void BeginTask();
    void SumPartialTime();
    void FinishTask(const detail::AsyncTaskStatus status, AsyncTaskTime& taskTime);
private:
    uint64_t ElapseTime = 0; // execution time
    const uint32_t StackSize;
public:
    virtual ~AsyncTaskNodeBase() {}
};

template<typename RetType>
struct AsyncTaskNode : public AsyncTaskNodeBase
{
    friend class ::common::asyexe::AsyncManager;
private:
    std::function<RetType(const AsyncAgent&)> Func;
    std::promise<RetType> InnerPms;
    std::shared_ptr<AsyncTaskResult<RetType>> OutterPms;
    template<typename F>
    AsyncTaskNode(const std::u16string name, uint32_t stackSize, F&& func) : AsyncTaskNodeBase(name, stackSize),
        Func(std::forward<F>(func)), InnerPms(), OutterPms(std::make_shared<AsyncTaskResult<RetType>>(InnerPms))
    { }
    virtual void operator()(const AsyncAgent& agent) override
    {
        BeginTask();
        try
        {
            if constexpr (std::is_same_v<RetType, void>)
            {
                Func(agent);
                FinishTask(detail::AsyncTaskStatus::Finished, *OutterPms);
                InnerPms.set_value();
            }
            else
            {
                auto ret = Func(agent);
                FinishTask(detail::AsyncTaskStatus::Finished, *OutterPms);
                InnerPms.set_value(std::move(ret));
            }
        }
        catch (boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
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
        FinishTask(detail::AsyncTaskStatus::Error, *OutterPms);
        InnerPms.set_exception(e);
    }
public:
    virtual ~AsyncTaskNode() override {}
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

    bool AddNode(detail::AsyncTaskNodeBase* node);

    void Resume(detail::AsyncTaskStatus status);
    virtual LoopState OnLoop() override;
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

    template<typename Func, typename Ret = std::invoke_result_t<Func, const AsyncAgent&>>
    PromiseResult<Ret> AddTask(Func&& task, std::u16string taskname = u"", uint32_t stackSize = 0)
    {
        static_assert(std::is_invocable_v<Func, const AsyncAgent&>, "Unsupported Task Func Type");
        const auto tuid = TaskUid.fetch_add(1, std::memory_order_relaxed);
        if (taskname == u"")
            taskname = u"task" + common::str::to_u16string(tuid);
        if (!AllowStopAdd && !IsRunning()) //has stopped
        {
            Logger.warning(u"New task cancelled due to termination [{}] [{}]\n", tuid, taskname);
            COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, u"Executor was terminated when adding task.");
        }
        auto node = new detail::AsyncTaskNode<Ret>(taskname, stackSize, std::forward<Func>(task));
        AddNode(node);
        Logger.debug(FMT_STRING(u"Add new task [{}] [{}]\n"), tuid, node->Name);
        return node->OutterPms;
    }
};


}
}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif