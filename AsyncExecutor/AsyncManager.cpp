#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include "common/IntToString.hpp"



namespace common::asyexe
{

namespace detail
{

AsyncTaskNodeBase::AsyncTaskNodeBase(const std::u16string name, uint32_t stackSize) : Name(name),
    StackSize(stackSize == 0 ? static_cast<uint32_t>(boost::context::fixedsize_stack::traits_type::default_size()) : stackSize) 
{ }
void AsyncTaskNodeBase::BeginTask()
{
    Status = detail::AsyncTaskStatus::Ready;
    TaskTimer.Start();
}
void AsyncTaskNodeBase::SumPartialTime()
{
    TaskTimer.Stop();
    ElapseTime += TaskTimer.ElapseNs();
}
void AsyncTaskNodeBase::FinishTask(const detail::AsyncTaskStatus status, AsyncTaskTime& taskTime)
{
    SumPartialTime();
    taskTime.ElapseTime = ElapseTime;
    Status = status;
}
AsyncTaskNodeBase::~AsyncTaskNodeBase() { }

}


bool AsyncManager::AddNode(detail::AsyncTaskNodeBase* node)
{
    if (TaskList.AppendNode(node)) // need to notify worker
        Wakeup();
    return true;
}

void AsyncManager::Resume(detail::AsyncTaskStatus status)
{
    Current->SumPartialTime();
    Current->Status = status;
    Context = Context.resume();
    if (!IsRunning())
        COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Terminated, u"Task was terminated, due to executor was terminated.");
}

common::loop::LoopBase::LoopAction AsyncManager::OnLoop()
{
    if (TaskList.IsEmpty())
        return LoopAction::Sleep();
    common::SimpleTimer timer;
    timer.Start();
    bool hasExecuted = false;
    for (Current = TaskList.Begin(); Current != nullptr;)
    {
        switch (Current->Status)
        {
        case detail::AsyncTaskStatus::New:
            Current->Context = boost::context::callcc(std::allocator_arg, boost::context::fixedsize_stack(Current->StackSize),
                [&](boost::context::continuation&& context)
                {
                    Context = std::move(context);
                    (*Current)(Agent);
                    return std::move(Context);
                });
            break;
        case detail::AsyncTaskStatus::Wait:
        {
            if (Current->Promise->State() < PromiseState::Executed) // not ready for execution
                break;
            Current->Status = detail::AsyncTaskStatus::Ready;
        }
        [[fallthrough]] ;
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
            Current = TaskList.ToNext(Current);
        }
        else //has returned
        {
            Logger.debug(FMT_STRING(u"Task [{}] finished, reported executed {}us\n"), Current->Name, Current->ElapseTime / 1000);
            auto tmp = Current;
            Current = TaskList.PopNode(Current);
            delete tmp;
        }
    }
    timer.Stop();
    if (!hasExecuted && timer.ElapseMs() < TimeSensitive) //not executing and not elapse enough time, sleep to conserve energy
        return LoopAction::SleepFor(TimeYieldSleep);
    return LoopAction::Continue();
}
bool AsyncManager::SleepCheck() noexcept
{
    return TaskList.IsEmpty();
}

bool AsyncManager::OnStart(std::any cookie) noexcept
{
    AsyncAgent::GetRawAsyncAgent() = &Agent;
    Logger.info(u"AsyncProxy started\n");
    Injector initer;
    std::tie(initer, ExitCallback) = std::any_cast<std::pair<Injector, Injector>>(std::move(cookie));
    if (initer)
        initer();
    return true;
}

void AsyncManager::OnStop() noexcept
{
    Logger.verbose(u"AsyncExecutor [{}] begin to exit\n", Name);
    Current = nullptr;
    //destroy all task
    TaskList.ForEach([&](detail::AsyncTaskNodeBase* node)
        {
            switch (node->Status)
            {
            case detail::AsyncTaskStatus::New:
                Logger.warning(u"Task [{}] cancelled due to termination.\n", node->Name);
                try
                {
                    COMMON_THROW(AsyncTaskException, AsyncTaskException::Reason::Cancelled, u"Task was cancelled and not executed, due to executor was terminated.");
                }
                catch (AsyncTaskException&)
                {
                    node->OnException(std::current_exception());
                }
                break;
            case detail::AsyncTaskStatus::Wait:
                [[fallthrough]] ;
            case detail::AsyncTaskStatus::Ready:
                node->Context = node->Context.resume(); // need to resume so that stack will be released
                break;
            default:
                break;
            }
            delete node;
        }, true);
    AsyncAgent::GetRawAsyncAgent() = nullptr;
    if (ExitCallback)
        ExitCallback();
}


bool AsyncManager::Start(Injector initer, Injector exiter)
{
    return LoopBase::Start(std::pair(initer, exiter));
}

bool AsyncManager::Stop()
{
    return LoopBase::Stop();
}

bool AsyncManager::RequestStop()
{
    return LoopBase::RequestStop();
}

common::loop::LoopExecutor& AsyncManager::GetHost()
{
    return LoopBase::GetHost();
}


AsyncManager::AsyncManager(const bool isthreaded, const std::u16string& name, const uint32_t timeYieldSleep, const uint32_t timeSensitive, const bool allowStopAdd) :
    LoopBase(isthreaded ? LoopBase::GetThreadedExecutor : LoopBase::GetInplaceExecutor),
    Name(name), Agent(*this), 
    Logger(u"Asy-" + Name, { common::mlog::GetConsoleBackend() }),
    TimeYieldSleep(timeYieldSleep), TimeSensitive(timeSensitive), AllowStopAdd(allowStopAdd)
    { }

AsyncManager::~AsyncManager()
{ }

}
