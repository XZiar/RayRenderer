#include "RenderCoreRely.h"
#include "ModelImage.h"
#include "TextureUtil/TexCompressor.h"
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


static map<u16string, FakeTex> TEX_CACHE;


struct TexCompManExecutor : public NonCopyable, public NonMovable
{
    common::asyexe::AsyncManager Executor;
    TexCompManExecutor() : Executor(u"TexCompMan")
    {
        Executor.Start([] 
        { 
            common::SetThreadName(u"TexCompress");
            basLog().success(u"TexCompress thread start running.\n");
        });
    }
    ~TexCompManExecutor()
    {
        Executor.Stop();
    }
};


static common::PromiseResult<FakeTex> LoadImgToFakeTex(const fs::path& picPath, xziar::img::Image&& img, const TextureInnerFormat format)
{
    static TexCompManExecutor dummy;
    
    const auto w = img.GetWidth(), h = img.GetHeight();
    if (w <= 4 || h <= 4)
        COMMON_THROW(BaseException, u"image size to small");
    if (!IsPower2(w) || !IsPower2(h))
    {
        const auto newW = 1 << uint32_t(std::round(std::log2(w)));
        const auto newH = 1 << uint32_t(std::round(std::log2(h)));
        basLog().debug(u"decide to resize image[{}*{}] to [{}*{}].\n", w, h, newW, newH);
        auto newImg = xziar::img::Image(img);
        newImg.Resize(newW, newH, true, false);
        return LoadImgToFakeTex(picPath, std::move(img), format);
    }
    img.FlipVertical(); // pre-flip since after compression, OGLU won't care about vertical coordnate system

    return dummy.Executor.AddTask([img = std::move(img), format, picPath](const auto&) mutable
    {
        FakeTex tex;
        if (oglu::TexFormatUtil::IsCompressType(format))
        {
            try
            {
                auto dat = oglu::texutil::CompressToDat(img, format);
                tex = std::make_shared<detail::_FakeTex>(std::move(dat), format, img.GetWidth(), img.GetHeight());
            }
            catch (const BaseException& be)
            {
                basLog().error(u"Error when compress texture file [{}] into [{}]: {}\n", 
                    picPath.filename().u16string(), oglu::TexFormatUtil::GetFormatName(format), be.message);
            }
        }
        else
        {
            const auto width = img.GetWidth(), height = img.GetHeight();
            tex = std::make_shared<detail::_FakeTex>(std::move(img.ExtractData()), format, width, height);
        }
        if (tex)
        {
            tex->Name = picPath.filename().u16string();
            TEX_CACHE.try_emplace(picPath.u16string(), tex);
        }
        return tex;
    }, picPath.filename().u16string(), StackSize::Big);

}


std::optional<xziar::img::Image> ModelImage::ReadImage(const fs::path& picPath)
{
    try
    {
        return xziar::img::ReadImage(picPath);
    }
    catch (const FileException& fe)
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

ModelImage::LoadResult ModelImage::GetTexureAsync(const fs::path& picPath, const TextureInnerFormat format)
{
    if (auto tex = FindInMap(TEX_CACHE, picPath.u16string()))
        return *tex;
    if (auto img = ReadImage(picPath))
        return LoadImgToFakeTex(picPath, std::move(img.value()), format);
    return FakeTex();
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