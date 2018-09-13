#include "oclRely.h"
#include "oclPlatform.h"
#include "oclUtil.h"

using common::container::FindInVec;

namespace oclu
{


namespace detail
{
vector<cl_context_properties> _oclPlatform::GetCLProps(const oglu::oglContext & context) const
{
#if defined(_WIN32)
    constexpr cl_context_properties glPropName = CL_WGL_HDC_KHR;
#else
    constexpr cl_context_properties glPropName = CL_GLX_DISPLAY_KHR;
#endif
    vector<cl_context_properties> props;
    //OpenCL platform
    props.assign({ CL_CONTEXT_PLATFORM, (cl_context_properties)PlatformID });
    if (context)
        props.insert(props.cend(),
        {
            //OpenGL context
            CL_GL_CONTEXT_KHR,   (cl_context_properties)context->Hrc,
            //HDC used to create the OpenGL context
            glPropName,          (cl_context_properties)context->Hdc
        });
    props.push_back(0);
    return props;
}

oclDevice _oclPlatform::GetGLDevice(const vector<cl_context_properties>& props) const
{
    if (!FuncClGetGLContext) return {};
    {
        cl_device_id dID;
        size_t retSize = 0;
        const auto ret = FuncClGetGLContext(props.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &dID, &retSize);
        if (ret == CL_SUCCESS && retSize)
            if (auto dev = FindInVec(Devices, [=](const oclDevice& d) { return d->deviceID == dID; }); dev)
                return *dev;
        if (ret != CL_SUCCESS && ret != CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
            oclLog().warning(u"Failed to get current device for glContext: [{}]\n", oclUtil::getErrorString(ret));
    }
    //try context that may be associated 
    {
        std::vector<cl_device_id> dIDs(Devices.size());
        size_t retSize = 0;
        const auto ret = FuncClGetGLContext(props.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id) * Devices.size(), dIDs.data(), &retSize);
        if (ret == CL_SUCCESS && retSize)
            if (auto dev = FindInVec(Devices, [=](const oclDevice& d) { return d->deviceID == dIDs[0]; }); dev)
                return *dev;
        if (ret != CL_SUCCESS && ret != CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
            oclLog().warning(u"Failed to get associate device for glContext: [{}]\n", oclUtil::getErrorString(ret));
    }
    return {};
}

static Vendor JudgeBand(const u16string& name)
{
    const auto capName = str::ToUpperEng(name, str::Charset::UTF16LE);
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

_oclPlatform::_oclPlatform(const cl_platform_id pID)
    : PlatformID(pID), Extensions(common::str::Split(GetStr(PlatformID, CL_PLATFORM_EXTENSIONS), ' ', false)), 
    Name(GetUStr(pID, CL_PLATFORM_NAME)), Ver(GetUStr(pID, CL_PLATFORM_VERSION)), PlatVendor(JudgeBand(Name))
{
    if (Ver.find(u"beignet") == u16string::npos) // beignet didn't implement that
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