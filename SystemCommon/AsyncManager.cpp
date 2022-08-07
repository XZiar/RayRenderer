#include "SystemCommonPch.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include "StringFormat.h"
#define BOOST_CONTEXT_STATIC_LINK 1
#define BOOST_CONTEXT_NO_LIB 1
#include "3rdParty/boost.context/include/boost/context/continuation.hpp"
#include <chrono>


namespace common::asyexe
{

COMMON_EXCEPTION_IMPL(AsyncTaskException)

using namespace std::string_view_literals;

namespace detail
{

class AsyncSleeper : public ::common::PromiseResultCore, public ::common::PromiseProvider
{
    friend class common::asyexe::AsyncAgent;
protected:
    std::chrono::high_resolution_clock::time_point Target;
    ::common::PromiseProvider& GetPromise() noexcept override { return *this; }
    PromiseState GetState() noexcept override
    {
        return std::chrono::high_resolution_clock::now() < Target ? common::PromiseState::Executing : common::PromiseState::Success;
    }
    PromiseState WaitPms() noexcept override { return GetState(); }
    void ExecuteCallback() override { }
    void AddCallback(std::function<void()> func) override { assert(false); }
    void AddCallback(std::function<void(const PmsCore&)> func) override { assert(false); }
public:
    AsyncSleeper(const uint32_t sleepTimeMs)
    {
        Target = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(sleepTimeMs);
    }
    ~AsyncSleeper() override { }
};


AsyncTaskPromiseProvider::~AsyncTaskPromiseProvider()
{ }
uint64_t AsyncTaskPromiseProvider::ElapseNs() noexcept
{ 
    return ElapseTime;
}
uint64_t AsyncTaskPromiseProvider::E2EElapseNs() noexcept
{
    return BasicPromiseProvider::ElapseNs();
}


std::pair<void*, std::byte*> AsyncTaskNodeBase::CreateSpace(size_t align, size_t size) noexcept
{
    using B = boost::context::continuation;
    constexpr auto BSize = sizeof(B);
    constexpr auto BAlign = alignof(B);
    const auto finalAlign = std::max(align, BAlign);
    const auto offset = (BSize + align - 1) / align * align;
    const auto ptr = malloc_align(offset + size, finalAlign);
    new (ptr)B();
    return { ptr, reinterpret_cast<std::byte*>(ptr) + offset };
}
void AsyncTaskNodeBase::ReleaseNode(AsyncTaskNodeBase* node)
{
    const auto ptr = node->PtrContext;
    node->~AsyncTaskNodeBase();
    free_align(ptr);
}

AsyncTaskNodeBase::AsyncTaskNodeBase(void* ptrCtx, std::u16string& name, uint32_t tuid, uint32_t stackSize) noexcept :
    PtrContext(ptrCtx), Name(std::move(name)), TaskUid(tuid),
    StackSize(stackSize == 0 ? static_cast<uint32_t>(boost::context::fixedsize_stack::traits_type::default_size()) : stackSize) 
{ }
AsyncTaskNodeBase::~AsyncTaskNodeBase() {}

void AsyncTaskNodeBase::Execute(const AsyncAgent& agent)
{
    Status = AsyncTaskStatus::Ready;
    TaskTimer.Start();
    try
    {
        OnExecute(agent);
    }
    catch (const boost::context::detail::forced_unwind&) //bypass forced_unwind since it's needed for context destroying
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (...)
    {
        OnException(std::current_exception());
    }
}
void AsyncTaskNodeBase::SumPartialTime() noexcept
{
    TaskTimer.Stop();
    ElapseTime += TaskTimer.ElapseNs();
}
void AsyncTaskNodeBase::FinishTask(const AsyncTaskStatus status, AsyncTaskPromiseProvider& taskTime) noexcept
{
    SumPartialTime();
    taskTime.ElapseTime = ElapseTime;
    Status = status;
}

}


const AsyncAgent*& AsyncAgent::GetRawAsyncAgent()
{
    thread_local const AsyncAgent* agent = nullptr;
    return agent;
}

void AsyncAgent::AddPms(::common::PmsCore pmscore) const
{
    pmscore->Prepare();

    Manager.Current->Promise = std::move(pmscore);
    Manager.Resume(detail::AsyncTaskStatus::Wait);
    Manager.Current->Promise = nullptr; //don't hold pms
}
void AsyncAgent::YieldThis() const
{
    Manager.Resume(detail::AsyncTaskStatus::Yield);
}
void AsyncAgent::Sleep(const uint32_t ms) const
{
    auto pms = std::make_shared<detail::AsyncSleeper>(ms);
    AddPms(pms);
}

const common::asyexe::AsyncAgent* AsyncAgent::GetAsyncAgent()
{
    return GetRawAsyncAgent();
}


struct AsyncManager::FiberContext : public boost::context::continuation
{};
boost::context::continuation& ToContext(void* ptr) noexcept
{
    return *reinterpret_cast<boost::context::continuation*>(ptr);
}


uint32_t AsyncManager::PreCheckTask(std::u16string& taskName)
{
    const auto tuid = TaskUid.fetch_add(1, std::memory_order_relaxed);
    if (taskName == u"")
        taskName = fmt::format(u"task {}", tuid);
    if (!AllowStopAdd && !IsRunning()) //has stopped
    {
        Logger.Warning(u"New task cancelled due to termination [{}] [{}]\n"sv, tuid, taskName);
        COMMON_THROW(AsyncTaskException, AsyncTaskException::Reasons::Cancelled, u"Executor was terminated when adding task.");
    }
    return tuid;
}

bool AsyncManager::AddNode(detail::AsyncTaskNodeBase* node)
{
    if (TaskList.AppendNode(node)) // need to notify worker
        Wakeup();
    Logger.Debug(FmtString(u"Add new task [{}] [{}]\n"sv), node->TaskUid, node->Name);
    return true;
}

void AsyncManager::Resume(detail::AsyncTaskStatus status)
{
    Current->SumPartialTime();
    Current->Status = status;
    ToContext(Context.get()) = ToContext(Context.get()).resume();
    if (!IsRunning())
        COMMON_THROW(AsyncTaskException, AsyncTaskException::Reasons::Terminated, u"Task was terminated, due to executor was terminated.");
}

common::loop::LoopBase::LoopAction AsyncManager::OnLoop()
{
    if (TaskList.IsEmpty())
        return LoopAction::Sleep();
    common::SimpleTimer timer;
    timer.Start();
    bool hasExecuted = false;
    for (Current = TaskList.Begin(); Current != nullptr;)
    {
        switch (Current->Status)
        {
        case detail::AsyncTaskStatus::New:
            ToContext(Current->PtrContext) = boost::context::callcc(std::allocator_arg, boost::context::fixedsize_stack(Current->StackSize),
                [&](boost::context::continuation&& context)
                {
                    ToContext(Context.get()) = std::move(context);
                    Current->Execute(Agent);
                    return std::move(ToContext(Context.get()));
                });
            break;
        case detail::AsyncTaskStatus::Wait:
        {
            if (Current->Promise->State() < PromiseState::Executed) // not ready for execution
                break;
            Current->Status = detail::AsyncTaskStatus::Ready;
        }
        [[fallthrough]];
        case detail::AsyncTaskStatus::Ready:
            Current->TaskTimer.Start();
            ToContext(Current->PtrContext) = ToContext(Current->PtrContext).resume();
            hasExecuted = true;
            break;
        default:
            break;
        }
        //after processing
        if (ToContext(Current->PtrContext))
        {
            if (Current->Status == detail::AsyncTaskStatus::Yield)
                Current->Status = detail::AsyncTaskStatus::Ready;
            Current = TaskList.ToNext(Current);
        }
        else //has returned
        {
            Logger.Debug(FmtString(u"Task [{}] finished, reported executed {}us\n"sv), Current->Name, Current->ElapseTime / 1000);
            auto tmp = Current;
            Current = TaskList.PopNode(Current);
            detail::AsyncTaskNodeBase::ReleaseNode(tmp);
        }
    }
    timer.Stop();
    if (!hasExecuted && timer.ElapseMs() < TimeSensitive) //not executing and not elapse enough time, sleep to conserve energy
        return LoopAction::SleepFor(TimeYieldSleep);
    return LoopAction::Continue();
}
bool AsyncManager::SleepCheck() noexcept
{
    return TaskList.IsEmpty();
}

bool AsyncManager::OnStart(std::any cookie) noexcept
{
    AsyncAgent::GetRawAsyncAgent() = &Agent;
    Logger.Info(u"AsyncProxy started\n");
    Injector initer;
    std::tie(initer, ExitCallback) = std::any_cast<std::pair<Injector, Injector>>(std::move(cookie));
    if (initer)
        initer();
    return true;
}

void AsyncManager::OnStop() noexcept
{
    Logger.Verbose(u"AsyncExecutor [{}] begin to exit\n", Name);
    Current = nullptr;
    //destroy all task
    TaskList.ForEach([&](detail::AsyncTaskNodeBase* node)
        {
            switch (node->Status)
            {
            case detail::AsyncTaskStatus::New:
                Logger.Warning(u"Task [{}] cancelled due to termination.\n", node->Name);
                try
                {
                    COMMON_THROW(AsyncTaskException, AsyncTaskException::Reasons::Cancelled, u"Task was cancelled and not executed, due to executor was terminated.");
                }
                catch (const AsyncTaskException&)
                {
                    node->OnException(std::current_exception());
                }
                break;
            case detail::AsyncTaskStatus::Wait:
                [[fallthrough]] ;
            case detail::AsyncTaskStatus::Ready:
                ToContext(node->PtrContext) = ToContext(node->PtrContext).resume(); // need to resume so that stack will be released
                break;
            default:
                break;
            }
            detail::AsyncTaskNodeBase::ReleaseNode(node);
        }, true);
    AsyncAgent::GetRawAsyncAgent() = nullptr;
    if (ExitCallback)
        ExitCallback();
}


bool AsyncManager::Start(Injector initer, Injector exiter)
{
    return LoopBase::Start(std::pair(initer, exiter));
}

bool AsyncManager::Stop()
{
    return LoopBase::Stop();
}

bool AsyncManager::RequestStop()
{
    return LoopBase::RequestStop();
}

common::loop::LoopExecutor& AsyncManager::GetHost()
{
    return LoopBase::GetHost();
}


AsyncManager::AsyncManager(const bool isthreaded, const std::u16string& name, const uint32_t timeYieldSleep, const uint32_t timeSensitive, const bool allowStopAdd) :
    LoopBase(isthreaded ? LoopBase::GetThreadedExecutor : LoopBase::GetInplaceExecutor),
    Context(std::make_unique<FiberContext>()), Name(name), Agent(*this), 
    Logger(u"Asy-" + Name, { common::mlog::GetConsoleBackend() }),
    TimeYieldSleep(timeYieldSleep), TimeSensitive(timeSensitive), AllowStopAdd(allowStopAdd)
    { }

AsyncManager::~AsyncManager()
{ }

}
