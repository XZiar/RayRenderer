#pragma once
#include "RenderCoreRely.h"

namespace rayr
{

constexpr forceinline bool IsPower2(const uint32_t num) { return (num & (num - 1)) == 0; }

namespace detail
{
class TextureLoader;
}

struct RAYCOREAPI alignas(16) RawMaterialData : public common::AlignBase<16>
{
public:
    b3d::Vec4 ambient, diffuse, specular, emission;
    float shiness, reflect, refract, rfr;
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
    vector<common::AlignedBuffer> TexData;
    u16string Name;
    oglu::TextureInnerFormat TexFormat;
    uint32_t Width, Height;
    _FakeTex(common::AlignedBuffer&& texData, const oglu::TextureInnerFormat format, const uint32_t width, const uint32_t height)
        : TexData(vector<common::AlignedBuffer>{std::move(texData)}), TexFormat(format), Width(width), Height(height) {}
    _FakeTex(vector<common::AlignedBuffer>&& texData, const oglu::TextureInnerFormat format, const uint32_t width, const uint32_t height)
        : TexData(std::move(texData)), TexFormat(format), Width(width), Height(height) {}
    ~_FakeTex() {}
    uint8_t GetMipmapCount() const
    {
        return static_cast<uint8_t>(TexData.size());
    }
};
}
using FakeTex = std::shared_ptr<detail::_FakeTex>;

struct RAYCOREAPI TexHolder : public std::variant<std::monostate, oglu::oglTex2D, FakeTex>
{
    using std::variant<std::monostate, oglu::oglTex2D, FakeTex>::variant;
    oglu::TextureInnerFormat GetInnerFormat() const;
    u16string GetName() const;
    std::pair<uint32_t, uint32_t> GetSize() const;
    std::weak_ptr<void> GetWeakRef() const;
    uint8_t GetMipmapCount() const;
};

struct RAYCOREAPI alignas(16) PBRMaterial : public common::AlignBase<16>, public xziar::respak::Serializable
{
public:
    b3d::Vec3 Albedo;
    TexHolder DiffuseMap, NormalMap, MetalMap, RoughMap, AOMap;
    float Metalness;
    float Roughness;
    float Specular;
    float AO;
    bool UseDiffuseMap = false, UseNormalMap = false, UseMetalMap = false, UseRoughMap = false, UseAOMap = false;
    std::u16string Name;
    PBRMaterial(const std::u16string& name) : Albedo(b3d::Vec3(0.58, 0.58, 0.58)), Metalness(0.0f), Roughness(0.8f), Specular(0.0f), AO(1.0f), Name(name) { }
    uint32_t WriteData(std::byte *ptr) const;

    RESPAK_OVERRIDE_TYPE("rayr#PBRMaterial")
    virtual ejson::JObject Serialize(SerializeUtil& context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};


namespace detail
{
union TexTag
{
    uint64_t Val;
    struct Dummy
    {
        oglu::TextureInnerFormat Format;
        uint16_t Width;
        uint16_t Height;
        uint8_t Mipmap;
    } Info;
    template<typename T>
    TexTag(const oglu::TextureInnerFormat format, const T width, const T height, const uint8_t mipmap) 
        : Info{ format, static_cast<uint16_t>(width), static_cast<uint16_t>(height), mipmap } {}
    bool operator<(const TexTag& other) const noexcept { return Val < other.Val; }
    bool operator==(const TexTag& other) const noexcept { return Val == other.Val; }
};
}

struct RAYCOREAPI MultiMaterialHolder : public common::NonCopyable, public xziar::respak::Serializable
{
public:
    using Mapping = std::pair<detail::TexTag, uint16_t>;
    using ArrangeMap = map<TexHolder, Mapping>;
private:
    vector<PBRMaterial> Materials;
    // tex -> (size, layer)
    ArrangeMap Arrangement;
    map<detail::TexTag, oglu::oglTex2DArray> Textures;
    map<detail::TexTag, uint8_t> TextureLookup;
public:
    static constexpr size_t UnitSize = 12 * sizeof(float);
    static oglu::oglTex2DV GetCheckTex();

    std::weak_ptr<detail::ThumbnailManager> ThumbMan;

    MultiMaterialHolder() { }
    MultiMaterialHolder(const uint8_t count) : Materials(count, PBRMaterial(u"unnamed")) { }

    vector<PBRMaterial>::iterator begin() { return Materials.begin(); }
    vector<PBRMaterial>::iterator end() { return Materials.end(); }
    PBRMaterial& operator[](const size_t index) { return Materials[index]; }

    void Refresh();
    void BindTexture(oglu::detail::ProgDraw& drawcall) const;
    uint32_t WriteData(std::byte *ptr) const; 
    
    RESPAK_OVERRIDE_TYPE("rayr#MultiMaterialHolder")
    virtual ejson::JObject Serialize(SerializeUtil & context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};


}
