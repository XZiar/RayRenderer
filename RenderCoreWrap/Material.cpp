#include "RenderCoreWrapRely.h"
#include "Material.h"
#include "ImageUtil.h"

using common::container::FindInMap;
using namespace xziar::img;


namespace RayRender
{

private ref class ThumbnailContainer
{
private:
    static std::map<std::weak_ptr<ImageView>, gcroot<BitmapSource^>, std::owner_less<void>> *ThumbnailMap = nullptr;
    
public:
    static ThumbnailContainer()
    {
        ThumbnailMap = new std::map<std::weak_ptr<ImageView>, gcroot<BitmapSource^>, std::owner_less<void>>();
    }
    static BitmapSource^ GetThumbnail(const std::shared_ptr<ImageView>& img)
    {
        const auto bmp = FindInMap(*ThumbnailMap, img, std::in_place);
        if (bmp)
            return bmp.value();

        BitmapSource^ timg = XZiar::Img::ImageUtil::Convert(img->AsRawImage());
        if (!timg)
            return nullptr;
        timg->Freeze();
        ThumbnailMap->emplace(std::weak_ptr<ImageView>(img), gcroot<BitmapSource^>(timg));
        return timg;
    }
};

TexMap::TexMap(rayr::TexHolder& holder, const std::shared_ptr<rayr::ThumbnailManager>& thumbman) : Holder(holder)
{
    const auto matName = Holder.GetName();
    name = matName.empty() ? "(None)" : ToStr(matName);
    if (Holder.index() != 0)
    {
        const auto texsize = Holder.GetSize();
        const auto& strBuffer = common::mlog::detail::StrFormater<char16_t>::ToU16Str(u"{}x{}[{}]",
            texsize.first, texsize.second, oglu::TexFormatUtil::GetFormatName(Holder.GetInnerFormat()));
        description = ToStr(strBuffer);
        if (thumbman)
        {
            const auto timg = thumbman->GetThumbnail(holder);
            if (timg)
                thumbnail = ThumbnailContainer::GetThumbnail(timg);
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

PBRMaterial::PBRMaterial(std::weak_ptr<rayr::Drawable>* drawable, rayr::PBRMaterial& material) 
    : Drawable(*drawable), Material(material)
{
    const auto thumbman = Drawable.lock()->MaterialHolder.ThumbMan.lock();
    albedoMap = gcnew TexMap(material.DiffuseMap, thumbman);
    normalMap = gcnew TexMap(material.NormalMap, thumbman);
}


}
