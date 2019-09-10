#include "LoopBase.h"
#include <mutex>
#include <thread>
#include <condition_variable>

namespace common::asyexe
{


struct BasicLockObject : public LoopExecutor::LockObject
{
    std::unique_lock<std::mutex> Lock;
    BasicLockObject(std::mutex& mutex) : Lock(mutex) {}
    virtual ~BasicLockObject() {}
};

struct LoopExecutor::ControlBlock
{
    std::mutex ControlMtx, RunningMtx;
    std::condition_variable_any StopCond;
};

bool LoopExecutor::Start()
{
    std::unique_lock<std::mutex> ctrlLock(Control->ControlMtx); //ensure no one call stop
    if (IsRunning.exchange(true)) // not exit yet
        return false;
    return OnStart();
}
bool LoopExecutor::Stop()
{
    std::unique_lock<std::mutex> ctrlLock(Control->ControlMtx); //ensure no one call stop again
    if (!IsRunning)
        return false;
    RequestStop = true;
    return OnStop();
}
void LoopExecutor::RunLoop() noexcept
{
    Loop.MainLoop();
    IsRunning = false;
}
std::unique_ptr<LoopExecutor::LockObject> LoopExecutor::AcquireRunningLock()
{
    return std::make_unique<BasicLockObject>(Control->ControlMtx);
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
    std::unique_lock<std::mutex> RunningLock;
protected:
    virtual void Sleep() override
    {
        SleepCond.wait(RunningLock);
    }
    virtual void Wakeup() override
    {
        SleepCond.notify_one();
    }
    virtual bool OnStart() override
    {
        if (MainThread.joinable())
            MainThread.join();
        RequestStop = false;
        MainThread = std::thread([&]() 
            {
                RunningLock = std::unique_lock<std::mutex>(Control->RunningMtx);
                this->RunLoop();
                RunningLock = {};
                Control->StopCond.notify_one();
            });
        return true;
    }
    virtual bool OnStop() override
    {
        std::unique_lock<std::mutex> runningLock(Control->RunningMtx); //wait until thread sleep or ends
        if (IsRunning)
        {
            Wakeup();
            Control->StopCond.wait(runningLock, [&]() { return !IsRunning; });
        }
        return true;
    }
public:
    ThreadedExecutor(LoopBase& loop) : LoopExecutor(loop) {}
    virtual ~ThreadedExecutor() {}
};


void InplaceExecutor::Sleep() { } // do nothing
void InplaceExecutor::Wakeup() { } // do nothing
bool InplaceExecutor::OnStart()
{
    return true;
}
bool InplaceExecutor::OnStop() 
{
    const auto lock = AcquireRunningLock();
    //std::unique_lock<std::mutex> runningLock(Control->RunningMtx);
    return true;
}
bool InplaceExecutor::RunInplace()
{
    if (IsRunning)
    {
        const auto lock = AcquireRunningLock();
        //std::unique_lock<std::mutex> runningLock(Control->RunningMtx);
        this->RunLoop();
        return true;
    }
    return false;
}




void LoopBase::MainLoop() noexcept
{
    if (!OnStart())
        return;
    while (!Host->RequestStop.load(std::memory_order_relaxed))
    {
        try
        {
            const auto state = OnLoop();
            if (state == LoopState::Finish)
                break;
            if (state == LoopState::Sleep)
                Host->Sleep();
        }
        catch (...)
        {
            if (!OnError(std::current_exception()))
                break;
        }
    }
    OnStop();
}

void LoopBase::Wakeup() const
{
    Host->Wakeup();
}

LoopBase::~LoopBase()
{
    Stop();
}

bool LoopBase::Start()
{
    return Host->Start();
}

bool LoopBase::Stop()
{
    return Host->Stop();
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
