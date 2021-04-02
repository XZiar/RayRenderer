#include "DxPch.h"
#include "DxDevice.h"

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
    SMVer(0), WaveSize(0), 
    HeapUpload(QueryHeapProp(Device, D3D12_HEAP_TYPE_UPLOAD)), HeapDefault(QueryHeapProp(Device, D3D12_HEAP_TYPE_DEFAULT)), HeapReadback(QueryHeapProp(Device, D3D12_HEAP_TYPE_READBACK)),
    Arch(Architecture::None), DtypeSupport(ShaderDType::None), OptSupport(OptionalSupport::None)
{
#define CheckFeat(dev, feat) CheckFeat_<D3D12_FEATURE_DATA_##feat>(D3D12_FEATURE_##feat, dev)
#define CheckFeat2(dev, feat, ...) CheckFeat_<D3D12_FEATURE_DATA_##feat>(D3D12_FEATURE_##feat, dev, __VA_ARGS__)
    if (const auto feat = CheckFeat2(Device, SHADER_MODEL, D3D_SHADER_MODEL_6_5))
        SMVer = (feat->HighestShaderModel & 0xf) + (feat->HighestShaderModel >> 4) * 10;
    else
        COMMON_THROWEX(DxException, feat.HResult, u"Failed to check SM version");
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS))
    {
        if (feat->DoublePrecisionFloatShaderOps) DtypeSupport |= ShaderDType::FP64;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS1))
    {
        if (feat->WaveOps) WaveSize = feat->WaveLaneCountMin;
        if (feat->Int64ShaderOps) DtypeSupport |= ShaderDType::INT64;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS2))
    {
        if (feat->DepthBoundsTestSupported) OptSupport |= OptionalSupport::DepthBoundTest;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS3))
    {
        if (feat->CopyQueueTimestampQueriesSupported) OptSupport |= OptionalSupport::CopyQueueTimeQuery;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS4))
    {
        if (feat->Native16BitShaderOpsSupported) DtypeSupport |= ShaderDType::FP16 | ShaderDType::INT16;
    }
    if (const auto feat = CheckFeat(Device, D3D12_OPTIONS6))
    {
        if (feat->BackgroundProcessingSupported) OptSupport |= OptionalSupport::BackgroundProcessing;
    }
    
    if (const auto feat1 = CheckFeat(Device, ARCHITECTURE1))
    {
        if (feat1->TileBasedRenderer) Arch |= Architecture::TBR;
        if (feat1->UMA)               Arch |= Architecture::UMA;
        if (feat1->CacheCoherentUMA)  Arch |= Architecture::CacheCoherent;
        if (feat1->IsolatedMMU)       Arch |= Architecture::IsolatedMMU;
    }
    else if (const auto feat2 = CheckFeat(Device, ARCHITECTURE))
    {
        if (feat2->TileBasedRenderer) Arch |= Architecture::TBR;
        if (feat2->UMA)               Arch |= Architecture::UMA;
        if (feat2->CacheCoherentUMA)  Arch |= Architecture::CacheCoherent;
    }
    else
    {
        dxLog().warning(FMT_STRING(u"Failed to check architecture: [{}, {}]"), feat1.HResult, feat2.HResult);
        COMMON_THROWEX(DxException, feat2.HResult, u"Failed to check architecture");
    }
}
DxDevice_::~DxDevice_()
{ }

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
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                dxLog().verbose(u"Skip software adapter [{}].\n", adapterName);
                continue;
            }
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

        return devs;
    }();

    static_assert(sizeof(std::unique_ptr<DxDevice_>) == sizeof(DxDevice));
    return { reinterpret_cast<const DxDevice*>(devices.data()), devices.size() };
}


}
