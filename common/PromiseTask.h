#pragma once

#include "CommonRely.hpp"
#include <memory>
#include <functional>

namespace common
{


enum class PromiseState : uint8_t
{
    Invalid = 0, Unissued = 1, Issued = 8, Executing = 64, Executed = 128, Success = 144, Error = 192
};
namespace detail
{

struct COMMONAPI PromiseResultCore : public NonCopyable
{
    PromiseState virtual state() { return PromiseState::Invalid; }
};

template<class T>
class COMMONTPL PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    PromiseResult_()
    { }
public:
    T virtual wait() = 0;
    virtual ~PromiseResult_() {}
    PromiseResult_(PromiseResult_&&) = default;
};


}

template<class T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;

template<class T>
class COMMONTPL PromiseTask : public NonCopyable
{
protected:
    std::function<T(void)> task;
public:
    PromiseTask(std::function<T(void)> task_) : task(task_)
    { }
    template<class... ARGS>
    PromiseTask(std::function<T(ARGS...)> task_, ARGS... args) : task(std::bind(task_, args))
    { }
    virtual ~PromiseTask() {}
    void virtual dowork() = 0;
    PromiseResult<T> virtual getResult() = 0;
};


}