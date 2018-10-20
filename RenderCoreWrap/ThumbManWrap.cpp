#include "RenderCoreWrapRely.h"
#include "ThumbManWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"


using common::container::FindInMap;
namespace Dizz
{


ThumbnailMan::ThumbnailMan(const common::Wrapper<rayr::ThumbnailManager>& thumbMan) 
    : ThumbnailMap(new std::map<xziar::img::ImageView, gcroot<BitmapSource^>, common::AlignBufLessor>())
{
    ThumbMan = new std::weak_ptr<rayr::ThumbnailManager>(thumbMan);
}

ThumbnailMan::!ThumbnailMan()
{
    if (const auto ptr = ExchangeNullptr(ThumbMan); ptr)
        delete ptr;
}

BitmapSource^ ThumbnailMan::GetThumbnail(const xziar::img::ImageView& img)
{
    const auto bmp = FindInMap(*ThumbnailMap, img, std::in_place);
    if (bmp.has_value())
        return bmp.value();

    BitmapSource^ timg = XZiar::Img::ImageUtil::Convert(img.AsRawImage());
    if (!timg)
        return nullptr;
    timg->Freeze();
    ThumbnailMap->emplace(img, gcroot<BitmapSource^>(timg));
    return timg;
}

BitmapSource ^ ThumbnailMan::GetThumbnail(const rayr::TexHolder & holder)
{
    auto img = ThumbMan->lock()->GetThumbnail(holder)->Wait();
    if (!img.has_value())
        return nullptr;
    return GetThumbnail(img.value());
}


}