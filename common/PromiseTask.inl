#pragma once

#include "PromiseTask.h"
#include <future>


namespace common
{


template<class T>
class PromiseResultSTD : public detail::PromiseResult_<T>
{
protected:
    std::future<T> fut;
public:
    PromiseResultSTD(std::promise<T>& pms) : fut(pms.get_future())
    { }
    PromiseResultSTD(std::future<T>&& fut_) : fut(std::move(fut_))
    { }
    T virtual wait() override
    {
        return fut.get();
    }
    PromiseState virtual state() override
    {
        if (!fut.valid())
            return PromiseState::INVALID;
        switch (fut.wait_for(std::chrono::seconds(0)))
        {
        case std::future_status::deferred:
            return PromiseState::UNISSUED;
        case std::future_status::timeout:
            return PromiseState::EXECUTING;
        case std::future_status::ready:
            return PromiseState::EXECUTED;
        default:
            return PromiseState::INVALID;
        }
    }
};


template<class T>
class PromiseTaskSTD : PromiseTask<T>
{
protected:
    std::promise<T> pms;
    PromiseResult<T> ret;
public:
    PromiseTaskSTD(std::function<T(void)> task_) : PromiseTask(task_), ret(new PromiseResultSTD<T>(pms.get_future()))
    { }
    template<class... ARGS>
    PromiseTaskSTD(std::function<T(ARGS...)> task_, ARGS... args) : PromiseTask(std::bind(task_, args)), ret(pms.get_future())
    { }
    void virtual dowork() override
    {
        pms.set_value(task());
    }
    PromiseResult<T> getResult() override
    {
        return ret;
    }
};


}
