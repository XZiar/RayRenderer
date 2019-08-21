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

template<typename T>
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

template<typename RetType, typename MidType>
class StagedResult_ : PromiseResult_<RetType>
{
public:
    using PostExecute = std::function<RetType(MidType&&)>;
private:
    std::shared_ptr<PromiseResult_<MidType>> Stage1;
    PostExecute Stage2;
protected:
    RetType virtual WaitPms() override
    {
        auto midArg = Stage1->Wait();
        return Stage2(std::move(midArg));
    }
    PromiseState virtual State() override
    { 
        const auto state = Stage1->GetState();
        if (state == PromiseState::Success)
            return PromiseState::Executed;
        return state;
    }
public:
    StagedResult_(std::shared_ptr<PromiseResult_<MidType>>&& stage1, PostExecute&& stage2)
        : Stage1(std::move(stage1)), Stage2(std::move(stage2))
    { }
    virtual ~StagedResult_() override {}
};

}

template<typename T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;
template<typename R, typename M>
using StagedResult = std::shared_ptr<detail::StagedResult_<R, M>>;


template<typename T>
class FinishedResult
{
private:
    class FinishedResult_ : public detail::PromiseResult_<T>
    {
        friend class FinishedResult<T>;
    private:
        T Data;
    protected:
        PromiseState virtual State() override { return PromiseState::Executed; }
        T virtual WaitPms() override { return std::move(Data); }
    public:
        FinishedResult_(T&& data) : Data(data) {}
        virtual ~FinishedResult_() override {}
    };
public:
    template<typename U>
    static PromiseResult<T> Get(U&& data)
    {
        return std::make_shared<FinishedResult_>(std::forward<U>(data));
    }
};

}