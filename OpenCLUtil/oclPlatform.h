#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
using oclPlatform = const oclPlatform_*;


class OCLUAPI oclPlatform_ : public detail::oclCommon
{
    friend class oclUtil;
    friend class GLInterop;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    CLHandle<detail::CLPlatform> PlatformID;
    std::vector<oclDevice_> Devices;
    oclDevice DefDevice;
    common::container::FrozenDenseStringSet<char> Extensions;

    oclPlatform_(const detail::PlatFuncs* funcs, void* platID);
    void InitDevice();
    std::vector<intptr_t> GetCLProps() const;
    oclContext CreateContext(const std::vector<oclDevice>& devs, const std::vector<intptr_t>& props) const;
public:
    std::u16string Name, Ver;
    uint32_t Version;
    Vendors PlatVendor;
    bool BeignetFix = false;
    COMMON_NO_COPY(oclPlatform_)
    COMMON_NO_MOVE(oclPlatform_)
    [[nodiscard]] std::vector<oclDevice> GetDevices() const;
    [[nodiscard]] const common::container::FrozenDenseStringSet<char>& GetExtensions() const { return Extensions; }
    [[nodiscard]] oclDevice GetDefaultDevice() const { return DefDevice; }
    [[nodiscard]] oclContext CreateContext(const std::vector<oclDevice>& devs) const { return CreateContext(devs, GetCLProps()); }
    [[nodiscard]] oclContext CreateContext(oclDevice dev) const { return CreateContext(std::vector<oclDevice>{ dev }); }
    [[nodiscard]] oclContext CreateContext() const;

    [[nodiscard]] static common::span<const oclPlatform> GetPlatforms();

};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif