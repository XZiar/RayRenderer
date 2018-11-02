#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include <thread>

namespace common::mlog
{
namespace detail
{
struct ThreadWrapper : public std::thread
{
    using thread::thread;
};
}

void LoggerQBackend::LoggerWorker()
{
    std::unique_lock<std::mutex> lock(RunningMtx);
    OnStart();
    LogMessage* msg = nullptr;
    while (ShouldRun)
    {
        for (uint32_t i = 0; !MsgQueue.pop(msg); ++i)
        {
            if (i < 16)
            {
                std::this_thread::yield();
                continue;
            }
            IsWaiting = true;
            CondWait.wait(lock, [&]()
            {
                const bool poped = MsgQueue.pop(msg);
                return poped || !ShouldRun;
            });
            IsWaiting = false;
            break;
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
        RunningThread = std::make_unique<detail::ThreadWrapper>(&LoggerQBackend::LoggerWorker, this);
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