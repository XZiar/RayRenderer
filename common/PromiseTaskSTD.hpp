#pragma once

#include "PromiseTask.hpp"
#include <future>


namespace common
{


template<typename T, bool Shared = false>
class PromiseResultSTD : public detail::PromiseResult_<T>
{
private:
    template<typename U, bool IsShared> struct Helper;
    template<typename U> struct Helper<U, true>
    {
        using FutType = std::shared_future<U>;
    };
    template<typename U> struct Helper<U, false>
    {
        using FutType = std::future<U>;
    };
protected:
    typename Helper<T, Shared>::FutType fut;
public:
    PromiseResultSTD(std::promise<T>& pms) : fut(pms.get_future())
    { }
    PromiseResultSTD(std::future<T>&& fut_) : fut(std::move(fut_))
    { }
    virtual ~PromiseResultSTD() override { }
    T virtual wait() override
    {
        return fut.get();
    }
    PromiseState virtual state() override
    {
        if (!fut.valid())
            return PromiseState::Invalid;
        switch (fut.wait_for(std::chrono::seconds(0)))
        {
        case std::future_status::deferred:
            return PromiseState::Unissued;
        case std::future_status::timeout:
            return PromiseState::Executing;
        case std::future_status::ready:
            return PromiseState::Executed;
        default:
            return PromiseState::Invalid;
        }
    }
};


template<class T>
class PromiseTaskSTD : public PromiseTask<T>
{
protected:
    std::promise<T> pms;
    PromiseResult<T> ret;
public:
    PromiseTaskSTD(std::function<T(void)> task_) : PromiseTask<T>(task_), ret(new PromiseResultSTD<T>(pms.get_future()))
    { }
    template<class... Args>
    PromiseTaskSTD(std::function<T(Args...)> task_, Args&&... args) : PromiseTask<T>(std::bind(task_, std::forward<Args>(args)...)), ret(pms.get_future())
    { }
    virtual ~PromiseTaskSTD() override { }
    void virtual dowork() override
    {
        pms.set_value(this->Task());
    }
    PromiseResult<T> getResult() override
    {
        return ret;
    }
};


}
