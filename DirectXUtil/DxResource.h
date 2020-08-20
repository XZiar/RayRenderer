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
using DxBuffer = std::shared_ptr<DxBuffer_>;



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

enum class ResourceState : uint32_t
{
    Common              = 0, 
    ConstBuffer         = 0x1, 
    IndexBuffer         = 0x2, 
    RenderTarget        = 0x4, 
    UnorderAccess       = 0x8, 
    DepthWrite          = 0x10, 
    DepthRead           = 0x20, 
    NonPSRes            = 0x40, 
    PsRes               = 0x80, 
    StreamOut           = 0x100, 
    IndirectArg         = 0x200,
    CopyDst             = 0x400, 
    CopySrc             = 0x800, 
    ResolveSrc          = 0x1000, 
    ResolveDst          = 0x2000, 
    RTStruct            = 0x400000, 
    ShadeRateRes        = 0x1000000, 
    Read                = ConstBuffer | IndexBuffer | NonPSRes | PsRes | IndirectArg | CopySrc, 
    Present             = 0, 
    Pred                = 0x200,
    VideoDecodeRead     = 0x10000, 
    VideoDecodeWrite    = 0x20000, 
    VideoProcessRead    = 0x40000, 
    VideoProcessWrite   = 0x80000, 
    VideoEncodeRead     = 0x200000, 
    VideoEncodeWrite    = 0x800000,
};


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
public:
    virtual ~DxResource_();

protected:
    DxDevice Device;
    PtrProxy<ResProxy> Resource;
    ResourceState State;
public:
    const HeapProps HeapInfo;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
