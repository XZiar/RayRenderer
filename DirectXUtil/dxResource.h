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


enum class HeapType : uint8_t { Default, Upload, Readback, Custom };
enum class CPUPageProps : uint8_t { Unknown, NotAvailable, WriteCombine, WriteBack };
enum class MemoryPool : uint8_t { Unknown, PreferCPU, PreferGPU };


enum class HeapFlags : uint32_t
{
    Empty = 0x0,
    Shared = 0x1, AdapterShared = 0x20,
    DenyBuffer = 0x4, DenyRTV = 0x40, DenyTex = 0x80,
    AllowDisplay = 0x8, AllowAtomics = 0x400,
};
MAKE_ENUM_BITFIELD(HeapFlags)

enum class ResourceState : uint32_t
{
    Common, ConstBuffer, IndexBuffer, RenderTarget, UnorderAccess, DepthWrite, DepthRead, NonPSRes, PsRes, StreamOut, IndirectArg,
    CopyDst, CopySrc, RTStruct, ShadeRateRes, Read, Present, Pred, 
    VideoDecodeRead, VideoDecodeWrite, VideoProcessRead, VideoProcessWrite, VideoEncodeRead, VideoEncodeWrite
};


class DXMapPtr;


class DXUAPI DXResource_ : public common::NonCopyable, public std::enable_shared_from_this<DXResource_>
{
    friend class DXMapPtr;
protected:
    MAKE_ENABLER();
    struct HeapProps;
    struct ResDesc;
    DXResource_(DXDevice device, const HeapProps& heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState);
    struct ResProxy;
    PtrProxy<ResProxy> Resource;
private:
    class DXUAPI DXMapPtr_ : public common::NonCopyable, public common::NonMovable
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
    virtual ~DXResource_();
    [[nodiscard]] DXMapPtr Map(size_t offset, size_t size);
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


class DXUAPI DXBuffer_ : public DXResource_
{
protected:
    MAKE_ENABLER();
    DXBuffer_(DXDevice device, const HeapProps& heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState);
public:
    const size_t Size;
    ~DXBuffer_() override;
    [[nodiscard]] static DXBuffer Create(DXDevice device, HeapType type, HeapFlags flag, const size_t size);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
