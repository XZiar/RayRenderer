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
private:
    friend class ::oclu::oclUtil;
    vector<cl_context_properties> GetCLProps(const oglu::oglContext& context) const;
    oclDevice GetGLDevice(const vector<cl_context_properties>& props) const;
    const cl_platform_id PlatformID;
    vector<oclDevice> Devices;
    set<string, std::less<>> Extensions;
    oclDevice DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    _oclPlatform(const cl_platform_id pID);
public:
    const u16string Name, Ver;
    const Vendor PlatVendor;
    const vector<oclDevice>& GetDevices() const { return Devices; }
    const set<string, std::less<>>& GetExtensions() const { return Extensions; }
    const oclDevice& GetDefaultDevice() const { return DefDevice; }
    bool IsGLShared(const oglu::oglContext& context) const;
    oclContext CreateContext(const oglu::oglContext& context = {}) const;
};


}
using oclPlatform = Wrapper<detail::_oclPlatform>;

}