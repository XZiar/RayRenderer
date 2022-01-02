#include "DxPch.h"
#include "DxDevice.h"
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

DxDevice_::DxDevice_(PtrProxy<detail::Adapter> adapter, PtrProxy<detail::Device> device, std::u16string_view name) :
    Adapter(std::move(adapter)), Device(std::move(device)), AdapterName(name), 
    HeapUpload  (QueryHeapProp(Device, D3D12_HEAP_TYPE_UPLOAD)), 
    HeapDefault (QueryHeapProp(Device, D3D12_HEAP_TYPE_DEFAULT)), 
    HeapReadback(QueryHeapProp(Device, D3D12_HEAP_TYPE_READBACK))
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
    if (const auto feat = CheckFeat2(Device, SHADER_MODEL, D3D_SHADER_MODEL_6_5))
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
        dxLog().warning(FMT_STRING(u"Failed to check architecture: [{}, {}]"), feat1.HResult, feat2.HResult);
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
                dxLog().warning(u"Failed to enable debug layer: {}.\n", hr);
            }
        }
#endif
        IDXGIFactory4* factory = nullptr;
        THROW_HR(CreateDXGIFactory(IID_PPV_ARGS(&factory)), u"Cannot create DXGIFactory");

        std::vector<std::unique_ptr<DxDevice_>> devs;
        for (uint32_t idx = 0; ; ++idx)
        {
            IDXGIAdapter1* adapter = nullptr;
            if (factory->EnumAdapters1(idx, &adapter) == DXGI_ERROR_NOT_FOUND)
                break;
            DXGI_ADAPTER_DESC1 desc = {};
            if (FAILED(adapter->GetDesc1(&desc)))
                continue;
            std::u16string_view adapterName = reinterpret_cast<const char16_t*>(desc.Description);
            /*if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                dxLog().verbose(u"Skip software adapter [{}].\n", adapterName);
                continue;
            }*/
            ID3D12Device* device = nullptr;
            {
                constexpr std::array<D3D_FEATURE_LEVEL, 2> FeatureLvs =
                    { D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_1_0_CORE };
                std::array<common::HResultHolder, 2> hrs = { };
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
            devs.emplace_back(MAKE_ENABLER_UNIQUE(DxDevice_, (PtrProxy<detail::Adapter>{ adapter }, PtrProxy<detail::Device>{ device }, adapterName)));
        }

#ifdef HAS_DXCORE
        const auto dxcore = common::DynLib::TryCreate(L"dxcore.dll");
        if (dxcore)
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
                if (adapter)
                {
                    size_t descSize = 0;
                    THROW_HR(adapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription, &descSize), u"Cannot get adapter name size");
                    std::string desc(descSize, '\0');
                    THROW_HR(adapter->GetProperty(DXCoreAdapterProperty::DriverDescription, descSize, desc.data()), u"Cannot get adapter name");
                    std::u16string adapterName(desc.begin(), desc.end());

                    ID3D12Device* device = nullptr;
                    {
                        common::HResultHolder hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_1_0_CORE, IID_PPV_ARGS(&device));
                        if (!device)
                        {
                            dxLog().warning(u"Failed to create device on [{}]:\nCore1_0: {}\n", adapterName, hr);
                            continue;
                        }
                    }
                    dxLog().verbose(u"Create device on {}.\n", adapterName);
                    devs.emplace_back(MAKE_ENABLER_UNIQUE(DxDevice_, (PtrProxy<detail::Adapter>{ adapter }, PtrProxy<detail::Device>{ device }, adapterName)));
                }
            }
        }
#endif

        return devs;
    }();

    static_assert(sizeof(std::unique_ptr<DxDevice_>) == sizeof(DxDevice));
    return { reinterpret_cast<const DxDevice*>(devices.data()), devices.size() };
}


}
