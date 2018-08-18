#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include "common/IntToString.hpp"
#include "common/SpinLock.hpp"
#include "common/StrCharset.hpp"
#include "common/ThreadEx.inl"
#include <mutex>



namespace common::asyexe
{

bool AsyncManager::AddNode(detail::AsyncTaskNode* node)
{
    node->Next = nullptr;
    common::SpinLocker::Lock(ModifyFlag);//ensure add is done
    if (!ShouldRun)
    {
        common::SpinLocker::Unlock(ModifyFlag);
        return false;
    }
    node->Prev = Tail;
    if (Head)
    {
        Tail = node;
        node->Prev->Next = node;
        common::SpinLocker::Unlock(ModifyFlag);
    }
    else //list became not empty
    {
        RunningMtx.lock(); //ensure that mainloop is waiting
        Head = Tail = node;
        RunningMtx.unlock();
        common::SpinLocker::Unlock(ModifyFlag);
        CondWait.notify_one();
    }
    return true;
}

detail::AsyncTaskNode* AsyncManager::DelNode(detail::AsyncTaskNode* node)
{
    common::SpinLocker locker(ModifyFlag);
    auto prev = node->Prev, next = node->Next;
    if (prev)
        prev->Next = next;
    else
        Head = next;
    if (next)
        next->Prev = prev;
    else
        Tail = prev;
    delete node;
    return next;
}

void AsyncManager::Resume()
{
    Context = Context.resume();
}

void AsyncManager::MainLoop()
{
    std::unique_lock<std::mutex> cvLock(RunningMtx);
    AsyncAgent::GetRawAsyncAgent() = &Agent;
    common::SimpleTimer timer;
    while (ShouldRun)
    {
        if (Head == nullptr)
        {
            CondWait.wait(cvLock);
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
                Current->Context = boost::context::callcc(std::allocator_arg, boost::context::fixedsize_stack(Current->StackSize),
                    [&](boost::context::continuation && context)
                {
                    Context = std::move(context);
                    (*Current)(Agent);
                    return std::move(Context);
                });
                break;
            case detail::AsyncTaskStatus::Wait:
                {
                    const uint8_t state = (uint8_t)Current->Promise->state();
                    if (state < (uint8_t)PromiseState::Executed) // not ready for execution
                        break;
                    Current->Status = detail::AsyncTaskStatus::Ready;
                }
                [[fallthrough]];
            case detail::AsyncTaskStatus::Ready:
                Current->TaskTimer.Start();
                Current->Context = Current->Context.resume();
                hasExecuted = true;
                break;
            default:
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
                Logger.debug(u"Task [{}] finished, reported executed {}us\n", Current->Name, Current->ElapseTime / 1000);
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
}

void AsyncManager::OnTerminate(const std::function<void(void)>& exiter)
{
    ShouldRun = false; //in case self-terminate
    Logger.verbose(u"AsyncExecutor [{}] begin to exit\n", Name);
    //destroy all task
    common::SpinLocker locker(ModifyFlag); //ensure no pending adding
    for (Current = Head; Current != nullptr;)
    {
        auto next = Current->Next;
        try
        {
            switch (Current->Status)
            {
            case detail::AsyncTaskStatus::New:
                COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, u"Task was cancelled and not executed, due to executor was terminated.");
            case detail::AsyncTaskStatus::Wait:
                [[fallthrough]];
            case detail::AsyncTaskStatus::Ready:
                COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Terminated, u"Task was terminated, due to executor was terminated.");
            default:
                break;
            }
        }
        catch (AsyncTaskException& e)
        {
            Logger.warning(u"Task [{}] {} due to termination.\n", Current->Name, e.reason == AsyncTaskException::Reason::Cancelled ? u"cancelled" : u"terminated");
            Current->OnException(std::current_exception());
        }
        delete Current;
        Current = next;
    }
    if (exiter)
        exiter();
}

bool AsyncManager::Start(const std::function<void(void)>& initer, const std::function<void(void)>& exiter)
{
    if (RunningThread.joinable() || ShouldRun.exchange(true)) //has turn state into true, just return
        return false;
    RunningThread = std::thread([&](const std::function<void(void)> initer, const std::function<void(void)> exiter)
    {
        if (initer)
            initer();

        this->MainLoop();

        this->OnTerminate(exiter);
    }, initer, exiter);
    return true;
}

void AsyncManager::Stop()
{
    std::unique_lock<std::mutex> terminateLock(TerminateMtx); //ensure Mainloop no enter before waiting
    if (!RunningThread.joinable()) //thread has been terminated
        return;
    ShouldRun = false;
    CondWait.notify_all();
    RunningThread.join();
}


AsyncManager::AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep, const uint32_t timeSensitive) :
    TimeYieldSleep(timeYieldSleep), TimeSensitive(timeSensitive), Name(name), Agent(*this),
    Logger(u"Asy-" + Name, { common::mlog::GetConsoleBackend() })
{ }
AsyncManager::~AsyncManager()
{
    this->Stop();
}



}

