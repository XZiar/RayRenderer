#include "RenderCoreRely.h"
#include "ModelImage.h"
#include "../Material.h"
#include "TextureCompressor/TexCompressor.h"
#include "common/PromiseTaskSTD.hpp"
#include "common/AsyncExecutor/AsyncManager.h"
#include <thread>

namespace rayr::detail
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using common::asyexe::StackSize;
using oglu::TextureInnerFormat;
using namespace xziar::img;


static map<u16string, oglTex2DV> TEX_CACHE;


struct TexCompManExecutor : public NonCopyable
{
    common::asyexe::AsyncManager Executor;
    TexCompManExecutor() : Executor(u"TexCompMan")
    {
        std::thread([this]() { Executor.MainLoop(); }).detach();
    }
};



static common::PromiseResult<FakeTex> LoadImgToFakeTex(const fs::path& picPath, xziar::img::Image&& img, const TextureInnerFormat format)
{
    static TexCompManExecutor dummy;
    
    const auto w = img.Width, h = img.Height;
    if (w <= 4 || h <= 4)
        COMMON_THROW(BaseException, L"image size to small");
    if (!IsPower2(w) || !IsPower2(h))
    {
        const auto newW = 1 << uint32_t(std::round(std::log2(w)));
        const auto newH = 1 << uint32_t(std::round(std::log2(h)));
        basLog().debug(u"decide to resize image[{}*{}] to [{}*{}].\n", w, h, newW, newH);
        auto newImg = xziar::img::Image(img);
        newImg.Resize(newW, newH);
        return LoadImgToFakeTex(picPath, std::move(img), format);
    }

    const auto pms = std::make_shared<std::promise<FakeTex>>();
    auto ret = std::make_shared<common::PromiseResultSTD<FakeTex, true>>(*pms);

    dummy.Executor.AddTask([img = std::move(img), picPath, format, pms](const auto&)
    {
        auto dat = oglu::texcomp::CompressToDat(img, format);

        const auto tex = std::make_shared<detail::_FakeTex>(std::move(dat), picPath.filename().u16string(), format, img.Width, img.Height);
        pms->set_value(tex);
    }, u"LoadImgToFakeTex", StackSize::Big);

    return ret;
}


std::optional<xziar::img::Image> ModelImage::ReadImage(const fs::path& picPath)
{
    try
    {
        return xziar::img::ReadImage(picPath);
    }
    catch (FileException& fe)
    {
        if (fe.reason == FileException::Reason::ReadFail)
            basLog().error(u"Fail to read image file\t[{}]\n", picPath.u16string());
        else
            basLog().error(u"Cannot find image file\t[{}]\n", picPath.u16string());
    }
    catch (const BaseException&)
    {
        basLog().error(u"Fail to decode image file\t[{}]\n", picPath.u16string());
    }
    return {};
}

oglTex2DV ModelImage::GetTexure(const fs::path& picPath, const xziar::img::Image& img)
{
    const auto tex = MultiMaterialHolder::LoadImgToTex(img, oglu::TextureInnerFormat::RGBA8)->GetTextureView();
    tex->Name = picPath.filename().u16string();
    TEX_CACHE.try_emplace(picPath.u16string(), tex);
    return tex;
}

oglTex2DV ModelImage::GetTexure(const fs::path& picPath)
{
    if (auto tex = FindInMap(TEX_CACHE, picPath.u16string()))
        return *tex;
    if (const auto img = ReadImage(picPath))
        return GetTexure(picPath, img.value());
    else
        return oglTex2DV();
}

ModelImage::LoadResult ModelImage::GetTexureAsync(const fs::path& picPath)
{
    if (auto tex = FindInMap(TEX_CACHE, picPath.u16string()))
        return *tex;
    if (const auto img = ReadImage(picPath))
    {
        const auto pms = std::make_shared<std::promise<oglTex2DV>>();
        auto ret = std::make_shared<common::PromiseResultSTD<oglTex2DV, true>>(*pms);
        oglu::oglUtil::invokeSyncGL([img = std::move(img.value()), fpath = std::move(picPath), pms](const auto& agent)
        {
            const auto tex = GetTexure(fpath, img);
            agent.Await(oglu::oglUtil::SyncGL());
            pms->set_value(tex);
        }, u"GetImage");
        return ret;
    }
    return oglTex2DV();
}

#pragma warning(disable:4996)
void ModelImage::Shrink()
{
    auto it = TEX_CACHE.cbegin();
    while (it != TEX_CACHE.end())
    {
        if (it->second.unique()) //deprecated, may lead to bug
            it = TEX_CACHE.erase(it);
        else
            ++it;
    }
}
#pragma warning(default:4996)


}