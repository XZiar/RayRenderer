#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include <thread>

namespace common::mlog
{

bool LoggerQBackend::SleepCheck() noexcept
{
    return MsgQueue.empty();
}
loop::LoopBase::LoopState LoggerQBackend::OnLoop()
{
    LogMessage* msg = nullptr;
    for (uint32_t i = 16; !MsgQueue.pop(msg) && i--;)
    {
            std::this_thread::yield();
    }
    if (msg)
    {
        OnPrint(*msg);
        LogMessage::Consume(msg);
        return LoopState::Continue;
    }
    else
    {
        return LoopState::Sleep;
    }
}

void LoggerQBackend::EnsureRunning()
{
    if (!IsRunning())
        Start();
}

LoggerQBackend::LoggerQBackend(const size_t initSize) :
    LoopBase(LoopBase::GetThreadedExecutor), MsgQueue(initSize)
{
}
LoggerQBackend::~LoggerQBackend()
{
    //release the msgs left
    LogMessage* msg;
    while (MsgQueue.pop(msg))
        LogMessage::Consume(msg);
}

void LoggerQBackend::Print(LogMessage* msg)
{
    if (msg->Level < LeastLevel || !IsRunning())
    {
        LogMessage::Consume(msg);
    }
    else
    {
        MsgQueue.push(msg);
        Wakeup();
    }
}


}