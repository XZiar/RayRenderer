#include "oglPch.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "oglPromise.hpp"


namespace oglu
{
using std::string;
using std::string_view;
using std::u16string;
using std::set;
using std::optional;
using std::vector;
using common::PromiseResult;


void SetFuncShouldPrint(const bool printSuc, const bool printFail) noexcept;

void oglUtil::InJectRenderDoc(const common::fs::path& dllPath)
{
    PlatFuncs::InJectRenderDoc(dllPath);
}

void oglUtil::InitGLEnvironment()
{
    PlatFuncs::InitEnvironment();
}
void oglUtil::SetFuncLoadingDebug(const bool printSuc, const bool printFail) noexcept
{
    SetFuncShouldPrint(printSuc, printFail);
}

void oglUtil::InitLatestVersion()
{
    constexpr uint32_t VERSIONS[] = { 46,45,44,43,42,41,40,33,32,31,30 };
    const auto glctx = oglContext_::Refresh();
    oglLog().info(u"GL Version:{}\n", GetVersionStr());
    for (const auto ver : VERSIONS)
    {
        const auto ctx = oglContext_::NewContext(glctx, false, ver);
        if (ctx)
        {
            ctx->UseContext();
            oglLog().info(u"Latest GL Version:{}\n", GetVersionStr());
            glctx->UseContext();
            break;
        }
    }
}
u16string oglUtil::GetVersionStr()
{
    const auto str = (const char*)CtxFunc->ogluGetString(GL_VERSION);
    const auto len = strlen(str);
    return u16string(str, str + len);
}

optional<string_view> oglUtil::GetError()
{
    return CtxFunc->GetError();
}


//void oglUtil::applyTransform(Mat4x4& matModel, const TransformOP& op)
//{
//    switch (op.type)
//    {
//    case TransformType::RotateXYZ:
//        matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
//            Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
//            Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x))) * matModel;
//        return;
//    case TransformType::Rotate:
//        matModel = Mat4x4(Mat3x3::RotateMat(op.vec)) * matModel;
//        return;
//    case TransformType::Translate:
//        matModel = Mat4x4::TranslateMat(op.vec) * matModel;
//        return;
//    case TransformType::Scale:
//        matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
//        return;
//    }
//}
//
//void oglUtil::applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op)
//{
//    switch (op.type)
//    {
//    case TransformType::RotateXYZ:
//        {
//            /*const auto rMat = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
//                Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
//                Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x)));*/
//            const auto rMat = Mat4x4(Mat3x3::RotateMatXYZ(op.vec));
//            matModel = rMat * matModel;
//            matNormal = (Mat3x3)rMat * matNormal;
//        }return;
//    case TransformType::Rotate:
//        {
//            const auto rMat = Mat4x4(Mat3x3::RotateMat(op.vec));
//            matModel = rMat * matModel;
//            matNormal = (Mat3x3)rMat * matNormal;
//        }return;
//    case TransformType::Translate:
//        {
//            matModel = Mat4x4::TranslateMat(op.vec) * matModel;
//        }return;
//    case TransformType::Scale:
//        {
//            matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
//        }return;
//    }
//}


PromiseResult<void> oglUtil::SyncGL()
{
    return std::make_shared<oglPromiseVoid>();
}

PromiseResult<void> oglUtil::ForceSyncGL()
{
    return std::make_shared<oglPromiseVoid2>();
}


}