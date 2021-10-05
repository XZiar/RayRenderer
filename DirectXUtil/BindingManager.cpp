#include "DxPch.h"
#include "BindingManager.h"
#include "SystemCommon/StringDetect.h"


namespace dxu
{
using namespace std::string_view_literals;
using Microsoft::WRL::ComPtr;

struct DescHeap
{
    ComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE Handle = {};
    uint32_t Increment = 0;
    DescHeap(ID3D12Device& device, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
    {
        THROW_HR(device.CreateDescriptorHeap(&desc, IID_PPV_ARGS(Heap.GetAddressOf())), u"Failed to get desc");
        Handle = Heap->GetCPUDescriptorHandleForHeapStart();
        Increment = device.GetDescriptorHandleIncrementSize(desc.Type);
    }
    constexpr D3D12_CPU_DESCRIPTOR_HANDLE At(size_t idx) const noexcept
    {
        return { Handle.ptr + idx * Increment };
    }
};

PtrProxy<detail::DescHeap> DxBindingManager::GetCSUHeap(const DxDevice device) const
{
    if (BindBuffer.empty())
        return {};
    auto& dev = *device->Device;
    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc =
    {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        gsl::narrow_cast<uint32_t>(BindBuffer.size()),
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        0
    };
    DescHeap csuDescHeap(dev, descHeapDesc);
    for (const auto& binding : BindBuffer)
    {
        const auto& buf = *binding.Item.Buffer;
        const auto [format, isRaw] = [](const DxBuffer_::BufferView<DxBufferConst>& bufview) -> std::pair<DXGI_FORMAT, bool>
        {
            switch (bufview.Type & BoundedResourceType::InnerTypeMask)
            {
            case BoundedResourceType::InnerRaw:     return { DXGI_FORMAT_R32_TYPELESS, true };
            case BoundedResourceType::InnerTyped:   return { static_cast<DXGI_FORMAT>(detail::TexFormatToDXGIFormat(bufview.Format)), false };
            case BoundedResourceType::InnerStruct:  return { DXGI_FORMAT_UNKNOWN, false };
            default:                                return { DXGI_FORMAT_UNKNOWN, false };
            }
        }(binding.Item);
        const auto handle = csuDescHeap.At(binding.DescOffset);
        switch (binding.Item.Type & BoundedResourceType::CategoryMask)
        {
        case BoundedResourceType::CBVs:
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.SizeInBytes = binding.Item.Count;
            cbvDesc.BufferLocation = binding.Item.Buffer->GetGPUVirtualAddress() + binding.Item.Offset;
            dxLog().verbose(u"Create CBV to [{}].\n"sv, binding.Item.Buffer->GetName());
            dev.CreateConstantBufferView(&cbvDesc, handle);
        } break;
        case BoundedResourceType::SRVs:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement = binding.Item.Offset;
            srvDesc.Buffer.NumElements = binding.Item.Count;
            srvDesc.Buffer.StructureByteStride = binding.Item.Stride;
            srvDesc.Buffer.Flags = isRaw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
            dxLog().verbose(u"Create SRV to [{}], Format[{}] Stride[{}] Flag[{}].\n"sv, binding.Item.Buffer->GetName(),
                static_cast<uint32_t>(format), srvDesc.Buffer.StructureByteStride, static_cast<uint32_t>(srvDesc.Buffer.Flags));
            dev.CreateShaderResourceView(buf.Resource, &srvDesc, handle);
        } break;
        case BoundedResourceType::UAVs:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = binding.Item.Offset;
            uavDesc.Buffer.NumElements = binding.Item.Count;
            uavDesc.Buffer.StructureByteStride = binding.Item.Stride;
            uavDesc.Buffer.CounterOffsetInBytes = 0;
            uavDesc.Buffer.Flags = isRaw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
            dxLog().verbose(u"Create UAV to [{}], Format[{}] Stride[{}] Flag[{}].\n"sv, binding.Item.Buffer->GetName(),
                static_cast<uint32_t>(format), uavDesc.Buffer.StructureByteStride, static_cast<uint32_t>(uavDesc.Buffer.Flags));
            dev.CreateUnorderedAccessView(buf.Resource, nullptr, &uavDesc, handle);
        } break;
        default:
            Expects(false);
            break;
        }
    }
    common::mlog::SyncConsoleBackend();
    return PtrProxy<detail::DescHeap>{ csuDescHeap.Heap.Detach() };
}


uint16_t DxSharedBindingManager::SetBuf(DxBuffer_::BufferView<DxBufferConst> bufview, uint16_t offset)
{
    if (offset == UINT16_MAX)
    {
        for (auto& bind : BindBuffer)
        {
            if (bind.Item == bufview)
            {
                bind.Reference++;
                return bind.DescOffset;
            }
        }
        // not find, assign next slot
    }
    else
    {
        for (auto& bind : BindBuffer)
        {
            if (bind.DescOffset == offset)
            {
                if (bind.Reference > 1)
                {
                    bind.Reference--;
                    break;
                }
                bind.Item = std::move(bufview);
                return bind.DescOffset;
            }
        }
        // not find, or has multiple reference
    }
    const auto slot = gsl::narrow_cast<uint16_t>(BindBuffer.size() + BindTexture.size());
    BindBuffer.push_back({ std::move(bufview), slot });
    return slot;
}


DxUniqueBindingManager::DxUniqueBindingManager(size_t bufCount, size_t texCount, size_t samplerCount)
{
    BindBuffer.resize(bufCount);
    BindTexture.resize(texCount);
    BindSampler.resize(samplerCount);
    uint16_t idx = 0;
    for (auto& bind : BindBuffer)
        bind.DescOffset = idx++;
    for (auto& bind : BindTexture)
        bind.DescOffset = idx++;
    idx = 0;
    for (auto& bind : BindSampler)
        bind.DescOffset = idx++;
}

uint16_t DxUniqueBindingManager::SetBuf(DxBuffer_::BufferView<DxBufferConst> bufview, uint16_t offset)
{
    if (offset >= BindBuffer.size())
        COMMON_THROW(DxException, FMTSTR(u"Bind buffer [{}] to offset [{}] overflow of total [{}] desc",
            bufview.Buffer->GetName(), offset, BindBuffer.size()));
    BindBuffer[offset].Item = std::move(bufview);
    return offset;
}


}

