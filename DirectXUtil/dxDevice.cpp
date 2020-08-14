#include "dxPch.h"
#include "dxDevice.h"
#include "ProxyStruct.h"

namespace dxu
{

DXDevice_::DXDevice_(void* adapter, void* device, std::u16string_view name) :
    Adapter(reinterpret_cast<AdapterProxy*>(adapter)), Device(reinterpret_cast<DeviceProxy*>(device)),
    AdapterName(name)
{ }
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
            else
            {
                dxLog().warning(u"Failed to enable debug layer: {}.\n", common::HrToStr(hr));
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
            DXGI_ADAPTER_DESC1 desc = {};
            if (FAILED(adapter->GetDesc1(&desc)))
                continue;
            std::u16string_view adapterName = reinterpret_cast<const char16_t*>(desc.Description);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                dxLog().verbose(u"Skip software adapter [{}].\n", adapterName);
                continue;
            }
            ID3D12Device* device = nullptr;
            {
                constexpr std::array<D3D_FEATURE_LEVEL, 2> FeatureLvs =
                    { D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_1_0_CORE };
                std::array<common::HResultHolder, 2> hrs = { 0 };
                for (size_t fid = 0; fid < FeatureLvs.size(); ++fid)
                {
                    hrs[fid] = D3D12CreateDevice(adapter, FeatureLvs[fid], IID_PPV_ARGS(&device));
                    if (hrs[fid])
                        break;
                }
                if (!device)
                {
                    dxLog().warning(u"Failed to created device on [{}]:\n12_0: {}\nCore1_0: {}\n", adapterName, hrs[0], hrs[1]);
                    continue;
                }
            }
            dxLog().verbose(u"Created device on {}.\n", adapterName);
            devs.push_back(DXDevice_(adapter, device, adapterName));
        }

        return devs;
    }();
    return devices;
}


}
