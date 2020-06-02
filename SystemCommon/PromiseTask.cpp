#include "SystemCommonPch.h"
#include "PromiseTask.h"
#include "LoopBase.h"
#include "ThreadEx.h"
#include <future>


namespace common::detail
{


class PromiseActiveProxy final : private common::loop::LoopBase
{
public:
    using ErrorCallback = std::function<void(const BaseException&)>;
private:
    struct COMMON_EMPTY_BASES ProxyNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<ProxyNode>
    {
        const PmsCore Promise;
        uint32_t Id = 0;
        ProxyNode(PmsCore&& promise, uint32_t id) : Promise(std::move(promise)), Id(id)
        { }
    };

    common::container::IntrusiveDoubleLinkList<ProxyNode> TaskList;
    std::atomic_uint32_t TaskUid{ 0 };

    PromiseActiveProxy() : LoopBase(LoopBase::GetThreadedExecutor)
    {
        Start();
    }
    virtual ~PromiseActiveProxy() override {}

    virtual bool SleepCheck() noexcept override 
    {
        return TaskList.IsEmpty();
    }
    virtual bool OnStart(std::any) noexcept override
    {
        common::SetThreadName(u"PromiseActiveProxy");
        return true;
    }
    void AddNode(PmsCore&& promise)
    {
        const auto node = new ProxyNode(std::move(promise), TaskUid.fetch_add(1, std::memory_order_relaxed));
        if (TaskList.AppendNode(node))
        {
            //const auto lock = AcquireRunningLock();
            Wakeup();
        }
    }
    virtual LoopAction OnLoop() override
    {
        if (TaskList.IsEmpty())
            return LoopAction::Sleep();
        common::SimpleTimer timer;
        timer.Start();
        bool hasExecuted = false;
        for (auto Current = TaskList.Begin(); Current != nullptr;)
        {
            if (Current->Promise->GetState() >= PromiseState::Executed) // ready for callback
            {
                hasExecuted = true;
                Current->Promise->ExecuteCallback();
                Current = TaskList.PopNode(Current);
            }
            else
                Current = TaskList.ToNext(Current);
        }
        timer.Stop();
        if (!hasExecuted && timer.ElapseMs() < 10)
            return LoopAction::SleepFor(10);
        return LoopAction::Continue();
    }
public:
    static void Attach(PmsCore&& promise)
    {
        static PromiseActiveProxy proxy;
        proxy.AddNode(std::move(promise));
    }
};




PromiseResultCore::~PromiseResultCore()
{ }

PromiseState PromiseResultCore::GetState() noexcept
{
    return PromiseState::Invalid;
}

void PromiseResultCore::PreparePms()
{ }

void PromiseResultCore::MakeActive(PmsCore&& pms)
{
    PromiseActiveProxy::Attach(std::move(pms));
}

void PromiseResultCore::Prepare()
{
    if (!Flags.Add(PromiseFlags::Prepared))
        PreparePms();
}

PromiseState PromiseResultCore::State()
{
    Prepare();
    if (CachedState >= PromiseState::Executed)
        return CachedState;
    else
        return CachedState = GetState();
}

void PromiseResultCore::WaitFinish()
{
    Prepare();
    WaitPms();
}


struct BasicPromiseResult_::Waitable
{
    std::promise<void> Pms;
    std::future<void> Future;
public:
    Waitable() : Future(Pms.get_future()) {}
};

BasicPromiseResult_::BasicPromiseResult_() : Ptr(new Waitable())
{ }

BasicPromiseResult_::~BasicPromiseResult_()
{
    delete Ptr;
}

void BasicPromiseResult_::Wait() noexcept
{
    Ptr->Future.wait();
}

void BasicPromiseResult_::NotifyState(PromiseState state)
{
    const auto prev = TheState.exchange(state);
    Expects(prev < PromiseState::Executed);
    //COMMON_THROW(BaseException, u"Set result repeatedly");
    Ptr->Pms.set_value();
}

}