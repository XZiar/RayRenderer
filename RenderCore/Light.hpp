#pragma once

#include "RenderCoreRely.h"

namespace rayr
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct RAYCOREAPI alignas(b3d::Vec4) LightData : public common::AlignBase<alignof(b3d::Vec4)>
{
    b3d::Vec4 color = b3d::Vec4::one();
    b3d::Vec3 position = b3d::Vec3::zero();
    b3d::Vec3 direction = b3d::Vec3(0, -1, 0);
    b3d::Vec4 attenuation = b3d::Vec4(0.5, 0.3, 0.0, 10.0);
    float cutoffOuter, cutoffInner;
    const LightType type;
protected:
    LightData(const LightType type_) : type(type_) {}
public:
    void Move(const float x, const float y, const float z)
    {
        position += b3d::Vec3(x, y, z);
    }
    void Move(const b3d::Vec3& offset)
    {
        position += offset;
    }
    void Rotate(const float x, const float y, const float z)
    {
        Rotate(b3d::Vec3(x, y, z));
    }
    void Rotate(const b3d::Vec3& radius)
    {
        const auto rMat = b3d::Mat3x3::RotateMatXYZ(radius);
        direction = rMat * direction;
    }
    uint32_t WriteData(std::byte *ptr) const
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
};

class RAYCOREAPI alignas(LightData) Light : public LightData, public xziar::respak::Serializable
{
protected:
    Light(const LightType type_, const std::u16string& name_) : LightData(type_), name(name_) {}
public:
    bool isOn = true;
    std::u16string name;

    virtual std::string_view SerializedType() const override { return "Light"; }
    virtual ejson::JObject Serialize(SerializeUtil& context) const override
    {
        auto jself = context.NewObject();
        jself.Add("name", str::to_u8string(name, Charset::UTF16LE));
        jself.Add("position", detail::ToJArray(context, position));
        jself.Add("direction", detail::ToJArray(context, direction));
        jself.Add("color", detail::ToJArray(context, color));
        jself.Add("attenuation", detail::ToJArray(context, attenuation));
        jself.Add("cutoff", context.NewArray().Push(cutoffOuter, cutoffInner));
        jself.Add("lightType", static_cast<int32_t>(type));
        jself.Add("isOn", isOn);
        return jself;
    }
};

class alignas(16) ParallelLight : public Light
{
public:
    ParallelLight() : Light(LightType::Parallel, u"ParallelLight") { attenuation.w = 1.0f; }
};

class alignas(16) PointLight : public Light
{
public:
    PointLight() : Light(LightType::Point, u"PointLight") {}
};

class alignas(16) SpotLight : public Light
{
public:
    SpotLight() : Light(LightType::Spot, u"SpotLight") 
    {
        cutoffInner = 30.0f;
        cutoffOuter = 90.0f;
    }
};

}
