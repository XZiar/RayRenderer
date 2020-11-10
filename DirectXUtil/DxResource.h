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



enum class HeapFlags : uint32_t
{
    Empty = 0x0,
    Shared = 0x1, AdapterShared = 0x20,
    DenyBuffer = 0x4, DenyRTV = 0x40, DenyTex = 0x80,
    AllowDisplay = 0x8, AllowAtomics = 0x400,
};
MAKE_ENUM_BITFIELD(HeapFlags)

enum class ResourceFlags : uint32_t
{
    Empty = 0x0,
    AllowRT = 0x1, AllowDepthStencil = 0x2,
    AllowUnorderAccess = 0x4,
    DenyShaderResource = 0x8,
    AllowCrossAdpter = 0x10,
    AllowSimultaneousAccess = 0x20,
    VideoDecodeOnly = 0x40,
};
MAKE_ENUM_BITFIELD(ResourceFlags)

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


class DXUAPI DxResource_ : public common::NonCopyable, public std::enable_shared_from_this<DxResource_>
{
protected:
    MAKE_ENABLER();
    struct ResDesc;
    struct ResProxy;
    DxResource_(DxDevice device, HeapProps heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState);
    void CopyRegionFrom(const DxCmdList& list, const uint64_t offset,
        const DxResource_& src, const uint64_t srcOffset, const uint64_t numBytes) const;
    ResourceState TransitState(const DxCmdList& list, ResourceState newState) const;
public:
    virtual ~DxResource_();
protected:
    DxDevice Device;
    PtrProxy<ResProxy> Resource;
    std::atomic<ResourceState> State;
public:
    const HeapProps HeapInfo;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
