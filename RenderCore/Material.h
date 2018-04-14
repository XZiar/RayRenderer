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
    using Mapping = std::pair<uint8_t, uint16_t>;
    using ArrangeMap = map<oglu::oglTex2D, Mapping>;
    b3d::Vec3 Albedo;
    oglu::oglTex2D DiffuseMap, NormalMap, MetalMap, RoughMap, AOMap;
    float Metalness;
    float Roughness;
    float Specular;
    float AO;
    bool UseDiffuseMap = false, UseNormalMap = false, UseMetalMap = false, UseRoughMap = false, UseAOMap = false;
    std::u16string Name;
    PBRMaterial(const std::u16string& name) : Albedo(b3d::Vec3(0.58, 0.58, 0.58)), Metalness(0.0f), Roughness(0.8f), Specular(0.0f), AO(1.0f), Name(name) { }
    PBRMaterial(const PBRMaterial& other) = default;
    PBRMaterial(PBRMaterial&& other) = default;
    PBRMaterial& operator=(PBRMaterial&& other) = default;
    PBRMaterial& operator=(const PBRMaterial& other) = default;
    uint32_t WriteData(std::byte *ptr) const;
};

oglu::oglTex2DS GenTexture(const xziar::img::Image& img, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);
oglu::oglTex2DS GenTextureAsync(const xziar::img::Image& img, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3, const u16string& taskName = u"GenTextureAsync");

struct RAYCOREAPI MultiMaterialHolder : public common::NonCopyable
{
private:
    using Mapping = PBRMaterial::Mapping;
    using ArrangeMap = PBRMaterial::ArrangeMap;
    using SizePair = std::pair<uint16_t, uint16_t>;
    vector<PBRMaterial> Materials;
    vector<uint8_t> AllocatedTexture;
    ArrangeMap Arrangement;
    oglu::oglTex2DArray Textures[13 * 13];
    uint8_t TextureLookup[13 * 13];
public:
    static constexpr size_t UnitSize = 12 * sizeof(float);
    static oglu::oglTex2DV GetCheckTex();
    static oglu::oglTex2DS LoadImgToTex(const xziar::img::Image& img, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::BC3);

    MultiMaterialHolder() { AllocatedTexture.reserve(13 * 13); }
    MultiMaterialHolder(const uint8_t count) : Materials(count, PBRMaterial(u"unnamed")) { AllocatedTexture.reserve(13 * 13); }

    vector<PBRMaterial>::iterator begin() { return Materials.begin(); }
    vector<PBRMaterial>::iterator end() { return Materials.end(); }

    PBRMaterial& operator[](const size_t index) { return Materials[index]; }
    void Refresh();
    void BindTexture(oglu::detail::ProgDraw& drawcall) const;
    uint32_t WriteData(std::byte *ptr) const;
};


}