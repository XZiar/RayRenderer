#include "XCompRely.h"

#if !__has_include(<xf86drm.h>)
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
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/ConsoleEx.h"

#include <xf86drm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


namespace xcomp
{
using namespace std::string_view_literals;
#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)

common::span<const CommonDeviceInfo> ProbeDevice()
{
    static const auto devs = []() 
    {
        std::vector<CommonDeviceInfo> infos;
        const auto& console = common::console::ConsoleEx::Get();
        try
        {
            common::DynLib libdrm("libdrm.so");
#define GET_FUNC(name) const auto name = libdrm.GetFunction<decltype(&drm##name)>("drm" #name)
            GET_FUNC(GetDevices);
            GET_FUNC(GetVersion);
            GET_FUNC(FreeVersion);
            GET_FUNC(FreeDevices);
            auto count = GetDevices(nullptr, 0);
            if (count <= 0)
                COMMON_THROW(common::BaseException, u"cannot list drm devices"sv).Attach("detail", common::ErrnoHolder::GetCurError().ToStr());
            std::vector<drmDevicePtr> drmDevs(count, nullptr);
            count = GetDevices(drmDevs.data(), count);
            if (count <= 0) return infos;
            drmDevs.resize(count);
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
                const auto nodeIdx = (dev->available_nodes & (1 << DRM_NODE_PRIMARY)) ? DRM_NODE_PRIMARY : DRM_NODE_RENDER;
                if (dev->available_nodes & (1 << nodeIdx))
                {
                    const auto node = dev->nodes[nodeIdx];
                    info.DevicePath = common::str::to_u16string(node);
                    const auto fd = open(node, O_RDONLY | O_CLOEXEC, 0);
                    if (fd == -1)
                    {
                        const auto err = common::ErrnoHolder::GetCurError();
                        console.Print(common::CommonColor::BrightYellow, FMTSTR2(u"Failed when open device node [{}]: [{:#}]\n"sv, node, err));
                    }
                    else
                    {
                        const auto driVer = GetVersion(fd);
                        std::string_view name(driVer->name, driVer->name_len);
                        std::string_view date(driVer->date, driVer->date_len);
                        std::string_view desc(driVer->desc, driVer->desc_len);
                        //info.Name = common::str::to_u16string(name);
                        FreeVersion(driVer);
                        close(fd);
                    }
                }
            }
            FreeDevices(drmDevs.data(), count);
        }
        catch (const common::BaseException& be)
        {
            console.Print(common::CommonColor::BrightRed, fmt::format(u"Failed when load libdrm: [{}]\n{}\n"sv, be.Message(), be.GetDetailMessage()));
        }
        return infos;
    }();
    return devs;
}


}

#endif
