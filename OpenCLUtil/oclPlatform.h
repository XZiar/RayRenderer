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
    const cl_platform_id PlatformID;
    std::vector<oclDevice_> Devices;
    oclDevice DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    clGetKernelSubGroupInfoKHR_fn FuncClGetKernelSubGroupInfo = nullptr;
    common::container::FrozenDenseSet<std::string> Extensions;

    oclPlatform_(const cl_platform_id pID);
    std::vector<cl_context_properties> GetCLProps() const;
    oclContext CreateContext(const std::vector<oclDevice>& devs, const std::vector<cl_context_properties>& props) const;
public:
    std::u16string Name, Ver;
    Vendors PlatVendor;
    std::vector<oclDevice> GetDevices() const;
    const common::container::FrozenDenseSet<std::string>& GetExtensions() const { return Extensions; }
    oclDevice GetDefaultDevice() const { return DefDevice; }
    oclContext CreateContext(const std::vector<oclDevice>& devs) const { return CreateContext(devs, GetCLProps()); }
    oclContext CreateContext(oclDevice dev) const { return CreateContext(std::vector<oclDevice>{ dev }); }
    oclContext CreateContext() const;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif