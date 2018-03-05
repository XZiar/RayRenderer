#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include <thread>
#include <functional>

namespace common::mlog
{


void LoggerQBackend::LoggerWorker()
{
    std::unique_lock<std::mutex> lock(RunningMtx);
    OnStart();
    CurThread = common::ThreadObject::GetCurrentThreadObject();
    LogMessage* msg;
    while (ShouldRun)
    {
        if (!MsgQueue.pop(msg))
        {
            IsWaiting = true;
            CondWait.wait(lock, [&]() { return MsgQueue.pop(msg) || !ShouldRun; });
            IsWaiting = false;
        }
        if (msg)
        {
            OnPrint(*msg);
            LogMessage::Consume(msg);
            msg = nullptr;
        }
    }
    OnStop();
}

void LoggerQBackend::Start()
{
    if(!ShouldRun.exchange(true))
        std::thread(&LoggerQBackend::LoggerWorker, this).detach();
}

LoggerQBackend::LoggerQBackend(const size_t initSize) : MsgQueue(initSize)
{
}
LoggerQBackend::~LoggerQBackend()
{
    if (CurThread.IsAlive())
    {
        ShouldRun = false;
        {
            CondWait.notify_all();
            RunningMtx.lock(); //ensure worker stopped
            RunningMtx.unlock();
        }
    }
    //release the msgs left
    LogMessage* msg;
    while (MsgQueue.pop(msg))
        LogMessage::Consume(msg);
}

void LoggerQBackend::Print(LogMessage* msg)
{
    if ((uint8_t)msg->Level < (uint8_t)LeastLevel)
    {
        LogMessage::Consume(msg);
        return;
    }
    MsgQueue.push(msg);
    if (IsWaiting.load(std::memory_order_relaxed))
        CondWait.notify_one();
}
void LoggerQBackend::Flush()
{
    CondWait.notify_one();
}


}