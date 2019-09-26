#include "AsyncProxy.h"
#include "common/ThreadEx.inl"
#include <thread>

namespace common::asyexe
{


LoopBase::LoopState AsyncProxy::OnLoop()
{
    if (TaskList.IsEmpty())
        return LoopState::Sleep;
    common::SimpleTimer timer;
    timer.Start();
    bool hasExecuted = false;
    for (auto Current = TaskList.Begin(); Current != nullptr;)
    {
        if (Current->Promise->GetState() >= PromiseState::Executed) // ready for callback
        {
            hasExecuted = true;
            Current->Timer.Stop();
            Logger.verbose(u"Task[{}] spend {}ms\n", Current->Id, Current->Timer.ElapseMs());
            Current->Resolve([&](const BaseException& be) { Logger.warning(u"Task[{}] reported an unhandled error:\t{}\n", Current->Id, be.message); });
            Current = TaskList.PopNode(Current);
        }
        else
            Current = TaskList.ToNext(Current);
    }
    timer.Stop();
    if (!hasExecuted && timer.ElapseMs() < 10)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return LoopState::Continue;
}

bool AsyncProxy::OnStart(std::any) noexcept
{
    Logger.info(u"AsyncProxy started\n");
    common::SetThreadName(u"AsyncProxy");
    return true;
}

AsyncProxy::AsyncProxy()
    : LoopBase(LoopBase::GetThreadedExecutor),
    Logger(u"AsyncProxy", { common::mlog::GetConsoleBackend() })
{
    Start();
}

void AsyncProxy::AddNode(AsyncNodeBase* node)
{
    node->Id = TaskUid.fetch_add(1, std::memory_order_relaxed);
    if (TaskList.AppendNode(node))
    {
        //const auto lock = AcquireRunningLock();
        Wakeup();
    }
}


AsyncProxy& AsyncProxy::GetSelf()
{
    static AsyncProxy proxy;
    return proxy;
}


}
