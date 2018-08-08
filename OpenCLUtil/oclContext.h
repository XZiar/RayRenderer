#pragma once

#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclContext : public NonCopyable, public NonMovable
{
    friend class _oclPlatform;
    friend class _oclCmdQue;
    friend class _oclProgram;
    friend class _oclBuffer;
    friend class _oclGLBuffer;
    friend class _oclImage;
    friend class _oclGLImage;
public:
    const vector<oclDevice> Devices;
    const u16string PlatformName;
    const Vendor vendor;
private:
    const cl_context context;
    cl_context CreateContext(const cl_context_properties* props) const;
    _oclContext(const cl_context_properties* props, const vector<oclDevice>& devices, const u16string name, const Vendor thevendor);
    _oclContext(const cl_context_properties* props, const oclDevice& device, const u16string name, const Vendor thevendor);
    oclDevice GetDevice(const cl_device_id devid) const;
public:
    MessageCallBack onMessage = nullptr;
    ~_oclContext();
};

}
using oclContext = Wrapper<detail::_oclContext>;


}