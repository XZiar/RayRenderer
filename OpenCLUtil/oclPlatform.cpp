#include "oclRely.h"
#include "oclPlatform.h"
#include "oclException.h"
#include "oclUtil.h"

using common::container::FindInVec;
using common::linq::Linq;

namespace oclu
{


vector<cl_context_properties> oclPlatform_::GetCLProps() const
{
    vector<cl_context_properties> props{ CL_CONTEXT_PLATFORM, (cl_context_properties)PlatformID };
    props.push_back(0);
    return props;
}


static Vendors JudgeBand(const u16string& name)
{
    const auto capName = common::strchset::ToUpperEng(name, str::Charset::UTF16LE);
    if (capName.find(u"NVIDIA") != u16string::npos)
        return Vendors::NVIDIA;
    else if (capName.find(u"AMD") != u16string::npos)
        return Vendors::AMD;
    else if (capName.find(u"INTEL") != u16string::npos)
        return Vendors::Intel;
    else
        return Vendors::Other;
}

static string GetStr(const cl_platform_id platformID, const cl_platform_info type)
{
    thread_local string ret;
    size_t size = 0;
    clGetPlatformInfo(platformID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    clGetPlatformInfo(platformID, type, size, ret.data(), &size);
    if(!ret.empty())
        ret.pop_back();
    return ret;
}
static u16string GetUStr(const cl_platform_id platformID, const cl_platform_info type)
{
    const auto u8str = GetStr(platformID, type);
    return u16string(u8str.cbegin(), u8str.cend()); 
}

oclPlatform_::oclPlatform_(const cl_platform_id pID)
    : PlatformID(pID), DefDevice(nullptr)
{
    Extensions = common::str::Split(GetStr(PlatformID, CL_PLATFORM_EXTENSIONS), ' ', false);
    Name = GetUStr(pID, CL_PLATFORM_NAME);
    Ver = GetUStr(pID, CL_PLATFORM_VERSION);
    PlatVendor = JudgeBand(Name);

    if (Ver.find(u"beignet") == u16string::npos) // beignet didn't implement that
        FuncClGetGLContext = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(PlatformID, "clGetGLContextInfoKHR");
    FuncClGetKernelSubGroupInfo = (clGetKernelSubGroupInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(PlatformID, "clGetKernelSubGroupInfoKHR");
    
    cl_uint numDevices;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    // Get all Device Info
    vector<cl_device_id> DeviceIDs(numDevices);
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, numDevices, DeviceIDs.data(), nullptr);

    //const auto self = weak_from_this();
    Devices = Linq::FromIterable(DeviceIDs)
        .Select([](const auto dID) { return oclDevice_(dID); })
        .ToVector();

    cl_device_id defDevID;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, nullptr);
    DefDevice = Linq::FromIterable(Devices)
        .Where([=](const auto& dev) { return dev.DeviceID == defDevID; })
        .Select([](const auto& dev) { return &dev; })
        .TryGetFirst().value_or(nullptr);
}

oclContext oclPlatform_::CreateContext(const vector<oclDevice>& devs, const vector<cl_context_properties>& props) const
{

    for (const auto& dev : devs)
    {
        if (dev < &Devices.front() || dev > &Devices.back())
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"cannot using device from other platform", dev);
    }
    const auto self = shared_from_this();
    return MAKE_ENABLER_SHARED(oclContext_, self, props, devs);
}

vector<oclDevice> oclPlatform_::GetDevices() const
{
    return Linq::FromIterable(Devices).Select([](const auto& dev) { return &dev; }).ToVector();
}

oclContext oclPlatform_::CreateContext() const
{
    return CreateContext(GetDevices());
}

}
