#include "DxPch.h"
#include "DxResource.h"
#include "DxBuffer.h"

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
DxResource_::DxResource_(DxDevice device, HeapProps heapProps, HeapFlags heapFlag, const detail::ResourceDesc& desc, ResourceState initState) :
    Device(device), InitState(FixState(initState, heapProps.Type)),
    ResFlags(static_cast<ResourceFlags>(desc.Flags)), HeapInfo(ProcessProps(heapProps, device))
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
        static_cast<D3D12_RESOURCE_STATES>(InitState),
        nullptr,
        IID_PPV_ARGS(&Resource)
    ), u"Failed to create committed resource");
}
DxResource_::~DxResource_()
{
}

void* DxResource_::GetD3D12Object() const noexcept
{
    return static_cast<ID3D12Object*>(Resource.Ptr());
}

bool DxResource_::IsBufOrSATex() const noexcept
{
    return false;
}

void DxResource_::CopyRegionFrom(const DxCmdList& list, const uint64_t offset, const DxResource_& src, const uint64_t srcOffset, const uint64_t numBytes) const
{
    list->CmdList->CopyBufferRegion(Resource, offset, src.Resource, srcOffset, numBytes);
}

void DxResource_::TransitState(const DxCmdList& list, ResourceState newState, bool fromInitState) const
{
    // common check
    switch (HeapInfo.Type)
    {
    case HeapType::Readback:
        if (newState != ResourceState::CopyDst)
            COMMON_THROW(DxException, u"committed resources created in READBACK heaps must start in and cannot change from the COPY_DEST state");
        return; // skip because cannot change state
    case HeapType::Upload:
        if (newState != ResourceState::Read)
            COMMON_THROW(DxException, u"resources created on UPLOAD heaps must start in and cannot change from the GENERIC_READ state");
        return; // skip because cannot change state
    default:
        break;
    }
    const bool isBufOrSATex = IsBufOrSATex();
    if (isBufOrSATex && HAS_FIELD(newState, ResourceState::DepthWrite | ResourceState::DepthRead))
        COMMON_THROW(DxException, FMTSTR(u"Buffer and Simultaneous-Access Texture can not set to type [DepthWrite|DepthRead], get[{}]",
            common::enum_cast(newState)));
    list->UpdateResState(this, newState, fromInitState);
}

uint64_t DxResource_::GetGPUVirtualAddress() const
{
    return Resource->GetGPUVirtualAddress();
}



}
