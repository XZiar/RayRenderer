#pragma once
#include "oclRely.h"


#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#include "3rdParty/CL/opencl.h"
#include "3rdParty/CL/cl_ext.h"

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>



namespace oclu
{

namespace detail
{
#define DefType(type, cltype) struct type { using RealType = cltype; }
DefType(CLPlatform, cl_platform_id);
DefType(CLContext, cl_context);
DefType(CLProgram, cl_program);
DefType(CLDevice, cl_device_id);
DefType(CLKernel, cl_kernel);
DefType(CLCmdQue, cl_command_queue);
DefType(CLEvent, cl_event);
DefType(CLMem, cl_mem);
#undef DefType

#define PLATFUNCS BOOST_PP_VARIADIC_TO_SEQ(\
    clGetPlatformIDs, clGetPlatformInfo, \
    clGetDeviceIDs, clGetDeviceInfo, \
    clGetSupportedImageFormats, \
    clCreateContext, clGetContextInfo, clReleaseContext, \
    clCreateUserEvent, clSetUserEventStatus, clRetainEvent, clReleaseEvent, \
    clGetEventInfo, clGetEventProfilingInfo, clWaitForEvents, clSetEventCallback, \
    clCreateCommandQueue, clReleaseCommandQueue, clEnqueueBarrier, clFlush, clFinish, \
    clCreateBuffer, clCreateSubBuffer, clEnqueueMapBuffer, clEnqueueReadBuffer, clEnqueueWriteBuffer, \
    clCreateImage, clCreateImage2D, clCreateImage3D, clEnqueueMapImage, clEnqueueReadImage, clEnqueueWriteImage, \
    clGetMemObjectInfo, clReleaseMemObject, clEnqueueUnmapMemObject, \
    clCreateProgramWithSource, clReleaseProgram, clBuildProgram, clGetProgramBuildInfo, clGetProgramInfo, \
    clCreateKernelsInProgram, clGetKernelInfo, clGetKernelArgInfo, clGetKernelWorkGroupInfo, clSetKernelArg, \
    clEnqueueNDRangeKernel, \
    clEnqueueAcquireGLObjects, clEnqueueReleaseGLObjects, clCreateFromGLBuffer, clCreateFromGLTexture)
#define PLATFUNCS_EACH_(r, func, f) func(f)
#define PLATFUNCS_EACH(func) BOOST_PP_SEQ_FOR_EACH(PLATFUNCS_EACH_, func, PLATFUNCS)

struct PlatFuncs
{
private:
    void* Library = nullptr;
    void* TryLoadFunc(const char* fname) const noexcept;
    void InitExtensionFunc() noexcept;
public:
#define DEF_FUNC_PTR(f) decltype(&::f) f = nullptr;
    DEF_FUNC_PTR(clGetExtensionFunctionAddress)
    DEF_FUNC_PTR(clGetExtensionFunctionAddressForPlatform)
    DEF_FUNC_PTR(clGetGLContextInfoKHR)
    DEF_FUNC_PTR(clGetKernelSubGroupInfoKHR)
    PLATFUNCS_EACH(DEF_FUNC_PTR)
#undef DEF_FUNC_PTR
    PlatFuncs();
    PlatFuncs(std::string_view dllName);
    ~PlatFuncs();
};

}

OCLUAPI bool LogCLError(cl_int err, std::u16string_view msg);

}
