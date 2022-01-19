#include "RenderCorePch.h"
#include "Light.h"

namespace dizz
{
using std::map;
using common::str::Encoding;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;

template<typename T>
forceinline void Save(const mbase::vec::Vec4Base<T>& vec, float* ptr) noexcept
{
    ptr[0] = vec[0], ptr[1] = vec[1], ptr[2] = vec[2], ptr[3] = vec[3];
}

void LightData::WriteData(const common::span<std::byte> space) const
{
    Expects((size_t)space.size() >= Light::WriteSize);
    float *ptrFloat = reinterpret_cast<float*>(space.data());
    uint32_t *ptrI32 = reinterpret_cast<uint32_t*>(space.data());
    Color.Save(ptrFloat);
    ptrI32[3] = (int32_t)Type;
    const float cosOuter = std::cos(math::Ang2Rad(CutoffOuter)),
        cosInner = std::cos(math::Ang2Rad(CutoffInner)),
        cosDiff_1 = 1.0f / (cosInner - cosOuter);
    Position.Save(ptrFloat + 4);
    ptrFloat[7] = cosOuter;
    Direction.Save(ptrFloat + 8);
    ptrFloat[11] = cosDiff_1;
    Attenuation.Save(ptrFloat + 12);
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
        .RegistMemberProxy<Light>([](auto & light) -> auto & { return light.Attenuation.W; });
    RegistItem<mbase::Vec4>("Color", "", u"颜色", ArgType::Color, {}, u"灯光颜色")
        .RegistMember(&Light::Color);
    if (Type != LightType::Parallel)
    {
        RegistItem<mbase::Vec3>("Position", "", u"位置", ArgType::RawValue, {}, u"灯光位置")
            .RegistMember<false>(&Light::Position);
    }
    if (Type != LightType::Point)
    {
        RegistItem<mbase::Vec3>("Direction", "", u"方向", ArgType::RawValue, {}, u"灯光朝向")
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
    jself.Add("Name", common::str::to_u8string(Name));
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
        common::str::to_u16string(object.Get<string>("Name"), Encoding::UTF8));
}


}
