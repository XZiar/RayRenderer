#include "oclRely.h"
#include "oclUtil.h"

namespace oclu
{



common::mlog::MiniLogger<false>& oclUtil::GetOCLLog()
{
    return oclLog();
}

void oclUtil::LogCLInfo()
{
    for (const auto& plat : GetPlatforms())
    {
        auto& strBuffer = common::mlog::detail::StrFormater::GetBuffer<char16_t>();
        fmt::format_to(strBuffer, u"\nPlatform {} --- {}\n", plat->Name, plat->Ver);
        for (const auto dev : plat->GetDevices())
            fmt::format_to(strBuffer, u"--Device {}: {} -- {} -- {}\n", dev->GetTypeName(),
                dev->Name, dev->Vendor, dev->Version);
        oclLog().verbose(strBuffer);
    }
}

const vector<oclPlatform>& oclUtil::GetPlatforms()
{
    static vector<oclPlatform> Plats = []()
    {
        vector<oclPlatform> plats;
        cl_uint numPlatforms = 0;
        clGetPlatformIDs(0, nullptr, &numPlatforms);
        //Get all Platforms
        vector<cl_platform_id> platformIDs(numPlatforms);
        clGetPlatformIDs(numPlatforms, platformIDs.data(), nullptr);
        for (const auto& pID : platformIDs)
        {
            auto plt = oclPlatform(new oclPlatform_(pID));
            plt->Init();
            plats.push_back(plt);
        }
        return plats;
    }();
    return Plats;
}


using namespace std::literals;

u16string_view oclUtil::GetErrorString(const cl_int err)
{
    switch (err)
    {
    // run-time and JIT compiler errors
    case 0: return u"CL_SUCCESS"sv;
    case -1: return u"CL_DEVICE_NOT_FOUND"sv;
    case -2: return u"CL_DEVICE_NOT_AVAILABLE"sv;
    case -3: return u"CL_COMPILER_NOT_AVAILABLE"sv;
    case -4: return u"CL_MEM_OBJECT_ALLOCATION_FAILURE"sv;
    case -5: return u"CL_OUT_OF_RESOURCES"sv;
    case -6: return u"CL_OUT_OF_HOST_MEMORY"sv;
    case -7: return u"CL_PROFILING_INFO_NOT_AVAILABLE"sv;
    case -8: return u"CL_MEM_COPY_OVERLAP"sv;
    case -9: return u"CL_IMAGE_FORMAT_MISMATCH"sv;
    case -10: return u"CL_IMAGE_FORMAT_NOT_SUPPORTED"sv;
    case -11: return u"CL_BUILD_PROGRAM_FAILURE"sv;
    case -12: return u"CL_MAP_FAILURE"sv;
    case -13: return u"CL_MISALIGNED_SUB_BUFFER_OFFSET"sv;
    case -14: return u"CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"sv;
    case -15: return u"CL_COMPILE_PROGRAM_FAILURE"sv;
    case -16: return u"CL_LINKER_NOT_AVAILABLE"sv;
    case -17: return u"CL_LINK_PROGRAM_FAILURE"sv;
    case -18: return u"CL_DEVICE_PARTITION_FAILED"sv;
    case -19: return u"CL_KERNEL_ARG_INFO_NOT_AVAILABLE"sv;
    // compile-time errors
    case -30: return u"CL_INVALID_VALUE"sv;
    case -31: return u"CL_INVALID_DEVICE_TYPE"sv;
    case -32: return u"CL_INVALID_PLATFORM"sv;
    case -33: return u"CL_INVALID_DEVICE"sv;
    case -34: return u"CL_INVALID_CONTEXT"sv;
    case -35: return u"CL_INVALID_QUEUE_PROPERTIES"sv;
    case -36: return u"CL_INVALID_COMMAND_QUEUE"sv;
    case -37: return u"CL_INVALID_HOST_PTR"sv;
    case -38: return u"CL_INVALID_MEM_OBJECT"sv;
    case -39: return u"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"sv;
    case -40: return u"CL_INVALID_IMAGE_SIZE"sv;
    case -41: return u"CL_INVALID_SAMPLER"sv;
    case -42: return u"CL_INVALID_BINARY"sv;
    case -43: return u"CL_INVALID_BUILD_OPTIONS"sv;
    case -44: return u"CL_INVALID_PROGRAM"sv;
    case -45: return u"CL_INVALID_PROGRAM_EXECUTABLE"sv;
    case -46: return u"CL_INVALID_KERNEL_NAME"sv;
    case -47: return u"CL_INVALID_KERNEL_DEFINITION"sv;
    case -48: return u"CL_INVALID_KERNEu"sv;
    case -49: return u"CL_INVALID_ARG_INDEX"sv;
    case -50: return u"CL_INVALID_ARG_VALUE"sv;
    case -51: return u"CL_INVALID_ARG_SIZE"sv;
    case -52: return u"CL_INVALID_KERNEL_ARGS"sv;
    case -53: return u"CL_INVALID_WORK_DIMENSION"sv;
    case -54: return u"CL_INVALID_WORK_GROUP_SIZE"sv;
    case -55: return u"CL_INVALID_WORK_ITEM_SIZE"sv;
    case -56: return u"CL_INVALID_GLOBAL_OFFSET"sv;
    case -57: return u"CL_INVALID_EVENT_WAIT_LIST"sv;
    case -58: return u"CL_INVALID_EVENT"sv;
    case -59: return u"CL_INVALID_OPERATION"sv;
    case -60: return u"CL_INVALID_GL_OBJECT"sv;
    case -61: return u"CL_INVALID_BUFFER_SIZE"sv;
    case -62: return u"CL_INVALID_MIP_LEVEu"sv;
    case -63: return u"CL_INVALID_GLOBAL_WORK_SIZE"sv;
    case -64: return u"CL_INVALID_PROPERTY"sv;
    case -65: return u"CL_INVALID_IMAGE_DESCRIPTOR"sv;
    case -66: return u"CL_INVALID_COMPILER_OPTIONS"sv;
    case -67: return u"CL_INVALID_LINKER_OPTIONS"sv;
    case -68: return u"CL_INVALID_DEVICE_PARTITION_COUNT"sv;
    // extension errors
    case -1000: return u"CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR"sv;
    case -1001: return u"CL_PLATFORM_NOT_FOUND_KHR"sv;
    case -1002: return u"CL_INVALID_D3D10_DEVICE_KHR"sv;
    case -1003: return u"CL_INVALID_D3D10_RESOURCE_KHR"sv;
    case -1004: return u"CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR"sv;
    case -1005: return u"CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR"sv;
    default: return u"Unknown OpenCL error"sv;
    }
}


}