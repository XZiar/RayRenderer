#include "XCompRely.h"
#include "SystemCommon/HResultHelper.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/AlignedBuffer.hpp"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include <dxgi.h>
#pragma comment (lib, "DXGI.lib")

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201)
#endif

#ifndef NTSTATUS
using NTSTATUS = LONG;
constexpr NTSTATUS STATUS_SUCCESS = 0x00000000L;
constexpr NTSTATUS STATUS_BUFFER_TOO_SMALL = 0xC0000023;
#endif

#define THROW_HR(eval, msg) if (const common::HResultHolder hr___ = eval; !hr___) COMMON_THROWEX(common::BaseException, msg)\
    .Attach("HResult", hr___).Attach("detail", hr___.ToStr())

typedef struct _D3DKMT_OPENADAPTERFROMLUID {
    LUID AdapterLuid;
    UINT hAdapter;
} D3DKMT_OPENADAPTERFROMLUID;

typedef _Check_return_ NTSTATUS(APIENTRY* PFN_D3DKMTOpenAdapterFromLuid)(const D3DKMT_OPENADAPTERFROMLUID* unnamedParam1);

typedef struct _D3DKMT_CLOSEADAPTER {
    UINT hAdapter;
} D3DKMT_CLOSEADAPTER;

typedef _Check_return_ NTSTATUS(APIENTRY* PFN_D3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER* unnamedParam1);

//typedef enum _KMTQUERYADAPTERINFOTYPE {
//    KMTQAITYPE_UMDRIVERPRIVATE,
//    KMTQAITYPE_UMDRIVERNAME,
//    KMTQAITYPE_UMOPENGLINFO,
//    KMTQAITYPE_GETSEGMENTSIZE,
//    KMTQAITYPE_ADAPTERGUID,
//    KMTQAITYPE_FLIPQUEUEINFO,
//    KMTQAITYPE_ADAPTERADDRESS,
//    KMTQAITYPE_SETWORKINGSETINFO,
//    KMTQAITYPE_ADAPTERREGISTRYINFO,
//    KMTQAITYPE_CURRENTDISPLAYMODE,
//    KMTQAITYPE_MODELIST,
//    KMTQAITYPE_CHECKDRIVERUPDATESTATUS,
//    KMTQAITYPE_VIRTUALADDRESSINFO,
//    KMTQAITYPE_DRIVERVERSION,
//    KMTQAITYPE_ADAPTERTYPE = 15,
//    KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT,
//    KMTQAITYPE_WDDM_1_2_CAPS,
//    KMTQAITYPE_UMD_DRIVER_VERSION,
//    KMTQAITYPE_DIRECTFLIP_SUPPORT,
//    KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT,
//    KMTQAITYPE_DLIST_DRIVER_NAME,
//    KMTQAITYPE_WDDM_1_3_CAPS,
//    KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT,
//    KMTQAITYPE_WDDM_2_0_CAPS,
//    KMTQAITYPE_NODEMETADATA,
//    KMTQAITYPE_CPDRIVERNAME,
//    KMTQAITYPE_XBOX,
//    KMTQAITYPE_INDEPENDENTFLIP_SUPPORT,
//    KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME,
//    KMTQAITYPE_PHYSICALADAPTERCOUNT,
//    KMTQAITYPE_PHYSICALADAPTERDEVICEIDS,
//    KMTQAITYPE_DRIVERCAPS_EXT,
//    KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE,
//    KMTQAITYPE_QUERY_GPUMMU_CAPS,
//    KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT,
//    KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT,
//    KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED,
//    KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT,
//    KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT,
//    KMTQAITYPE_PANELFITTER_SUPPORT,
//    KMTQAITYPE_PHYSICALADAPTERPNPKEY,
//    KMTQAITYPE_GETSEGMENTGROUPSIZE,
//    KMTQAITYPE_MPO3DDI_SUPPORT,
//    KMTQAITYPE_HWDRM_SUPPORT,
//    KMTQAITYPE_MPOKERNELCAPS_SUPPORT,
//    KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT,
//    KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO,
//    KMTQAITYPE_QUERYREGISTRY,
//    KMTQAITYPE_KMD_DRIVER_VERSION,
//    KMTQAITYPE_BLOCKLIST_KERNEL,
//    KMTQAITYPE_BLOCKLIST_RUNTIME,
//    KMTQAITYPE_ADAPTERGUID_RENDER,
//    KMTQAITYPE_ADAPTERADDRESS_RENDER,
//    KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER,
//    KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER,
//    KMTQAITYPE_DRIVERVERSION_RENDER,
//    KMTQAITYPE_ADAPTERTYPE_RENDER,
//    KMTQAITYPE_WDDM_1_2_CAPS_RENDER,
//    KMTQAITYPE_WDDM_1_3_CAPS_RENDER,
//    KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID,
//    KMTQAITYPE_NODEPERFDATA,
//    KMTQAITYPE_ADAPTERPERFDATA,
//    KMTQAITYPE_ADAPTERPERFDATA_CAPS,
//    KMTQUITYPE_GPUVERSION,
//    KMTQAITYPE_DRIVER_DESCRIPTION,
//    KMTQAITYPE_DRIVER_DESCRIPTION_RENDER,
//    KMTQAITYPE_SCANOUT_CAPS,
//    KMTQAITYPE_DISPLAY_UMDRIVERNAME,
//    KMTQAITYPE_PARAVIRTUALIZATION_RENDER,
//    KMTQAITYPE_SERVICENAME,
//    KMTQAITYPE_WDDM_2_7_CAPS,
//    KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT,
//    KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT,
//    KMTQAITYPE_DISPLAY_CAPS,
//    KMTQAITYPE_WDDM_2_9_CAPS,
//    KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT,
//    KMTQAITYPE_WDDM_3_0_CAPS
//} KMTQUERYADAPTERINFOTYPE;

typedef enum _KMTQUERYADAPTERINFOTYPE {
    KMTQAITYPE_ADAPTERGUID = 4,
    KMTQAITYPE_ADAPTERADDRESS = 6,
    KMTQAITYPE_QUERYREGISTRY = 48,
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_ADAPTERADDRESS {
    UINT BusNumber;
    UINT DeviceNumber;
    UINT FunctionNumber;
} D3DKMT_ADAPTERADDRESS;

typedef struct _D3DKMT_QUERYADAPTERINFO {
    UINT handle;
    KMTQUERYADAPTERINFOTYPE type;
    VOID *private_data;
    UINT private_data_size;
} D3DKMT_QUERYADAPTERINFO;

typedef _Check_return_ NTSTATUS(APIENTRY* PFN_D3DKMTQueryAdapterInfo)(const D3DKMT_QUERYADAPTERINFO* unnamedParam1);

typedef enum _D3DDDI_QUERYREGISTRY_TYPE {
    D3DDDI_QUERYREGISTRY_ADAPTERKEY = 1,
} D3DDDI_QUERYREGISTRY_TYPE;

typedef struct _D3DDDI_QUERYREGISTRY_FLAGS {
    union {
        struct {
            UINT TranslatePath : 1;
            UINT MutableValue : 1;
            UINT Reserved : 30;
        };
        UINT Value;
    };
} D3DDDI_QUERYREGISTRY_FLAGS;

typedef enum _D3DDDI_QUERYREGISTRY_STATUS {
    D3DDDI_QUERYREGISTRY_STATUS_SUCCESS,
    D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW,
    D3DDDI_QUERYREGISTRY_STATUS_FAIL,
    D3DDDI_QUERYREGISTRY_STATUS_MAX
} D3DDDI_QUERYREGISTRY_STATUS;

typedef struct _D3DDDI_QUERYREGISTRY_INFO {
    D3DDDI_QUERYREGISTRY_TYPE   QueryType;
    D3DDDI_QUERYREGISTRY_FLAGS  QueryFlags;
    WCHAR                       ValueName[MAX_PATH];
    ULONG                       ValueType;
    ULONG                       PhysicalAdapterIndex;
    ULONG                       OutputValueSize;
    D3DDDI_QUERYREGISTRY_STATUS Status;
    union {
        DWORD  OutputDword;
        UINT64 OutputQword;
        WCHAR  OutputString[1];
        BYTE   OutputBinary[1];
    };
} D3DDDI_QUERYREGISTRY_INFO;


namespace xcomp
{


static std::optional<std::u16string> QueryReg(PFN_D3DKMTQueryAdapterInfo QueryAdapterInfo, UINT adpater, std::wstring_view path)
{
    if (path.empty()) return {};

    D3DDDI_QUERYREGISTRY_INFO queryArgs = { };
    queryArgs.QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
    queryArgs.QueryFlags.TranslatePath = TRUE;
    wcscpy_s(queryArgs.ValueName, path.data());
    queryArgs.ValueType = REG_SZ;

    D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {};
    queryAdapterInfo.handle = adpater;
    queryAdapterInfo.type = KMTQAITYPE_QUERYREGISTRY;
    queryAdapterInfo.private_data = &queryArgs;
    queryAdapterInfo.private_data_size = sizeof(queryArgs);
    auto status = QueryAdapterInfo(&queryAdapterInfo);
    if (status != STATUS_SUCCESS)
    {
        queryArgs.ValueType = REG_MULTI_SZ;
        status = QueryAdapterInfo(&queryAdapterInfo);
    }
    const D3DDDI_QUERYREGISTRY_INFO* regInfo = &queryArgs;
    common::AlignedBuffer buf;
    if (status == STATUS_SUCCESS && regInfo->Status == D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW)
    {
        const auto queryBufferSize = gsl::narrow_cast<UINT>(sizeof(D3DDDI_QUERYREGISTRY_INFO) + queryArgs.OutputValueSize);
        buf = common::AlignedBuffer(queryBufferSize, std::byte(0), alignof(D3DDDI_QUERYREGISTRY_INFO));
        const auto ptr = buf.GetRawPtr<D3DDDI_QUERYREGISTRY_INFO>();
        *ptr = queryArgs;
        regInfo = ptr;
        queryAdapterInfo.private_data = buf.GetRawPtr();
        queryAdapterInfo.private_data_size = queryBufferSize;
        status = QueryAdapterInfo(&queryAdapterInfo);
    }
    if (status == STATUS_SUCCESS && regInfo->Status == D3DDDI_QUERYREGISTRY_STATUS_SUCCESS)
    {
        if (buf.GetSize())
        {
            return reinterpret_cast<const char16_t*>(regInfo->OutputString);
        }
    }
    return {};
}

static std::vector<CommonDeviceInfo> GatherDeviceInfo()
{
    std::vector<CommonDeviceInfo> infos;
    const auto& console = common::console::ConsoleEx::Get();
    const auto gdi = LoadLibraryW(L"gdi32.dll");
    if (gdi)
    {
        const auto OpenAdapterFromLuid  = (PFN_D3DKMTOpenAdapterFromLuid)GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid");
        const auto QueryAdapterInfo     = (PFN_D3DKMTQueryAdapterInfo)   GetProcAddress(gdi, "D3DKMTQueryAdapterInfo");
        const auto CloseAdapter         = (PFN_D3DKMTCloseAdapter)       GetProcAddress(gdi, "D3DKMTCloseAdapter");
        if (OpenAdapterFromLuid && QueryAdapterInfo && CloseAdapter)
        {
            IDXGIFactory* factory = nullptr;
            if (const common::HResultHolder hr = CreateDXGIFactory(IID_PPV_ARGS(&factory)); hr)
            {
                for (uint32_t idx = 0; ; ++idx)
                {
                    IDXGIAdapter* adapter = nullptr;
                    if (factory->EnumAdapters(idx, &adapter) == DXGI_ERROR_NOT_FOUND)
                        break;
                    DXGI_ADAPTER_DESC desc = {};
                    if (FAILED(adapter->GetDesc(&desc)))
                        continue;

                    auto& info = infos.emplace_back();
                    info.Name = reinterpret_cast<const char16_t*>(desc.Description);
                    memcpy_s(info.Luid.data(), sizeof(info.Luid), &desc.AdapterLuid, sizeof(desc.AdapterLuid));
                    info.VendorId = desc.VendorId, info.DeviceId = desc.DeviceId;

                    D3DKMT_OPENADAPTERFROMLUID openFromLuid = {};
                    openFromLuid.AdapterLuid = desc.AdapterLuid;
                    openFromLuid.hAdapter = 0;
                    if (const auto status = OpenAdapterFromLuid(&openFromLuid); status == STATUS_SUCCESS)
                    {
                        {
                            D3DKMT_ADAPTERADDRESS queryArgs = {};
                            D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {};
                            queryAdapterInfo.handle = openFromLuid.hAdapter;
                            queryAdapterInfo.type = KMTQAITYPE_ADAPTERADDRESS;
                            queryAdapterInfo.private_data = &queryArgs;
                            queryAdapterInfo.private_data_size = sizeof(queryArgs);
                            if (const auto ret = QueryAdapterInfo(&queryAdapterInfo); ret == STATUS_SUCCESS)
                                info.PCIEAddress = { queryArgs.BusNumber, queryArgs.DeviceNumber, queryArgs.FunctionNumber };
                        }
                        {
                            GUID guid;
                            D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {};
                            queryAdapterInfo.handle = openFromLuid.hAdapter;
                            queryAdapterInfo.type = KMTQAITYPE_ADAPTERGUID;
                            queryAdapterInfo.private_data = &guid;
                            queryAdapterInfo.private_data_size = sizeof(guid);
                            if (const auto ret = QueryAdapterInfo(&queryAdapterInfo); ret == STATUS_SUCCESS)
                                memcpy_s(info.Guid.data(), sizeof(info.Guid), &guid, sizeof(guid));
                        }
#if COMMON_OSBIT == 64
                        constexpr std::wstring_view OpenGLPath = L"OpenGLDriverName";
                        constexpr std::wstring_view OpenCLPath = L"OpenCLDriverName";
                        constexpr std::wstring_view VulkanPath = L"VulkanDriverName";
                        constexpr std::wstring_view LvZeroPath = L"LevelZeroDriverPath";
#else
                        constexpr std::wstring_view OpenGLPath = L"OpenGLDriverNameWow";
                        constexpr std::wstring_view OpenCLPath = L"OpenCLDriverNameWow";
                        constexpr std::wstring_view VulkanPath = L"VulkanDriverNameWow";
                        constexpr std::wstring_view LvZeroPath = L"";
#endif
                        if (auto ret = QueryReg(QueryAdapterInfo, openFromLuid.hAdapter, OpenGLPath); ret)
                            info.OpenGLICDPath = std::move(*ret);
                        if (auto ret = QueryReg(QueryAdapterInfo, openFromLuid.hAdapter, OpenCLPath); ret)
                            info.OpenCLICDPath = std::move(*ret);
                        if (auto ret = QueryReg(QueryAdapterInfo, openFromLuid.hAdapter, VulkanPath); ret)
                            info.VulkanICDPath = std::move(*ret);
                        if (auto ret = QueryReg(QueryAdapterInfo, openFromLuid.hAdapter, LvZeroPath); ret)
                            info.LvZeroICDPath = std::move(*ret);
                        D3DKMT_CLOSEADAPTER closeApadter = {};
                        closeApadter.hAdapter = openFromLuid.hAdapter;
                        CloseAdapter(&closeApadter);
                    }
                }
                factory->Release();
            }
            else
            {
                console.Print(common::CommonColor::BrightRed, fmt::format(u"Cannot create DXGIFactory: {}\n", hr.ToStr()));
            }
        }
        FreeLibrary(gdi);
    }
    return infos;
}

common::span<const CommonDeviceInfo> ProbeDevice()
{
    static const auto infos = GatherDeviceInfo();
    return infos;
}


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
