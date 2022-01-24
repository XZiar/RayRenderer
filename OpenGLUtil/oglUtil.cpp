#include "oglPch.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "oglPromise.hpp"
#include <mutex>


namespace oglu
{
using std::string;
using std::string_view;
using std::u16string;
using std::set;
using std::optional;
using std::vector;
using common::PromiseResult;
MAKE_ENABLER_IMPL(oglContext_)


GLHost_::~GLHost_() {}

struct oglLoader::Pimpl
{
    std::once_flag InitFlag;
};
oglLoader::oglLoader() : Impl(std::make_unique<Pimpl>()) {}
oglLoader::~oglLoader() {}

//void oglLoader::InitEnvironment()
//{
//    std::call_once(Impl->InitFlag, [&]() { this->Init(); });
//}

auto& AllLoaders() noexcept
{
    static std::vector<std::unique_ptr<oglLoader>> Loaders;
    return Loaders;
}
void oglLoader::LogError(const common::BaseException& be) noexcept
{
    oglLog().warning(u"Failed to create loader: {}\n", be.Message());
}
oglLoader* oglLoader::RegisterLoader(std::unique_ptr<oglLoader> loader) noexcept
{
    const auto ptr = loader.get();
    AllLoaders().emplace_back(std::move(loader));
    return ptr;
}
common::span<const std::unique_ptr<oglLoader>> oglLoader::GetLoaders() noexcept
{
    return AllLoaders();
}

oglContext oglLoader::CreateContext_(const GLHost& host, CreateInfo cinfo, const oglContext_* sharedCtx) noexcept
{
    Expects(!sharedCtx || sharedCtx->Host == host);
    constexpr uint32_t DesktopVersion[] = { 46,45,44,43,42,41,40,33,32,31,30 };
    constexpr uint32_t ESVersion[] = { 32, 31, 30, 20 };
    const auto Versions = cinfo.Type == GLType::Desktop ? common::to_span(DesktopVersion) : common::to_span(ESVersion);
    auto& LatestVer = cinfo.Type == GLType::Desktop ? host->VersionDesktop : host->VersionES;
    const auto targetVer = cinfo.Version;
    if (LatestVer == 0) // perform update
    {
        for (const auto ver : Versions)
        {
            cinfo.Version = ver;
            if (const auto ctx = CreateContext(host, cinfo, nullptr); ctx)
            {
                std::optional<ContextBaseInfo> binfo;
                host->TemporalInsideContext(ctx, [&](const auto) 
                    {
                        binfo.emplace();
                        FillCurrentBaseInfo(binfo.value());
                    });
                if (binfo.has_value())
                {
                    oglLog().info(u"Latest GL version [{}] from [{}]\n", binfo->VersionString, binfo->VendorString);
                    const uint16_t realVer = gsl::narrow_cast<uint16_t>(binfo->Version);
                    common::UpdateAtomicMaximum(LatestVer, realVer);
                    host->DeleteGLContext(ctx);
                    break;
                }
            }
        }
    }
    if (targetVer == 0)
        cinfo.Version = LatestVer.load();

    oglContext newCtx;
    const auto ctx = CreateContext(host, cinfo, sharedCtx ? sharedCtx->Hrc : nullptr);
    if (ctx)
    {
        host->TemporalInsideContext(ctx, [&](const auto)
            {
                const auto ctxfunc = CtxFuncs::PrepareCurrent(*host, ctx, { cinfo.PrintFuncLoadSuccess, cinfo.PrintFuncLoadFail });
                newCtx = MAKE_ENABLER_SHARED(oglContext_, (sharedCtx ? sharedCtx->SharedCore : std::make_shared<detail::SharedContextCore>(), host, ctx, ctxfunc));
                oglContext_::PushToMap(newCtx);
            });
    }
    if (!newCtx)
        host->ReportFailure(u"create context");
    return newCtx;
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
    return std::make_shared<oglPromise<void>>(common::detail::EmptyDummy{});
}

PromiseResult<void> oglUtil::ForceSyncGL()
{
    return std::make_shared<oglFinishPromise>();
}


}