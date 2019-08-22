#include "LoopBase.h"
#include <mutex>
#include <thread>
#include <condition_variable>

namespace common::asyexe
{


struct LoopBase::LoopHost : public NonCopyable, public NonMovable
{
    std::thread MainThread;
    std::mutex RunningMtx, ControlMtx;
    std::condition_variable SleepCond, StopCond;
};


void LoopBase::MainLoop() noexcept
{
    std::unique_lock<std::mutex> runningLock(Host->RunningMtx);
    if (!OnStart())
        return;
    while (!RequestStop.load(std::memory_order_relaxed))
    {
        try 
        {
            const auto state = OnLoop();
            if (state == LoopState::Finish)
                break;
            if (state == LoopState::Sleep)
                Host->SleepCond.wait(runningLock);
        }
        catch (...)
        {
            if (!OnError(std::current_exception()))
                break;
        }
    }
    OnStop();
    IsRunning = false;
    Host->StopCond.notify_one();
}

void LoopBase::Wakeup() const
{
    Host->SleepCond.notify_one();
}

LoopBase::LoopBase() : Host(new LoopHost())
{
}

LoopBase::~LoopBase()
{
    Stop();
    delete Host;
}

bool LoopBase::Start()
{
    std::unique_lock<std::mutex> ctrlLock(Host->ControlMtx); //ensure no one call stop
    if (IsRunning.exchange(true)) // not exit yet
        return false;
    if (Host->MainThread.joinable())
        Host->MainThread.join();
    RequestStop = false;
    Host->MainThread = std::thread([&]() { this->MainLoop(); });
    return true;
}

bool LoopBase::Stop()
{
    std::unique_lock<std::mutex> ctrlLock(Host->ControlMtx); //ensure no one call stop again
    if (!IsRunning)
        return false;
    RequestStop = true;
    {
        std::unique_lock<std::mutex> runningLock(Host->RunningMtx); //wait until thread sleep or ends
        if (IsRunning)
        {
            Wakeup();
            Host->StopCond.wait(runningLock, [&]() { return !IsRunning; });
        }
    }
    return true;
}

}
