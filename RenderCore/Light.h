#pragma once

#include "RenderCoreRely.h"

namespace rayr
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct RAYCOREAPI LightData : public common::AlignBase<alignof(b3d::Vec4)>
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
    uint32_t WriteData(std::byte *ptr) const;
};

class RAYCOREAPI alignas(LightData) Light : public LightData, public xziar::respak::Serializable
{
protected:
    Light(const LightType type_, const std::u16string& name_) : LightData(type_), name(name_) {}
public:
    bool isOn = true;
    std::u16string name;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Light")
    virtual ejson::JObject Serialize(SerializeUtil& context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
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
