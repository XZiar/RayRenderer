#pragma once
#include "RenderCoreRely.h"

namespace b3d
{


struct RAYCOREAPI alignas(16) RawMaterialData : public common::AlignBase<16>
{
public:
    Vec4 ambient, diffuse, specular, emission;
    float shiness, reflect, refract, rfr;//�߹�Ȩ�أ�������ʣ�������ʣ�������
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
        //�߹�Ȩ�أ�������ʣ�������ʣ�������
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

struct RAYCOREAPI alignas(16) PBRMaterialData : public common::AlignBase<16>
{
public:
    union
    {
        struct { float AlbegoRed, AlbegoGreen, AlbegoBlue, Metalness; };
        Vec3 Albedo;
    };
    float Roughness;
    float Specular;
    float AO;
    PBRMaterialData() : AO(1.0f) {}
};


}