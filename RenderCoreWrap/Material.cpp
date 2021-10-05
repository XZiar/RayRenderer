#include "RenderCoreWrapRely.h"
#include "Material.h"
#include "ImageUtil.h"

using common::container::FindInMap;
using namespace xziar::img;


namespace DizzRender
{

private ref class ThumbnailContainer
{
private:
    static std::map<ImageView, gcroot<BitmapSource^>, common::AlignBufLessor> *ThumbnailMap = nullptr;
    
public:
    static ThumbnailContainer()
    {
        ThumbnailMap = new std::map<ImageView, gcroot<BitmapSource^>, common::AlignBufLessor>();
    }
    static BitmapSource^ GetThumbnail(const ImageView& img, const dizz::TexHolder& holder)
    {
        const auto bmp = FindInMap(*ThumbnailMap, img, std::in_place);
        if (bmp)
            return bmp.value();

        BitmapSource^ timg = XZiar::Img::ImageUtil::Convert(img.AsRawImage());
        if (!timg)
            return nullptr;
        timg->Freeze();
        ThumbnailMap->emplace(img, gcroot<BitmapSource^>(timg));
        return timg;
    }
};

TexMap::TexMap(dizz::TexHolder& holder, const std::shared_ptr<dizz::ThumbnailManager>& thumbman) : Holder(holder)
{
    const auto matName = Holder.GetName();
    name = matName.empty() ? "(None)" : ToStr(matName);
    if (Holder.index() != 0)
    {
        const auto texsize = Holder.GetSize();
        const auto& strBuffer = common::mlog::detail::StrFormater::ToU16Str(u"{}x{}[{}]",
            texsize.first, texsize.second, xziar::img::TexFormatUtil::GetFormatName(Holder.GetInnerFormat()));
        description = ToStr(strBuffer);
        if (thumbman)
        {
            const auto timg = thumbman->GetThumbnail(holder)->Get();
            if (timg.has_value())
                thumbnail = ThumbnailContainer::GetThumbnail(timg.value(), holder);
        }
    }
    else
    {
        description = nullptr;
    }
}


void PBRMaterial::RefreshMaterial()
{
    const auto d = Drawable.lock();
    d->AssignMaterial();
}

PBRMaterial::PBRMaterial(std::weak_ptr<dizz::Drawable>* drawable, dizz::PBRMaterial& material, const std::shared_ptr<dizz::ThumbnailManager>& thumbman)
    : Drawable(*drawable), Material(material)
{
    albedoMap = gcnew TexMap(material.DiffuseMap, thumbman);
    normalMap = gcnew TexMap(material.NormalMap, thumbman);
}


}
