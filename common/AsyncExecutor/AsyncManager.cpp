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

void AsyncManager::CallWrapper(detail::AsyncTaskNode* node, const AsyncAgent& agent)
{
    try
    {
        node->Status = detail::AsyncTaskStatus::Ready;
        node->TaskTimer.Start();
        node->Func(agent);
        node->TaskTimer.Stop();
        node->ResPms->ElapseTime += node->TaskTimer.ElapseNs();
        node->Status = detail::AsyncTaskStatus::Finished;
        node->Pms.set_value();
    }
    catch (boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (...)
    {
        node->TaskTimer.Stop();
        node->ResPms->ElapseTime += node->TaskTimer.ElapseNs();
        node->Status = detail::AsyncTaskStatus::Error;
        node->Pms.set_exception(std::current_exception());
    }
}

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
        CondInner.notify_one();
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

PromiseResult<void> AsyncManager::AddTask(const AsyncTaskFunc& task, std::u16string taskname, uint32_t stackSize)
{
    const auto tuid = TaskUid.fetch_add(1, std::memory_order_relaxed);
    if (taskname == u"")
        taskname = u"task" + common::str::to_u16string(tuid);
    if (stackSize == 0)
        stackSize = static_cast<uint32_t>(boost::context::fixedsize_stack::traits_type::default_size());
    auto node = new detail::AsyncTaskNode(taskname, stackSize);
    node->Func = task;
    node->ResPms = std::make_shared<detail::AsyncTaskResult>(node->Pms);
    const auto ret = std::static_pointer_cast<common::detail::PromiseResult_<void>>(node->ResPms);
    if (AddNode(node))
    {
        Logger.debug(u"Add new task [{}] [{}]\n", tuid, node->Name);
        return ret;
    }
    else //has stopped
    {
        Logger.warning(u"New task cancelled due to termination [{}] [{}]\n", tuid, node->Name);
        delete node;
        COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, L"Executor was terminated when adding task.");
    }
}

void AsyncManager::MainLoop(const std::function<void(void)>& initer, const std::function<void(void)>& exiter)
{
    if (ShouldRun.exchange(true))
        //has turn state into true, just return
        return;
    std::unique_lock<std::mutex> cvLock(RunningMtx);
    RunningThread = ThreadObject::GetCurrentThreadObject();
    if (initer)
        initer();
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
                Current->Context = boost::context::callcc(std::allocator_arg, boost::context::fixedsize_stack(Current->StackSize),
                    [&](boost::context::continuation && context)
                {
                    Context = std::move(context);
                    CallWrapper(Current, Agent);
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
                Logger.debug(u"Task [{}] finished, reported executed {}us\n", Current->Name, Current->ResPms->ElapseTime / 1000);
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
    OnTerminate(exiter);
}

void AsyncManager::Terminate()
{
    std::unique_lock<std::mutex> terminateLock(TerminateMtx); //ensure Mainloop no enter before waiting
    if (!ShouldRun.exchange(false))
        //has turn state into false, just return
        return;
    if (!RunningThread.IsAlive())
        return;
    if (RunningMtx.try_lock()) //MainLoop waiting
    {
        RunningMtx.unlock();
        CondInner.notify_all();
    }
    CondOuter.wait(terminateLock);
}

void AsyncManager::OnTerminate(const std::function<void(void)>& exiter)
{
    ShouldRun = false; //in case self-terminate
    Logger.verbose(u"begin to exit");
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
                COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, L"Task was cancelled and not executed, due to executor was terminated.");
            case detail::AsyncTaskStatus::Wait:
                [[fallthrough]];
            case detail::AsyncTaskStatus::Ready:
                COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Terminated, L"Task was terminated, due to executor was terminated.");
            }
        }
        catch (AsyncTaskException& e)
        {
            Logger.warning(u"Task [{}] {} due to termination.\n", Current->Name, e.reason == AsyncTaskException::Reason::Cancelled ? u"cancelled" : u"terminated");
            Current->Pms.set_exception(std::current_exception());
        }
        delete Current;
        Current = next;
    }
    if (exiter)
        exiter();
    {
        TerminateMtx.lock();
        TerminateMtx.unlock();
        CondOuter.notify_all();//in case waiting
    }
}


AsyncManager::AsyncManager(const std::u16string& name, const uint32_t timeYieldSleep, const uint32_t timeSensitive) :
    Name(name), TimeYieldSleep(timeYieldSleep), TimeSensitive(timeSensitive), Agent(*this),
    Logger(u"Asy-" + Name, { common::mlog::GetConsoleBackend(), common::mlog::GetDebuggerBackend() })
{ }
AsyncManager::~AsyncManager()
{
    OnTerminate();
}



}

