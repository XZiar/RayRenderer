#pragma once

#include "RenderCoreRely.h"

namespace b3d
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct RAYCOREAPI alignas(Vec4) LightData : public common::AlignBase<alignof(Vec4)>
{
    Vec4 color = Vec4::one();
    Vec3 position = Vec3::zero();
    Vec3 direction = Vec3(0, -1, 0);
    Vec4 attenuation = Vec4(0.5, 0.3, 0.0, 10.0);
    float cutoffOuter, cutoffInner;
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
