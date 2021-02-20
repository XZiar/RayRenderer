#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxCmdQue.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxResource_;
using DxResource = std::shared_ptr<DxResource_>;
class DxBuffer_;
class DxBindingManager;


enum class HeapFlags : uint32_t
{
    Empty = 0x0,
    Shared = 0x1, AdapterShared = 0x20,
    DenyBuffer = 0x4, DenyRTV = 0x40, DenyTex = 0x80,
    AllowDisplay = 0x8, AllowAtomics = 0x400,
};
MAKE_ENUM_BITFIELD(HeapFlags)

struct HeapProps
{
    HeapType Type;
    CPUPageProps CPUPage;
    MemPrefer Memory;
    constexpr HeapProps(HeapType type) noexcept
        : Type(type), CPUPage(CPUPageProps::Unknown), Memory(MemPrefer::Unknown)
    {
        Expects(Type != HeapType::Custom);
    }
    constexpr HeapProps(CPUPageProps pageProp, MemPrefer memPrefer) noexcept
        : Type(HeapType::Custom), CPUPage(pageProp), Memory(memPrefer)
    { }
    constexpr CPUPageProps GetCPUPage(bool conformance = false) const noexcept
    {
        return (conformance && Type != HeapType::Custom) ? CPUPageProps::Unknown : CPUPage;
    }
    constexpr MemPrefer GetMemory(bool conformance = false) const noexcept
    {
        return (conformance && Type != HeapType::Custom) ? MemPrefer::Unknown : Memory;
    }
};


enum class MapFlags : uint8_t
{
    ReadOnly = 0x1, WriteOnly = 0x2, ReadWrite = ReadOnly | WriteOnly,
};
MAKE_ENUM_BITFIELD(MapFlags)


class DXUAPI DxResource_ : public DxNamable, public std::enable_shared_from_this<DxResource_>
{
    friend DxBindingManager;
    friend DxCmdList_;
private:
    [[nodiscard]] void* GetD3D12Object() const noexcept final;
    [[nodiscard]] virtual bool IsBufOrSATex() const noexcept; // Buffers and Simultaneous-Access Textures
protected:
    MAKE_ENABLER();
    DxResource_(DxDevice device, HeapProps heapProps, HeapFlags heapFlag, const detail::ResourceDesc& desc, ResourceState initState);
    void CopyRegionFrom(const DxCmdList& list, const uint64_t offset,
        const DxResource_& src, const uint64_t srcOffset, const uint64_t numBytes) const;
    DxDevice Device;
    PtrProxy<detail::Resource> Resource;
    ResourceState InitState;
public:
    const ResourceFlags ResFlags;
    const HeapProps HeapInfo;
    ~DxResource_() override;
    COMMON_NO_COPY(DxResource_)
    COMMON_NO_MOVE(DxResource_)
    void TransitState(const DxCmdList& list, ResourceState newState, bool fromInitState = false) const;
    [[nodiscard]] uint64_t GetGPUVirtualAddress() const;
    [[nodiscard]] constexpr bool CanBindToShader() const noexcept
    {
        return !(HAS_FIELD(ResFlags, ResourceFlags::DenyShaderResource) && !HAS_FIELD(ResFlags, ResourceFlags::AllowUnorderAccess));
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
