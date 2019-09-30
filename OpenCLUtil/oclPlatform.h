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


class OCLUAPI oclPlatform_ : public NonCopyable, public NonMovable, public std::enable_shared_from_this<oclPlatform_>
{
    friend class ::oclu::oclUtil;
    friend class oclKernel_;
private:
    const cl_platform_id PlatformID;
    vector<oclDevice> Devices;
    oclDevice DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    clGetKernelSubGroupInfoKHR_fn FuncClGetKernelSubGroupInfo = nullptr;

    oclPlatform_(const cl_platform_id pID);
    vector<cl_context_properties> GetCLProps(const oglu::oglContext& context) const;
    oclDevice GetGLDevice(const vector<cl_context_properties>& props) const;
    void Init();
    oclContext CreateContext(const vector<oclDevice>& devs, const vector<cl_context_properties>& props) const;
public:
    const common::container::FrozenDenseSet<string> Extensions;
    const u16string Name, Ver;
    const Vendor PlatVendor;
    const vector<oclDevice>& GetDevices() const { return Devices; }
    const common::container::FrozenDenseSet<string>& GetExtensions() const { return Extensions; }
    const oclDevice& GetDefaultDevice() const { return DefDevice; }
    bool IsGLShared(const oglu::oglContext& context) const;
    oclContext CreateContext(const oglu::oglContext& context = {}) const;
    oclContext CreateContext(const oclDevice& dev) const { return CreateContext({ dev }, GetCLProps({})); }
    oclContext CreateContext(const vector<oclDevice>& devs) const { return CreateContext(devs, GetCLProps({})); }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif