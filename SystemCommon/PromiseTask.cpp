#include "SystemCommonPch.h"
#include "PromiseTask.h"
#include "LoopBase.h"
#include "ThreadEx.h"
#include <future>


namespace common
{


class PromiseActiveProxy final : private common::loop::LoopBase
{
public:
    using ErrorCallback = std::function<void(const BaseException&)>;
private:
    struct COMMON_EMPTY_BASES ProxyNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<ProxyNode>
    {
        const PmsCore Core;
        PromiseProvider& Promise;
        uint32_t Id = 0;
        ProxyNode(PmsCore&& promise, uint32_t id) : Core(std::move(promise)), Promise(Core->GetPromise()), Id(id)
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
            if (Current->Promise.GetState() >= PromiseState::Executed) // ready for callback
            {
                hasExecuted = true;
                Current->Core->ExecuteCallback();
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




PromiseProvider::~PromiseProvider()
{ }

PromiseState PromiseProvider::GetState() noexcept
{
    return PromiseState::Invalid;
}

void PromiseProvider::PreparePms()
{ }

void PromiseProvider::MakeActive(PmsCore && pms)
{
    PromiseActiveProxy::Attach(std::move(pms));
}


PromiseResultCore::~PromiseResultCore()
{ }

void PromiseResultCore::Prepare()
{
    if (!GetPromise().Flags.Add(PromiseFlags::Prepared))
        GetPromise().PreparePms();
}

PromiseState PromiseResultCore::State()
{
    Prepare();
    return GetPromise().GetState();
}

void PromiseResultCore::WaitFinish()
{
    Prepare();
    GetPromise().WaitPms();
}

uint64_t PromiseResultCore::ElapseNs() noexcept 
{ 
    return GetPromise().ElapseNs();
}


struct BasicPromiseProvider::Waitable
{
    std::promise<PromiseState> Pms;
    std::future<PromiseState> Future;
public:
    Waitable() : Future(Pms.get_future()) {}
};

BasicPromiseProvider::BasicPromiseProvider() : Ptr(new Waitable())
{ 
    Timer.Start();
}
BasicPromiseProvider::~BasicPromiseProvider()
{
    delete Ptr;
}

PromiseState BasicPromiseProvider::GetState() noexcept
{
    return TheState;
}

PromiseState BasicPromiseProvider::WaitPms() noexcept
{
    return Ptr->Future.get();
}

uint64_t BasicPromiseProvider::ElapseNs() noexcept
{
    return Timer.ElapseNs();
}

void BasicPromiseProvider::NotifyState(PromiseState state)
{
    const auto prev = TheState.exchange(state);
    Expects(prev < PromiseState::Executed);
    //COMMON_THROW(BaseException, u"Set result repeatedly");
    Timer.Stop();
    Ptr->Pms.set_value(state);
}

}