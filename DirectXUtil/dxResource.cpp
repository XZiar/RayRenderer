#include "dxPch.h"
#include "dxResource.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DXResource_);
MAKE_ENABLER_IMPL(DXResource_::DXMapPtr_);
MAKE_ENABLER_IMPL(DXBuffer_);


DXResource_::DXMapPtr_::DXMapPtr_(DXResource_* res, size_t offset, size_t size) 
    : Resource(*res->Resource)
{
    void* ptr = nullptr;
    D3D12_RANGE range{ offset, size };
    THROW_HR(Resource.Map(0, &range, &ptr), u"Failed to map directx resource");
    MemSpace = { reinterpret_cast<std::byte*>(ptr), size };
}
DXResource_::DXMapPtr_::~DXMapPtr_()
{
    Resource.Unmap(0, nullptr);
}

class DXMapPtr::DXMemInfo : public common::AlignedBuffer::ExternBufInfo
{
    std::shared_ptr<const DXResource_::DXMapPtr_> Ptr;
    [[nodiscard]] size_t GetSize() const noexcept override
    {
        return Ptr->MemSpace.size();
    }
    [[nodiscard]] std::byte* GetPtr() const noexcept override
    {
        return Ptr->MemSpace.data();
    }
public:
    DXMemInfo(std::shared_ptr<const DXResource_::DXMapPtr_> ptr) noexcept : Ptr(std::move(ptr)) { }
    ~DXMemInfo() override {}
};
common::AlignedBuffer DXMapPtr::AsBuffer() const noexcept
{
    return common::AlignedBuffer::CreateBuffer(std::make_unique<DXMemInfo>(Ptr));
}


DXResource_::DXResource_(DXDevice device, const HeapProps& heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState) 
{
    THROW_HR(device->Device->CreateCommittedResource(
        &static_cast<const D3D12_HEAP_PROPERTIES&>(heapProps),
        static_cast<D3D12_HEAP_FLAGS>(heapFlag),
        &static_cast<const D3D12_RESOURCE_DESC&>(desc),
        static_cast<D3D12_RESOURCE_STATES>(initState),
        nullptr,
        IID_PPV_ARGS(Resource.PPtr())
    ), u"Failed to create committed resource");
}

DXResource_::~DXResource_()
{
    Resource->Release();
}

DXMapPtr DXResource_::Map(size_t offset, size_t size)
{
    return DXMapPtr(shared_from_this(), MAKE_ENABLER_SHARED(const DXMapPtr_, (this, offset, size)));
}


DXBuffer_::DXBuffer_(DXDevice device, const HeapProps& heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState)
    : DXResource_(device, heapProps, heapFlag, desc, initState), Size(gsl::narrow_cast<size_t>(desc.Width))
{}

DXBuffer_::~DXBuffer_()
{ }

DXBuffer DXBuffer_::Create(DXDevice device, HeapType type, HeapFlags flag, const size_t size)
{
    const HeapProps heapProps =
    { 
        static_cast<D3D12_HEAP_TYPE>(type), 
        D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };
    const ResDesc desc =
    {
        D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER,  // Dimension
        0,                                                          // Alignment
        static_cast<uint64_t>(size),                                // Width
        1,                                                          // Height
        1,                                                          // DepthOrArraySize
        1,                                                          // MipLevels
        DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,                           // Format
        {1, 0},                                                     // SampleDesc
        D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };
    return MAKE_ENABLER_SHARED(DXBuffer_, (device, heapProps, flag, desc, ResourceState::Common));
}



}
