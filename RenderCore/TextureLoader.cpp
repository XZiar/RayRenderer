#include "RenderCorePch.h"
#include "TextureLoader.h"
#include "TextureUtil/TexCompressor.h"
#include "TextureUtil/TexMipmap.h"
#include "AsyncExecutor/AsyncManager.h"
#include <thread>

namespace rayr
{
using std::set;
using std::vector;
using common::file::FileErrReason;
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using common::asyexe::StackSize;
using xziar::img::TextureFormat;
using namespace xziar::img;


static std::vector<std::pair<int32_t, std::u16string>> ProcPairs = 
{ 
    {(int32_t)TexProcType::Plain,       u"Plain"},
    {(int32_t)TexProcType::CompressBC5, u"BC5"},
    {(int32_t)TexProcType::CompressBC7, u"BC7"}
};
static const u16string TextureLoaderName = u"纹理加载";
void TextureLoader::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue)
        .RegistObject<false>(TextureLoaderName);
    Controllable::EnumSet<int32_t> EnumProcType(ProcPairs);
    RegistItem<bool>("color_mipmap", "Color", u"Mipmap")
        .RegistMemberProxy<TextureLoader>([](auto& control) -> auto&
    { return control.ProcessMethod.find(TexLoadType::Color)->second.NeedMipmap; });
    RegistItem<int32_t>("color_type", "Color", u"Type", ArgType::Enum, EnumProcType)
        .RegistMemberProxy<TextureLoader>([](auto& control) -> auto&
    { return control.ProcessMethod.find(TexLoadType::Color)->second.Proc; });

    RegistItem<bool>("normal_mipmap", "Normal", u"Mipmap")
        .RegistMemberProxy<TextureLoader>([](auto& control) -> auto&
    { return control.ProcessMethod.find(TexLoadType::Normal)->second.NeedMipmap; });
    RegistItem<int32_t>("normal_type", "Normal", u"Type", ArgType::Enum, EnumProcType)
        .RegistMemberProxy<TextureLoader>([](auto& control) -> auto&
    { return control.ProcessMethod.find(TexLoadType::Normal)->second.Proc; });
}

TextureLoader::TextureLoader(const std::shared_ptr<oglu::texutil::TexMipmap>& mipmapper) 
    : Compressor(std::make_unique<common::asyexe::AsyncManager>(u"TexCompMan")), MipMapper(mipmapper)
{
    ProcessMethod = 
    { 
        { TexLoadType::Color,  TexProc{TexProcType::CompressBC7, true} },
        { TexLoadType::Normal, TexProc{TexProcType::CompressBC5, true} }
    };
    Compressor->Start([]
    {
        common::SetThreadName(u"TexCompress");
        dizzLog().success(u"TexCompress thread start running.\n");
    });
    RegistControllable();
}
TextureLoader::~TextureLoader()
{
    Compressor->Stop();
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
        img.Resize(newW, newH, true, false);
        return LoadImgToFakeTex(picPath, std::move(img), type, proc);
    }
    img.FlipVertical(); // pre-flip since after compression, OGLU won't care about vertical coordnate system

    return Compressor->AddTask([this, imgview = ImageView(std::move(img)), type, proc, picPath](const auto& agent) mutable
    {
        FakeTex tex;
        vector<ImageView> layers;
        if (proc.NeedMipmap)
        {
            const auto pms = MipMapper->GenerateMipmaps(imgview, type == TexLoadType::Color);
            layers = common::linq::FromContainer(agent.Await(pms)).template Cast<ImageView>().ToVector();
        }
        layers.insert(layers.begin(), imgview);
        xziar::img::TextureFormat format = xziar::img::TextureFormat::EMPTY,
            srgbMask = (type == TexLoadType::Color ? xziar::img::TextureFormat::MASK_SRGB : xziar::img::TextureFormat::EMPTY);
        switch (proc.Proc)
        {
        case TexProcType::Plain:
            format = xziar::img::TextureFormat::RGBA8 | srgbMask; break;
        case TexProcType::CompressBC5:
            format = xziar::img::TextureFormat::BC5;              break;
        case TexProcType::CompressBC7:
            format = xziar::img::TextureFormat::BC7   | srgbMask; break;
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
            CacheLock.LockWrite();
            TexCache.try_emplace(picPath.u16string(), tex);
            CacheLock.UnlockWrite();
        }
        catch (const BaseException& be)
        {
            dizzLog().error(u"Error when compress texture file [{}] into [{}]: {}\n",
                picPath.filename().u16string(), xziar::img::TexFormatUtil::GetFormatName(format), be.Message());
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
    catch (const common::file::FileException& fe)
    {
        if (fe.reason == (FileErrReason::OpenFail | FileErrReason::NotExist))
            dizzLog().error(u"Cannot find image file\t[{}]\n", picPath.u16string());
        else
            dizzLog().error(u"Fail to read image file\t[{}]\n", picPath.u16string());
    }
    catch (const BaseException&)
    {
        dizzLog().error(u"Fail to decode image file\t[{}]\n", picPath.u16string());
    }
    return {};
}

TextureLoader::LoadResult TextureLoader::GetTexureAsync(const fs::path& picPath, const TexLoadType type, const bool async)
{
    CacheLock.LockRead();
    auto tex = FindInMap(TexCache, picPath.u16string());
    CacheLock.UnlockRead();
    if (tex)
        return *tex;
    if (async)
    {
        return Compressor->AddTask([this, picPath, type](const auto& agent)
        {
            if (auto img = TryReadImage(picPath); img)
                return agent.Await(LoadImgToFakeTex(picPath, std::move(img.value()), type, ProcessMethod[type]));
            else
                return FakeTex();
        });
    }
    if (auto img = TryReadImage(picPath); img)
        return LoadImgToFakeTex(picPath, std::move(img.value()), type, ProcessMethod[type]);
    return FakeTex();
}

TextureLoader::LoadResult TextureLoader::GetTexureAsync(const fs::path& picPath, Image&& img, const TexLoadType type, const bool async)
{
    CacheLock.LockRead();
    auto tex = FindInMap(TexCache, picPath.u16string());
    CacheLock.UnlockRead();
    if (tex)
        return *tex;
    if (async)
    {
        return Compressor->AddTask([this, picPath, img = std::move(img), type](const auto& agent) mutable
        {
            return agent.Await(LoadImgToFakeTex(picPath, std::move(img), type, ProcessMethod[type]));
        });
    }
    return LoadImgToFakeTex(picPath, std::move(img), type, ProcessMethod[type]);
}

#pragma warning(disable:4996)
void TextureLoader::Shrink()
{
    auto it = TexCache.cbegin();
    while (it != TexCache.end())
    {
        if (it->second.use_count() == 1) //deprecated, may lead to bug
            it = TexCache.erase(it);
        else
            ++it;
    }
}
#pragma warning(default:4996)


}