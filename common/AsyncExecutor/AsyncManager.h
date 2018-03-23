#pragma once
#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "common/miniLogger/miniLogger.h"
#include "common/ThreadEx.h"
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
enum class AsyncTaskStatus : uint8_t
{
    New = 0, Ready = 1, Yield = 128, Wait = 129, Error = 250, Finished = 251
};
struct AsyncTaskNode
{
    const std::u16string Name;
    boost::context::continuation Context;
    AsyncTaskNode *Prev = nullptr, *Next = nullptr;//spin-locker's memory_order_seq_cst promise their order
    AsyncTaskFunc Func;
    PmsCore Promise = nullptr;
    std::promise<void> Pms;
    AsyncTaskStatus Status = AsyncTaskStatus::New;
    AsyncTaskNode(const std::u16string& name) : Name(name) { }
};
}


class ASYEXEAPI AsyncManager
{
    friend class AsyncAgent;
public:
//private:
    ThreadObject RunningThread;
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