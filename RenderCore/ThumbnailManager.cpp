#include "RenderCoreRely.h"
#include "ThumbnailManager.h"
#include "TextureUtil/TexResizer.h"


namespace rayr::detail
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

std::shared_ptr<ImageView> ThumbnailManager::GetThumbnail(const std::weak_ptr<void>& weakref) const
{
    return FindInMap(ThumbnailMap, weakref, std::in_place).value_or(nullptr);
}

common::PromiseResult<Image> ThumbnailManager::InnerPrepareThumbnail(const TexHolder& holder)
{
    auto weakref = holder.GetWeakRef();
    if (GetThumbnail(weakref))
        return {};
    const auto ret = CalcSize(holder);
    if (ret.index() == 0) // no need
    {
        switch (holder.index())
        {
        case 1:
            {
                const auto& tex = std::get<oglTex2D>(holder);
                ThumbnailMap.emplace(weakref, std::make_shared<ImageView>(tex->GetImage(ImageDataType::RGB)));
            } break;
        case 2:
            {
                const auto& fakeTex = std::get<FakeTex>(holder);
                const auto mipmap = std::get<0>(ret);
                std::shared_ptr<ImageView> img;
                if (oglu::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                {   //promise in GL's thread
                    oglu::oglTex2DS tex(fakeTex->Width >> mipmap, fakeTex->Height >> mipmap, fakeTex->TexFormat);
                    tex->SetCompressedData(fakeTex->TexData[mipmap].GetRawPtr(), fakeTex->TexData[mipmap].GetSize());
                    img = std::make_shared<ImageView>(tex->GetImage(ImageDataType::RGB));
                }
                else
                {
                    img = std::make_shared<ImageView>(Image(fakeTex->TexData[mipmap].CreateSubBuffer(), fakeTex->Width >> mipmap, fakeTex->Height >> mipmap,
                        oglu::TexFormatUtil::ConvertToImgType(fakeTex->TexFormat)));
                }
                ThumbnailMap.emplace(weakref, img);
            } break;
        default: break;
        }
    }
    else
    {
        const auto[neww, newh] = std::get<1>(ret);
        switch (holder.index())
        {
        case 1:
            {
                const auto& tex = std::get<oglTex2D>(holder);
                return Resizer->ResizeToImg<ResizeMethod::Compute>(tex, neww, newh, ImageDataType::RGBA);
            }
        case 2:
            {
                const auto& fakeTex = std::get<FakeTex>(holder);
                const uint8_t mipmap = fakeTex->GetMipmapCount() - 1;
                const auto srcSize = std::pair<uint32_t, uint32_t>{ fakeTex->Width >> mipmap, fakeTex->Height >> mipmap };
                if (oglu::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                    return Resizer->ResizeToImg<ResizeMethod::Compute>(fakeTex->TexData[mipmap], srcSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
                else
                    return Resizer->ResizeToImg<ResizeMethod::Compute>(fakeTex->TexData[mipmap], srcSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
            }
        default: break;
        }
    }
    return {};
}

void ThumbnailManager::InnerWaitPmss(const PmssType& pmss)
{
    for (const auto&[holder, result] : pmss)
    {
        auto img = AsyncAgent::SafeWait(result);
        if (img.GetDataType() != xziar::img::ImageDataType::RGB)
            img = img.ConvertTo(xziar::img::ImageDataType::RGB);
        ThumbnailMap.emplace(holder.GetWeakRef(), std::make_shared<ImageView>(std::move(img)));
    }
}


ThumbnailManager::ThumbnailManager(const std::shared_ptr<oglu::texutil::TexUtilWorker>& worker)
    : Resizer(std::make_shared<oglu::texutil::TexResizer>(worker))
{ }

}