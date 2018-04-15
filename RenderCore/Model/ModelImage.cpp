#include "RenderCoreRely.h"
#include "ModelImage.h"
#include "../Material.h"
#include "common/PromiseTaskSTD.hpp"


namespace rayr::detail
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using common::asyexe::StackSize;
using namespace xziar::img;


static map<u16string, oglTex2DV> TEX_CACHE;

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