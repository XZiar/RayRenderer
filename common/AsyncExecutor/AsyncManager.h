#pragma once
#include "AsyncExecutorRely.h"
#include <atomic>
#include <future>
#include <condition_variable>
#define BOOST_CONTEXT_STATIC_LINK 1
#define BOOST_CONTEXT_NO_LIB 1
#include <boost/context/all.hpp>

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
    boost::context::continuation Context;
    AsyncTaskNode *Prev = nullptr, *Next = nullptr;//spin-locker's memory_order_seq_cst promise their order
    AsyncTaskFunc Func;
    PmsCore Promise = nullptr;
    std::promise<void> Pms;
    AsyncTaskStatus Status = AsyncTaskStatus::New;
};
}


class AsyncTaskException : public BaseException
{
public:
    EXCEPTION_CLONE_EX(AsyncTaskException);
    enum class Reason : uint8_t { Terminated, Timeout, Cancelled };
    const Reason reason;
    AsyncTaskException(const Reason reason_, const std::wstring& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data), reason(reason)
    { }
    virtual ~AsyncTaskException() {}
};


class ASYEXEAPI AsyncManager
{
    friend class AsyncAgent;
public:
//private:
    std::atomic_flag ModifyFlag; //spinlock for modify TaskNode OR ShouldRun
    std::atomic_bool ShouldRun;
    std::mutex RunningMtx, TerminateMtx;
    std::condition_variable CondInner, CondOuter;
    boost::context::continuation Context;
    std::atomic<detail::AsyncTaskNode*> Head = nullptr, Tail = nullptr;
    detail::AsyncTaskNode *Current = nullptr;
    uint32_t TimeYieldSleep, TimeSensitive;
    const AsyncAgent Agent;

    bool AddNode(detail::AsyncTaskNode* node);
    detail::AsyncTaskNode* DelNode(detail::AsyncTaskNode* node);//only called from self thread
    void Resume();
public:
    AsyncManager(const uint32_t timeYieldSleep = 20, const uint32_t timeSensitive = 20) : 
        TimeYieldSleep(timeYieldSleep), TimeSensitive(timeSensitive), Agent(*this) 
    { }
    PromiseResult<void> AddTask(const AsyncTaskFunc& task);
    void MainLoop();
    void Terminate();
};


}
}