#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include <chrono>

namespace common::asyexe
{

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

}

