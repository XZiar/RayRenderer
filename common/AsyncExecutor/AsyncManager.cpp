#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#define NO_DATE_FORMATE
#include "common/TimeUtil.hpp"
#include "common/SpinLock.hpp"
#include "common/PromiseTask.inl"
#include <mutex>



namespace common::asyexe
{


bool AsyncManager::AddNode(detail::AsyncTaskNode* node)
{
    node->Next = nullptr;
    while (!ModifyFlag.test_and_set())//ensure add is done
    { }
    if (!ShouldRun)
    {
        ModifyFlag.clear();
        return false;
    }
    node->Prev = Tail;
    if (Head)
    {
        Tail = node;
        node->Prev->Next = node;
        ModifyFlag.clear();
    }
    else //list became not empty
    {
        RunningMtx.lock(); //ensure that mainloop is waiting
        Head = Tail = node;
        RunningMtx.unlock();
        ModifyFlag.clear();
        CondInner.notify_one();
    }
    return true;
}

detail::AsyncTaskNode* AsyncManager::DelNode(detail::AsyncTaskNode* node)
{
    common::SpinLocker locker(ModifyFlag);
    if (node->Prev)
        node->Prev->Next = node->Next;
    else
        Head = node->Next;
    if (!node->Next)
        Tail = node->Prev;
    auto next = node->Next;
    delete node;
    return next;
}

void AsyncManager::Resume()
{
    Context = Context.resume();
}

PromiseResult<void> AsyncManager::AddTask(const AsyncTaskFunc& task)
{
    auto node = new detail::AsyncTaskNode;
    node->Func = task;
    PromiseResult<void> ret(new PromiseResultSTD<void>(node->Pms));
    if (AddNode(node))
        return ret;
    else //has stopped
    {
        delete node;
        COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, L"Executor was terminated when adding task.");
    }
}

void CallWrapper(detail::AsyncTaskNode* node, const AsyncAgent& agent)
{
    try
    {
        node->Status = detail::AsyncTaskStatus::Ready;
        node->Func(agent);
        node->Status = detail::AsyncTaskStatus::Finished;
        node->Pms.set_value();
    }
    catch (boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (...)
    {
        node->Status = detail::AsyncTaskStatus::Error;
        node->Pms.set_exception(std::current_exception());
    }
}

void AsyncManager::MainLoop()
{
    if (ShouldRun.exchange(true))
        //has turn state into true, just return
        return;
    std::unique_lock<std::mutex> cvLock(RunningMtx);
    common::SimpleTimer timer;
    while (ShouldRun)
    {
        if (Head == nullptr)
        {
            CondInner.wait(cvLock);
            if (!ShouldRun.load(std::memory_order_relaxed)) //in case that waked by terminator
                break;
        }
        timer.Start();
        bool hasExecuted = false;
        for (Current = Head; Current != nullptr;)
        {
            switch (Current->Status)
            {
            case detail::AsyncTaskStatus::New:
                Current->Context = boost::context::callcc([&](boost::context::continuation && context)
                {
                    Context = std::move(context);
                    CallWrapper(Current, Agent);
                    return std::move(Context);
                });
                break;
            case detail::AsyncTaskStatus::Wait:
                {
                    const uint8_t state = (uint8_t)Current->Promise->state();
                    if (state < (uint8_t)PromiseState::EXECUTED) // not ready for execution
                        break;
                    Current->Status = detail::AsyncTaskStatus::Ready;
                }
                [[fallthrough]];
            case detail::AsyncTaskStatus::Ready:
                Current->Context = Current->Context.resume();
                hasExecuted = true;
                break;
            }
            //after processing
            if (Current->Context)
            {
                if (Current->Status == detail::AsyncTaskStatus::Yield)
                    Current->Status = detail::AsyncTaskStatus::Ready;
                common::SpinLocker locker(ModifyFlag); //ensure 'next' not changed when accessing it
                Current = Current->Next;
            }
            else //has returned
            {
                Current = DelNode(Current);
            }
            //quick exit when terminate
            if (!ShouldRun.load(std::memory_order_relaxed)) 
            {
                hasExecuted = true;
                break;
            }
        }
        timer.Stop();
        if (!hasExecuted && timer.ElapseMs() < TimeSensitive) //not executing and not elapse enough time, sleep to conserve energy
            std::this_thread::sleep_for(std::chrono::milliseconds(TimeYieldSleep));
    }
    //destroy all task
    {
        common::SpinLocker locker(ModifyFlag); //ensure no pending adding
        //since ShouldRun is false and ModifyFlag locked, further adding will be blocked.
        for (Current = Head; Current != nullptr;)
        {
            auto next = Current->Next;
            try
            {
                switch (Current->Status)
                {
                case detail::AsyncTaskStatus::New:
                    COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, L"Task was cancelled and not executed, due to executor was terminated.");
                case detail::AsyncTaskStatus::Wait:
                    [[fallthrough]];
                case detail::AsyncTaskStatus::Ready:
                    COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Terminated, L"Task was terminated, due to executor was terminated.");
                }
            }
            catch (AsyncTaskException&)
            {
                Current->Pms.set_exception(std::current_exception());
            }
            delete Current;
            Current = next;
        }
        TerminateMtx.lock(); //ensure that Terminator is waiting
        TerminateMtx.unlock();
        CondOuter.notify_all();
    }
}

void AsyncManager::Terminate()
{
    std::unique_lock<std::mutex> terminateLock(TerminateMtx); //ensure Mainloop no enter before waiting
    if (!ShouldRun.exchange(false))
        //has turn state into false, just return
        return;
    if (RunningMtx.try_lock()) //MainLoop waiting
    {
        RunningMtx.unlock();
        CondInner.notify_all();
    }
    CondOuter.wait(terminateLock);
}


}

