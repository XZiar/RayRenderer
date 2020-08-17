#include "dxPch.h"
#include "dxResource.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DXResource_);
MAKE_ENABLER_IMPL(DXResource_::DXMapPtr_);


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


static ResourceState FixState(ResourceState state, HeapType type) noexcept
{
    switch (type)
    {
    case HeapType::Upload:      return ResourceState::Read;
    case HeapType::Readback:    return ResourceState::CopyDst;
    default:                    return state;
    }
}
static HeapProps FillProps(HeapProps props, DXDevice dev) noexcept
{
    switch (props.Type)
    {
    case HeapType::Upload:      std::tie(props.CPUPage, props.Memory) = dev->HeapUpload;    break;
    case HeapType::Default:     std::tie(props.CPUPage, props.Memory) = dev->HeapDefault;   break;
    case HeapType::Readback:    std::tie(props.CPUPage, props.Memory) = dev->HeapReadback;  break;
    default:                    break;
    }
    return props;
}
DXResource_::DXResource_(DXDevice device, HeapProps heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState) 
    : State(FixState(initState, heapProps.Type)), HeapInfo(FillProps(heapProps, device))
{
    //const auto tmp = CD3DX12_RESOURCE_DESC::Buffer(desc.Width);
    D3D12_HEAP_PROPERTIES props
    {
        static_cast<D3D12_HEAP_TYPE>(heapProps.Type),
        static_cast<D3D12_CPU_PAGE_PROPERTY>(heapProps.CPUPage),
        static_cast<D3D12_MEMORY_POOL>(heapProps.Memory),
        0,
        0
    };

    THROW_HR(device->Device->CreateCommittedResource(
        &props,
        static_cast<D3D12_HEAP_FLAGS>(heapFlag),
        &desc,
        static_cast<D3D12_RESOURCE_STATES>(State),
        nullptr,
        IID_PPV_ARGS(&Resource)
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


}
