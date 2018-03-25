#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include "common/PromiseTaskSTD.hpp"
#include <chrono>

namespace common::asyexe
{

namespace detail
{

class AsyncSleeper : public ::common::detail::PromiseResultCore
{
    friend class common::asyexe::AsyncAgent;
protected:
    std::chrono::high_resolution_clock::time_point Target;
public:
    AsyncSleeper(const uint32_t sleepTimeMs)
    {
        Target = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(sleepTimeMs);
    }
    virtual ~AsyncSleeper() {}
    AsyncSleeper(AsyncSleeper&&) = default;
    PromiseState virtual state() 
    {
        return std::chrono::high_resolution_clock::now() < Target ? common::PromiseState::Executing : common::PromiseState::Success;
    }
};

}

void AsyncAgent::AddPms(const PmsCore& pmscore) const
{
    Manager.Current->Promise = pmscore;
    Manager.Current->Status = detail::AsyncTaskStatus::Wait;
    Manager.Resume();
    Manager.Current->Promise = nullptr; //don't hold pms
};
void AsyncAgent::YieldThis() const
{
    Manager.Current->Status = detail::AsyncTaskStatus::Yield;
    Manager.Resume();
}
void AsyncAgent::Sleep(const uint32_t ms) const
{
    auto pms = std::make_shared<detail::AsyncSleeper>(ms);
    auto pmscore = std::dynamic_pointer_cast<common::detail::PromiseResultCore>(pms);
    AddPms(pmscore);
}

}
