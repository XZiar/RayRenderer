#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{


namespace detail
{

static string GetStr(const cl_device_id deviceID, const cl_device_info type)
{
    string ret;
    size_t size = 0;
    clGetDeviceInfo(deviceID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    clGetDeviceInfo(deviceID, type, size, ret.data(), &size);
    if (size > 0)
        ret.pop_back();//null-terminated
    return ret;
}
static u16string GetUStr(const cl_device_id deviceID, const cl_device_info type)
{
    const auto u8str = GetStr(deviceID, type);
    return u16string(u8str.cbegin(), u8str.cend()); 
}

template<typename T>
static T GetNum(const cl_device_id deviceID, const cl_device_info type)
{
    static_assert(std::is_integral_v<T>, "T should be numeric type");
    T num = 0;
    clGetDeviceInfo(deviceID, type, sizeof(T), &num, nullptr);
    return num;
}

static DeviceType GetDevType(const cl_device_id deviceID)
{
    cl_device_type dtype;
    clGetDeviceInfo(deviceID, CL_DEVICE_TYPE, sizeof(dtype), &dtype, nullptr);
    switch (dtype)
    {
    case CL_DEVICE_TYPE_CPU:
        return DeviceType::CPU;
    case CL_DEVICE_TYPE_GPU:
        return DeviceType::GPU;
    case CL_DEVICE_TYPE_ACCELERATOR:
        return DeviceType::Accelerator;
    case CL_DEVICE_TYPE_CUSTOM:
        return DeviceType::Custom;
    default:
        return DeviceType::Default;
    }
}

_oclDevice::_oclDevice(const cl_device_id dID) 
    : deviceID(dID), Name(GetUStr(dID, CL_DEVICE_NAME)), Vendor(GetUStr(dID, CL_DEVICE_VENDOR)), Version(GetUStr(dID, CL_DEVICE_VERSION)), 
    Extensions(common::str::Split(GetStr(dID, CL_DEVICE_EXTENSIONS), ' ', false)),
    ConstantBufSize(GetNum<uint64_t>(dID, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE)), GlobalMemSize(GetNum<uint64_t>(dID, CL_DEVICE_GLOBAL_MEM_SIZE)),
    LocalMemSize(GetNum<uint64_t>(dID, CL_DEVICE_LOCAL_MEM_SIZE)), MaxMemSize(GetNum<uint64_t>(dID, CL_DEVICE_MAX_MEM_ALLOC_SIZE)),
    GlobalCacheSize(GetNum<uint64_t>(dID, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE)), GlobalCacheLine(GetNum<uint32_t>(dID, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE)),
    SupportProfiling((GetNum<cl_command_queue_properties>(dID, CL_DEVICE_QUEUE_PROPERTIES) & CL_QUEUE_PROFILING_ENABLE) != 0),
    SupportOutOfOrder((GetNum<cl_command_queue_properties>(dID, CL_DEVICE_QUEUE_PROPERTIES) & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) != 0),
    SupportImplicitGLSync(Extensions.Has("cl_khr_gl_event")),
    Type(GetDevType(deviceID)) { }

using namespace std::literals;

u16string_view _oclDevice::GetDeviceTypeName(const DeviceType type)
{
    switch (type)
    {
    case DeviceType::CPU:           return u"CPU"sv;
    case DeviceType::GPU:           return u"GPU"sv;
    case DeviceType::Accelerator:   return u"Accelerator"sv;
    case DeviceType::Custom:        return u"Custom"sv;
    default:                        return u"Unknown"sv;
    }
}





}


}