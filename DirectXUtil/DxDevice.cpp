#include "DxPch.h"
#include "DxDevice.h"
#include "common/StringEx.hpp"
#if __has_include("dxcore.h")
#   include <initguid.h>
#   include <dxcore.h>
#   define HAS_DXCORE 1
#   pragma message("Compiling DirectXUtil with DXCore")
#   include "SystemCommon/DynamicLibrary.h"
#endif

typedef _Check_return_ HRESULT(APIENTRY* PFN_DXCoreCreateAdapterFactory)(REFIID riid, _COM_Outptr_ void** ppvFactory);

namespace dxu
{
MAKE_ENABLER_IMPL(DxDevice_);
using common::HResultHolder;


template<typename T, typename... Args>
static forceinline detail::OptRet<T> CheckFeat_(D3D12_FEATURE feat, ID3D12Device* dev, Args... args)
{
    detail::OptRet<T> ret = { args... };
    ret.HResult.Value = dev->CheckFeatureSupport(feat, &ret.Data, sizeof(T));
    return ret;
}

static std::pair<CPUPageProps, MemPrefer> QueryHeapProp(ID3D12Device* dev, D3D12_HEAP_TYPE type)
{
    const auto prop = dev->GetCustomHeapProperties(0, type);
    return { static_cast<CPUPageProps>(prop.CPUPageProperty), static_cast<MemPrefer>(prop.MemoryPoolPreference) };
}

DxDevice_::DxDevice_(PtrProxy<detail::Adapter> adapter, PtrProxy<detail::Device> device, 
    std::u16string_view name, FeatureLevels featLv, bool isDxcore) :
    Adapter(std::move(adapter)), Device(std::move(device)), AdapterName(name), 
    HeapUpload  (QueryHeapProp(Device, D3D12_HEAP_TYPE_UPLOAD)), 
    HeapDefault (QueryHeapProp(Device, D3D12_HEAP_TYPE_DEFAULT)), 
    HeapReadback(QueryHeapProp(Device, D3D12_HEAP_TYPE_READBACK)),
    FeatureLevel(featLv), IsDxCore(isDxcore)
{
    {
        const auto luid = GetLUID();
        for (const auto& dev : xcomp::ProbeDevice())
        {
            if (dev.Luid == luid)
            {
                XCompDevice = &dev;
                PCIEAddress = dev.PCIEAddress;
                break;
            }
        }
    }

#define CheckFeat(dev, feat) CheckFeat_<D3D12_FEATURE_DATA_##feat>(D3D12_FEATURE_##feat, dev)
#define CheckFeat2(dev, feat, ...) CheckFeat_<D3D12_FEATURE_DATA_##feat>(D3D12_FEATURE_##feat, dev, __VA_ARGS__)
    for (int smver = 0x69; smver >= 0x60; smver--) // loop smver to fit different OS version.
    {
        const auto ret = CheckFeat2(Device, SHADER_MODEL, static_cast<D3D_SHADER_MODEL>(smver));
        if (ret)
        {
            SMVer = (ret->HighestShaderModel & 0xf) + (ret->HighestShaderModel >> 4) * 10;
            break;
        }
        else if (ret.HResult.Value != E_INVALIDARG)
            COMMON_THROWEX(DxException, ret.HResult, u"Failed to check SM version");
    }
    SMVer = std::max(SMVer, 51u); // at least SM5.1 for DX12
    if (const auto feat = CheckFeat2(Device, SHADER_MODEL, D3D_SHADER_MODEL(0x66)))
        SMVer = (feat->HighestShaderModel & 0xf) + (feat->HighestShaderModel >> 4) * 10;
    else
        COMMON_THROWEX(DxException, feat.HResult, u"Failed to check SM version");
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS))
    {
        SupportFP64 = feat->DoublePrecisionFloatShaderOps;
        SupportROV  = feat->ROVsSupported;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS1))
    {
        if (feat->WaveOps)
        {
            WaveSize.first  = feat->WaveLaneCountMin;
            WaveSize.second = feat->WaveLaneCountMax;
        }
        SupportINT64 = feat->Int64ShaderOps;
        ExtComputeResState = feat->ExpandedComputeResourceStates;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS2))
    {
        DepthBoundTest = feat->DepthBoundsTestSupported;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS3))
    {
        CopyQueueTimeQuery = feat->CopyQueueTimestampQueriesSupported;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS4))
    {
        SupportINT16 = SupportFP16 = feat->Native16BitShaderOpsSupported;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS6))
    {
        BackgroundProcessing = feat->BackgroundProcessingSupported;
    }
    
    if (const auto feat1 = CheckFeat(Device, ARCHITECTURE1))
    {
        IsTBR                       = feat1->TileBasedRenderer;
        IsUMA                       = feat1->UMA;
        IsUMACacheCoherent = IsUMA  = feat1->CacheCoherentUMA;
        IsIsolatedMMU               = feat1->IsolatedMMU;
    }
    else if (const auto feat2 = CheckFeat(Device, ARCHITECTURE))
    {
        IsTBR                       = feat2->TileBasedRenderer;
        IsUMA                       = feat2->UMA;
        IsUMACacheCoherent = IsUMA  = feat2->CacheCoherentUMA;
    }
    else
    {
        dxLog().Warning(FmtString(u"Failed to check architecture: [{}, {}]"), feat1.HResult.ToStr(), feat2.HResult.ToStr());
        COMMON_THROWEX(DxException, feat2.HResult, u"Failed to check architecture");
    }
}
DxDevice_::~DxDevice_()
{ }

std::array<std::byte, 8> DxDevice_::GetLUID() const noexcept
{
    const auto luid = Device->GetAdapterLuid();
    std::array<std::byte, 8> data;
    memcpy_s(data.data(), sizeof(data), &luid, sizeof(luid));
    return data;
}

common::span<const DxDevice> DxDevice_::GetDevices()
{
    static const auto devices = []()
    {
#if defined(DEBUG) || defined(_DEBUG)
        {
            ID3D12Debug* debug = nullptr;
            if (const HResultHolder hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug)); hr)
            {
                debug->EnableDebugLayer();
                debug->Release();
            }
            else
            {
                dxLog().Warning(u"Failed to enable debug layer: {}.\n", hr.ToStr());
            }
        }
#endif
        std::vector<std::unique_ptr<DxDevice_>> devs;

        const auto CreateDevice = [&](IUnknown* adapter, std::u16string_view name, bool isDxcore) -> DxDevice_*
        {
            constexpr std::array<std::pair<D3D_FEATURE_LEVEL, FeatureLevels>, 2> FeatureLvs = 
            { 
                std::pair{ D3D_FEATURE_LEVEL_12_0,     FeatureLevels::Dx12  }, 
                std::pair{ D3D_FEATURE_LEVEL_1_0_CORE, FeatureLevels::Core1 } 
            };
            std::array<common::HResultHolder, FeatureLvs.size()> hrs = {};

            ID3D12Device* device = nullptr;
            for (size_t fid = 0; fid < FeatureLvs.size(); ++fid)
            {
                const auto [featEnum, featLv] = FeatureLvs[fid];
                hrs[fid] = D3D12CreateDevice(adapter, featEnum, IID_PPV_ARGS(&device));
                if (hrs[fid] && device)
                {
                    dxLog().Verbose(u"Created device on [{}].\n", name);
                    auto& dev = devs.emplace_back(MAKE_ENABLER_UNIQUE(DxDevice_, (PtrProxy<detail::Adapter>{ adapter }, PtrProxy<detail::Device>{ device }, 
                        name, featLv, isDxcore)));
                    return dev.get();
                }
            }
            Expects(!device);
            dxLog().Warning(u"Failed to created device on [{}]:\n12_0: {}\nCore1_0: {}\n", name, hrs[0].ToStr(), hrs[1].ToStr());
            return nullptr;
        };

        // dx12 need dxgi1.4, and dxgi1.4 need win10
        IDXGIFactory4* dxgiFactory = nullptr;
        THROW_HR(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)), u"Cannot create DXGIFactory4");
        std::vector<LUID> dxgiLUIDs;
        const auto CreateByDxgi = [&](IDXGIAdapter1* adapter) -> bool
        {
            DXGI_ADAPTER_DESC1 desc = {};
            if (FAILED(adapter->GetDesc1(&desc)))
                return false;
            for (const auto& luid : dxgiLUIDs)
                if (luid.HighPart == desc.AdapterLuid.HighPart && luid.LowPart == desc.AdapterLuid.LowPart)
                    return true; // adapter already created a device, skip

            const auto name = common::str::TrimStringView<char16_t>(reinterpret_cast<const char16_t*>(desc.Description), u' ');
            const auto dev = CreateDevice(adapter, name, false);
            if (dev)
            {
                dxgiLUIDs.emplace_back(desc.AdapterLuid);
                dev->IsSoftware = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
                return true;
            }
            return false;
        };

#ifdef HAS_DXCORE
        if (const auto dxcore = common::DynLib::TryCreate(L"dxcore.dll"); dxcore)
        {
            const auto CreateAdapterFactory = dxcore.GetFunction<PFN_DXCoreCreateAdapterFactory>("DXCoreCreateAdapterFactory");

            IDXCoreAdapterFactory* coreFactory = nullptr;
            THROW_HR(CreateAdapterFactory(IID_PPV_ARGS(&coreFactory)), u"Cannot create DXCoreFactory");
            IDXCoreAdapterList* coreAdapters = nullptr;
            const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };
            THROW_HR(coreFactory->CreateAdapterList(1, dxGUIDs, IID_PPV_ARGS(&coreAdapters)), u"Cannot create DXCoreAdapterList");
            for (uint32_t i = 0; i < coreAdapters->GetAdapterCount(); i++)
            {
                IDXCoreAdapter* adapter = nullptr;
                coreAdapters->GetAdapter(i, &adapter);
                if (!adapter)
                    continue;
                // use dxgi for GPU
                // see https://github.com/microsoft/Windows-Machine-Learning/blob/master/Tools/WinMLRunner/src/LearningModelDeviceHelper.cpp#L167-L171
                if (adapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
                {
                    LUID luid;
                    THROW_HR(adapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, &luid), u"Cannot get adapter LUID");
                    IDXGIAdapter1* dxgiAdapter = nullptr;
                    if (const common::HResultHolder hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&dxgiAdapter)); hr && dxgiAdapter)
                    {
                        if (CreateByDxgi(dxgiAdapter))
                            continue;
                    }
                }

                //use dxcore for creation
                size_t nameSize = 0;
                THROW_HR(adapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription, &nameSize), u"Cannot get adapter name size");
                std::string name_(nameSize, '\0');
                THROW_HR(adapter->GetProperty(DXCoreAdapterProperty::DriverDescription, nameSize, name_.data()), u"Cannot get adapter name");
                const auto name = common::str::TrimStringView<char>(name_);
                std::u16string adapterName(name.begin(), name.end());
                bool isHW = true;
                THROW_HR(adapter->GetProperty(DXCoreAdapterProperty::IsHardware, &isHW), u"Cannot get adapter isHardware");

                const auto dev = CreateDevice(adapter, adapterName, true);
                if (dev)
                {
                    dev->IsSoftware = !isHW;
                }
            }
        }
#endif
        for (uint32_t idx = 0; ; ++idx)
        {
            IDXGIAdapter1* adapter = nullptr;
            if (dxgiFactory->EnumAdapters1(idx, &adapter) == DXGI_ERROR_NOT_FOUND)
                break;

            Ensures(adapter);
            CreateByDxgi(adapter);
        }

        return devs;
    }();

    static_assert(sizeof(std::unique_ptr<DxDevice_>) == sizeof(DxDevice));
    return { reinterpret_cast<const DxDevice*>(devices.data()), devices.size() };
}


}
