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
    static const AsyncAgent*& GetRawAsyncAgent();
public:
    void YieldThis() const;
    void Sleep(const uint32_t ms) const;
    template<typename T>
    T Await(const PromiseResult<T>& pms) const
    {
        auto pmscore = std::static_pointer_cast<common::detail::PromiseResultCore>(pms);
        AddPms(pmscore);
        return pms->Wait();
    }
    template<typename T>
    T Await(const AsyncResult<T>& pms) const
    {
        auto pmscore = std::static_pointer_cast<common::detail::PromiseResultCore>(pms);
        AddPms(pmscore);
        return pms->Wait();
    }
    static const AsyncAgent* GetAsyncAgent();
    ///<summary>Safe wait, in case you are waiting for a task posted into the same thread, which may cause dead-lock</summary>  
    ///<param name="pms">promise</param>
    ///<returns>T</returns>
    template<typename T>
    static T SafeWait(const PromiseResult<T>& pms)
    {
        const auto agent = GetAsyncAgent();
        if (agent)
            return agent->Await(pms);
        else
            return pms->Wait();
    }
};


}
}
