#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include <thread>

namespace common::mlog
{


void LoggerQBackend::LoggerWorker()
{
    std::unique_lock<std::mutex> lock(RunningMtx);
    OnStart();
    LogMessage* msg;
    while (ShouldRun)
    {
        if (!MsgQueue.pop(msg))
        {
            IsWaiting = true;
            CondWait.wait(lock, [&]()
            {
                const bool poped = MsgQueue.pop(msg);
                return poped || !ShouldRun;
            });
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
    if(!ShouldRun.exchange(true) && !RunningThread)
        RunningThread = std::make_unique<std::thread>(&LoggerQBackend::LoggerWorker, this);
}

LoggerQBackend::LoggerQBackend(const size_t initSize) : MsgQueue(initSize)
{
}
LoggerQBackend::~LoggerQBackend()
{
    if (RunningThread && RunningThread->joinable())
    {
        ShouldRun = false;
        CondWait.notify_all();
        RunningThread->join();
    }
    //release the msgs left
    LogMessage* msg;
    while (MsgQueue.pop(msg))
        LogMessage::Consume(msg);
}

void LoggerQBackend::Print(LogMessage* msg)
{
    if ((uint8_t)msg->Level < (uint8_t)LeastLevel || !ShouldRun.load(std::memory_order_relaxed))
    {
        LogMessage::Consume(msg);
        return;
    }
    MsgQueue.push(msg);
    if (IsWaiting.load(std::memory_order_relaxed))
        CondWait.notify_one(); //may leak message, but whatever
}
void LoggerQBackend::Flush()
{
    CondWait.notify_one();
}


}