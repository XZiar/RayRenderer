#include "AsyncExecutorRely.h"
#include "AsyncAgent.h"
#include "AsyncManager.h"
#include "../PromiseTask.inl"

namespace common::asyexe
{


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
    std::promise<void> rawpms;
    auto pms = std::make_shared<common::PromiseResultSTD<void>>(rawpms);
    auto pmscore = std::dynamic_pointer_cast<common::detail::PromiseResultCore>(pms);
    std::thread([&](std::promise<void> thepms)
    { 
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        thepms.set_value();
    }, std::move(rawpms)).detach();
    AddPms(pmscore);
    return pms->wait();
}

}

