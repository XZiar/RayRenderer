#pragma once

#include "PromiseTask.h"
#include <future>


namespace common
{


template<typename T, bool IsShared = false>
class PromiseResultSTD : public detail::PromiseResult_<T>
{
private:
    using FutType = std::conditional_t<IsShared, std::shared_future<T>, std::future<T>>;
protected:
    FutType Future;
    PromiseState WaitPms() noexcept override
    {
        Future.wait();
        return GetState();
    }
    T GetResult() override
    {
        return Future.get();
    }
    PromiseState GetState() noexcept override
    {
        if (!Future.valid())
            return PromiseState::Invalid;
        switch (Future.wait_for(std::chrono::seconds(0)))
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
public:
    PromiseResultSTD(std::promise<T>& pms) : Future(pms.get_future())
    { }
    PromiseResultSTD(std::future<T>&& fut) noexcept : Future(std::move(fut))
    { }
    template<bool R = IsShared, typename = std::enable_if_t<R>>
    PromiseResultSTD(std::shared_future<T>&& fut) noexcept : Future(std::move(fut))
    { }
    ~PromiseResultSTD() override { }
    template<typename U>
    static PromiseResult<T> Get(U&& data)
    {
        return std::make_shared<PromiseResultSTD<T, IsShared>>(std::forward<U>(data));
    }
};

template<typename T, bool IsShared = false>
class PromiseWrappedResultSTD : public detail::PromiseResult_<T>
{
private:
    PromiseResultSTD<std::unique_ptr<T>, IsShared> Inner;
    PromiseState WaitPms() noexcept override
    {
        return Inner.WaitPms();
    }
    T GetResult() override
    {
        return *Inner.WaitPms().release();
    }
    PromiseState GetState() noexcept override
    {
        return Inner.State();
    }
public:
    PromiseWrappedResultSTD(std::promise<std::unique_ptr<T>>& pms) : Inner(pms)
    { }
    ~PromiseWrappedResultSTD() override { }
};



}
