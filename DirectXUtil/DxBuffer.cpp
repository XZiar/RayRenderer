#include "DxPch.h"
#include "DxBuffer.h"
#include "ProxyStruct.h"


namespace dxu
{
MAKE_ENABLER_IMPL(DxBuffer_);


DxBuffer_::DxMapPtr_::DxMapPtr_(DxBuffer_* res, size_t offset, size_t size)
    : Resource(*res->Resource)
{
    void* ptr = nullptr;
    D3D12_RANGE range{ offset, size };
    THROW_HR(Resource.Map(0, &range, &ptr), u"Failed to map directx resource");
    MemSpace = { reinterpret_cast<std::byte*>(ptr), size };
}
DxBuffer_::DxMapPtr_::~DxMapPtr_()
{
    Resource.Unmap(0, nullptr);
}

class DxBuffer_::DxMapPtr2_ : public DxBuffer_::DxMapPtr_
{
private:
    MAKE_ENABLER();
    DxBuffer RealResource;
    MapFlags MapType;
    DxMapPtr2_(DxBuffer res, MapFlags flag, size_t offset, size_t size)
        : DxMapPtr_(res.get(), offset, size), RealResource(std::move(res)), MapType(flag)
    {

    }
public:
    ~DxMapPtr2_() override
    {
        if (HAS_FIELD(MapType, MapFlags::WriteOnly))
        {
            // TODO: do copy
        }
    }
    std::shared_ptr<const DxBuffer_::DxMapPtr_> Create(DxBuffer_* res, const DxCopyCmdQue& que, MapFlags flag, size_t offset, size_t size)
    {
        auto buf = DxBuffer_::Create(res->Device,
            flag == MapFlags::ReadOnly ? HeapType::Readback : HeapType::Upload,
            HeapFlags::Empty, size, ResourceFlags::Empty);
        if (flag != MapFlags::WriteOnly)
        {
            // TODO: do copy
            COMMON_THROWEX(DxException, u"unimplemented");
        }
        return MAKE_ENABLER_SHARED(DxMapPtr2_, (buf, flag, offset, size));
    }
};
MAKE_ENABLER_IMPL(DxBuffer_::DxMapPtr2_);

class DxBufMapPtr::DxMemInfo : public common::AlignedBuffer::ExternBufInfo
{
    std::shared_ptr<const DxBuffer_::DxMapPtr_> Ptr;
    [[nodiscard]] size_t GetSize() const noexcept override
    {
        return Ptr->MemSpace.size();
    }
    [[nodiscard]] std::byte* GetPtr() const noexcept override
    {
        return Ptr->MemSpace.data();
    }
public:
    DxMemInfo(std::shared_ptr<const DxBuffer_::DxMapPtr_> ptr) noexcept : Ptr(std::move(ptr)) { }
    ~DxMemInfo() override {}
};
common::AlignedBuffer DxBufMapPtr::AsBuffer() const noexcept
{
    return common::AlignedBuffer::CreateBuffer(std::make_unique<DxMemInfo>(Ptr));
}


DxResource_::ResDesc DxBuffer_::BufferDesc(ResourceFlags rFlag, size_t size) noexcept
{
    return 
    {
        D3D12_RESOURCE_DIMENSION_BUFFER,    // Dimension
        0,                                  // Alignment
        static_cast<uint64_t>(size),        // Width
        1,                                  // Height
        1,                                  // DepthOrArraySize
        1,                                  // MipLevels
        DXGI_FORMAT_UNKNOWN,                // Format
        {1, 0},                             // SampleDesc
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        static_cast<D3D12_RESOURCE_FLAGS>(rFlag)
    };
}
DxBuffer_::DxBuffer_(DxDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size)
    : DxResource_(device, heapProps, hFlag, BufferDesc(rFlag, size), ResourceState::Common), Size(size)
{ }
DxBuffer_::~DxBuffer_()
{ }

DxBufMapPtr DxBuffer_::Map(size_t offset, size_t size)
{
    if (HeapInfo.CPUPage == CPUPageProps::NotAvailable || HeapInfo.Memory == MemPrefer::PreferGPU)
        COMMON_THROWEX(DxException, u"Cannot map resource with CPUPageProps = NotAvaliable.\n")
        .Attach("heapinfo", HeapInfo);
    return DxBufMapPtr(std::static_pointer_cast<DxBuffer_>(shared_from_this()),
        std::make_shared<DxMapPtr_>(this, offset, size));
}

DxBufMapPtr DxBuffer_::Map(const DxCopyCmdQue & que, MapFlags flag, size_t offset, size_t size)
{
    if (HeapInfo.CPUPage != CPUPageProps::NotAvailable && HeapInfo.Memory != MemPrefer::PreferGPU)
        return Map(offset, size);
    return {};
    //return DxMapPtr(shared_from_this(), MAKE_ENABLER_SHARED(const DxMapPtr2_, (this, offset, size)));
}

DxBuffer DxBuffer_::Create(DxDevice device, HeapProps heapProps, HeapFlags hFlag, const size_t size, ResourceFlags rFlag)
{
    return MAKE_ENABLER_SHARED(DxBuffer_, (device, heapProps, hFlag, rFlag, size));
}


}
