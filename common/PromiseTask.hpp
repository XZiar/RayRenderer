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
    virtual ~PromiseResultCore() {}
    PromiseState virtual state() { return PromiseState::Invalid; }
    uint64_t virtual ElapseNs() { return 0; };
};

template<class T>
class COMMONTPL PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    PromiseResult_()
    { }
public:
    PromiseResult_(PromiseResult_&&) = default;
    T virtual wait() = 0;
};


}

template<class T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;

template<class T>
class COMMONTPL PromiseTask : public NonCopyable
{
protected:
    std::function<T(void)> Task;
public:
    PromiseTask(std::function<T(void)> task) : Task(task)
    { }
    template<class... Args>
    PromiseTask(const std::function<T(Args&&...)>& task, Args&&... args) : Task(std::bind(task, std::forward<Args>(args)...))
    { }
    virtual ~PromiseTask() {}
    void virtual dowork() = 0;
    PromiseResult<T> virtual getResult() = 0;
};


}