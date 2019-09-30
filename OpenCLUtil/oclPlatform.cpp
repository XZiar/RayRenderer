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


static Vendor JudgeBand(const u16string& name)
{
    const auto capName = common::strchset::ToUpperEng(name, str::Charset::UTF16LE);
    if (capName.find(u"NVIDIA") != u16string::npos)
        return Vendor::NVIDIA;
    else if (capName.find(u"AMD") != u16string::npos)
        return Vendor::AMD;
    else if (capName.find(u"INTEL") != u16string::npos)
        return Vendor::Intel;
    else
        return Vendor::Other;
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
    : PlatformID(pID), Extensions(common::str::Split(GetStr(PlatformID, CL_PLATFORM_EXTENSIONS), ' ', false)), 
    Name(GetUStr(pID, CL_PLATFORM_NAME)), Ver(GetUStr(pID, CL_PLATFORM_VERSION)), PlatVendor(JudgeBand(Name))
{
    if (Ver.find(u"beignet") == u16string::npos) // beignet didn't implement that
        FuncClGetGLContext = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(PlatformID, "clGetGLContextInfoKHR");
    FuncClGetKernelSubGroupInfo = (clGetKernelSubGroupInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(PlatformID, "clGetKernelSubGroupInfoKHR");
}

void oclPlatform_::Init()
{
    cl_uint numDevices;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    // Get all Device Info
    vector<cl_device_id> deviceIDs(numDevices);
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, numDevices, deviceIDs.data(), nullptr);

    const auto self = weak_from_this();
    Devices = Linq::FromIterable(deviceIDs)
        .Select([&self](const auto dID) { return MAKE_ENABLER_SHARED_CONST(oclDevice_, self, dID); })
        .ToVector();

    cl_device_id defDevID;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, nullptr);
    DefDevice = Linq::FromIterable(Devices)
        .Where([=](const auto& dev) { return dev->deviceID == defDevID; })
        .TryGetFirst().value_or(oclDevice{});
}


oclContext oclPlatform_::CreateContext(const vector<oclDevice>& devs, const vector<cl_context_properties>& props) const
{
    const auto self = shared_from_this();
    for (const auto& dev : devs)
        if (!detail::owner_equals(dev->Plat, self))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"cannot using device from other platform", dev);
    return MAKE_ENABLER_SHARED(oclContext_, self, props, devs, Name, PlatVendor);
}


}
