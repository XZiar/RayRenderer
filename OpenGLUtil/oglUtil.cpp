#include "oglRely.h"
#include "oglException.h"
#include "oglUtil.h"
#include "MTWorker.hpp"
#include "common/PromiseTask.inl"
#include <GL/wglew.h>

namespace oglu
{
using common::PromiseResultSTD;


namespace detail
{

class PromiseResultGL : public common::asyexe::detail::AsyncResult_<void>
{
    friend class oglUtil;
protected:
    GLsync SyncObj;
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
    void virtual wait() override
    {
        while (glClientWaitSync(SyncObj, NULL, 1000'000'000) == GL_TIMEOUT_EXPIRED)
        { }
        //glDeleteSync(SyncObj);
    }
public:
    PromiseResultGL(GLsync objSync_) : SyncObj(objSync_)
    {
        glFlush(); //ensure sync object sended
    }
    ~PromiseResultGL() 
    {
        glDeleteSync(SyncObj);
    }
};

class PromiseResultGL2 : public common::detail::PromiseResult_<void>
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


boost::circular_buffer<std::shared_ptr<oglu::DebugMessage>> oglUtil::msglist(512);
boost::circular_buffer<std::shared_ptr<oglu::DebugMessage>> oglUtil::errlist(128);

void GLAPIENTRY oglUtil::onMsg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    DebugMessage msg(source, type, severity);
    const DBGLimit& limit = *(DBGLimit*)userParam;
    if (((limit.src & (uint8_t)msg.from) != 0) 
        && ((limit.type & (uint16_t)msg.type) != 0) 
        && limit.minLV <= (uint8_t)msg.level)
    {
        auto theMsg = std::make_shared<DebugMessage>(msg);
        theMsg->msg.assign(message, message + length);
        
        msglist.push_back(theMsg);
        if (theMsg->type == MsgType::Error)
        {
            errlist.push_back(theMsg);
            oglLog().error(L"OpenGL ERROR\n{}\n", theMsg->msg);
        }
        else
        {
            oglLog().verbose(L"OpenGL message\n{}\n", theMsg->msg);
        }
    }
}

detail::MTWorker& getWorker(const uint8_t idx)
{
    static detail::MTWorker syncGL(L"SYNC"), asyncGL(L"ASYNC");
    return idx == 0 ? syncGL : asyncGL;
}

void oglUtil::init()
{
    glewInit();
#if defined(_DEBUG) || 1
    setDebug(0x2f, 0x1ff, MsgLevel::Notfication);
#endif
    oglLog().info(L"GL Version:{}\n", getVersion());
    auto hdc = wglGetCurrentDC();
    auto hrc = wglGetCurrentContext();
    int ctxAttrb[] =
    {
        /*WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,*/
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        0
    };
    getWorker(0).start(hdc, wglCreateContextAttribsARB(hdc, hrc, ctxAttrb));
    getWorker(1).start(hdc, wglCreateContextAttribsARB(hdc, NULL, ctxAttrb));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void oglUtil::setDebug(uint8_t src, uint16_t type, MsgLevel minLV)
{
    thread_local DBGLimit limit;
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    limit = { type, src, (uint8_t)minLV };
    glDebugMessageCallback(oglUtil::onMsg, &limit);
}

wstring oglUtil::getVersion()
{
    const auto str = (const char*)glGetString(GL_VERSION);
    const auto len = strlen(str);
    return wstring(str, str + len);
}

optional<wstring> oglUtil::getError()
{
    const auto err = glGetError();
    if(err == GL_NO_ERROR)
        return {};
    else
        return gluErrorUnicodeStringEXT(err);
}


void oglUtil::applyTransform(Mat4x4& matModel, const TransformOP& op)
{
    switch (op.type)
    {
    case TransformType::RotateXYZ:
        matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
            Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
            Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x))) * matModel;
        return;
    case TransformType::Rotate:
        matModel = Mat4x4(Mat3x3::RotateMat(op.vec)) * matModel;
        return;
    case TransformType::Translate:
        matModel = Mat4x4::TranslateMat(op.vec) * matModel;
        return;
    case TransformType::Scale:
        matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
        return;
    }
}

void oglUtil::applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op)
{
    switch (op.type)
    {
    case TransformType::RotateXYZ:
        {
            const auto rMat = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
                Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
                Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x)));
            matModel = rMat * matModel;
            matNormal = (Mat3x3)rMat * matNormal;
        }return;
    case TransformType::Rotate:
        {
            const auto rMat = Mat4x4(Mat3x3::RotateMat(op.vec));
            matModel = rMat * matModel;
            matNormal = (Mat3x3)rMat * matNormal;
        }return;
    case TransformType::Translate:
        {
            matModel = Mat4x4::TranslateMat(op.vec) * matModel;
        }return;
    case TransformType::Scale:
        {
            matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
        }return;
    }
}

PromiseResult<void> oglUtil::invokeSyncGL(const AsyncTaskFunc& task, const std::wstring& taskName)
{
    return getWorker(0).doWork(task, taskName);
}

PromiseResult<void> oglUtil::invokeAsyncGL(const AsyncTaskFunc& task, const std::wstring& taskName)
{
    return getWorker(1).doWork(task, taskName);
}

common::asyexe::AsyncResult<void> oglUtil::SyncGL()
{
    auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    return std::dynamic_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<detail::PromiseResultGL>(fence));
}

common::asyexe::AsyncResult<void> oglUtil::ForceSyncGL()
{
    return std::dynamic_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<detail::PromiseResultGL2>());
}


}