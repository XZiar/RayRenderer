#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "common/CLikeConfig.hpp"
#include "common/StringPool.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxShader_;
using DxShader = std::shared_ptr<DxShader_>;
class DxComputeShader_;
using DxComputeShader = std::shared_ptr<DxComputeShader_>;
class DxShaderStub_;
template<typename T>
class DxShaderStub;


enum class ShaderType
{
    Vertex, 
    Geometry, 
    Pixel,
    Hull,
    Domain,
    Compute
};

enum class BoundedResourceType : uint8_t
{
    Other, CBuffer, TBuffer, Texture, Sampler, TypedUAV, UntypedUAV, ByteAddressUAV,
};

struct DxShaderConfig
{
    common::CLikeDefines Defines;
    std::set<std::string> Flags;
    uint32_t SMVersion = 0;
    std::u16string EntryPoint;
};


class DXUAPI DxShader_
{
    friend class DxShaderStub_;
    friend class DxShaderStub<DxShader_>;
private:
    struct BoundedResWrapper
    {
        std::string_view Name;
        uint32_t Index;
        uint32_t Count;
        BoundedResourceType Type;
    };
    BoundedResWrapper GetBufSlot(const size_t idx) const noexcept;
    BoundedResWrapper GetTexSlot(const size_t idx) const noexcept;
    using ItTypeBuf = common::container::IndirectIterator<const DxShader_, BoundedResWrapper, &DxShader_::GetBufSlot>;
    using ItTypeTex = common::container::IndirectIterator<const DxShader_, BoundedResWrapper, &DxShader_::GetTexSlot>;
    friend ItTypeBuf;
    friend ItTypeTex;
    class BufSlotList
    {
        friend class DxShader_;
        const DxShader_* Host;
        constexpr BufSlotList(const DxShader_* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeBuf begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeBuf end()   const noexcept { return { Host, Host->BufferSlots.size() }; }
    };
    class TexSlotList
    {
        friend class DxShader_;
        const DxShader_* Host;
        constexpr TexSlotList(const DxShader_* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeBuf begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeBuf end()   const noexcept { return { Host, Host->TextureSlots.size() }; }
    };
    struct ShaderBlob;
protected:
    struct T_ {};
    struct BoundedResource
    {
        uint64_t Hash;
        common::StringPiece<char> Name;
        uint32_t Index;
        uint32_t Count;
        BoundedResourceType Type;
    };
    const std::string Source;
    PtrProxy<ShaderBlob> Blob;
    std::string ShaderHash;
    common::StringPool<char> StrPool;
    std::vector<BoundedResource> BufferSlots;
    std::vector<BoundedResource> TextureSlots;
    ShaderType Type;
    uint32_t Version;

    const BoundedResource* GetSlot(const std::vector<BoundedResource>& container, common::str::HashedStrView<char> name);
public:
    DxShader_(T_, DxShaderStub_* stub);
    virtual ~DxShader_();
    constexpr BufSlotList BufSlots() const noexcept { return this; }
    constexpr TexSlotList TexSlots() const noexcept { return this; }

    [[nodiscard]] static DxShaderStub<DxShader_> Create(DxDevice dev, ShaderType type, std::string str);
    [[nodiscard]] static DxShader CreateAndBuild(DxDevice dev, ShaderType type, std::string str, const DxShaderConfig& config);
    [[nodiscard]] static std::string_view GetBoundedResTypeName(const BoundedResourceType type) noexcept;
};


class DXUAPI [[nodiscard]] DxShaderStub_ : public common::NonCopyable
{
    friend class DxShader_;
private:
    DxDevice Device;
    std::string Source;
    PtrProxy<DxShader_::ShaderBlob> Blob;
    ShaderType Type;
protected:
    DxShaderStub_(DxDevice dev, ShaderType type, std::string&& str);
public:
    ~DxShaderStub_();
    void Build(const DxShaderConfig& config);
    [[nodiscard]] std::u16string GetBuildLog() const;
};
template<typename T>
class [[nodiscard]] DxShaderStub : public DxShaderStub_
{
    friend T;
private:
    using DxShaderStub_::DxShaderStub_;
public:
    [[nodiscard]] std::shared_ptr<T> Finish()
    {
        using X = typename T::T_;
        return std::make_shared<T>(X{}, this);
    }
};


class DXUAPI DxComputeShader_ : public DxShader_
{
    friend class DxShaderStub<DxComputeShader_>;
private:
    struct T_ {};
public:
    DxComputeShader_(T_, DxShaderStub_* stub);
    ~DxComputeShader_() override;

    [[nodiscard]] static DxComputeShader Create(DxDevice dev, std::string str, const DxShaderConfig& config);
};


}