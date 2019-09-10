#pragma once
#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "common/miniLogger/miniLogger.h"
#include "common/PromiseTaskSTD.hpp"
#include "common/IntToString.hpp"
#include <atomic>
#include <future>
#include <thread>
#include <condition_variable>
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

template<typename T>
class AsyncTaskResult : public PromiseResultSTD<T>
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
    template<typename> friend struct TaskNode;
private:
    uint64_t ElapseTime = 0;
public:
    AsyncTaskResult(std::promise<T>& pms) : PromiseResultSTD<T>(pms) { }
    ~AsyncTaskResult() override { }
    uint64_t ElapseNs() override { return ElapseTime; }
};

enum class AsyncTaskStatus : uint8_t
{
    New = 0, Ready = 1, Yield = 128, Wait = 129, Error = 250, Finished = 251
};

struct ASYEXEAPI AsyncTaskNodeData
{
    boost::context::continuation Context;
};

struct ASYEXEAPI AsyncTaskNode 
    : public AsyncTaskNodeData, 
      public common::container::IntrusiveDoubleLinkListNodeBase<AsyncTaskNode>
{
    std::u16string Name;
    PmsCore Promise = nullptr; // current waiting promise
    common::SimpleTimer TaskTimer; // execution timer
    uint64_t ElapseTime = 0; // execution time
    uint32_t StackSize;
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    virtual ~AsyncTaskNode() {}
    virtual void operator()(const AsyncAgent& agent) = 0;
    virtual void OnException(std::exception_ptr e) = 0;
protected:
    AsyncTaskNode(const std::u16string name, uint32_t stackSize) : Name(name), 
        StackSize(stackSize == 0 ? static_cast<uint32_t>(boost::context::fixedsize_stack::traits_type::default_size()) : stackSize) { }
};

template<typename RetType>
struct TaskNode : public AsyncTaskNode
{
    std::function<RetType(const AsyncAgent&)> Func;
    std::promise<RetType> InnerPms;
    std::shared_ptr<AsyncTaskResult<RetType>> OutterPms;
    template<typename F>
    TaskNode(const std::u16string name, uint32_t stackSize, F&& func) : AsyncTaskNode(name, stackSize),
        Func(std::forward<F>(func)), InnerPms(), OutterPms(std::make_shared<AsyncTaskResult<RetType>>(InnerPms))
    { }
    virtual ~TaskNode() override {}
    void FinishTask(const detail::AsyncTaskStatus status)
    {
        TaskTimer.Stop();
        ElapseTime += TaskTimer.ElapseNs();
        OutterPms->ElapseTime = ElapseTime;
        Status = status;
    }
    virtual void operator()(const AsyncAgent& agent) override
    {
        try
        {
            Status = detail::AsyncTaskStatus::Ready;
            TaskTimer.Start();
            if constexpr (std::is_same_v<RetType, void>)
            {
                Func(agent);
                FinishTask(detail::AsyncTaskStatus::Finished);
                InnerPms.set_value();
            }
            else
            {
                auto ret = Func(agent);
                FinishTask(detail::AsyncTaskStatus::Finished);
                InnerPms.set_value(std::move(ret));
            }
        }
        catch (boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
        {
            std::rethrow_exception(std::current_exception());
        }
        catch (...)
        {
            FinishTask(detail::AsyncTaskStatus::Error);
            InnerPms.set_exception(std::current_exception());
        }
    }
    virtual void OnException(std::exception_ptr e) override { InnerPms.set_exception(e); }
};

}



class ASYEXEAPI AsyncManager : public NonCopyable, public NonMovable
{
    friend class AsyncAgent;
private:
    common::container::IntrusiveDoubleLinkList<detail::AsyncTaskNode> TaskList;
    //std::atomic_flag ModifyFlag = ATOMIC_FLAG_INIT; //spinlock for modify TaskNode
    std::atomic_bool ShouldRun { false };
    std::atomic_uint32_t TaskUid { 0 };
    std::mutex RunningMtx, TerminateMtx;
    std::condition_variable CondWait;
    boost::context::continuation Context;
    detail::AsyncTaskNode *Current = nullptr;
    const std::u16string Name;
    const AsyncAgent Agent;
    common::mlog::MiniLogger<false> Logger;
    std::thread RunningThread;
    uint32_t TimeYieldSleep, TimeSensitive;
    bool AllowStopAdd;

    bool AddNode(detail::AsyncTaskNode* node);

    void Resume();
    void MainLoop();
    void OnTerminate(const std::function<void(void)>& exiter = {}); //run at worker thread
public:
    AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20, const bool allowStopAdd = false);
    ~AsyncManager();
    bool Start(const std::function<void(void)>& initer = {}, const std::function<void(void)>& exiter = {});
    void Stop();
    bool Run(const std::function<void(std::function<void(void)>)>& initer = {});

    template<typename Func, typename Ret = std::invoke_result_t<Func, const AsyncAgent&>>
    PromiseResult<Ret> AddTask(Func&& task, std::u16string taskname = u"", uint32_t stackSize = 0)
    {
        static_assert(std::is_invocable_v<Func, const AsyncAgent&>, "Unsupported Task Func Type");
        const auto tuid = TaskUid.fetch_add(1, std::memory_order_relaxed);
        if (taskname == u"")
            taskname = u"task" + common::str::to_u16string(tuid);
        auto node = new detail::TaskNode<Ret>(taskname, stackSize, std::forward<Func>(task));
        const auto ret = std::static_pointer_cast<common::detail::PromiseResult_<Ret>>(node->OutterPms);
        if (AddNode(node))
        {
            Logger.debug(FMT_STRING(u"Add new task [{}] [{}]\n"), tuid, node->Name);
            return ret;
        }
        else //has stopped
        {
            Logger.warning(u"New task cancelled due to termination [{}] [{}]\n", tuid, node->Name);
            delete node;
            COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, u"Executor was terminated when adding task.");
        }
    }
};


}
}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif