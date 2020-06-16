#pragma once
#include "oglPch.h"
#include "AsyncExecutor/AsyncAgent.h"

namespace oglu
{

class COMMON_EMPTY_BASES oglPromiseCore : public detail::oglCtxObject<true>
{
protected:
    GLsync SyncObj;
    uint64_t TimeBegin = 0, TimeEnd = 0;
    GLuint Query;
    oglPromiseCore()
    {
        CtxFunc->ogluGenQueries(1, &Query);
        CtxFunc->ogluGetInteger64v(GL_TIMESTAMP, (GLint64*)&TimeBegin); //suppose it is the time all commands are issued.
        CtxFunc->ogluQueryCounter(Query, GL_TIMESTAMP); //this should be the time all commands are completed.
        SyncObj = CtxFunc->ogluFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        CtxFunc->ogluFlush(); //ensure sync object sended
    }
    ~oglPromiseCore()
    {
        if (EnsureValid())
        {
            CtxFunc->ogluDeleteSync(SyncObj);
            CtxFunc->ogluDeleteQueries(1, &Query);
        }
    }
    common::PromiseState State() noexcept
    {
        CheckCurrent();
        switch (CtxFunc->ogluClientWaitSync(SyncObj, 0, 0))
        {
        case GL_TIMEOUT_EXPIRED:
            return common::PromiseState::Executing;
        case GL_ALREADY_SIGNALED:
            return common::PromiseState::Success;
        case GL_WAIT_FAILED:
            return common::PromiseState::Error;
        case GL_CONDITION_SATISFIED:
            return common::PromiseState::Executed;
        case GL_INVALID_VALUE:
            [[fallthrough]];
        default:
            return common::PromiseState::Invalid;
        }
    }
    common::PromiseState Wait() noexcept
    {
        CheckCurrent();
        do
        {
            switch (CtxFunc->ogluClientWaitSync(SyncObj, 0, 1000'000'000))
            {
            case GL_TIMEOUT_EXPIRED:
                continue;
            case GL_ALREADY_SIGNALED:
                return common::PromiseState::Success;
            case GL_WAIT_FAILED:
                return common::PromiseState::Error;
            case GL_CONDITION_SATISFIED:
                return common::PromiseState::Executed;
            case GL_INVALID_VALUE:
                [[fallthrough]];
            default:
                return common::PromiseState::Invalid;
            }
        } while (true);
        return common::PromiseState::Invalid;
    }
    uint64_t ElapseNs() noexcept
    {
        if (TimeEnd == 0)
        {
            CheckCurrent();
            CtxFunc->ogluGetQueryObjectui64v(Query, GL_QUERY_RESULT, &TimeEnd);
        }
        if (TimeEnd == 0)
            return 0;
        else
            return TimeEnd - TimeBegin;
    }
};


template<typename T>
class COMMON_EMPTY_BASES oglPromise : public common::detail::PromiseResult_<T>, public oglPromiseCore
{
    friend class oglUtil;
protected:
    common::PromiseState State() noexcept override
    { 
        return oglPromiseCore::State();
    }
    common::PromiseState WaitPms() noexcept override
    {
        return oglPromiseCore::Wait();
    }
    T GetResult() override
    {
        this->CheckResultExtracted();
        return std::move(Result);
    }
public:
    T Result;
    template<typename U>
    oglPromise(U&& data) : Result(std::forward<U>(data)) { }
    ~oglPromise() override { }
    uint64_t ElapseNs() noexcept override
    { 
        return oglPromiseCore::ElapseNs();
    }
};


class COMMON_EMPTY_BASES oglPromiseVoid : public common::detail::PromiseResult_<void>, public oglPromiseCore
{
    friend class oglUtil;
protected:
    common::PromiseState GetState() noexcept override 
    { 
        return oglPromiseCore::State();
    }
    common::PromiseState WaitPms() noexcept override
    { 
        return oglPromiseCore::Wait();
    }
    void GetResult() override
    { }
public:
    oglPromiseVoid() { }
    ~oglPromiseVoid() override { }
    uint64_t ElapseNs() noexcept override
    { 
        return oglPromiseCore::ElapseNs();
    }
};


class COMMON_EMPTY_BASES oglPromiseVoid2 : public common::detail::PromiseResult_<void>
{
protected:
    common::PromiseState GetState() noexcept override
    {
        return common::PromiseState::Executed; // simply return, make it invoke wait
    }
    void PreparePms() override 
    { 
        glFinish();
    }
    common::PromiseState WaitPms() noexcept override
    {
        return common::PromiseState::Executed;
    }
    void GetResult() override
    { }
public:
};


}
