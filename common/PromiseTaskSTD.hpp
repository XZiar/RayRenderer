#pragma once

#include "PromiseTask.hpp"
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
    virtual ~PromiseTaskSTD() override { }
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
