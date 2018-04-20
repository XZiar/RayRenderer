#pragma once
#include "RenderCoreRely.h"

namespace rayr
{
constexpr forceinline bool IsPower2(const uint32_t num) { return (num & (num - 1)) == 0; }


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

namespace detail
{
struct RAYCOREAPI _FakeTex : public NonCopyable
{
public:
    common::AlignedBuffer<32> TexData;
    u16string Name;
    oglu::TextureInnerFormat TexFormat;
    uint32_t Width, Height;
    _FakeTex(common::AlignedBuffer<32>&& texData, const oglu::TextureInnerFormat format, const uint32_t width, const uint32_t height)
        : TexData(std::move(texData)), TexFormat(format), Width(width), Height(height) {}
};
}
using FakeTex = std::shared_ptr<detail::_FakeTex>;

struct RAYCOREAPI alignas(16) PBRMaterial : public common::AlignBase<16>
{
public:
    using TexHolder = std::variant<std::monostate, oglu::oglTex2D, FakeTex>;
    using Mapping = std::pair<uint8_t, uint16_t>;
    static oglu::TextureInnerFormat GetInnerFormat(const TexHolder& holder);
    static u16string GetName(const TexHolder& holder);
    static std::pair<uint32_t, uint32_t> GetSize(const TexHolder& holder);
    b3d::Vec3 Albedo;
    TexHolder DiffuseMap, NormalMap, MetalMap, RoughMap, AOMap;
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


struct RAYCOREAPI MultiMaterialHolder : public common::NonCopyable
{
public:
    using Mapping = PBRMaterial::Mapping;
    using TexHolder = PBRMaterial::TexHolder;
    using ArrangeMap = map<TexHolder, Mapping>;
    using SizePair = std::pair<uint16_t, uint16_t>;
private:
    vector<PBRMaterial> Materials;
    vector<uint8_t> AllocatedTexture;
    ArrangeMap Arrangement;
    oglu::oglTex2DArray Textures[13 * 13];
    oglu::TextureInnerFormat TexFormat;
    uint8_t TextureLookup[13 * 13];
public:
    static constexpr size_t UnitSize = 12 * sizeof(float);
    static oglu::oglTex2DV GetCheckTex();

    MultiMaterialHolder() { AllocatedTexture.reserve(13 * 13); }
    MultiMaterialHolder(const uint8_t count, const oglu::TextureInnerFormat format = oglu::TextureInnerFormat::RGBA8)
        : Materials(count, PBRMaterial(u"unnamed")), TexFormat(format) { AllocatedTexture.reserve(13 * 13); }

    vector<PBRMaterial>::iterator begin() { return Materials.begin(); }
    vector<PBRMaterial>::iterator end() { return Materials.end(); }

    PBRMaterial& operator[](const size_t index) { return Materials[index]; }
    void Refresh();
    void BindTexture(oglu::detail::ProgDraw& drawcall) const;
    uint32_t WriteData(std::byte *ptr) const;
};


}