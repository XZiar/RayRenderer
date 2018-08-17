#include "oglRely.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "oglPromise.hpp"
#include "MTWorker.h"
#if defined(_WIN32)
#   include "glew/wglew.h"
#else
#   define GLEW_NO_GLU
#   include "glew/glxew.h"
#endif
namespace oglu
{
using common::PromiseResultSTD;


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
    glctx->UnloadContext();
    //const HDC hdc = (HDC)glctx->Hdc;
    //const HGLRC hrc = (HGLRC)glctx->Hrc;
    //wglMakeCurrent(hdc, nullptr);
    getWorker(0).start(oglContext::NewContext(glctx, true));
    getWorker(1).start(oglContext::NewContext(glctx, false));
    glctx->UseContext();
    //wglMakeCurrent(hdc, hrc);
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

optional<u16string> oglUtil::getError()
{
    const auto err = glGetError();
    if (err == GL_NO_ERROR)
        return {};
    else
#if defined(_WIN32)
        return u16string(reinterpret_cast<const char16_t*>(gluErrorUnicodeStringEXT(err)));
#else
        return str::to_u16string(std::to_string(err));
#endif
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
    return std::static_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<PromiseResultGLVoid>());
}

common::asyexe::AsyncResult<void> oglUtil::ForceSyncGL()
{
    return std::static_pointer_cast<common::asyexe::detail::AsyncResult_<void>>(std::make_shared<PromiseResultGLVoid2>());
}

void oglUtil::MemBarrier(const GLMemBarrier mbar)
{
    glMemoryBarrier((GLenum)mbar);
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