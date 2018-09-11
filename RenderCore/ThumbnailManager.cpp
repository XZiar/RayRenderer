#include "RenderCoreRely.h"
#include "ThumbnailManager.h"
#include "TextureUtil/GLTexResizer.h"
#include "TextureUtil/CLTexResizer.h"


namespace rayr::detail
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;
using oglu::oglTex2D;
using xziar::img::Image;
using xziar::img::ImageDataType;


std::shared_ptr<Image> ThumbnailManager::GetThumbnail(const std::weak_ptr<void>& weakref) const
{
    return FindInMap(ThumbnailMap, weakref, std::in_place).value_or(nullptr);
}

common::PromiseResult<Image> ThumbnailManager::InnerPrepareThumbnail(const TexHolder& holder)
{
    auto weakref = holder.GetWeakRef();
    if (GetThumbnail(weakref))
        return {};
    switch (holder.index())
    {
    case 1:
        {
            const auto& tex = std::get<oglTex2D>(holder);
            const auto&[needResize, neww, newh] = CalcSize(tex->GetSize());
            if (needResize)
                return GlResizer->ResizeToDat(tex, neww, newh, ImageDataType::RGBA);
            else
                ThumbnailMap.emplace(weakref, std::make_shared<Image>(tex->GetImage(ImageDataType::RGB)));
        }
        break;
    case 2:
        {
            const auto& fakeTex = std::get<FakeTex>(holder);
            const std::pair<uint32_t, uint32_t> imgSize{ fakeTex->Width, fakeTex->Height };
            const auto&[needResize, neww, newh] = CalcSize(imgSize);
            if (needResize)
            {
                if (oglu::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                    return GlResizer->ResizeToDat(fakeTex->TexData, imgSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
                else
                    return GlResizer->ResizeToDat(fakeTex->TexData, imgSize, fakeTex->TexFormat, neww, newh, ImageDataType::RGBA);
            }
            else
            {
                std::shared_ptr<Image> img;
                if (oglu::TexFormatUtil::IsCompressType(fakeTex->TexFormat))
                {   //promise in GL's thread
                    oglu::oglTex2DS tex(fakeTex->Width, fakeTex->Height, fakeTex->TexFormat);
                    tex->SetCompressedData(fakeTex->TexData.GetRawPtr(), fakeTex->TexData.GetSize());
                    img = std::make_shared<Image>(tex->GetImage(ImageDataType::RGB));
                }
                else
                {
                    img = std::make_shared<Image>(fakeTex->TexData, fakeTex->Width, fakeTex->Height, 
                        oglu::TexFormatUtil::ConvertToImgType(fakeTex->TexFormat));
                }
                ThumbnailMap.emplace(weakref, img);
            }
        }
        break;
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
        ThumbnailMap.emplace(holder.GetWeakRef(), std::make_shared<Image>(std::move(img)));
    }
}


ThumbnailManager::ThumbnailManager(const oglu::oglContext& ctx) 
    : GlResizer(std::make_shared<oglu::texutil::GLTexResizer>(oglu::oglContext::NewContext(ctx, true))),
    ClResizer(std::make_shared<oglu::texutil::CLTexResizer>(oglu::oglContext::NewContext(ctx, true)))
{ }

}