#include "XCompRely.h"

#if !__has_include(<xf86drm.h>) || !__has_include(<libdrm/drm.h>)
#   pragma message("Missing libdrm header, skip libdrm device discovery")

namespace xcomp
{
common::span<const CommonDeviceInfo> ProbeDevice()
{
    return {};
}
}

#else

#include "SystemCommon/ErrorCodeHelper.h"
#include "SystemCommon/DynamicLibrary.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/ConsoleEx.h"

#include <libdrm/drm.h>
#if __has_include(<libdrm/amdgpu.h>) && __has_include(<libdrm/amdgpu_drm.h>) 
#   include <libdrm/amdgpu.h>
#   include <libdrm/amdgpu_drm.h>
#   define XC_USE_AMDGPU 1
#else
#   define XC_USE_AMDGPU 0
#endif
#if __has_include(<drm/i915_drm.h>) 
#   include <sys/ioctl.h>
#   include <drm/i915_drm.h>
#   define XC_USE_I915 1
#else
#   define XC_USE_I915 0
#endif
//#include <libdrm/i915_drm.h>
#include <xf86drm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


namespace xcomp
{
using namespace std::string_view_literals;
#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)

struct DrmHelper
{
    std::string_view DriverName;
    DrmHelper(std::string_view name) noexcept : DriverName(name) {}
    virtual ~DrmHelper() {}
    virtual void FillInfo(CommonDeviceInfo& info, int fd, const drmVersion& version) const noexcept = 0;
};

#define DEF_FUNC(pfx, name) decltype(&pfx##name) name = nullptr
#define GET_FUNC(pfx, name) name = Lib.GetFunction<decltype(&pfx##name)>(#pfx #name)

#if XC_USE_AMDGPU
struct DrmAmd final : public DrmHelper
{
    common::DynLib Lib;
    DEF_FUNC(amdgpu_, device_initialize);
    DEF_FUNC(amdgpu_, device_deinitialize);
    DEF_FUNC(amdgpu_, query_gpu_info);
    DEF_FUNC(amdgpu_, query_info);
    DEF_FUNC(amdgpu_, get_marketing_name);
    DrmAmd(std::string_view name_) : DrmHelper(name_), Lib("libdrm_amdgpu.so")
    {
        GET_FUNC(amdgpu_, device_initialize);
        GET_FUNC(amdgpu_, device_deinitialize);
        GET_FUNC(amdgpu_, query_gpu_info);
        GET_FUNC(amdgpu_, query_info);
        GET_FUNC(amdgpu_, get_marketing_name);
    }
    ~DrmAmd() final {}
    void FillInfo(CommonDeviceInfo& info, int fd, const drmVersion&) const noexcept final
    {
        uint32_t verMajor = 0, verMinor = 0;
        amdgpu_device_handle amdHandle{};
        const auto err = device_initialize(fd, &verMajor, &verMinor, &amdHandle); 
        if (err == 0)
        {
            amdgpu_gpu_info gpuInfo{};
            query_gpu_info(amdHandle, &gpuInfo);
            drm_amdgpu_memory_info memInfo{};
            query_info(amdHandle, AMDGPU_INFO_MEMORY, sizeof(memInfo), &memInfo);
            info.DedicatedVRAM = static_cast<uint32_t>(memInfo.vram.total_heap_size / 1048576u);
            info.SharedSysRAM  = static_cast<uint32_t>(memInfo.gtt.total_heap_size  / 1048576u);
            if (const auto name = get_marketing_name(amdHandle); name)
                info.Name = common::str::to_u16string(name);
            else
                info.Name = u"Unknown AMD GPU";
            device_deinitialize(amdHandle);
        }
    }
};
#endif

struct DrmI915 final : public DrmHelper
{
    DrmI915(std::string_view name_) : DrmHelper(name_)
    {
    }
    ~DrmI915() final {}
    void FillInfo(CommonDeviceInfo& info, int fd, const drmVersion& version) const noexcept final
    {
#if XC_USE_I915
        drm_i915_query_item item = 
        {
        	.query_id = DRM_I915_QUERY_MEMORY_REGIONS
        };
        drm_i915_query query = 
        {
        	.num_items = 1,
        	.items_ptr = (uintptr_t)&item,
        };
        common::ErrnoHolder errret;
        errret = ioctl(fd, DRM_IOCTL_I915_QUERY, &query);
        if (errret)
        {
            if (item.length < 0)
                errret = -item.length;
            else
            {
                std::unique_ptr<std::byte[]> data(new std::byte[item.length]);
                item.data_ptr = reinterpret_cast<uintptr_t>(&data[0]);
                errret = ioctl(fd, DRM_IOCTL_I915_QUERY, &query);
                if (errret)
                {
                    const auto& regions = *reinterpret_cast<const drm_i915_query_memory_regions*>(&data[0]);
                    for (uint32_t i = 0; i < regions.num_regions; i++) 
                    {
                        const auto& region = regions.regions[i];
                        if (region.probed_size == UINT64_MAX) continue;
                        switch (region.region.memory_class)
                        {
                        case I915_MEMORY_CLASS_SYSTEM:
                            info.DedicatedSysRAM = static_cast<uint32_t>(region.probed_size / 1048576u);
                            break;
                        case I915_MEMORY_CLASS_DEVICE:
                            info.DedicatedVRAM   = static_cast<uint32_t>(region.probed_size / 1048576u);
                            break;
                        default: break;
                        }
                    }
                }
            }
        }
        if (!errret)
        {
            common::console::ConsoleEx::Get().Print(common::CommonColor::BrightYellow, 
                FMTSTR2(u"Failed when ioctl libdrm-i915: [{:#}]\n"sv, errret));
        }
#endif
        info.Name = common::str::to_u16string(version.desc, version.desc_len);
    }
};

struct DrmPanfrost final : public DrmHelper
{
    DrmPanfrost(std::string_view name_) : DrmHelper(name_)
    {
    }
    void FillInfo(CommonDeviceInfo& info, int fd, const drmVersion& version) const noexcept final
    {
        info.Name = u"Mali GPU";
    }
};

#undef GET_FUNC
#undef DEF_FUNC

template<typename T, typename... Args>
std::unique_ptr<T> TryLoadLib(std::string_view name, Args&&... args)
{
    try
    {
        return std::make_unique<T>(name, std::forward<Args>(args)...);
    }
    catch(const common::BaseException& be)
    {
        common::console::ConsoleEx::Get().Print(common::CommonColor::BrightYellow, 
            FMTSTR2(u"Failed when load libdrm-[{}]: [{}]\n{}\n"sv, name, be.Message(), be.GetDetailMessage()));
    }
    return {};
}


common::span<const CommonDeviceInfo> ProbeDevice()
{
    static const auto devs = []() 
    {
        std::vector<CommonDeviceInfo> infos;
        const auto& console = common::console::ConsoleEx::Get();

        std::unique_ptr<common::DynLib> libdrm;
        constexpr std::string_view paths[] = { "libdrm.so"sv, "/vendor/lib64/libdrm.so", "/system/lib64/libdrm.so" };
        for (const auto& path : paths)
        {
            try
            {
                libdrm = std::make_unique<common::DynLib>(path);
                break;
            }
            catch (const common::BaseException& be)
            {
                console.Print(common::CommonColor::BrightRed, 
                    FMTSTR2(u"Failed when load libdrm[{}]: [{}]\n{}\n"sv, path, be.Message(), be.GetDetailMessage()));
            }
        }
        if (!libdrm)
            return infos;

        try
        {
#define GET_FUNC(name) const auto name = libdrm->GetFunction<decltype(&drm##name)>("drm" #name)
            GET_FUNC(GetDevices);
            GET_FUNC(GetVersion);
            GET_FUNC(GetLibVersion);
            GET_FUNC(FreeVersion);
            GET_FUNC(FreeDevices);
            GET_FUNC(GetBusid);
#undef GET_FUNC
            auto count = GetDevices(nullptr, 0);
            if (count <= 0)
                COMMON_THROW(common::BaseException, u"cannot list drm devices"sv).Attach("detail", common::ErrnoHolder::GetCurError().ToStr());
            std::vector<drmDevicePtr> drmDevs(count, nullptr);
            count = GetDevices(drmDevs.data(), count);
            if (count <= 0) return infos;
            drmDevs.resize(count);

            std::vector<std::unique_ptr<DrmHelper>> drmLibs;
#if XC_USE_AMDGPU
            if (auto drmAmd = TryLoadLib<DrmAmd>("amdgpu"sv); drmAmd)
                drmLibs.emplace_back(std::move(drmAmd));
#endif
            if (auto drmI915 = TryLoadLib<DrmI915>("i915"sv); drmI915)
                drmLibs.emplace_back(std::move(drmI915));
            if (auto drmPanfrost = TryLoadLib<DrmPanfrost>("panfrost"sv); drmPanfrost)
                drmLibs.emplace_back(std::move(drmPanfrost));

            for (const auto& dev : drmDevs)
            {
                auto& info = infos.emplace_back();
                switch (dev->bustype)
                {
                case DRM_BUS_PCI:
                    info.PCIEAddress = { dev->businfo.pci->bus, dev->businfo.pci->dev, dev->businfo.pci->func };
                    info.VendorId = dev->deviceinfo.pci->vendor_id;
                    info.DeviceId = dev->deviceinfo.pci->device_id;
                    break;
                case DRM_BUS_USB:
                    info.VendorId = dev->deviceinfo.usb->vendor;
                    info.DeviceId = dev->deviceinfo.usb->product;
                    break;
                default:
                    break;
                }
                info.DevicePath = common::str::to_u16string(dev->nodes[DRM_NODE_PRIMARY]);
                info.SupportRender  = dev->available_nodes & (1 << DRM_NODE_RENDER);
                //info.SupportDisplay = dev->available_nodes & (1 << DRM_NODE_PRIMARY);
                // Use render node which requires no permission: https://dri.freedesktop.org/docs/drm/gpu/drm-uapi.html#render-nodes
                const auto fdNode = dev->nodes[info.SupportRender ? DRM_NODE_RENDER : DRM_NODE_PRIMARY];
                if (const auto fd = open(fdNode, O_RDONLY | O_CLOEXEC, 0); fd < 0)
                {
                    const auto err = common::ErrnoHolder::GetCurError();
                    console.Print(common::CommonColor::BrightYellow, FMTSTR2(u"Failed when open device node [{}]: [{:#}]\n"sv, fdNode, err));
                }
                else
                {
                    const auto driVer  = GetVersion(fd);
                    const auto driVer2 = GetLibVersion(fd);
                    const auto busid   = GetBusid(fd);
                    std::string_view name(driVer->name, driVer->name_len);
                    std::string_view date(driVer->date, driVer->date_len);
                    std::string_view desc(driVer->desc, driVer->desc_len);
                    for (const auto& drmLib : drmLibs)
                    {
                        if (drmLib->DriverName == name)
                        {
                            drmLib->FillInfo(info, fd, *driVer);
                            break;
                        }
                    }

                    //info.Name = common::str::to_u16string(name);
                    FreeVersion(driVer);
                    FreeVersion(driVer2);
                    close(fd);
                }
            }
            FreeDevices(drmDevs.data(), count);
        }
        catch (const common::BaseException& be)
        {
            console.Print(common::CommonColor::BrightRed, FMTSTR2(u"Failed when load libdrm: [{}]\n{}\n"sv, be.Message(), be.GetDetailMessage()));
        }
        return infos;
    }();
    return devs;
}


}

#endif
