#pragma once

#include "RenderCoreRely.h"

namespace b3d
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct RAYCOREAPI alignas(Vec4) LightData : public common::AlignBase<alignof(Vec4)>
{
    Vec3 position = Vec3::zero();
    Vec3 direction = Vec3(0, 0, -1);
    Vec4 color = Vec4::one();
    Vec4 attenuation = Vec4(0.5, 0.3, 0.0, 0.0);
    float coang, exponent;
    const LightType type;
protected:
    LightData(const LightType type_) : type(type_) {}
public:
    void Move(const float x, const float y, const float z)
    {
        position += Vec3(x, y, z);
    }
    void Move(const Vec3& offset)
    {
        position += offset;
    }
    void Rotate(const float x, const float y, const float z)
    {
        Rotate(Vec3(x, y, z));
    }
    void Rotate(const Vec3& radius)
    {
        const auto rMat = Mat3x3::RotateMatXYZ(radius);
        direction = rMat * direction;
    }
};

class RAYCOREAPI alignas(LightData) Light : public LightData
{
protected:
    Light(const LightType type_, const std::u16string& name_) : LightData(type_), name(name_) {}
public:
    bool isOn = true;
    std::u16string name;
};

class alignas(16) ParallelLight : public Light
{
public:
    ParallelLight() : Light(LightType::Parallel, u"ParallelLight") {}
};

class alignas(16) PointLight : public Light
{
public:
    PointLight() : Light(LightType::Point, u"PointLight") {}
};

class alignas(16) SpotLight : public Light
{
public:
    SpotLight() : Light(LightType::Spot, u"SpotLight") {}
};

}
