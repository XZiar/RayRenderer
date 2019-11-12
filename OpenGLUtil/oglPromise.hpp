#pragma once
#include "oglPch.h"
#include "AsyncExecutor/AsyncAgent.h"

namespace oglu
{

class oglPromiseCore : public detail::oglCtxObject<true>
{
protected:
    GLsync SyncObj;
    uint64_t TimeBegin = 0, TimeEnd = 0;
    GLuint Query;
    oglPromiseCore()
    {
        DSA->ogluGenQueries(1, &Query);
        DSA->ogluGetInteger64v(GL_TIMESTAMP, (GLint64*)&TimeBegin); //suppose it is the time all commands are issued.
        DSA->ogluQueryCounter(Query, GL_TIMESTAMP); //this should be the time all commands are completed.
        SyncObj = DSA->ogluFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        DSA->ogluFlush(); //ensure sync object sended
    }
    ~oglPromiseCore()
    {
        if (EnsureValid())
        {
            DSA->ogluDeleteSync(SyncObj);
            DSA->ogluDeleteQueries(1, &Query);
        }
    }
    common::PromiseState State()
    {
        CheckCurrent();
        switch (DSA->ogluClientWaitSync(SyncObj, 0, 0))
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
    void Wait()
    {
        CheckCurrent();
        while (DSA->ogluClientWaitSync(SyncObj, 0, 1000'000'000) == GL_TIMEOUT_EXPIRED)
        { }
    }
    uint64_t ElapseNs()
    {
        if (TimeEnd == 0)
        {
            CheckCurrent();
            DSA->ogluGetQueryObjectui64v(Query, GL_QUERY_RESULT, &TimeEnd);
        }
        if (TimeEnd == 0)
            return 0;
        else
            return TimeEnd - TimeBegin;
    }
};


template<typename T>
class oglPromise : public common::detail::PromiseResult_<T>, public oglPromiseCore
{
    friend class oglUtil;
protected:
    common::PromiseState virtual State() override 
    { 
        return oglPromiseCore::State();
    }
    T virtual WaitPms() override
    {
        oglPromiseCore::Wait();
        return std::move(Result);
    }
public:
    T Result;
    template<typename U>
    oglPromise(U&& data) : Result(std::forward<U>(data)) { }
    ~oglPromise() override { }
    virtual uint64_t ElapseNs() override 
    { 
        return oglPromiseCore::ElapseNs();
    }
};


class oglPromiseVoid : public common::detail::PromiseResult_<void>, public oglPromiseCore
{
    friend class oglUtil;
protected:
    common::PromiseState virtual State() override 
    { 
        return oglPromiseCore::State();
    }
    void virtual WaitPms() override 
    { 
        oglPromiseCore::Wait();
    }
public:
    oglPromiseVoid() { }
    ~oglPromiseVoid() override { }
    virtual uint64_t ElapseNs() override 
    { 
        return oglPromiseCore::ElapseNs();
    }
};


class oglPromiseVoid2 : public common::detail::PromiseResult_<void>
{
protected:
    common::PromiseState virtual State() override
    {
        return common::PromiseState::Executed; // simply return, make it invoke wait
    }
    virtual void PreparePms() override 
    { 
        glFinish();
    }
    void virtual WaitPms() override
    { }
public:
};


}
