#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


namespace oclu
{


namespace detail
{

class OCLUAPI _oclPlatform : public NonCopyable, public NonMovable
{
    friend class ::oclu::oclUtil;
private:
    const cl_platform_id PlatformID;
    vector<oclDevice> Devices;
    oclDevice DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    vector<cl_context_properties> GetCLProps(const oglu::oglContext& context) const;
    oclDevice GetGLDevice(const vector<cl_context_properties>& props) const;
    _oclPlatform(const cl_platform_id pID);
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
using oclPlatform = Wrapper<detail::_oclPlatform>;

}