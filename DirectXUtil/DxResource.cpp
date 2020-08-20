#include "DxPch.h"
#include "DxResource.h"
#include "DxBuffer.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DxResource_);



static ResourceState FixState(ResourceState state, HeapType type) noexcept
{
    switch (type)
    {
    case HeapType::Upload:      return ResourceState::Read;
    case HeapType::Readback:    return ResourceState::CopyDst;
    default:                    return state;
    }
}
static HeapProps ProcessProps(HeapProps props, DxDevice dev) noexcept
{
    switch (props.Type)
    {
    case HeapType::Upload:      std::tie(props.CPUPage, props.Memory) = dev->HeapUpload;    break;
    case HeapType::Default:     std::tie(props.CPUPage, props.Memory) = dev->HeapDefault;   break;
    case HeapType::Readback:    std::tie(props.CPUPage, props.Memory) = dev->HeapReadback;  break;
    case HeapType::Custom:
    {
        constexpr auto FixProp = [](auto& prop, auto value, std::u16string_view reason)
        {
            if (prop != value)
            {
                prop = value;
                dxLog().warning(reason);
            }
        };
        if (dev->IsUMA()) 
            FixProp(props.Memory, MemPrefer::PreferCPU, u"Force PreferCPU because UMA arch.\n");
        if (props.Memory == MemPrefer::PreferGPU) 
            FixProp(props.CPUPage, CPUPageProps::NotAvailable, u"Force CPU-Not-Available because PreferGPU.\n");
    } break;
    default: break;
    }
    return props;
}
DxResource_::DxResource_(DxDevice device, HeapProps heapProps, HeapFlags heapFlag, const ResDesc& desc, ResourceState initState) 
    : Device(device), State(FixState(initState, heapProps.Type)), 
    HeapInfo(ProcessProps(heapProps, device))
{
    const D3D12_HEAP_PROPERTIES props
    {
        static_cast<D3D12_HEAP_TYPE>(HeapInfo.Type),
        static_cast<D3D12_CPU_PAGE_PROPERTY>(HeapInfo.GetCPUPage(true)),
        static_cast<D3D12_MEMORY_POOL>(HeapInfo.GetMemory(true)),
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
DxResource_::~DxResource_()
{
    Resource->Release();
}


}
