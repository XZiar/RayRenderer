#include "MiniLoggerRely.h"
#include "QueuedBackend.h"
#include "common/PromiseTaskSTD.hpp"
#include <thread>

namespace common::mlog
{

bool LoggerQBackend::SleepCheck() noexcept
{
    return MsgQueue.empty();
}
loop::LoopBase::LoopAction LoggerQBackend::OnLoop()
{
    uintptr_t ptr = 0;
    for (uint32_t i = 16; !MsgQueue.pop(ptr) && i--;)
    {
        std::this_thread::yield();
    }
    if (ptr == 0)
        return LoopAction::Sleep();
    switch (ptr % 4)
    {
    case 0:
    {
        const auto msg = reinterpret_cast<LogMessage*>(ptr);
        OnPrint(*msg);
        LogMessage::Consume(msg);
        return LoopAction::Continue();
    }
    case 1:
    {
        const auto pms = reinterpret_cast<std::promise<void>*>(ptr - 1);
        pms->set_value();
        return LoopAction::Continue();
    }
    default:
        Expects(false);
        return LoopAction::Continue(); // Shuld not enter
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
        MsgQueue.push(reinterpret_cast<uintptr_t>(msg));
        Wakeup();
    }
}

PromiseResult<void> LoggerQBackend::Synchronize()
{
    if (!IsRunning())
        return common::FinishedResult<void>::Get();
    const auto pms = new std::promise<void>();
    const auto ptr = reinterpret_cast<uintptr_t>(pms);
    Ensures(ptr % 4 == 0);
    MsgQueue.push(ptr + 1);
    Wakeup();
    return common::PromiseResultSTD<void, false>::Get(*pms);
}


}