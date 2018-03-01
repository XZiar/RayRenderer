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
    template<typename T>
    PromiseResult<T> Await(const PromiseResult<T>& pms) const
    {
        auto pmscore = std::dynamic_pointer_cast<common::detail::PromiseResultCore>(pms);
        AddPms(pmscore);
        return pms->wait();
    }
    void Sleep(const uint32_t ms) const;
};


}
}
