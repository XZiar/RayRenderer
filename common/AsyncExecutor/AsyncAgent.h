#pragma once

#include "AsyncExecutorRely.h"

namespace common
{
namespace asyexe
{


class ASYEXEAPI AsyncAgent
{
    friend class AsyncManager;
private:
    AsyncManager &Manager;
    void AddPms(const PmsCore& pmscore) const;
    AsyncAgent(AsyncManager& manager) : Manager(manager) {}
public:
    void YieldThis() const;
    void Sleep(const uint32_t ms) const;
    template<typename T>
    T Await(const PromiseResult<T>& pms) const
    {
        auto pmscore = std::static_pointer_cast<common::detail::PromiseResultCore>(pms);
        AddPms(pmscore);
        return pms->wait();
    }
    template<typename T>
    T Await(const AsyncResult<T>& pms) const
    {
        auto pmscore = std::static_pointer_cast<common::detail::PromiseResultCore>(pms);
        AddPms(pmscore);
        return pms->wait();
    }
};


}
}
