#include "oglRely.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "MTWorker.h"
#include "common/PromiseTaskSTD.hpp"
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
    void virtual wait() override
    {
        while (glClientWaitSync(SyncObj, NULL, 1000'000'000) == GL_TIMEOUT_EXPIRED)
        { }
    }
public:
    PromiseResultGL()
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

class PromiseResultGL2 : public common::asyexe::detail::AsyncResult_<void>
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


static detail::MTWorker& getWorker(const uint8_t idx)
{
    static detail::MTWorker syncGL(u"SYNC"), asyncGL(u"ASYNC");
    return idx == 0 ? syncGL : asyncGL;
}

void oglUtil::init()
{
    glewInit();
    oglLog().info(u"GL Version:{}\n", getVersion());
    auto glctx = oglContext::CurrentContext();
#if defined(_DEBUG) || 1
    glctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
#endif
    const HDC hdc = (HDC)glctx->Hdc;
    const HGLRC hrc = (HGLRC)glctx->Hrc;
    wglMakeCurrent(hdc, nullptr);
    getWorker(0).start(oglContext::NewContext(glctx, true));
    getWorker(1).start(oglContext::NewContext(glctx, false));
    wglMakeCurrent(hdc, hrc);
    //for reverse-z
    glctx->SetDepthClip(true);
    glctx->SetDepthTest(DepthTestType::Greater);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

u16string oglUtil::getVersion()
{
    const auto str = (const char*)glGetString(GL_VERSION);
    const auto len = strlen(str);
    return u16string(str, str + len);
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
            /*const auto rMat = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
                Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
                Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x)));*/
            const auto rMat = Mat4x4(Mat3x3::RotateMatXYZ(op.vec));
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

using common::asyexe::StackSize;

PromiseResult<void> oglUtil::invokeSyncGL(const AsyncTaskFunc& task, const u16string& taskName, const StackSize stackSize)
{
    return getWorker(0).DoWork(task, taskName, static_cast<uint32_t>(stackSize));
}

PromiseResult<void> oglUtil::invokeAsyncGL(const AsyncTaskFunc& task, const u16string& taskName, const StackSize stackSize)
{
    return getWorker(1).DoWork(task, taskName, static_cast<uint32_t>(stackSize));
}

common::asyexe::AsyncResult<void> oglUtil::SyncGL()
{
    return std::static_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<detail::PromiseResultGL>());
}

common::asyexe::AsyncResult<void> oglUtil::ForceSyncGL()
{
    return std::static_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<detail::PromiseResultGL2>());
}

void oglUtil::TryTask()
{
    std::array<uint32_t, 128 * 128> empty{};
    oglTex2DS tex(128, 128, TextureInnerFormat::BC7);
    tex->SetData(TextureDataFormat::BGRA8, empty.data());
    oglTex2DArray texarr(128, 128, 1, TextureInnerFormat::BC7);
    texarr->SetTextureLayer(0, tex);
    glFlush();
    glFinish();
}


}