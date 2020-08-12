#include "dxPch.h"
#include "dxDevice.h"
#include "ProxyStruct.h"

namespace dxu
{

DXDevice_::DXDevice_(void* adapter, void* device) : 
    Adapter(reinterpret_cast<AdapterProxy*>(adapter)), Device(reinterpret_cast<DeviceProxy*>(device))
{
    DXGI_ADAPTER_DESC1 desc;
    if (Adapter->GetDesc1(&desc) >= 0)
    {
        AdapterName.assign(reinterpret_cast<const char16_t*>(desc.Description));
        dxLog().verbose(u"Created device on {}.\n", AdapterName);
    }
}
DXDevice_::DXDevice_(DXDevice_&& other) noexcept : 
    Adapter(other.Adapter), Device(other.Device),
    AdapterName(std::move(other.AdapterName))
{
    other.Device  = nullptr;
    other.Adapter = nullptr;
}
DXDevice_::~DXDevice_()
{
    if (Device)
        Device->Release();
    if (Adapter)
        Adapter->Release();
}

common::span<const DXDevice_> DXDevice_::GetDevices()
{
    static const auto devices = []()
    {
#if defined(DEBUG) || defined(_DEBUG)
        {
            ID3D12Debug* debug;
            if (const auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug)); !FAILED(hr))
            {
                debug->EnableDebugLayer();
                debug->Release();
            }
        }
#endif
        IDXGIFactory4* factory = nullptr;
        CreateDXGIFactory(IID_PPV_ARGS(&factory));

        std::vector<DXDevice_> devs;
        for (uint32_t idx = 0; ; ++idx)
        {
            IDXGIAdapter1* adapter = nullptr;
            if (factory->EnumAdapters1(idx, &adapter) == DXGI_ERROR_NOT_FOUND)
                break;
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;
            ID3D12Device* device = nullptr;
            if (const auto hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_1_0_CORE, IID_PPV_ARGS(&device)); FAILED(hr))
            {
                if (const auto hr2 = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)); FAILED(hr2))
                    COMMON_THROWEX(DXException, hr2, u"Failed to create device");
            }
            devs.push_back(DXDevice_(adapter, device));
        }

        return devs;
    }();
    return devices;
}


}
