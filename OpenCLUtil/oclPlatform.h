#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
using oclPlatform = std::shared_ptr<const oclPlatform_>;


class OCLUAPI oclPlatform_ : public common::NonCopyable, public common::NonMovable, public std::enable_shared_from_this<oclPlatform_>
{
    friend class oclUtil;
    friend class GLInterop;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    const cl_platform_id PlatformID;
    std::vector<oclDevice_> Devices;
    oclDevice DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    clGetKernelSubGroupInfoKHR_fn FuncClGetKernelSubGroupInfo = nullptr;
    common::container::FrozenDenseSet<std::string> Extensions;

    oclPlatform_(const cl_platform_id pID);
    void InitDevice();
    std::vector<cl_context_properties> GetCLProps() const;
    oclContext CreateContext(const std::vector<oclDevice>& devs, const std::vector<cl_context_properties>& props) const;
public:
    std::u16string Name, Ver;
    uint32_t Version;
    Vendors PlatVendor;
    [[nodiscard]] std::vector<oclDevice> GetDevices() const;
    [[nodiscard]] const common::container::FrozenDenseSet<std::string>& GetExtensions() const { return Extensions; }
    [[nodiscard]] oclDevice GetDefaultDevice() const { return DefDevice; }
    [[nodiscard]] oclContext CreateContext(const std::vector<oclDevice>& devs) const { return CreateContext(devs, GetCLProps()); }
    [[nodiscard]] oclContext CreateContext(oclDevice dev) const { return CreateContext(std::vector<oclDevice>{ dev }); }
    [[nodiscard]] oclContext CreateContext() const;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif