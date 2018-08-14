#include "oclRely.h"
#include "oclPlatform.h"


using common::container::FindInVec;

namespace oclu
{


namespace detail
{
vector<cl_context_properties> _oclPlatform::GetCLProps(const oglu::oglContext & context) const
{
    vector<cl_context_properties> props;
    //OpenCL platform
    props.assign({ CL_CONTEXT_PLATFORM, (cl_context_properties)PlatformID });
    if (context)
        props.insert(props.cend(),
        {
            //OpenGL context
            CL_GL_CONTEXT_KHR,   (cl_context_properties)context->Hrc,
            //HDC used to create the OpenGL context
            CL_WGL_HDC_KHR,      (cl_context_properties)context->Hdc
        });
    props.push_back(0);
    return props;
}

oclDevice _oclPlatform::GetGLDevice(const vector<cl_context_properties>& props) const
{
    if (!FuncClGetGLContext) return {};
    cl_device_id dID;
    size_t retSize = 0;
    const auto ret = FuncClGetGLContext(props.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &dID, &retSize);
    if (ret == CL_SUCCESS && retSize)
        if (auto dev = FindInVec(Devices, [=](const oclDevice& d) { return d->deviceID == dID; }); dev)
            return *dev;
    return {};
}

static Vendor JudgeBand(const u16string& name)
{
    const wstring& wname = *reinterpret_cast<const std::wstring*>(&name);
    if (str::ifind_first(wname, L"nvidia").has_value())
        return Vendor::NVIDIA;
    else if (str::ifind_first(wname, L"amd").has_value())
        return Vendor::AMD;
    else if (str::ifind_first(wname, L"intel").has_value())
        return Vendor::Intel;
    else
        return Vendor::Other;
}

static u16string GetStr(const cl_platform_id platformID, const cl_platform_info type)
{
    thread_local string ret;
    size_t size = 0;
    clGetPlatformInfo(platformID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    clGetPlatformInfo(platformID, type, size, ret.data(), &size);
    return u16string(ret.cbegin(), ret.cend() - 1); //null-terminated
}

_oclPlatform::_oclPlatform(const cl_platform_id pID)
    : PlatformID(pID), Name(GetStr(pID, CL_PLATFORM_NAME)), Ver(GetStr(pID, CL_PLATFORM_VERSION)), PlatVendor(JudgeBand(Name))
{
    FuncClGetGLContext = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(PlatformID, "clGetGLContextInfoKHR");
    cl_device_id defDevID;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, nullptr);
    cl_uint numDevices;
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    // Get all Device Info
    vector<cl_device_id> deviceIDs(numDevices);
    clGetDeviceIDs(PlatformID, CL_DEVICE_TYPE_ALL, numDevices, deviceIDs.data(), nullptr);

    for (const auto & dID : deviceIDs)
    {
        Devices.push_back(oclDevice(new _oclDevice(dID)));
        if (dID == defDevID)
            DefDevice = Devices.back();
    }
}

bool _oclPlatform::IsGLShared(const oglu::oglContext & context) const
{
    return (bool)GetGLDevice(GetCLProps(context));
}

oclContext _oclPlatform::CreateContext(const oglu::oglContext& context) const
{
    const auto props = GetCLProps(context);
    if (context)
    {
        if (const auto dev = GetGLDevice(props); dev)
            return oclContext(new _oclContext(props.data(), dev, Name, PlatVendor));
        return {};
    }
    else
        return oclContext(new _oclContext(props.data(), Devices, Name, PlatVendor));
}



}


}