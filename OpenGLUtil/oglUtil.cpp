#include "oglPch.h"
#include "oglUtil.h"
#include "oglContext.h"
#include "oglProgram.h"
#include "oglPromise.hpp"
#include <mutex>


namespace oglu
{
using namespace std::string_view_literals;
using std::string;
using std::string_view;
using std::u16string;
using std::set;
using std::optional;
using std::vector;
using common::PromiseResult;
MAKE_ENABLER_IMPL(oglContext_)


GLHost::~GLHost() {}

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

auto& LoaderCreators() noexcept
{
    static std::vector<std::pair<std::string_view, std::function<std::unique_ptr<oglLoader>()>>> Creators;
    return Creators;
}
void detail::RegisterLoader(std::string_view name, std::function<std::unique_ptr<oglLoader>()> creator) noexcept
{
    LoaderCreators().emplace_back(name, std::move(creator));
}
common::span<const std::unique_ptr<oglLoader>> oglLoader::GetLoaders() noexcept
{
    static std::vector<std::unique_ptr<oglLoader>> Loaders = []() 
    {
        std::vector<std::unique_ptr<oglLoader>> loaders;
        for (const auto& [name, creator] : LoaderCreators())
        {
            try
            {
                loaders.emplace_back(creator());
            }
            catch (const common::BaseException& be)
            {
                oglLog().warning(u"Failed to create loader [{}]: {}\n{}\n", name, be.Message(), be.GetDetailMessage());
            }
            catch (const std::exception& ex)
            {
                oglLog().warning(u"Failed to create loader [{}]: {}\n", name, ex.what());
            }
            catch (...)
            {
                oglLog().warning(u"Failed to create loader [{}]\n", name);
            }
        }
        return loaders;
    }();
    return Loaders;
}

constexpr uint32_t DesktopVersion[] = { 46,45,44,43,42,41,40,33,32,31,30 };
constexpr uint32_t ESVersion[] = { 32, 31, 30, 20 };
oglContext GLHost::CreateContext(CreateInfo cinfo, const oglContext_* sharedCtx)
{
    Expects(!sharedCtx || sharedCtx->Host.get() == this);
    if (!CheckSupport(cinfo.Type))
        COMMON_THROWEX(OGLException, OGLException::GLComponent::Loader,
            fmt::format(u"Loader [{}] does not support [{}] context"sv, Loader.Name(), cinfo.Type == GLType::Desktop ? u"GL"sv : u"GLES"sv));
    if (cinfo.FlushWhenSwapContext && !SupportFlushControl)
        oglLog().warning(u"Request for FlushControl[{}] not supported, will be ignored\n"sv, cinfo.FlushWhenSwapContext.value());
    if (cinfo.FramebufferSRGB && !SupportSRGBFrameBuffer)
        oglLog().warning(u"Request for FrameBufferSRGB[{}] not supported, will be ignored\n"sv, cinfo.FramebufferSRGB.value());
    const auto Versions = cinfo.Type == GLType::Desktop ? common::to_span(DesktopVersion) : common::to_span(ESVersion);
    auto& LatestVer = cinfo.Type == GLType::Desktop ? VersionDesktop : VersionES;
    if (LatestVer == 0) // perform update
    {
        CreateInfo tmpCinfo;
        tmpCinfo.Type = cinfo.Type;
        for (const auto ver : Versions)
        {
            tmpCinfo.Version = ver;
            if (const auto ctx = CreateContext_(tmpCinfo, nullptr); ctx)
            {
                if (const auto binfo = FillBaseInfo(ctx); binfo)
                {
                    oglLog().info(u"Latest GL version [{}] from [{}]\n", binfo->VersionString, binfo->VendorString);
                    const uint16_t realVer = gsl::narrow_cast<uint16_t>(binfo->Version);
                    common::UpdateAtomicMaximum(LatestVer, realVer);
                    DeleteGLContext(ctx);
                    break;
                }
            }
            ReportFailure(fmt::format(u"test version [{}.{}] context"sv, ver / 10, ver % 10));
        }
    }
    if (cinfo.Version == 0)
        cinfo.Version = LatestVer.load();

    oglContext newCtx;
    const auto ctx = CreateContext_(cinfo, sharedCtx ? sharedCtx->Hrc : nullptr);
    if (ctx)
    {
        TemporalInsideContext(ctx, [&](const auto)
        {
            const auto ctxfunc = CtxFuncs::PrepareCurrent(*this, ctx, { cinfo.PrintFuncLoadSuccess, cinfo.PrintFuncLoadFail });
            newCtx = MAKE_ENABLER_SHARED(oglContext_, (sharedCtx ? sharedCtx->SharedCore : std::make_shared<detail::SharedContextCore>(), shared_from_this(), ctx, ctxfunc));
            oglContext_::PushToMap(newCtx);
        });
    }
    if (!newCtx)
        ReportFailure(u"create context");
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


}