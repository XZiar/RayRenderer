#include "oclRely.h"
#include "oclContext.h"
#include "oclException.h"
#include "oclDevice.h"
#include "oclUtil.h"
#include "CL/cl_ext_intel.h"


using common::container::FindInVec;
namespace oclu
{

namespace detail
{


static void CL_CALLBACK onNotify(const char * errinfo, [[maybe_unused]]const void * private_info, size_t, void *user_data)
{
    const _oclContext& ctx = *(_oclContext*)user_data;
    const auto u16Info = str::to_u16string(errinfo);
    oclLog().verbose(u16Info);
    if (ctx.onMessage)
        ctx.onMessage(u16Info);
    return;
}

cl_context _oclContext::CreateContext(vector<cl_context_properties>& props) const
{
    cl_int ret;
    vector<cl_device_id> deviceIDs;
    bool supportIntelDiag = true;
    for (const auto& dev : Devices)
    {
        deviceIDs.push_back(dev->deviceID);
        supportIntelDiag &= dev->Extensions.Has("cl_intel_driver_diagnostics");
    }
    constexpr cl_context_properties intelDiagnostics = CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL | CL_CONTEXT_DIAGNOSTICS_LEVEL_GOOD_INTEL | CL_CONTEXT_DIAGNOSTICS_LEVEL_NEUTRAL_INTEL;
    if (supportIntelDiag)
        props.insert(props.cend(), { CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL, intelDiagnostics }); 
    const auto ctx = clCreateContext(props.data(), (cl_uint)deviceIDs.size(), deviceIDs.data(), &onNotify, const_cast<_oclContext*>(this), &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create opencl-context", ret));
    return ctx;
}

_oclContext::_oclContext(vector<cl_context_properties> props, const vector<oclDevice>& devices, const u16string name, const Vendor thevendor)
    : Devices(devices), PlatformName(name), vendor(thevendor), context(CreateContext(props)) { }
_oclContext::_oclContext(vector<cl_context_properties> props, const oclDevice& device, const u16string name, const Vendor thevendor)
    : Devices({ device }), PlatformName(name), vendor(thevendor), context(CreateContext(props)) { }

oclDevice _oclContext::GetDevice(const cl_device_id devid) const
{
    const auto it = FindInVec(Devices, [=](const oclDevice& dev) {return dev->deviceID == devid; });
    return it ? *it : oclDevice{};
}

_oclContext::~_oclContext()
{
#ifdef _DEBUG
    uint32_t refCount = 0;
    clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
    if (refCount == 1)
    {
        oclLog().debug(u"oclContext {:p} named {}, has {} reference being release.\n", (void*)context, PlatformName, refCount);
        clReleaseContext(context);
    }
    else
        oclLog().warning(u"oclContext {:p} named {}, has {} reference and not able to release.\n", (void*)context, PlatformName, refCount);
#else
    clReleaseContext(context);
#endif
}


}



}