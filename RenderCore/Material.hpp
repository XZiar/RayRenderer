#pragma once
#include "RenderCoreRely.h"

namespace b3d
{


struct RAYCOREAPI alignas(16) RawMaterialData : public common::AlignBase<16>
{
public:
    Vec4 ambient, diffuse, specular, emission;
    float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
    enum class Property : uint8_t
    {
        Ambient = 0x1,
        Diffuse = 0x2,
        Specular = 0x4,
        Emission = 0x8,
        Shiness = 0x10,
        Reflect = 0x20,
        Refract = 0x40,
        RefractRate = 0x80
    };
};

struct RAYCOREAPI alignas(16) MaterialData : public common::AlignBase<16>
{
public:
    union
    {
        struct { float ambient, diffuse, specular, emission; };
        Vec4 Intensity;
    };
    union  
    {
        //高光权重，反射比率，折射比率，折射率
        struct { float shiness, reflect, refract, rfr; };
        Vec4 Other;
    };
    MaterialData() : Intensity(Vec4::one()), Other(Vec4::zero()) {}
};

struct RAYCOREAPI alignas(16) Material : public MaterialData
{
public:
    std::u16string Name;
    Material(const std::u16string& name) : Name(name) { }
};

struct RAYCOREAPI alignas(16) PBRMaterial : public common::AlignBase<16>
{
public:
    Vec3 Albedo;
    float Metalness;
    float Roughness;
    float Specular;
    float AO;
    bool UseDiffuseMap, UseNormalMap;
    std::u16string Name;
    PBRMaterial(const std::u16string& name) : Albedo(Vec3(0.58, 0.58, 0.58)), Metalness(0.0f), Roughness(0.0f), Specular(0.0f), AO(1.0f),
        UseDiffuseMap(false), UseNormalMap(false), Name(name) { }

    uint32_t WriteData(std::byte *ptr) const
    {
        float *ptrFloat = reinterpret_cast<float*>(ptr);
        Vec4 basic(Albedo, Metalness);
        if (UseDiffuseMap)
            basic.x *= -1.0f;
        if (UseNormalMap)
            basic.y *= -1.0f;
        memcpy_s(ptrFloat, sizeof(Vec4), &basic, sizeof(Vec4));
        ptrFloat[4] = Roughness;
        ptrFloat[5] = Specular;
        ptrFloat[6] = AO;
        return 8 * sizeof(float);
    }
};


}