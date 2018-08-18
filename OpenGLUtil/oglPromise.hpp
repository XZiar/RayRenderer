#pragma once
#include "oglRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"

namespace oglu
{

template<typename T>
class PromiseResultGL : public common::detail::PromiseResult_<T>
{
    friend class oglUtil;
    friend class PromiseResultGLVoid;
protected:
    T Result;
    GLsync SyncObj;
    uint64_t TimeBegin;
    GLuint Query;
    common::PromiseState virtual state() override
    {
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
    T virtual wait() override
    {
        while (glClientWaitSync(SyncObj, 0, 1000'000'000) == GL_TIMEOUT_EXPIRED)
        {
        }
        return Result;
    }
public:
    template<typename U>
    PromiseResultGL(U&& data) : Result(std::forward<U>(data))
    {
        glGenQueries(1, &Query);
        glGetInteger64v(GL_TIMESTAMP, (GLint64*)&TimeBegin); //suppose it is the time all commands are issued.
        glQueryCounter(Query, GL_TIMESTAMP); //this should be the time all commands are completed.
        SyncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush(); //ensure sync object sended
    }
    ~PromiseResultGL() override
    {
        glDeleteSync(SyncObj);
        glDeleteQueries(1, &Query);
    }
    uint64_t ElapseNs() override
    {
        uint64_t timeEnd = 0;
        glGetQueryObjectui64v(Query, GL_QUERY_RESULT, &timeEnd);
        if (timeEnd == 0)
            return 0;
        else
            return timeEnd - TimeBegin;
    }
};


class PromiseResultGLVoid : public common::detail::PromiseResult_<void>
{
protected:
    PromiseResultGL<int> Promise;
    common::PromiseState virtual state() override
    {
        return Promise.state();
    }
public:
    PromiseResultGLVoid() : Promise(0) {}
    ~PromiseResultGLVoid() override {}
    void virtual wait() override
    {
        Promise.wait();
    }
};
class PromiseResultGLVoid2 : public common::detail::PromiseResult_<void>
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
