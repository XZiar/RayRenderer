#pragma once

#include <memory>
#include <functional>

namespace common
{


enum class PromiseState : uint8_t
{
    INVALID = 0, UNISSUED = 1, ISSUED = 8, EXECUTING = 64, EXECUTED = 128, SUCCESS = 144, ERROR = 192
};
namespace detail
{

struct COMMONTPL PromiseResultCore : public NonCopyable
{
    PromiseState virtual state() { return PromiseState::INVALID; }
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