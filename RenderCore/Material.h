#pragma once
#include "RenderCoreRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace rayr
{
class TextureLoader;
class ThumbnailManager;

constexpr forceinline bool IsPower2(const uint32_t num) { return (num & (num - 1)) == 0; }

struct RAYCOREAPI RawMaterialData : public common::AlignBase<16>
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
struct RAYCOREAPI _FakeTex : public NonCopyable, public xziar::respak::Serializable
{
public:
    vector<common::AlignedBuffer> TexData;
    u16string Name;
    xziar::img::TextureFormat TexFormat;
    uint32_t Width, Height;
    _FakeTex(common::AlignedBuffer&& texData, const xziar::img::TextureFormat format, const uint32_t width, const uint32_t height)
        : TexData(vector<common::AlignedBuffer>{std::move(texData)}), TexFormat(format), Width(width), Height(height) {}
    _FakeTex(vector<common::AlignedBuffer>&& texData, const xziar::img::TextureFormat format, const uint32_t width, const uint32_t height)
        : TexData(std::move(texData)), TexFormat(format), Width(width), Height(height) {}
    ~_FakeTex() {}
    uint8_t GetMipmapCount() const
    {
        return static_cast<uint8_t>(TexData.size());
    }
    RESPAK_DECL_COMP_DESERIALIZE("rayr#FakeTex")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};
}
using FakeTex = std::shared_ptr<detail::_FakeTex>;

struct RAYCOREAPI TexHolder : public std::variant<std::monostate, oglu::oglTex2D, FakeTex>
{
    using std::variant<std::monostate, oglu::oglTex2D, FakeTex>::variant;
    xziar::img::TextureFormat GetInnerFormat() const;
    u16string GetName() const;
    std::pair<uint32_t, uint32_t> GetSize() const;
    std::weak_ptr<void> GetWeakRef() const;
    uint8_t GetMipmapCount() const;
    uintptr_t GetRawPtr() const;
};

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
struct RAYCOREAPI PBRMaterial : public common::AlignBase<16>, public xziar::respak::Serializable, public Controllable
{
protected:
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"Material"sv;
    }
    void RegistControllable();
public:
    b3d::Vec3 Albedo;
    TexHolder DiffuseMap, NormalMap, MetalMap, RoughMap, AOMap;
    float Metalness;
    float Roughness;
    float Specular;
    float AO;
    bool UseDiffuseMap = false, UseNormalMap = false, UseMetalMap = false, UseRoughMap = false, UseAOMap = false;
    std::u16string Name;
    PBRMaterial(const std::u16string& name = u"");
    virtual ~PBRMaterial() override {}
    uint32_t WriteData(std::byte *ptr) const;

    RESPAK_DECL_SIMP_DESERIALIZE("rayr#PBRMaterial")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif


namespace detail
{
union TexTag
{
    uint64_t Val;
    struct Dummy
    {
        xziar::img::TextureFormat Format;
        uint16_t Width;
        uint16_t Height;
        uint8_t Mipmap;
    } Info;
    template<typename T>
    TexTag(const xziar::img::TextureFormat format, const T width, const T height, const uint8_t mipmap)
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
    vector<std::shared_ptr<PBRMaterial>> Materials;
    // tex -> (size, layer)
    ArrangeMap Arrangement;
    map<detail::TexTag, oglu::oglTex2DArray> Textures;
    map<detail::TexTag, uint8_t> TextureLookup;
public:
    static constexpr size_t WriteSize = 12 * sizeof(float);
    static oglu::oglTex2DV GetCheckTex();

    MultiMaterialHolder() { }
    MultiMaterialHolder(const uint8_t count);

    vector<std::shared_ptr<PBRMaterial>>::iterator begin() { return Materials.begin(); }
    vector<std::shared_ptr<PBRMaterial>>::iterator end() { return Materials.end(); }
    std::shared_ptr<PBRMaterial>& operator[](const size_t index) noexcept { return Materials[index]; }
    const std::shared_ptr<PBRMaterial>& operator[](const size_t index) const noexcept { return Materials[index]; }
    uint8_t GetSize()const { return static_cast<uint8_t>(Materials.size()); }

    void Refresh();
    void BindTexture(oglu::ProgDraw& drawcall) const;
    uint32_t WriteData(std::byte *ptr) const; 
    
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
    RESPAK_DECL_SIMP_DESERIALIZE("rayr#MultiMaterialHolder")
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
