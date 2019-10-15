#include "RenderCorePch.h"
#include "Light.h"

namespace rayr
{
using std::map;
using common::str::Charset;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


void LightData::WriteData(std::byte *ptr) const
{
    float *ptrFloat = reinterpret_cast<float*>(ptr);
    uint32_t *ptrI32 = reinterpret_cast<uint32_t*>(ptr);
    Color.save(ptrFloat);
    ptrI32[3] = (int32_t)Type;
    const float cosOuter = std::cos(b3d::ang2rad(CutoffOuter)),
        cosInner = std::cos(b3d::ang2rad(CutoffInner)),
        cosDiff_1 = 1.0f / (cosInner - cosOuter);
    Position.save(ptrFloat + 4);
    ptrFloat[7] = cosOuter;
    Direction.save(ptrFloat + 8);
    ptrFloat[11] = cosDiff_1;
    Attenuation.save(ptrFloat + 12);
}



Light::Light(const LightType type, const std::u16string& name) : LightData(type), Name(name)
{
    RegistControllable();
}

void Light::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"灯光名称")
        .RegistMember(&Light::Name);
    RegistItem<bool>("IsOn", "", u"开启", ArgType::RawValue, {}, u"是否开启灯光")
        .RegistMember(&Light::IsOn);
    RegistItem<float>("Luminance", "", u"亮度", ArgType::RawValue, {}, u"灯光强度")
        .RegistMemberProxy<Light>([](auto & light) -> auto & { return light.Attenuation.w; });
    RegistItem<miniBLAS::Vec4>("Color", "", u"颜色", ArgType::Color, {}, u"灯光颜色")
        .RegistMember(&Light::Color);
    if (Type != LightType::Parallel)
    {
        RegistItem<miniBLAS::Vec3>("Position", "", u"位置", ArgType::RawValue, {}, u"灯光位置")
            .RegistMember<false>(&Light::Position);
    }
    if (Type != LightType::Point)
    {
        RegistItem<miniBLAS::Vec3>("Direction", "", u"方向", ArgType::RawValue, {}, u"灯光朝向")
            .RegistMember<false>(&Light::Direction);
    }
    if (Type == LightType::Spot)
    {
        RegistItem<std::pair<float, float>>("Cutoff", "", u"切光角", ArgType::RawValue, std::pair(0.f, 180.f), u"聚光灯切光角")
            .RegistGetterProxy<Light>([](const Light& light) { return std::pair(light.CutoffInner, light.CutoffOuter); })
            .RegistSetterProxy<Light>([](Light & light, const std::pair<float, float>& arg) { std::tie(light.CutoffInner, light.CutoffOuter) = arg; });
    }
}

void Light::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    jself.Add("Name", common::strchset::to_u8string(Name, Charset::UTF16LE));
    jself.Add("Position", detail::ToJArray(context, Position));
    jself.Add("Direction", detail::ToJArray(context, Direction));
    jself.Add("Color", detail::ToJArray(context, Color));
    jself.Add("Attenuation", detail::ToJArray(context, Attenuation));
    jself.Add("Cutoff", context.NewArray().Push(CutoffOuter, CutoffInner));
    jself.Add("LightType", static_cast<int32_t>(Type));
    jself.Add("IsOn", IsOn);
}
void Light::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object) 
{
    detail::FromJArray(object.GetArray("Position"), Position);
    detail::FromJArray(object.GetArray("Direction"), Direction);
    detail::FromJArray(object.GetArray("Color"), Color);
    detail::FromJArray(object.GetArray("Attenuation"), Attenuation);
    object.GetArray("Cutoff").TryGetMany(0, CutoffOuter, CutoffInner);
    IsOn = object.Get<bool>("IsOn");
}

RESPAK_IMPL_COMP_DESERIALIZE(Light, LightType, u16string)
{
    return std::make_tuple(static_cast<LightType>(object.Get<int32_t>("LightType")),
        common::strchset::to_u16string(object.Get<string>("Name"), Charset::UTF8));
}


}
