#include "oglRely.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "oglPromise.hpp"
namespace oglu
{


constexpr uint32_t VERSIONS[] = { 46,45,44,43,42,41,40,33,32,31,30 };
void oglUtil::Init(const bool initLatestVer)
{
    oglLog().info(u"GL Version:{}\n", GetVersionStr());
    oglContext::BasicInit();
    const auto glctx = oglContext::Refresh();
    if (initLatestVer)
    {
        for (const auto ver : VERSIONS)
        {
            const auto ctx = oglContext::NewContext(glctx, false, ver);
            if (ctx)
            {
                ctx->UseContext();
                oglLog().info(u"Latest GL Version:{}\n", GetVersionStr());
                glctx->UseContext();
                break;
            }
        }
    }
#if defined(_DEBUG) || 1
    glctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
#endif
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
u16string oglUtil::GetVersionStr()
{
    const auto str = (const char*)glGetString(GL_VERSION);
    const auto len = strlen(str);
    return u16string(str, str + len);
}

optional<string_view> oglUtil::GetError()
{
    const auto err = glGetError();
    switch (err)
    {
    case GL_NO_ERROR:                       return {};
    case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
    case GL_STACK_UNDERFLOW:                return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:                 return "GL_STACK_OVERFLOW";
    default:                                return "UNKNOWN ERROR";
    }
}

set<string_view, std::less<>> oglUtil::GetExtensions()
{
    set<string_view, std::less<>> exts;
    int32_t count;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for(int32_t i = 0; i < count; i++) 
    {
        const GLubyte *ext = glGetStringi(GL_EXTENSIONS, i);
        exts.emplace(reinterpret_cast<const char*>(ext));
    }
    return exts;
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


PromiseResult<void> oglUtil::SyncGL()
{
    return std::make_shared<oglPromiseVoid>();
}

PromiseResult<void> oglUtil::ForceSyncGL()
{
    return std::make_shared<oglPromiseVoid2>();
}

void oglUtil::MemBarrier(const GLMemBarrier mbar)
{
    glMemoryBarrier((GLenum)mbar);
}


}