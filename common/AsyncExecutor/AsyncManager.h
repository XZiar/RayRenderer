#pragma once
#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "common/miniLogger/miniLogger.h"
#include "common/ThreadEx.h"
#include "common/TimeUtil.hpp"
#include "common/PromiseTaskSTD.hpp"
#include <atomic>
#include <future>
#include <condition_variable>
#define BOOST_CONTEXT_STATIC_LINK 1
#define BOOST_CONTEXT_NO_LIB 1
#include <boost/context/continuation.hpp>

namespace common
{
namespace asyexe
{

namespace detail
{
struct AsyncTaskNode;

class AsyncTaskResult : public PromiseResultSTD<void>
{
    friend class ::common::asyexe::AsyncManager;
    friend class ::common::asyexe::AsyncAgent;
private:
    uint64_t ElapseTime = 0;
public:
    AsyncTaskResult(std::promise<void>& pms) : PromiseResultSTD<void>(pms) { }
    ~AsyncTaskResult() override { }
    uint64_t ElapseNs() override { return ElapseTime; }
};

enum class AsyncTaskStatus : uint8_t
{
    New = 0, Ready = 1, Yield = 128, Wait = 129, Error = 250, Finished = 251
};
struct AsyncTaskNode
{
    const std::u16string Name;
    AsyncTaskFunc Func;
    boost::context::continuation Context;
    AsyncTaskNode *Prev = nullptr, *Next = nullptr;//spin-locker's memory_order_seq_cst promise their order
    std::promise<void> Pms;
    std::shared_ptr<AsyncTaskResult> ResPms;
    PmsCore Promise = nullptr;
    common::SimpleTimer TaskTimer;
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    AsyncTaskNode(const std::u16string& name) : Name(name) { }
};
}


class ASYEXEAPI AsyncManager : public NonCopyable, public NonMovable
{
    friend class AsyncAgent;
private:
    static void CallWrapper(detail::AsyncTaskNode* node, const AsyncAgent& agent);
    std::atomic_flag ModifyFlag = ATOMIC_FLAG_INIT; //spinlock for modify TaskNode OR ShouldRun
    std::atomic_bool ShouldRun = false;
    std::atomic_uint32_t TaskUid = 0;
    std::mutex RunningMtx, TerminateMtx;
    std::condition_variable CondInner, CondOuter;
    boost::context::continuation Context;
    std::atomic<detail::AsyncTaskNode*> Head = nullptr, Tail = nullptr;
    detail::AsyncTaskNode *Current = nullptr;
    uint32_t TimeYieldSleep, TimeSensitive;
    const std::u16string Name;
    const AsyncAgent Agent;
    common::mlog::MiniLogger<false> Logger;
    ThreadObject RunningThread;

    bool AddNode(detail::AsyncTaskNode* node);
    detail::AsyncTaskNode* DelNode(detail::AsyncTaskNode* node);//only called from self thread
    void Resume();
public:
    AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20);
    ~AsyncManager();
    PromiseResult<void> AddTask(const AsyncTaskFunc& task, std::u16string taskname = u"");
    void MainLoop(const std::function<void(void)>& initer = {}, const std::function<void(void)>& exiter = {});
    void Terminate();
    void OnTerminate(const std::function<void(void)>& exiter = {});
};


}
}