#pragma once
#include "RenderCoreRely.h"

namespace rayr
{


struct RAYCOREAPI alignas(16) RawMaterialData : public common::AlignBase<16>
{
public:
    b3d::Vec4 ambient, diffuse, specular, emission;
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



struct RAYCOREAPI alignas(16) PBRMaterial : public common::AlignBase<16>
{
public:
    using ImageHolder = std::variant<std::monostate, std::shared_ptr<xziar::img::Image>, oglu::oglTex2D>;
    b3d::Vec3 Albedo;
    float Metalness;
    float Roughness;
    float Specular;
    float AO;
    bool UseDiffuseMap, UseNormalMap;
    std::u16string Name;
    ImageHolder DiffuseMap, NormalMap, MetalMap, RoughMap, AOMap;
    PBRMaterial(const std::u16string& name) : Albedo(b3d::Vec3(0.58, 0.58, 0.58)), Metalness(0.0f), Roughness(0.0f), Specular(0.0f), AO(1.0f),
        UseDiffuseMap(false), UseNormalMap(false), Name(name) { }
    uint32_t WriteData(std::byte *ptr) const;
};

oglu::oglTex2D GenTexture(const xziar::img::Image& img, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
oglu::oglTex2D GenTextureAsync(const xziar::img::Image& img, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3, const u16string& taskName = u"GenTextureAsync");

struct RAYCOREAPI MultiMaterialHolder : common::NonCopyable
{
private:
    using Mapping = std::pair<uint8_t, uint16_t>;
    using ArrangeMap = map<PBRMaterial::ImageHolder, Mapping>;
    vector<PBRMaterial> Materials;
    vector<oglu::oglTex2DArray> Textures;
    ArrangeMap Arrangement;
    uint8_t MaterialCount = 0;
public:
    MultiMaterialHolder() : Textures(15) {}
    MultiMaterialHolder(const uint8_t count) : MaterialCount(count), Textures(15) {}

    PBRMaterial& operator[](const size_t index) { return Materials[index]; }
    void Refresh();
};


}