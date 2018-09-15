#pragma once
#include "oglRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"

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
        glGenQueries(1, &Query);
        glGetInteger64v(GL_TIMESTAMP, (GLint64*)&TimeBegin); //suppose it is the time all commands are issued.
        glQueryCounter(Query, GL_TIMESTAMP); //this should be the time all commands are completed.
        SyncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush(); //ensure sync object sended
    }
    ~oglPromiseCore()
    {
        if (EnsureValid())
        {
            glDeleteSync(SyncObj);
            glDeleteQueries(1, &Query);
        }
    }
    common::PromiseState State()
    {
        CheckCurrent();
        switch (glClientWaitSync(SyncObj, 0, 0))
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
        while (glClientWaitSync(SyncObj, 0, 1000'000'000) == GL_TIMEOUT_EXPIRED)
        { }
    }
    uint64_t ElapseNs()
    {
        if (TimeEnd == 0)
        {
            CheckCurrent();
            glGetQueryObjectui64v(Query, GL_QUERY_RESULT, &TimeEnd);
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
    common::PromiseState virtual state() override { return State(); }
    T virtual wait() override
    {
        Wait();
        return std::move(Result);
    }
public:
    T Result;
    template<typename U>
    oglPromise(U&& data) : Result(std::forward<U>(data)) { }
    ~oglPromise() override { }
    uint64_t ElapseNs() override { return oglPromiseCore::ElapseNs(); }
};


class oglPromiseVoid : public common::detail::PromiseResult_<void>, public oglPromiseCore
{
    friend class oglUtil;
protected:
    common::PromiseState virtual state() override { return State(); }
    void virtual wait() override { Wait(); }
public:
    oglPromiseVoid() { }
    ~oglPromiseVoid() override { }
    uint64_t ElapseNs() override { return oglPromiseCore::ElapseNs(); }
};


class oglPromiseVoid2 : public common::detail::PromiseResult_<void>
{
protected:
    common::PromiseState virtual state() override
    {
        return common::PromiseState::Executed; // simply return, make it invoke wait
    }
public:
    void virtual wait() override
    {
        glFinish();
    }
};


}
