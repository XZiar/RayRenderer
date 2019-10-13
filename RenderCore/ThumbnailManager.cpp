#include "RenderCoreRely.h"
#include "ThumbnailManager.h"
#include "common/PromiseTaskSTD.hpp"
#include "OpenGLUtil/oglWorker.h"
#include "TextureUtil/TexResizer.h"


namespace rayr
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using oglu::oglTex2D;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::ImageDataType;
using oglu::texutil::ResizeMethod;


static std::variant<uint8_t, std::pair<uint16_t, uint16_t>> CalcSize(const TexHolder& holder)
{
    constexpr uint16_t thredshold = 128;
    auto size = holder.GetSize();
    const auto mipmap = holder.GetMipmapCount();
    for (uint8_t i = 0; i < mipmap;)
    {
        const auto larger = std::max(size.first, size.second);
        if (larger <= thredshold)
            return i;
        if (++i < mipmap)
            size.first /= 2, size.second /= 2;
    }
    const auto larger = std::max(size.first, size.second);
    return std::pair(static_cast<uint16_t>(size.first * thredshold / larger), static_cast<uint16_t>(size.second * thredshold / larger));
}

common::PromiseResult<std::optional<ImageView>> ThumbnailManager::GetThumbnail(const TexHolder& holder)
{
    if(holder.index() == 0)
        return common::FinishedResult<std::optional<ImageView>>::Get(std::optional<ImageView>{});
    CacheLock.LockRead();
    auto ret = FindInMap(ThumbnailMap, holder.GetWeakRef(), std::in_place);
    CacheLock.UnlockRead();
    if (ret.has_value())
        return common::FinishedResult<std::optional<ImageView>>::Get(std::move(ret));
    return InnerPrepareThumbnail(holder);
}

common::PromiseResult<std::optional<ImageView>> ThumbnailManager::InnerPrepareThumbnail(const TexHolder& holder)
{
    return GLWorker->InvokeShare([holder, this](const auto& agent) -> std::optional<ImageView>
    {
        auto weakref = holder.GetWeakRef();
        const auto sizeRet = CalcSize(holder);
        if (sizeRet.index() == 0) // no need
        {
            switch (holder.index())
            {
            case 1:
                {
                    const auto& tex = std::get<oglTex2D>(holder);
                    ImageView img(tex->GetImage(ImageDataType::RGB));
                    CacheLock.LockWrite();
                    ThumbnailMap.emplace(weakref, img);
                    CacheLock.UnlockWrite();
                    return img;
                }
            case 2:
                {
                    const auto& fakeTex = std::get<FakeTex>(holder);
                    const auto mipmap = std::get<0>(sizeRet);
                    std::optional<ImageView> ret;
                    if (xziar::img::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                    {   //promise in GL's thread
                        auto tex = oglu::oglTex2DStatic_::Create(fakeTex->Width >> mipmap, fakeTex->Height >> mipmap, fakeTex->TexFormat);
                        tex->SetCompressedData(fakeTex->TexData[mipmap].GetRawPtr(), fakeTex->TexData[mipmap].GetSize());
                        ret = ImageView(tex->GetImage(ImageDataType::RGB));
                    }
                    else
                    {
                        ret = ImageView(Image(fakeTex->TexData[mipmap].CreateSubBuffer(), fakeTex->Width >> mipmap, fakeTex->Height >> mipmap,
                            xziar::img::TexFormatUtil::ToImageDType(fakeTex->TexFormat)));
                    }
                    CacheLock.LockWrite();
                    ThumbnailMap.emplace(weakref, ret.value());
                    CacheLock.UnlockWrite();
                    return ret;
                }
            default: return {};
            }
        }
        else
        {
            common::PromiseResult<Image> pms;
            const auto[neww, newh] = std::get<1>(sizeRet);
            switch (holder.index())
            {
            case 1:
                {
                    const auto& tex = std::get<oglTex2D>(holder);
                    pms = Resizer->ResizeToImg<ResizeMethod::Compute>(tex, neww, newh, ImageDataType::RGBA);
                } break;
            case 2:
                {
                    const auto& fakeTex = std::get<FakeTex>(holder);
                    const uint8_t mipmap = fakeTex->GetMipmapCount() - 1;
                    const auto srcSize = std::pair<uint32_t, uint32_t>{ fakeTex->Width >> mipmap, fakeTex->Height >> mipmap };
                    if (xziar::img::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                        pms = Resizer->ResizeToImg<ResizeMethod::Compute>(fakeTex->TexData[mipmap], srcSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
                    else
                        pms = Resizer->ResizeToImg<ResizeMethod::Compute>(fakeTex->TexData[mipmap], srcSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
                } break;
            default: return {};
            }
            auto img = ImageView(agent.Await(pms));
            CacheLock.LockWrite();
            ThumbnailMap.emplace(weakref, img);
            CacheLock.UnlockWrite();
            return img;
        }
    });
}


ThumbnailManager::ThumbnailManager(const std::shared_ptr<oglu::texutil::TexUtilWorker>& texWorker, const std::shared_ptr<oglu::oglWorker>& glWorker)
    : Resizer(std::make_shared<oglu::texutil::TexResizer>(texWorker)), GLWorker(glWorker)
{ }

}