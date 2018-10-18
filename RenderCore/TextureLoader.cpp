#include "RenderCoreRely.h"
#include "TextureLoader.h"
#include "TextureUtil/TexCompressor.h"
#include "TextureUtil/TexMipmap.h"
#include "common/PromiseTaskSTD.hpp"
#include "common/AsyncExecutor/AsyncManager.h"
#include <thread>

namespace rayr
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using common::asyexe::StackSize;
using oglu::TextureInnerFormat;
using namespace xziar::img;


void TextureLoader::RegistControllable()
{

}

TextureLoader::TextureLoader(const std::shared_ptr<oglu::texutil::TexMipmap>& mipmapper) 
    : Compressor(u"TexCompMan"), MipMapper(mipmapper)
{
    ProcessMethod = 
    { 
        { TexLoadType::Color,  TexProc{TexProcType::CompressBC7, true} },
        { TexLoadType::Normal, TexProc{TexProcType::CompressBC5, true} }
    };
    Compressor.Start([]
    {
        common::SetThreadName(u"TexCompress");
        dizzLog().success(u"TexCompress thread start running.\n");
    });
    RegistControllable();
}
TextureLoader::~TextureLoader()
{
    Compressor.Stop();
}


common::PromiseResult<FakeTex> TextureLoader::LoadImgToFakeTex(const fs::path& picPath, Image&& img, const TexLoadType type, const TexProc proc)
{
    const auto w = img.GetWidth(), h = img.GetHeight();
    if (w <= 4 || h <= 4)
        COMMON_THROWEX(BaseException, u"image size to small");
    if (!IsPower2(w) || !IsPower2(h))
    {
        const auto newW = 1 << uint32_t(std::round(std::log2(w)));
        const auto newH = 1 << uint32_t(std::round(std::log2(h)));
        dizzLog().debug(u"decide to resize image[{}*{}] to [{}*{}].\n", w, h, newW, newH);
        auto newImg = Image(img);
        newImg.Resize(newW, newH, true, false);
        return LoadImgToFakeTex(picPath, std::move(img), type, proc);
    }
    img.FlipVertical(); // pre-flip since after compression, OGLU won't care about vertical coordnate system

    return Compressor.AddTask([this, imgview = ImageView(std::move(img)), type, proc, picPath](const auto& agent) mutable
    {
        FakeTex tex;
        vector<Image> layers;
        if (proc.NeedMipmap)
        {
            const auto pms = MipMapper->GenerateMipmaps((const Image&)imgview, type == TexLoadType::Color);
            layers = agent.Await(pms);
        }
        layers.insert(layers.begin(), (const Image&)imgview);
        TextureInnerFormat format = TextureInnerFormat::EMPTY_MASK,
            srgbMask = (type == TexLoadType::Color ? TextureInnerFormat::FLAG_SRGB : TextureInnerFormat::EMPTY_MASK);
        switch (proc.Proc)
        {
        case TexProcType::Plain:
            format = TextureInnerFormat::RGBA8 | srgbMask; break;
        case TexProcType::CompressBC5:
            format = TextureInnerFormat::BC5; break;
        case TexProcType::CompressBC7:
            format = TextureInnerFormat::BC7 | srgbMask; break;
        default:
            break;
        }
        try 
        {
            vector<common::AlignedBuffer> buffers;
            for (auto& layer : layers)
            {
                switch (proc.Proc)
                {
                case TexProcType::Plain:
                    buffers.emplace_back(layer.ExtractData()); break;
                case TexProcType::CompressBC5:
                    buffers.emplace_back(oglu::texutil::CompressToDat(layer, format)); break;
                case TexProcType::CompressBC7:
                    buffers.emplace_back(oglu::texutil::CompressToDat(layer, format)); break;
                default:
                    break;
                }
            }
            tex = std::make_shared<detail::_FakeTex>(std::move(buffers), format, imgview.GetWidth(), imgview.GetHeight());
            tex->Name = picPath.filename().u16string();
            TexCache.try_emplace(picPath.u16string(), tex);
        }
        catch (const BaseException& be)
        {
            dizzLog().error(u"Error when compress texture file [{}] into [{}]: {}\n",
                picPath.filename().u16string(), oglu::TexFormatUtil::GetFormatName(format), be.message);
        }
        return tex;
    }, picPath.filename().u16string(), StackSize::Big);
}


std::optional<Image> TryReadImage(const fs::path& picPath)
{
    try
    {
        return ReadImage(picPath);
    }
    catch (const FileException& fe)
    {
        if (fe.reason == FileException::Reason::ReadFail)
            dizzLog().error(u"Fail to read image file\t[{}]\n", picPath.u16string());
        else
            dizzLog().error(u"Cannot find image file\t[{}]\n", picPath.u16string());
    }
    catch (const BaseException&)
    {
        dizzLog().error(u"Fail to decode image file\t[{}]\n", picPath.u16string());
    }
    return {};
}

TextureLoader::LoadResult TextureLoader::GetTexureAsync(const fs::path& picPath, const TexLoadType type)
{
    if (auto tex = FindInMap(TexCache, picPath.u16string()))
        return *tex;
    if (auto img = TryReadImage(picPath))
        return LoadImgToFakeTex(picPath, std::move(img.value()), type, ProcessMethod[type]);
    return FakeTex();
}

#pragma warning(disable:4996)
void TextureLoader::Shrink()
{
    auto it = TexCache.cbegin();
    while (it != TexCache.end())
    {
        if (it->second.unique()) //deprecated, may lead to bug
            it = TexCache.erase(it);
        else
            ++it;
    }
}
#pragma warning(default:4996)


}