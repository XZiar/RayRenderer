#include "oclPch.h"
#include "oclDevice.h"
#include "oclPlatform.h"


namespace oclu
{
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using namespace std::literals;


static string GetStr(const detail::PlatFuncs* funcs, const cl_device_id DeviceID, const cl_device_info type)
{
    string ret;
    size_t size = 0;
    funcs->clGetDeviceInfo(DeviceID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    funcs->clGetDeviceInfo(DeviceID, type, size, ret.data(), &size);
    if (size > 0)
        ret.pop_back();//null-terminated
    return ret;
}
static u16string GetUStr(const detail::PlatFuncs* funcs, const cl_device_id DeviceID, const cl_device_info type)
{
    const auto u8str = GetStr(funcs, DeviceID, type);
    return u16string(u8str.cbegin(), u8str.cend());
}
static bool GetBool(const detail::PlatFuncs* funcs, const cl_device_id DeviceID, const cl_device_info type)
{
    cl_bool ret = CL_FALSE;
    funcs->clGetDeviceInfo(DeviceID, type, sizeof(cl_bool), &ret, nullptr);
    return ret == CL_TRUE;
}
template<typename T>
static T GetNum(const detail::PlatFuncs* funcs, const cl_device_id DeviceID, const cl_device_info type, T num = 0)
{
    static_assert(std::is_integral_v<T>, "T should be numeric type");
    funcs->clGetDeviceInfo(DeviceID, type, sizeof(T), &num, nullptr);
    return num;
}
template<typename T, size_t N>
static void GetNums(const detail::PlatFuncs* funcs, const cl_device_id DeviceID, const cl_device_info type, T (&ret)[N])
{
    static_assert(std::is_integral_v<T>, "T should be numeric type");
    funcs->clGetDeviceInfo(DeviceID, type, sizeof(T) * N, &ret, nullptr);
}

union cl_device_topology_amd
{
    struct { cl_uint type; cl_uint data[5]; } raw;
    struct { cl_uint type; cl_char unused[17]; cl_char bus; cl_char device; cl_char function; } pcie;
};

oclDevice_::oclDevice_(const oclPlatform_* plat, void* dID) : detail::oclCommon(*plat),
    DeviceID(dID), Platform(plat), PlatVendor(Platform->PlatVendor)
{
    cl_device_type dtype;
    Funcs->clGetDeviceInfo(*DeviceID, CL_DEVICE_TYPE, sizeof(dtype), &dtype, nullptr);
    switch (dtype)
    {
    case CL_DEVICE_TYPE_CPU:         Type = DeviceType::CPU; break;
    case CL_DEVICE_TYPE_GPU:         Type = DeviceType::GPU; break;
    case CL_DEVICE_TYPE_ACCELERATOR: Type = DeviceType::Accelerator; break;
    case CL_DEVICE_TYPE_CUSTOM:      Type = DeviceType::Custom; break;
    default:                         Type = DeviceType::Other; break;
    }

    Name    = GetUStr(Funcs, *DeviceID, CL_DEVICE_NAME);
    Vendor  = GetUStr(Funcs, *DeviceID, CL_DEVICE_VENDOR);
    Ver     = GetUStr(Funcs, *DeviceID, CL_DEVICE_VERSION);
    CVer    = GetUStr(Funcs, *DeviceID, CL_DEVICE_OPENCL_C_VERSION);
    {
        const auto version = ParseVersionString(Ver, 1);
        Version = version.first * 10 + version.second;
        const auto cversion = ParseVersionString(CVer, 2);
        CVersion = cversion.first * 10 + cversion.second;
    }

    Extensions = common::str::Split(GetStr(Funcs, *DeviceID, CL_DEVICE_EXTENSIONS), ' ', false);

    F64Caps = static_cast<FPConfig>(GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_DOUBLE_FP_CONFIG));
    F32Caps = static_cast<FPConfig>(GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_SINGLE_FP_CONFIG));
    F16Caps = static_cast<FPConfig>(GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_HALF_FP_CONFIG));
    ConstantBufSize     = GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
    GlobalMemSize       = GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_GLOBAL_MEM_SIZE);
    LocalMemSize        = GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_LOCAL_MEM_SIZE);
    MaxMemAllocSize     = GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    GlobalCacheSize     = GetNum<uint64_t>(Funcs, *DeviceID, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
    GetNums(Funcs, *DeviceID, CL_DEVICE_MAX_WORK_ITEM_SIZES, MaxWorkItemSize);
    MaxWorkGroupSize    = GetNum<  size_t>(Funcs, *DeviceID, CL_DEVICE_MAX_WORK_GROUP_SIZE);
    GlobalCacheLine     = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
    MemBaseAddrAlign    = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_MEM_BASE_ADDR_ALIGN);
    ComputeUnits        = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_MAX_COMPUTE_UNITS);
    MaxSubgroupCount    = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_MAX_NUM_SUB_GROUPS);
    VendorId            = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_VENDOR_ID);
    PCIEBus = PCIEDev = PCIEFunc = 0;
    if (Extensions.Has("cl_nv_device_attribute_query"))
    {
        WaveSize = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_WARP_SIZE_NV);
        PCIEBus  = GetNum<uint32_t>(Funcs, *DeviceID, 0x4008/*CL_DEVICE_PCI_BUS_ID_NV*/) & 0xff;
        const auto slot = GetNum<uint32_t>(Funcs, *DeviceID, 0x4009/*CL_DEVICE_PCI_SLOT_ID_NV*/);
        PCIEDev  = slot >> 3;
        PCIEFunc = slot & 0x7;
    }
    else if (Extensions.Has("cl_amd_device_attribute_query"))
    {
        WaveSize = GetNum<uint32_t>(Funcs, *DeviceID, CL_DEVICE_WAVEFRONT_WIDTH_AMD);
        cl_device_topology_amd topology;
        Funcs->clGetDeviceInfo(*DeviceID, CL_DEVICE_TOPOLOGY_AMD, sizeof(topology), &topology, nullptr);
        if (topology.raw.type == 1)
        {
            PCIEBus  = topology.pcie.bus;
            PCIEDev  = topology.pcie.device;
            PCIEFunc = topology.pcie.function;
        }
    }
    else if (PlatVendor == Vendors::Intel && Type == DeviceType::GPU)
    {
        WaveSize = 16;
    }
    {
        cl_device_pci_bus_info_khr pciinfo = {};
        if (Extensions.Has("cl_khr_pci_bus_info") && 
            CL_SUCCESS == Funcs->clGetDeviceInfo(*DeviceID, CL_DEVICE_PCI_BUS_INFO_KHR, sizeof(pciinfo), &pciinfo, nullptr))
        {
            PCIEBus = pciinfo.pci_bus;
            PCIEDev = pciinfo.pci_device;
            PCIEFunc = pciinfo.pci_function;
        }
    }

    const auto props        = GetNum<cl_command_queue_properties>(Funcs, *DeviceID, CL_DEVICE_QUEUE_PROPERTIES);
    SupportProfiling        = (props & CL_QUEUE_PROFILING_ENABLE) != 0;
    SupportOutOfOrder       = (props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) != 0;
    SupportImplicitGLSync   = Extensions.Has("cl_khr_gl_event");
    SupportImage            = GetBool(Funcs, *DeviceID, CL_DEVICE_IMAGE_SUPPORT);
    LittleEndian            = GetBool(Funcs, *DeviceID, CL_DEVICE_ENDIAN_LITTLE);
    HasCompiler             = GetBool(Funcs, *DeviceID, CL_DEVICE_COMPILER_AVAILABLE);
}

std::optional<uint32_t> oclDevice_::GetNvidiaSMVersion() const noexcept
{
    if (Extensions.Has("cl_nv_device_attribute_query"sv))
    {
        uint32_t major = 0, minor = 0;
        Funcs->clGetDeviceInfo(*DeviceID, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(uint32_t), &major, nullptr);
        Funcs->clGetDeviceInfo(*DeviceID, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(uint32_t), &minor, nullptr);
        return major * 10 + minor;
    }
    return {};
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

std::string oclDevice_::GetFPCapabilityStr(const FPConfig cap)
{
    if (cap == FPConfig::Empty)
        return "Empty";
    std::string ret;
#define CAP(cenum) if (HAS_FIELD(cap, FPConfig::cenum)) ret.append(#cenum " | ")
    CAP(Denorm);
    CAP(InfNaN);
    CAP(RoundNearest);
    CAP(RoundZero);
    CAP(RoundInf);
    CAP(FMA);
    CAP(SoftFloat);
    CAP(DivSqrtCorrect);
#undef CAP
    if (!ret.empty())
    {
        ret.pop_back();
        ret.pop_back();
        ret.pop_back();
    }
    return ret;
}


}
