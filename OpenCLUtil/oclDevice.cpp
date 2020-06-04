#include "oclPch.h"
#include "oclDevice.h"
#include "oclPlatform.h"


namespace oclu
{
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;


static string GetStr(const cl_device_id DeviceID, const cl_device_info type)
{
    string ret;
    size_t size = 0;
    clGetDeviceInfo(DeviceID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    clGetDeviceInfo(DeviceID, type, size, ret.data(), &size);
    if (size > 0)
        ret.pop_back();//null-terminated
    return ret;
}
static u16string GetUStr(const cl_device_id DeviceID, const cl_device_info type)
{
    const auto u8str = GetStr(DeviceID, type);
    return u16string(u8str.cbegin(), u8str.cend());
}
static bool GetBool(const cl_device_id DeviceID, const cl_device_info type)
{
    cl_bool ret = CL_FALSE;
    clGetDeviceInfo(DeviceID, type, sizeof(cl_bool), &ret, nullptr);
    return ret == CL_TRUE;
}
template<typename T>
static T GetNum(const cl_device_id DeviceID, const cl_device_info type)
{
    static_assert(std::is_integral_v<T>, "T should be numeric type");
    T num = 0;
    clGetDeviceInfo(DeviceID, type, sizeof(T), &num, nullptr);
    return num;
}
template<typename T, size_t N>
static void GetNums(const cl_device_id DeviceID, const cl_device_info type, T (&ret)[N])
{
    static_assert(std::is_integral_v<T>, "T should be numeric type");
    clGetDeviceInfo(DeviceID, type, sizeof(T) * N, &ret, nullptr);
}

oclDevice_::oclDevice_(const std::weak_ptr<const oclPlatform_>& plat, const cl_device_id dID) :
    Platform(plat), DeviceID(dID), PlatVendor(Platform.lock()->PlatVendor)
{
    cl_device_type dtype;
    clGetDeviceInfo(DeviceID, CL_DEVICE_TYPE, sizeof(dtype), &dtype, nullptr);
    switch (dtype)
    {
    case CL_DEVICE_TYPE_CPU:         Type = DeviceType::CPU; break;
    case CL_DEVICE_TYPE_GPU:         Type = DeviceType::GPU; break;
    case CL_DEVICE_TYPE_ACCELERATOR: Type = DeviceType::Accelerator; break;
    case CL_DEVICE_TYPE_CUSTOM:      Type = DeviceType::Custom; break;
    default:                         Type = DeviceType::Other; break;
    }

    Name    = GetUStr(DeviceID, CL_DEVICE_NAME);
    Vendor  = GetUStr(DeviceID, CL_DEVICE_VENDOR);
    Ver     = GetUStr(DeviceID, CL_DEVICE_VERSION);
    CVer    = GetUStr(DeviceID, CL_DEVICE_OPENCL_C_VERSION);
    {
        const auto version = ParseVersionString(Ver, 1);
        Version = version.first * 10 + version.second;
        const auto cversion = ParseVersionString(CVer, 2);
        CVersion = cversion.first * 10 + cversion.second;
    }

    Extensions = common::str::Split(GetStr(DeviceID, CL_DEVICE_EXTENSIONS), ' ', false);

    ConstantBufSize     = GetNum<uint64_t>(DeviceID, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
    GlobalMemSize       = GetNum<uint64_t>(DeviceID, CL_DEVICE_GLOBAL_MEM_SIZE);
    LocalMemSize        = GetNum<uint64_t>(DeviceID, CL_DEVICE_LOCAL_MEM_SIZE);
    MaxMemAllocSize     = GetNum<uint64_t>(DeviceID, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    GlobalCacheSize     = GetNum<uint64_t>(DeviceID, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
    GetNums(DeviceID, CL_DEVICE_MAX_WORK_ITEM_SIZES, MaxWorkItemSize);
    MaxWorkGroupSize    = GetNum<  size_t>(DeviceID, CL_DEVICE_MAX_WORK_GROUP_SIZE);
    GlobalCacheLine     = GetNum<uint32_t>(DeviceID, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
    MemBaseAddrAlign    = GetNum<uint32_t>(DeviceID, CL_DEVICE_MEM_BASE_ADDR_ALIGN);
    ComputeUnits        = GetNum<uint32_t>(DeviceID, CL_DEVICE_MAX_COMPUTE_UNITS);
    if (Extensions.Has("cl_nv_device_attribute_query"))
        WaveSize = GetNum<uint32_t>(DeviceID, CL_DEVICE_WARP_SIZE_NV);
    else if (Extensions.Has("cl_amd_device_attribute_query"))
        WaveSize = GetNum<uint32_t>(DeviceID, CL_DEVICE_WAVEFRONT_WIDTH_AMD);
    else if (PlatVendor == Vendors::Intel && Type == DeviceType::GPU)
        WaveSize = 16;

    const auto props        = GetNum<cl_command_queue_properties>(DeviceID, CL_DEVICE_QUEUE_PROPERTIES);
    SupportProfiling        = (props & CL_QUEUE_PROFILING_ENABLE) != 0;
    SupportOutOfOrder       = (props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) != 0;
    SupportImplicitGLSync   = Extensions.Has("cl_khr_gl_event");
    SupportImage            = GetBool(DeviceID, CL_DEVICE_IMAGE_SUPPORT);
    LittleEndian            = GetBool(DeviceID, CL_DEVICE_ENDIAN_LITTLE);
    HasCompiler             = GetBool(DeviceID, CL_DEVICE_COMPILER_AVAILABLE);
}

using namespace std::literals;

std::shared_ptr<const oclPlatform_> oclDevice_::GetPlatform() const
{
    return Platform.lock();
}

u16string_view oclDevice_::GetDeviceTypeName(const DeviceType type)
{
    switch (type)
    {
    case DeviceType::CPU:           return u"CPU"sv;
    case DeviceType::GPU:           return u"GPU"sv;
    case DeviceType::Accelerator:   return u"Accelerator"sv;
    case DeviceType::Custom:        return u"Custom"sv;
    default:                        return u"Other"sv;
    }
}


}
