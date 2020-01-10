#include "LoopBase.h"
#include <mutex>
#include <thread>
#include <condition_variable>

namespace common::loop
{


struct LoopExecutor::ControlBlock
{
    std::mutex ControlMtx, RunningMtx;

    std::unique_lock<std::mutex> AcquireRunningLock()
    {
        return std::unique_lock<std::mutex>(RunningMtx);
    }
    bool CheckRunning()
    {
        std::unique_lock<std::mutex> lock(RunningMtx, std::try_to_lock);
        return !lock;
    }
};

bool LoopExecutor::Start(std::any cookie)
{
    std::unique_lock<std::mutex> ctrlLock(Control->ControlMtx); //ensure no one call stop
    auto desire = ExeStates::Stopped;
    if (!ExeState.compare_exchange_strong(desire, ExeStates::Starting)) // not exit yet
        return false;
    SleepState = SleepStates::Running;
    Cookie = std::move(cookie);
    DoStart();
    return true;
}
bool LoopExecutor::Stop()
{
    std::unique_lock<std::mutex> ctrlLock(Control->ControlMtx); //ensure no one call stop again
    if (!RequestStop())
        return false;
    Wakeup();
    WaitUtilStop();
    return true;
}
void LoopExecutor::Wakeup()
{
    if (WakeupLock.exchange(true)) // others handling
        return;
    auto desire = SleepStates::Pending;
    if (!SleepState.compare_exchange_strong(desire, SleepStates::Forbit))
    {
        if (desire == SleepStates::Sleep)
            DoWakeup();
    }
    WakeupLock = false;
}
bool LoopExecutor::RequestStop()
{
    auto desire = LoopExecutor::ExeStates::Running;
    while (!ExeState.compare_exchange_strong(desire, LoopExecutor::ExeStates::Stopping))
    {
        if (desire == LoopExecutor::ExeStates::Stopped)
            return false;
    }
    return true;
}
bool LoopExecutor::TurnToRun() noexcept
{
    auto desire = ExeStates::Starting;
    return ExeState.compare_exchange_strong(desire, ExeStates::Running);
}
void LoopExecutor::RunLoop() noexcept
{
    std::unique_lock<std::mutex> runningLock(Control->RunningMtx);
    if (!Loop.OnStart(Cookie))
        return;
    while (ExeState.load(std::memory_order_relaxed) == ExeStates::Running)
    {
        try
        {
            const auto action = Loop.OnLoop();
            if (action == LoopBase::LoopAction::ValFinish)
                break;
            if (const auto sleepTime = action.GetSleepTime(); sleepTime > 0)
            {
                SleepState = SleepStates::Pending;
                if (Loop.SleepCheck())
                {
                    auto desire = SleepStates::Pending;
                    if (SleepState.compare_exchange_strong(desire, SleepStates::Sleep))
                        DoSleep(&runningLock, sleepTime);
                }
            }
            SleepState = SleepStates::Running;
        }
        catch (...)
        {
            if (!Loop.OnError(std::current_exception()))
                break;
        }
    }
    Loop.OnStop();
    ExeState = ExeStates::Stopped;
}

LoopExecutor::LoopExecutor(LoopBase& loop) : Control(new ControlBlock()), Loop(loop)
{ }
LoopExecutor::~LoopExecutor()
{
    delete Control;
}


class ThreadedExecutor : public LoopExecutor
{
    std::thread MainThread;
    std::condition_variable SleepCond;
protected:
    virtual void DoSleep(void* runningLock, const uint32_t sleepTime) noexcept override
    {
        auto& mtx = *reinterpret_cast<std::unique_lock<std::mutex>*>(runningLock);
        if (sleepTime > LoopBase::LoopAction::MaxSleepTime)
            SleepCond.wait(mtx);
        else
            SleepCond.wait_for(mtx, std::chrono::milliseconds(sleepTime));

    }
    virtual void DoWakeup() noexcept override
    {
        auto lock = Control->AcquireRunningLock(); // ensure in the sleep
        SleepCond.notify_one();
    }
    virtual void DoStart() override
    {
        if (MainThread.joinable()) // ensure MainThread invalid
            MainThread.join();
        MainThread = std::thread([&]() 
            {
                if (TurnToRun())
                    this->RunLoop();
            });
    }
    virtual void WaitUtilStop() override
    {
        if (MainThread.joinable()) // ensure MainThread invalid
        {
            MainThread.join();
        }
    }
public:
    ThreadedExecutor(LoopBase& loop) : LoopExecutor(loop) {}
    virtual ~ThreadedExecutor() {}
};


void InplaceExecutor::DoSleep(void*, const uint32_t) noexcept { } // do nothing
void InplaceExecutor::DoWakeup() noexcept { } // do nothing
void InplaceExecutor::DoStart() { }
void InplaceExecutor::WaitUtilStop()
{
    const auto runningLock = Control->AcquireRunningLock();
}
bool InplaceExecutor::RunInplace()
{
    if (TurnToRun())
    {
        this->RunLoop();
        return true;
    }
    return false;
}



bool LoopBase::IsRunning() const
{
    const auto state = Host->ExeState.load();
    return state == LoopExecutor::ExeStates::Starting || state == LoopExecutor::ExeStates::Running;
}

void LoopBase::Wakeup() const
{
    Host->Wakeup();
}

LoopBase::~LoopBase()
{
    Stop();
}

bool LoopBase::Start(std::any cookie)
{
    return Host->Start(std::move(cookie));
}

bool LoopBase::Stop()
{
    return Host->Stop();
}

bool LoopBase::RequestStop()
{
    return Host->RequestStop();
}

LoopExecutor& LoopBase::GetHost()
{
    return *Host;
}

std::unique_ptr<LoopExecutor> LoopBase::GetThreadedExecutor(LoopBase& loop)
{
    return std::unique_ptr<LoopExecutor>(new ThreadedExecutor(loop));
}
std::unique_ptr<LoopExecutor> LoopBase::GetInplaceExecutor(LoopBase& loop)
{
    return std::unique_ptr<LoopExecutor>(new InplaceExecutor(loop));
}



}
