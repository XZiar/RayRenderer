#pragma once
#include "dxRely.h"
#include "dxDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXResource_;
using DXResource = std::shared_ptr<DXResource_>;
class DXBuffer_;
using DXBuffer = std::shared_ptr<DXBuffer_>;



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
};


class DXMapPtr;


class DXUAPI DXResource_ : public common::NonCopyable, public std::enable_shared_from_this<DXResource_>
{
    friend class DXMapPtr; 
protected:
    MAKE_ENABLER();
    struct ResDesc;
    struct ResProxy;
    DXResource_(DXDevice device, HeapProps heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState);
public:
    virtual ~DXResource_();
    [[nodiscard]] virtual DXMapPtr Map(size_t offset, size_t size);

protected:
    PtrProxy<ResProxy> Resource;
    ResourceState State;
private:
    class DXUAPI COMMON_EMPTY_BASES DXMapPtr_ : public common::NonCopyable, public common::NonMovable
    {
        friend class DXMapPtr;
    private:
        ResProxy& Resource;
        common::span<std::byte> MemSpace;
    public:
        MAKE_ENABLER();
        DXMapPtr_(DXResource_* res, size_t offset, size_t size);
        ~DXMapPtr_();
    };
public:
    const HeapProps HeapInfo;
};


class DXUAPI DXMapPtr
{
    friend class DXResource_;
private:
    class DXMemInfo;
    DXResource Res;
    std::shared_ptr<const DXResource_::DXMapPtr_> Ptr;
    DXMapPtr(DXResource res, std::shared_ptr<const DXResource_::DXMapPtr_> ptr) noexcept 
        : Res(std::move(res)), Ptr(std::move(ptr)) { }
public:
    constexpr DXMapPtr() noexcept {}
    [[nodiscard]] common::span<std::byte> Get() const noexcept { return Ptr->MemSpace; }
    template<typename T>
    [[nodiscard]] common::span<T> AsType() const noexcept { return common::span<T>(reinterpret_cast<T*>(Get().data()), Get().size() / sizeof(T)); }
    [[nodiscard]] common::AlignedBuffer AsBuffer() const noexcept;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
