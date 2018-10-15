#include "RenderCoreRely.h"
#include "Light.h"

namespace rayr
{

uint32_t LightData::WriteData(std::byte *ptr) const
{
    float *ptrFloat = reinterpret_cast<float*>(ptr);
    uint32_t *ptrI32 = reinterpret_cast<uint32_t*>(ptr);
    color.save(ptrFloat);
    ptrI32[3] = (int32_t)type;
    const float cosOuter = std::cos(b3d::ang2rad(cutoffOuter)),
        cosInner = std::cos(b3d::ang2rad(cutoffInner)),
        cosDiff_1 = 1.0f / (cosInner - cosOuter);
    position.save(ptrFloat + 4);
    ptrFloat[7] = cosOuter;
    direction.save(ptrFloat + 8);
    ptrFloat[11] = cosDiff_1;
    attenuation.save(ptrFloat + 12);
    return 4 * 4 * sizeof(float);
}


void Light::Serialize(SerializeUtil & context, ejson::JObject& jself) const
{
    jself.Add("name", str::to_u8string(name, Charset::UTF16LE));
    jself.Add("position", detail::ToJArray(context, position));
    jself.Add("direction", detail::ToJArray(context, direction));
    jself.Add("color", detail::ToJArray(context, color));
    jself.Add("attenuation", detail::ToJArray(context, attenuation));
    jself.Add("cutoff", context.NewArray().Push(cutoffOuter, cutoffInner));
    jself.Add("lightType", static_cast<int32_t>(type));
    jself.Add("isOn", isOn);
}
void Light::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) 
{
    detail::FromJArray(object.GetArray("position"), position);
    detail::FromJArray(object.GetArray("direction"), direction);
    detail::FromJArray(object.GetArray("color"), color);
    detail::FromJArray(object.GetArray("attenuation"), attenuation);
    object.GetArray("position").TryGetMany(0, cutoffOuter, cutoffInner);
    isOn = object.Get<bool>("isOn");
}

RESPAK_IMPL_COMP_DESERIALIZE(Light, LightType, u16string)
{
    return std::make_tuple(static_cast<LightType>(object.Get<int32_t>("lightType")),
        str::to_u16string(object.Get<string>("name"), Charset::UTF8));
}


}
