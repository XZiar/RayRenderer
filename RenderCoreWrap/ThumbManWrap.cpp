#include "RenderCoreWrapRely.h"
#include "ThumbManWrap.h"
#include "ImageUtil.h"


using common::container::FindInMap;
namespace Dizz
{


TexHolder::TexHolder(const oglu::oglTex2D& tex) : TypeId(1)
{
    WeakRef.Construct(tex);
}
TexHolder::TexHolder(const rayr::FakeTex& tex) : TypeId(2)
{
    WeakRef.Construct(tex);
}

TexHolder::!TexHolder()
{
    WeakRef.Destruct();
}

TexHolder^ TexHolder::CreateTexHolder(const rayr::TexHolder& holder)
{
    switch (holder.index())
    {
    case 1: return gcnew TexHolder(std::get<1>(holder));
    case 2: return gcnew TexHolder(std::get<2>(holder));
    default: return nullptr;
    }
}


rayr::TexHolder TexHolder::ExtractHolder()
{
    WRAPPER_NATIVE_PTR_DO(WeakRef, holder, lock());
    if (!holder) return {};
    switch (TypeId)
    {
    case 1: return oglu::oglTex2D(std::reinterpret_pointer_cast<oglu::detail::_oglTexture2D>(holder));
    case 2: return std::reinterpret_pointer_cast<rayr::detail::_FakeTex>(holder);
    default: return {};
    }
}


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

BitmapSource^ ThumbnailMan::GetThumbnail2(Common::CLIWrapper<std::optional<xziar::img::ImageView>>^ img)
{
    auto opt = img->Extract();
    if (opt.has_value())
        return GetThumbnail(opt.value());
    else
        return nullptr;
}

BitmapSource ^ ThumbnailMan::GetThumbnail(const rayr::TexHolder & holder)
{
    auto img = ThumbMan->lock()->GetThumbnail(holder)->Wait();
    if (!img.has_value())
        return nullptr;
    return GetThumbnail(img.value());
}


Common::WaitObj<std::optional<xziar::img::ImageView>, BitmapSource^>^ ThumbnailMan::GetThumbnailAsync(TexHolder^ holder)
{
    auto convertor = gcnew Func<Common::CLIWrapper<std::optional<xziar::img::ImageView>>^, BitmapSource^>(this, &ThumbnailMan::GetThumbnail2);
    return gcnew Common::WaitObj<std::optional<xziar::img::ImageView>, BitmapSource^>
        (
            ThumbMan->lock()->GetThumbnail(holder->ExtractHolder()),
            convertor
        );
}


}

template ref class Common::WaitObj<std::optional<xziar::img::ImageView>, BitmapSource^>;