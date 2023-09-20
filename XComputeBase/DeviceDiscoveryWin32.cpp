#include "XCompRely.h"
#include "SystemCommon/DynamicLibrary.h"
#include "SystemCommon/ErrorCodeHelper.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/AlignedBuffer.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/StringPool.hpp"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <wrl/client.h>
//#include <Windows.h>
#define COM_NO_WINDOWS_H 1
#include <dxgi.h>
#include <winternl.h>
#include <D3dkmthk.h>
#include <Ntddvdeo.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <Devpkey.h>
#pragma comment(lib, "cfgmgr32.lib")

#if __has_include("dxcore.h")
#   include <dxcore.h>
#   define HAS_DXCORE 1
#endif

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201 26812)
#endif

#ifndef STATUS_SUCCESS
constexpr NTSTATUS STATUS_SUCCESS = 0x00000000L;
#endif

#define THROW_HR(eval, msg) if (const common::HResultHolder hr___ = eval; !hr___) \
    COMMON_THROWEX(common::BaseException, msg).Attach("HResult", hr___).Attach("detail", hr___.ToStr())

typedef _Check_return_ HRESULT(APIENTRY* PFN_DXCoreCreateAdapterFactory)(REFIID riid, _COM_Outptr_ void** ppvFactory);

namespace xcomp
{
using Microsoft::WRL::ComPtr;
using namespace std::string_view_literals;


struct Win32DeviceInfo final : public CommonDeviceInfo
{
    common::StringPool<char16_t> Texts;
    common::StringPiece<char16_t> DevicePath;
    common::StringPiece<char16_t> DevicePathUpper;
    common::StringPiece<char16_t> OpenGLICDPath;
    common::StringPiece<char16_t> OpenCLICDPath;
    common::StringPiece<char16_t> VulkanICDPath;
    common::StringPiece<char16_t> LvZeroICDPath;
    std::vector<uint32_t> Displays;
    std::u16string_view GetICDPath(ICDTypes type) const noexcept final
    {
        switch (type)
        {
        case ICDTypes::OpenGL:    return Texts.GetStringView(OpenGLICDPath);
        case ICDTypes::OpenCL:    return Texts.GetStringView(OpenCLICDPath);
        case ICDTypes::Vulkan:    return Texts.GetStringView(VulkanICDPath);
        case ICDTypes::LevelZero: return Texts.GetStringView(LvZeroICDPath);
        default: CM_UNREACHABLE();
        }
    }
    std::u16string_view GetDevicePath() const noexcept final { return Texts.GetStringView(DevicePath); }
    uint32_t GetDisplay() const noexcept final { return Displays.empty() ? UINT32_MAX : Displays[0]; }
    bool CheckDisplay(uint32_t id) const noexcept final
    {
        return std::find(Displays.begin(), Displays.end(), id) != Displays.end();
    }

    void SetDevicePath(std::u16string_view devPath) noexcept
    {
        DevicePath = Texts.AllocateString(devPath);
        DevicePathUpper = Texts.AllocateString(common::str::ToUpperEng(devPath));
    }
    void SetICDPath(ICDTypes type, std::u16string_view devPath) noexcept
    {
        switch (type)
        {
        case ICDTypes::OpenGL:    OpenGLICDPath = Texts.AllocateString(devPath); return;
        case ICDTypes::OpenCL:    OpenCLICDPath = Texts.AllocateString(devPath); return;
        case ICDTypes::Vulkan:    VulkanICDPath = Texts.AllocateString(devPath); return;
        case ICDTypes::LevelZero: LvZeroICDPath = Texts.AllocateString(devPath); return;
        default: CM_UNREACHABLE();
        }
    }
};

struct CommonInfoHelper
{
    common::DynLib Gdi32;
    decltype(&D3DKMTOpenAdapterFromLuid) OpenAdapterFromLuid = nullptr; // start from win8
    struct PathInfo
    {
        std::u16string Path;
        std::optional<std::pair<LUID, D3DKMT_HANDLE>> KMTInfo;
        PCI_BDF Address;
        PathInfo(std::u16string&& path, std::optional<std::pair<LUID, D3DKMT_HANDLE>> kmtInfo, PCI_BDF addr) noexcept :
            Path(std::move(path)), KMTInfo(kmtInfo), Address(addr) {}
    };
    struct DisplayInfo
    {
        uint32_t DisplayId;
        D3DKMT_HANDLE Handle;
        LUID Luid;
        uint32_t Flag;
        DisplayInfo(uint32_t id, const LUID& luid, D3DKMT_HANDLE handle, uint32_t flag) noexcept :
            DisplayId(id), Handle(handle), Luid(luid), Flag(flag) {}
    };
    std::vector<PathInfo> DevicePaths;
    std::vector<DisplayInfo> Displays;
    uint32_t PrimaryDispIndex = UINT32_MAX;
    static forceinline void CloseKMTHandle(D3DKMT_HANDLE handle) noexcept
    {
        D3DKMT_CLOSEADAPTER closeApadter = {};
        closeApadter.hAdapter = handle;
        [[maybe_unused]] const auto dummy = D3DKMTCloseAdapter(&closeApadter);
    }
    template<typename T>
    static forceinline bool GetDevProp(const DEVINST& devinst, const DEVPROPKEY& prop, T& dst)
    {
        ULONG size = sizeof(dst);
        DEVPROPTYPE type = 0;
        const auto ret = CM_Get_DevNode_PropertyW(devinst, &prop, &type, reinterpret_cast<PBYTE>(&dst), &size, 0);
        if (ret == CR_SUCCESS)
        {
            if constexpr (std::is_same_v<T, uint32_t>)
                return type == DEVPROP_TYPE_UINT32;
            else if constexpr (std::is_same_v<T, GUID>)
                return type == DEVPROP_TYPE_GUID;
            else
                static_assert(!common::AlwaysTrue<T>);
        }
        return false;
    }
    CommonInfoHelper() : Gdi32("gdi32.dll")
    {
        using namespace std::string_view_literals;
        OpenAdapterFromLuid = Gdi32.TryGetFunction<decltype(OpenAdapterFromLuid)>("D3DKMTOpenAdapterFromLuid");
        static constexpr GUID DisplayClz     = { 0x4d36e968, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18} };
        static constexpr GUID AcceleratorClz = { 0xf01a9d53, 0x3ff6, 0x48d2, {0x9f, 0x97, 0xc8, 0xa7, 0x00, 0x4b, 0xe1, 0x0c} };
        static constexpr GUID PCIEClz        = { 0xc8ebdfb0, 0xb510, 0x11d0, {0x80, 0xe5, 0x00, 0xa0, 0xc9, 0x25, 0x42, 0xe3} };
        //constexpr GUID clzs[] = { DisplayClz, AcceleratorClz };
        [[maybe_unused]] const auto& console = common::console::ConsoleEx::Get(); 
        constexpr std::wstring_view clzs[] = { L"{4d36e968-e325-11ce-bfc1-08002be10318}"sv, L"{f01a9d53-3ff6-48d2-9f97-c8a7004be10c}"sv };
        // also see https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Tools/DXGIAdapterRemovalSupportTest/src/main.cpp
        for (const auto& clz : clzs)
        {
            const auto devList = [](std::wstring_view clz) -> std::vector<wchar_t>
            {
                ULONG devListSize = 0;
                if (CM_Get_Device_ID_List_SizeW(&devListSize, clz.data(), CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_CLASS) != CR_SUCCESS)
                    return {};
                std::vector<wchar_t> devList(devListSize, 0);
                if (CM_Get_Device_ID_ListW(clz.data(), devList.data(), devListSize, CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_CLASS) != CR_SUCCESS)
                    return {};
                return devList;
            }(clz);
            for (const auto devId : common::str::SplitStream(devList, L'\0', false))
            {
                DEVINST devinst = 0;
                std::optional<std::pair<LUID, D3DKMT_HANDLE>> kmtInfo;
                PCI_BDF address;
                {
                    std::wstring newname = L"\\\\?\\";
                    for (const auto ch : devId)
                        newname.push_back(ch == L'\\' ? L'#' : ch);
                      newname.append(L"#{1CA05180-A699-450A-9A0C-DE4FBE3DDD89}");
                    D3DKMT_OPENADAPTERFROMDEVICENAME openFromDevName = {};
                    openFromDevName.pDeviceName = newname.data();
                    if (D3DKMTOpenAdapterFromDeviceName(&openFromDevName) == STATUS_SUCCESS)
                        kmtInfo.emplace(openFromDevName.AdapterLuid, openFromDevName.hAdapter);
                }
                if (CM_Locate_DevNodeW(&devinst, const_cast<wchar_t*>(devId.data()), CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS)
                {
                    GUID busGuid = {};
                    if (GetDevProp(devinst, DEVPKEY_Device_BusTypeGuid, busGuid) && busGuid == PCIEClz)
                    {
                        uint32_t bnum = 0, addr = 0;
                        if (GetDevProp(devinst, DEVPKEY_Device_BusNumber, bnum) && GetDevProp(devinst, DEVPKEY_Device_Address, addr))
                        {
                            const auto slot = (addr >> 16) & 0xFFFF, func = addr & 0xFFFF;
                            address = PCI_BDF(bnum, slot, func);
                        }
                    }
                }
                DevicePaths.emplace_back(common::str::to_u16string(devId), kmtInfo, address);
            }
        }
        {
            DISPLAY_DEVICEW dd;
            dd.cb = sizeof(DISPLAY_DEVICEW);
            std::u16string infoTxt = u"DisplayDevices:\n";
            DWORD deviceNum = 0;
            while (EnumDisplayDevicesW(nullptr, deviceNum, &dd, 0))
            {
                D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME openFromDName{};
                memcpy_s(openFromDName.DeviceName, sizeof(openFromDName.DeviceName), dd.DeviceName, sizeof(dd.DeviceName));
                if (const auto status = D3DKMTOpenAdapterFromGdiDisplayName(&openFromDName); status == STATUS_SUCCESS)
                {
                    std::wstring_view dName(dd.DeviceName);
                    Expects(dName.size() >= 12 && common::str::IsBeginWith(dName, L"\\\\.\\DISPLAY"sv));
                    uint32_t idx = 0;
                    bool correct = true;
                    for (uint32_t i = 11; i < dName.size(); ++i)
                    {
                        if (dd.DeviceName[i] >= L'0' && dd.DeviceName[i] <= L'9')
                            idx = idx * 10 + (dd.DeviceName[i] - L'0');
                        else
                        {
                            correct = false;
                            break;
                        }
                    }
                    if (correct)
                    {
                        Displays.emplace_back(idx, openFromDName.AdapterLuid, openFromDName.hAdapter, dd.StateFlags);
                        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                            PrimaryDispIndex = gsl::narrow_cast<uint32_t>(Displays.size() - 1);
                    }
                    else
                    {
                        CloseKMTHandle(openFromDName.hAdapter);
                    }
                }
                common::str::Formatter<char16_t>{}.FormatToStatic(infoTxt,
                    FmtString(u"[{}]{}\t[{:7}|{:3}]\n"sv), deviceNum, dd.DeviceName,
                    (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? u"Primary"sv : u""sv,
                    (dd.StateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE) ? u"VGA"sv : u""sv);
                deviceNum++;
            }
            console.Print(common::CommonColor::Magenta, infoTxt);
            // make primary first, or use lowest display id
            std::sort(Displays.begin(), Displays.end(), [](const DisplayInfo& l, const DisplayInfo& r)
                {
                    const bool isLPrimary = l.Flag & DISPLAY_DEVICE_PRIMARY_DEVICE;
                    const bool isRPrimary = r.Flag & DISPLAY_DEVICE_PRIMARY_DEVICE;
                    return isLPrimary == isRPrimary ? l.DisplayId < r.DisplayId : isLPrimary;
                });
        }
    }
    ~CommonInfoHelper()
    { 
        for (auto& path : DevicePaths)
        {
            if (path.KMTInfo)
            {
                CloseKMTHandle(path.KMTInfo->second);
            }
        }
        for (auto& disp : Displays)
        {
            CloseKMTHandle(disp.Handle);
        }
    }
    std::optional<std::u16string> QueryReg(UINT adpater, std::wstring_view path) const noexcept
    {
        if (path.empty()) return {};

        D3DDDI_QUERYREGISTRY_INFO queryArgs = { };
        queryArgs.QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
        queryArgs.QueryFlags.TranslatePath = TRUE;
        wcscpy_s(queryArgs.ValueName, path.data());
        queryArgs.ValueType = REG_SZ;

        D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {};
        queryAdapterInfo.hAdapter = adpater;
        queryAdapterInfo.Type = KMTQAITYPE_QUERYREGISTRY;
        queryAdapterInfo.pPrivateDriverData = &queryArgs;
        queryAdapterInfo.PrivateDriverDataSize = sizeof(queryArgs);
        auto status = D3DKMTQueryAdapterInfo(&queryAdapterInfo);
        if (status != STATUS_SUCCESS)
        {
            queryArgs.ValueType = REG_MULTI_SZ;
            status = D3DKMTQueryAdapterInfo(&queryAdapterInfo);
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
            queryAdapterInfo.pPrivateDriverData = buf.GetRawPtr();
            queryAdapterInfo.PrivateDriverDataSize = queryBufferSize;
            status = D3DKMTQueryAdapterInfo(&queryAdapterInfo);
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
    template<typename T>
    static bool QueryAdapterInfo(D3DKMT_HANDLE handle, KMTQUERYADAPTERINFOTYPE type, T& data) noexcept
    {
        D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {};
        queryAdapterInfo.hAdapter = handle;
        queryAdapterInfo.Type = type;
        queryAdapterInfo.pPrivateDriverData = &data;
        queryAdapterInfo.PrivateDriverDataSize = sizeof(data);
        const auto ret = D3DKMTQueryAdapterInfo(&queryAdapterInfo);
        return ret == STATUS_SUCCESS;
    }
    void FillDeviceInfo(Win32DeviceInfo& info) const noexcept
    {
        enum class HandleType { None, Persist, Temp };
        HandleType handleType = HandleType::None;
        D3DKMT_HANDLE handle = 0;
        for (const auto& dp : DevicePaths)
        {
            if (dp.KMTInfo && std::memcmp(&dp.KMTInfo->first, &info.Luid, sizeof(LUID)) == 0)
            {
                info.SetDevicePath(dp.Path);
                handle = dp.KMTInfo->second, handleType = HandleType::Persist;
                break;
            }
        }
        for (const auto& disp : Displays)
        {
            if (std::memcmp(&disp.Luid, &info.Luid, sizeof(LUID)) == 0)
            {
                if (disp.Flag & DISPLAY_DEVICE_PRIMARY_DEVICE)
                    info.Displays.insert(info.Displays.begin(), disp.DisplayId);
                else
                    info.Displays.push_back(disp.DisplayId);
                if (handleType == HandleType::None)
                {
                    handle = disp.Handle, handleType = HandleType::Persist;
                }
                break;
            }
        }
        if (handleType == HandleType::None && OpenAdapterFromLuid)
        {
            D3DKMT_OPENADAPTERFROMLUID openFromLuid{};
            memcpy_s(&openFromLuid.AdapterLuid, sizeof(openFromLuid.AdapterLuid), info.Luid.data(), sizeof(info.Luid));
            openFromLuid.hAdapter = 0;
            if (const auto status = OpenAdapterFromLuid(&openFromLuid); status == STATUS_SUCCESS)
            {
                handle = openFromLuid.hAdapter, handleType = HandleType::Temp;
            }
        }
        if (handleType != HandleType::None)
        {
            // WDDM Version
            uint32_t WDDMVer = 1000;
            if (D3DKMT_DRIVERVERSION driverVer = KMT_DRIVERVERSION_WDDM_1_0; QueryAdapterInfo(handle, KMTQAITYPE_DRIVERVERSION, driverVer))
            {
                WDDMVer = static_cast<uint32_t>(driverVer);
            }
            // PCI_BDF
            if (D3DKMT_ADAPTERADDRESS address{}; QueryAdapterInfo(handle, KMTQAITYPE_ADAPTERADDRESS, address))
            {
                info.PCIEAddress = { address.BusNumber, address.DeviceNumber, address.FunctionNumber };
                if (info.DevicePath.GetLength() == 0)
                {
                    for (const auto& path : DevicePaths)
                    {
                        if (path.Address == info.PCIEAddress)
                            info.SetDevicePath(path.Path);
                    }
                }
            }
            // GUID
            if (GUID guid{}; QueryAdapterInfo(handle, KMTQAITYPE_ADAPTERGUID, guid))
            {
                memcpy_s(info.Guid.data(), sizeof(info.Guid), &guid, sizeof(guid));
            }
            // Segment/RAM size
            if (D3DKMT_SEGMENTSIZEINFO segSize{}; QueryAdapterInfo(handle, KMTQAITYPE_GETSEGMENTSIZE, segSize))
            {
                info.DedicatedVRAM   = static_cast<uint32_t>(segSize.DedicatedVideoMemorySize  / 1048576u);
                info.DedicatedSysRAM = static_cast<uint32_t>(segSize.DedicatedSystemMemorySize / 1048576u);
                info.SharedSysRAM    = static_cast<uint32_t>(segSize.SharedSystemMemorySize    / 1048576u);
            }
            if (WDDMVer >= 1200)
            {
                // Adapter Type
                if (D3DKMT_ADAPTERTYPE type{}; QueryAdapterInfo(handle, KMTQAITYPE_ADAPTERTYPE, type))
                {
                    info.IsSoftware     = type.SoftwareDevice;
                    info.SupportRender  = type.RenderSupported;
                    info.SupportDisplay = type.DisplaySupported;
                    if (WDDMVer >= 2300)
                        info.SupportPV = type.Paravirtualized;
                    info.SupportCompute = info.SupportRender || (WDDMVer >= 2600 ? type.ComputeOnly : false);
                }
            }
            // ICD Path
            if (WDDMVer >= 2400) // Since Windows 10, version 1803
            {
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
                if (auto ret = QueryReg(handle, OpenGLPath); ret)
                    info.SetICDPath(CommonDeviceInfo::ICDTypes::OpenGL, *ret);
                if (auto ret = QueryReg(handle, OpenCLPath); ret)
                    info.SetICDPath(CommonDeviceInfo::ICDTypes::OpenCL, *ret);
                if (auto ret = QueryReg(handle, VulkanPath); ret)
                    info.SetICDPath(CommonDeviceInfo::ICDTypes::Vulkan, *ret);
                if (auto ret = QueryReg(handle, LvZeroPath); ret)
                    info.SetICDPath(CommonDeviceInfo::ICDTypes::LevelZero, *ret);
            }
            else
            {
                if (D3DKMT_OPENGLINFO glinfo{}; QueryAdapterInfo(handle, KMTQAITYPE_UMOPENGLINFO, glinfo))
                    info.SetICDPath(CommonDeviceInfo::ICDTypes::OpenGL, reinterpret_cast<const char16_t*>(glinfo.UmdOpenGlIcdFileName));
            }
            if (handleType == HandleType::Temp)
            {
                CloseKMTHandle(handle);
            }
        }
    }
};

#define FAILED_THEN(eval, exec, ...) if (const common::HResultHolder hr___ = eval; !hr___)                                  \
{                                                                                                                           \
    Console.Print(common::CommonColor::BrightRed, common::str::Formatter<char16_t>{}.FormatDynamic(__VA_ARGS__, hr___));    \
    exec;                                                                                                                   \
}

#define GET_FUNCTION(var, dll, name) const auto var = (PFN_##name)GetProcAddress(dll, #name);                                           \
do                                                                                                                                      \
{                                                                                                                                       \
    if (!var)                                                                                                                           \
    {                                                                                                                                   \
        const auto err = common::Win32ErrorHolder::GetLastError();                                                                      \
        COMMON_THROWEX(common::BaseException, u"Cannot Load function [" #name "]").Attach("errcode", err).Attach("detail", err.ToStr());\
    }                                                                                                                                   \
} while (0)

struct Win32DeviceInfoContainer final : public CommonDeviceContainer, public D3DDeviceLocator
{
    const common::console::ConsoleEx& Console;
    std::vector<Win32DeviceInfo> Infos;
    void PrintError(std::u16string msg, const common::BaseException& be) const noexcept
    {
        msg.append(u": ["sv).append(be.Message()).append(u"]\n"sv).append(be.GetDetailMessage()).append(u"\n"sv);
        Console.Print(common::CommonColor::BrightRed, msg);
    }
    Win32DeviceInfoContainer() : Console(common::console::ConsoleEx::Get())
    {
        try
        {
            const CommonInfoHelper helper;
#ifdef HAS_DXCORE
            try
            {
                common::DynLib dxcore(L"dxcore.dll");
                const auto CreateAdapterFactory = dxcore.GetFunction<PFN_DXCoreCreateAdapterFactory>("DXCoreCreateAdapterFactory");

                ComPtr<IDXCoreAdapterFactory> coreFactory;
                THROW_HR(CreateAdapterFactory(IID_PPV_ARGS(&coreFactory)), u"Cannot create DXCoreFactory");
                ComPtr<IDXCoreAdapterList> coreAdapters;
                const GUID dxGUIDs[] = { DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE };
                THROW_HR(coreFactory->CreateAdapterList(1, dxGUIDs, IID_PPV_ARGS(&coreAdapters)), u"Cannot create DXCoreAdapterList");

                for (uint32_t i = 0; i < coreAdapters->GetAdapterCount(); i++)
                {
                    ComPtr<IDXCoreAdapter> adapter;
                    coreAdapters->GetAdapter(i, adapter.GetAddressOf());
                    if (adapter)
                    {
                        size_t nameSize = 0;
                        FAILED_THEN(adapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription, &nameSize), continue, u"Cannot get adapter name size: {}\n"sv);
                        std::string name_(nameSize, '\0');
                        FAILED_THEN(adapter->GetProperty(DXCoreAdapterProperty::DriverDescription, nameSize, name_.data()), continue, u"Cannot get adapter name: {}\n"sv);
                        const auto name = common::str::TrimStringView<char>(name_);

                        LUID luid;
                        FAILED_THEN(adapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, &luid), continue, u"Cannot get adapter LUID for [{}]: {}\n"sv, name);

                        DXCoreHardwareID hwId;
                        FAILED_THEN(adapter->GetProperty(DXCoreAdapterProperty::HardwareID, &hwId), continue, u"Cannot get adapter ID for [{}]: {}\n"sv, name);

                        auto& info = Infos.emplace_back();
                        info.Name.assign(name.begin(), name.end());
                        memcpy_s(info.Luid.data(), sizeof(info.Luid), &luid, sizeof(luid));
                        info.VendorId = hwId.vendorID, info.DeviceId = hwId.deviceID;
                        helper.FillDeviceInfo(info);
                    }
                }

                if (!Infos.empty())
                    return;
            }
            catch (const common::BaseException& be)
            {
                PrintError(u"Failed when try using dxcore", be);
            }
#endif
            try
            {
                common::DynLib dxgi(L"dxgi.dll");
                const auto CreateFactory = dxgi.GetFunction<decltype(&CreateDXGIFactory)>("CreateDXGIFactory");

                ComPtr<IDXGIFactory> factory;
                THROW_HR(CreateFactory(IID_PPV_ARGS(&factory)), u"Cannot create DXGIFactory");

                for (uint32_t idx = 0; ; ++idx)
                {
                    ComPtr<IDXGIAdapter> adapter;
                    if (factory->EnumAdapters(idx, &adapter) == DXGI_ERROR_NOT_FOUND)
                        break;
                    DXGI_ADAPTER_DESC desc = {};
                    FAILED_THEN(adapter->GetDesc(&desc), continue, u"Cannot get adapter desc: {}\n"sv);

                    auto& info = Infos.emplace_back();
                    info.Name = reinterpret_cast<const char16_t*>(desc.Description);
                    memcpy_s(info.Luid.data(), sizeof(info.Luid), &desc.AdapterLuid, sizeof(desc.AdapterLuid));
                    info.VendorId = desc.VendorId, info.DeviceId = desc.DeviceId;
                    helper.FillDeviceInfo(info);
                }

                if (!Infos.empty())
                    return;
            }
            catch (const common::BaseException& be)
            {
                PrintError(u"Failed when try using dxgi", be);
            }
        }
        catch (const common::BaseException& be)
        {
            PrintError(u"Failed when load D3DKMT", be);
        }
    }

    const CommonDeviceInfo* LocateByDevicePath(std::u16string_view devPath) const noexcept final
    {
        const auto devPathUpper = common::str::ToUpperEng(devPath);
        for (const auto& dev : Infos)
        {
            if (dev.Texts.GetStringView(dev.DevicePathUpper) == devPathUpper)
                return &dev;
        }
        return nullptr;
    }
    const CommonDeviceInfo* LocateExactDevice(void* d3dHandle) const noexcept final
    {
        if (!d3dHandle)
            return nullptr;

        ComPtr<IDXGIDevice> device;
        FAILED_THEN(reinterpret_cast<IUnknown*>(d3dHandle)->QueryInterface(device.GetAddressOf()), return nullptr, u"Cannot get dxgi device: {}\n"sv);
        
        ComPtr<IDXGIAdapter> adapter;
        FAILED_THEN(device->GetAdapter(adapter.GetAddressOf()), return nullptr, u"Cannot get dxgi adapter: {}\n"sv);
        
        DXGI_ADAPTER_DESC desc = {};
        FAILED_THEN(adapter->GetDesc(&desc), return nullptr, u"Cannot get adapter desc: {}\n"sv);

        std::array<std::byte, 8> luid = { };
        memcpy_s(luid.data(), sizeof(luid), &desc.AdapterLuid, sizeof(desc.AdapterLuid));
        return CommonDeviceContainer::LocateExactDevice(&luid, nullptr, nullptr, {});
    }
    const CommonDeviceInfo& Get(size_t index) const noexcept final { return Infos[index]; }
    size_t GetSize() const noexcept final { return Infos.size(); }
};

const CommonDeviceContainer& ProbeDevice()
{
    static Win32DeviceInfoContainer container;
    return container;
}


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
