#pragma once

#include "CommonRely.hpp"
#include <memory>
#include <functional>
#include <atomic>

namespace common
{


enum class PromiseState : uint8_t
{
    Invalid = 0, Unissued = 1, Issued = 8, Executing = 64, Executed = 128, Success = 144, Error = 192
};
namespace detail
{

struct PromiseResultCore : public NonCopyable
{
private:
    std::atomic_flag HasPrepared = ATOMIC_FLAG_INIT;
protected:
    virtual void PreparePms() {}
    PromiseState virtual State() { return PromiseState::Invalid; }
public:
    virtual ~PromiseResultCore() {}
    void Prepare()
    {
        if (!HasPrepared.test_and_set())
            PreparePms();
    }
    PromiseState GetState()
    {
        Prepare();
        return State();
    }
    uint64_t virtual ElapseNs() { return 0; };
};

template<class T>
class PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    PromiseResult_()
    { }
    T virtual WaitPms() = 0;
public:
    PromiseResult_(PromiseResult_&&) = default;
    T Wait()
    {
        Prepare();
        return WaitPms();
    }
};


}

template<class T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;



}