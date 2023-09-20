#include "oclPch.h"
#include "oclUtil.h"
#include "oclPromise.h"
#include "oclPlatform.h"
//#include "3rdParty/CL/cl_dx9_media_sharing.h"
//#include "3rdParty/CL/cl_d3d10.h"
//#include "3rdParty/CL/cl_d3d11.h"
#include "3rdParty/CL/cl_egl.h"
//#include "3rdParty/CL/cl_va_api_media_sharing_intel.h"


namespace oclu
{
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::vector;


common::mlog::MiniLogger<false>& oclUtil::GetOCLLog()
{
    return oclLog();
}

bool LogCLError(cl_int err, std::u16string_view msg)
{
    if (err != CL_SUCCESS)
    {
        oclLog().Error(u"{}: [{}]({})\n", msg, oclUtil::GetErrorString(err), err);
        return false;
    }
    return true;
}

void oclUtil::LogCLInfo()
{
    common::str::Formatter<char16_t> fmter;
    for (const auto& plat : oclPlatform_::GetPlatforms())
    {
        auto strBuf = fmter.FormatStatic(FmtString(u"\nPlatform {} --- {}\n"), plat->Name, plat->Ver);
        for (const auto dev : plat->GetDevices())
            fmter.FormatToStatic(strBuf, FmtString(u"--Device {}: {} -- {} -- {}\n"), dev->GetTypeName(),
                dev->Name, dev->Vendor, dev->Ver);
        oclLog().Verbose(strBuf);
    }
}

OCLException::OCLException(const CLComponent source, int32_t errcode, std::u16string msg)
    : BaseException(T_<ExceptionInfo>{}, msg.append(u" --ERROR: ").append(oclUtil::GetErrorString(errcode)), source)
{ }


using namespace std::literals;

u16string_view oclUtil::GetErrorString(const cl_int err)
{
    switch (err)
    {
#define RET_ERR(err) case err: return u ## #err ## sv
    // run-time and JIT compiler errors
    RET_ERR(CL_SUCCESS);
    RET_ERR(CL_DEVICE_NOT_FOUND);
    RET_ERR(CL_DEVICE_NOT_AVAILABLE);
    RET_ERR(CL_COMPILER_NOT_AVAILABLE);
    RET_ERR(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    RET_ERR(CL_OUT_OF_RESOURCES);
    RET_ERR(CL_OUT_OF_HOST_MEMORY);
    RET_ERR(CL_PROFILING_INFO_NOT_AVAILABLE);
    RET_ERR(CL_MEM_COPY_OVERLAP);
    RET_ERR(CL_IMAGE_FORMAT_MISMATCH);
    RET_ERR(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    RET_ERR(CL_BUILD_PROGRAM_FAILURE);
    RET_ERR(CL_MAP_FAILURE);
    RET_ERR(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    RET_ERR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    RET_ERR(CL_COMPILE_PROGRAM_FAILURE);
    RET_ERR(CL_LINKER_NOT_AVAILABLE);
    RET_ERR(CL_LINK_PROGRAM_FAILURE);
    RET_ERR(CL_DEVICE_PARTITION_FAILED);
    RET_ERR(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
    // compile-time errors
    RET_ERR(CL_INVALID_VALUE);
    RET_ERR(CL_INVALID_DEVICE_TYPE);
    RET_ERR(CL_INVALID_PLATFORM);
    RET_ERR(CL_INVALID_DEVICE);
    RET_ERR(CL_INVALID_CONTEXT);
    RET_ERR(CL_INVALID_QUEUE_PROPERTIES);
    RET_ERR(CL_INVALID_COMMAND_QUEUE);
    RET_ERR(CL_INVALID_HOST_PTR);
    RET_ERR(CL_INVALID_MEM_OBJECT);
    RET_ERR(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    RET_ERR(CL_INVALID_IMAGE_SIZE);
    RET_ERR(CL_INVALID_SAMPLER);
    RET_ERR(CL_INVALID_BINARY);
    RET_ERR(CL_INVALID_BUILD_OPTIONS);
    RET_ERR(CL_INVALID_PROGRAM);
    RET_ERR(CL_INVALID_PROGRAM_EXECUTABLE);
    RET_ERR(CL_INVALID_KERNEL_NAME);
    RET_ERR(CL_INVALID_KERNEL_DEFINITION);
    RET_ERR(CL_INVALID_KERNEL);
    RET_ERR(CL_INVALID_ARG_INDEX);
    RET_ERR(CL_INVALID_ARG_VALUE);
    RET_ERR(CL_INVALID_ARG_SIZE);
    RET_ERR(CL_INVALID_KERNEL_ARGS);
    RET_ERR(CL_INVALID_WORK_DIMENSION);
    RET_ERR(CL_INVALID_WORK_GROUP_SIZE);
    RET_ERR(CL_INVALID_WORK_ITEM_SIZE);
    RET_ERR(CL_INVALID_GLOBAL_OFFSET);
    RET_ERR(CL_INVALID_EVENT_WAIT_LIST);
    RET_ERR(CL_INVALID_EVENT);
    RET_ERR(CL_INVALID_OPERATION);
    RET_ERR(CL_INVALID_GL_OBJECT);
    RET_ERR(CL_INVALID_BUFFER_SIZE);
    RET_ERR(CL_INVALID_MIP_LEVEL);
    RET_ERR(CL_INVALID_GLOBAL_WORK_SIZE);
    RET_ERR(CL_INVALID_PROPERTY);
    RET_ERR(CL_INVALID_IMAGE_DESCRIPTOR);
    RET_ERR(CL_INVALID_COMPILER_OPTIONS);
    RET_ERR(CL_INVALID_LINKER_OPTIONS);
    RET_ERR(CL_INVALID_DEVICE_PARTITION_COUNT);
    RET_ERR(CL_INVALID_PIPE_SIZE);
    RET_ERR(CL_INVALID_DEVICE_QUEUE);
    RET_ERR(CL_INVALID_SPEC_ID);
    RET_ERR(CL_MAX_SIZE_RESTRICTION_EXCEEDED);
    // extension errors
    RET_ERR(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
    RET_ERR(CL_PLATFORM_NOT_FOUND_KHR);
    case -1002: return u"CL_INVALID_D3D10_DEVICE_KHR"sv;
    case -1003: return u"CL_INVALID_D3D10_RESOURCE_KHR"sv;
    case -1004: return u"CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR"sv;
    case -1005: return u"CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR"sv;
    case -1006: return u"CL_INVALID_D3D11_DEVICE_KHR"sv;
    case -1007: return u"CL_INVALID_D3D11_RESOURCE_KHR"sv;
    case -1008: return u"CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR"sv;
    case -1009: return u"CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR"sv;
    case -1010: return u"CL_INVALID_DX9_MEDIA_ADAPTER_KHR"sv;
    case -1011: return u"CL_INVALID_DX9_MEDIA_SURFACE_KHR"sv;
    case -1012: return u"CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR"sv;
    case -1013: return u"CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR"sv;
    RET_ERR(CL_DEVICE_PARTITION_FAILED_EXT);
    RET_ERR(CL_INVALID_PARTITION_COUNT_EXT);
    RET_ERR(CL_INVALID_PARTITION_NAME_EXT);
    RET_ERR(CL_EGL_RESOURCE_NOT_ACQUIRED_KHR);
    RET_ERR(CL_INVALID_EGL_OBJECT_KHR);
    RET_ERR(CL_INVALID_ACCELERATOR_INTEL);
    RET_ERR(CL_INVALID_ACCELERATOR_TYPE_INTEL);
    RET_ERR(CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL);
    RET_ERR(CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL);
    case -1098: return u"CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL"sv;
    case -1099: return u"CL_INVALID_VA_API_MEDIA_SURFACE_INTEL"sv;
    case -1100: return u"CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL"sv;
    case -1101: return u"CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL"sv;
    RET_ERR(CL_CONTEXT_TERMINATED_KHR);
    // vendor errors
    case -9999: return u"Illegal read or write to a buffer"sv;
    default:    return u"Unknown OpenCL error"sv;
#undef RET_ERR
    }
}


void oclUtil::WaitMany(common::PromiseStub promises)
{
    DependEvents evts(promises);
    evts.FlushAllQueues();
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    clWaitForEvents(evtCnt, reinterpret_cast<const cl_event*>(evtPtr));
}

constexpr auto klp = sizeof(detail::PlatFuncs);

}