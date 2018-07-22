#include "RenderCoreWrapRely.h"
#include "Material.h"


namespace RayRender
{


TexMap::TexMap(rayr::PBRMaterial::TexHolder& holder) : Holder(holder)
{
    const auto matName = rayr::PBRMaterial::GetName(Holder);
    name = matName.empty() ? "(None)" : ToStr(matName);
    switch (Holder.index())
    {
    default:
        description = nullptr; break;
    case 1:
        {
            const auto tex = std::get<oglu::oglTex2D>(Holder);
            const auto&[w, h] = tex->GetSize();
            const auto& strBuffer = common::mlog::detail::StrFormater<char16_t>::ToU16Str(u"{}x{}[{}]", w, h, oglu::TexFormatUtil::GetFormatName(tex->GetInnerFormat()));
            description = ToStr(strBuffer);
        } break;
    case 2: 
        {
            const auto tex = std::get<rayr::FakeTex>(Holder);
            const auto& strBuffer = common::mlog::detail::StrFormater<char16_t>::ToU16Str(u"{}x{}[{}]", tex->Width, tex->Height, oglu::TexFormatUtil::GetFormatName(tex->TexFormat));
            description = ToStr(strBuffer);
        } break;
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
    albedoMap = gcnew TexMap(material.DiffuseMap);
    normalMap = gcnew TexMap(material.NormalMap);
}


}
