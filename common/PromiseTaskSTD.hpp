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

template<typename T, bool Shared = false>
class PromiseWrappedResultSTD : public detail::PromiseResult_<T>
{
private:
    PromiseResultSTD<std::unique_ptr<T>, Shared> Inner;
public:
    PromiseWrappedResultSTD(std::promise<std::unique_ptr<T>>& pms) : Inner(pms)
    { }
    virtual ~PromiseWrappedResultSTD() override { }
    T virtual wait() override
    {
        return *Inner.wait().release();
    }
    PromiseState virtual state() override
    {
        return Inner.state();
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


template<typename T, typename U>
inline PromiseResult<T> GetFinishedResult(U&& data)
{
    std::promise<T> pms;
    auto ret = std::make_shared<PromiseResultSTD<T>>(pms);
    pms.set_value(std::forward<U>(data));
    return ret;
}

}
