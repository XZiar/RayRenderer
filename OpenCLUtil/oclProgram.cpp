#include "oclRely.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu
{


namespace detail
{


_oclProgram::_oclProgram(const oclContext& ctx_, const string& str) : ctx(ctx_), src(str), progID(loadProgram())
{
}

_oclProgram::~_oclProgram()
{
    clReleaseProgram(progID);
}

vector<oclDevice> _oclProgram::getDevs() const
{
    cl_int ret;
    cl_uint devCount = 0;
    ret = clGetProgramInfo(progID, CL_PROGRAM_NUM_DEVICES, sizeof(devCount), &devCount, nullptr);
    vector<cl_device_id> devids(devCount);
    ret = clGetProgramInfo(progID, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*devCount, devids.data(), nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"ANY ERROR in get devices from program", ret));
    vector<oclDevice> devs;
    while (devCount--)
        devs.push_back(oclDevice(new _oclDevice(devids[devCount])));
    return devs;
}

wstring _oclProgram::getBuildLog(const oclDevice & dev) const
{
    cl_build_status status;
    clGetProgramBuildInfo(progID, dev->deviceID, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
    switch (status)
    {
    case CL_BUILD_NONE:
        return L"Not been built yet";
    case CL_BUILD_SUCCESS:
        return L"Build successfully";
    case CL_BUILD_ERROR:
        {
            size_t logsize;
            clGetProgramBuildInfo(progID, dev->deviceID, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logsize);
            string logstr(std::max((size_t)1024, logsize), '\0');
            clGetProgramBuildInfo(progID, dev->deviceID, CL_PROGRAM_BUILD_LOG, logstr.size(), logstr.data(), nullptr);
            return str::to_wstring(logstr);
        }
    default:
        return L"";
    }
}

cl_program _oclProgram::loadProgram() const
{
    cl_int errcode;
    auto *str = src.c_str();
    size_t size = src.length();
    const auto prog = clCreateProgramWithSource(ctx->context, 1, &str, &size, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create program", errcode));
    return prog;
}

void _oclProgram::initKers()
{
    cl_int ret;
    char buf[8192];
    size_t len = 0;
    ret = clGetProgramInfo(progID, CL_PROGRAM_KERNEL_NAMES, sizeof(buf) - 2, &buf, &len);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot find kernels", ret));
    buf[len] = '\0';
    auto names = str::split(string_view(buf), ';', false);
    kers.clear();
    kers.assign(names.begin(), names.end());
}

void _oclProgram::build(const string& options, const oclDevice dev)
{
    cl_int ret;
    if (dev)
        ret = clBuildProgram(progID, 1, &dev->deviceID, options.c_str(), nullptr, nullptr);
    else
        ret = clBuildProgram(progID, 0, nullptr, options.c_str(), nullptr, nullptr);
    wstring buildlog;
    for (auto& dev : getDevs())
        buildlog += dev->name + L":\n" + getBuildLog(dev) + L"\n";
    if (ret == CL_SUCCESS)
    {
        oclLog().success(L"build program {:p} success:\n{}\n", (void*)progID, buildlog);
    }
    else
    {
        oclLog().error(L"build program {:p} failed:\n{}\n", (void*)progID, buildlog);
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"Build Program failed", ret), buildlog);
    }

    initKers();
}



cl_kernel _oclKernel::createKernel() const
{
    cl_int errorcode;
    auto kid = clCreateKernel(prog->progID, name.c_str(), &errorcode);
    if(errorcode!=CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"canot create kernel from program", errorcode));
    return kid;
}

_oclKernel::_oclKernel(const Wrapper<_oclProgram>& prog_, const string& name_) : name(name_), prog(prog_), kernel(createKernel())
{
}

_oclKernel::~_oclKernel()
{
    clReleaseKernel(kernel);
}

WorkGroupInfo _oclKernel::GetWorkGroupInfo(const oclDevice& dev)
{
    const cl_device_id devid = dev->deviceID;
    WorkGroupInfo info;
    size_t size;
    clGetKernelWorkGroupInfo(kernel, devid, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(uint64_t), &info.LocalMemorySize, &size);
    clGetKernelWorkGroupInfo(kernel, devid, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(uint64_t), &info.PrivateMemorySize, &size);
    clGetKernelWorkGroupInfo(kernel, devid, CL_KERNEL_WORK_GROUP_SIZE, sizeof(uint64_t), &info.WorkGroupSize, &size);
    clGetKernelWorkGroupInfo(kernel, devid, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(uint64_t) * 3, &info.CompiledWorkGroupSize, &size);
    clGetKernelWorkGroupInfo(kernel, devid, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(uint64_t), &info.PreferredWorkGroupSizeMultiple, &size);
    return info;
}


void _oclKernel::setArg(const uint32_t idx, const oclBuffer& buf)
{
    auto ret = clSetKernelArg(kernel, idx, sizeof(cl_mem), &buf->memID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"set kernel argument error", ret));
}

void _oclKernel::setArg(const uint32_t idx, const void *dat, const size_t size)
{
    auto ret = clSetKernelArg(kernel, idx, size, dat);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"set kernel argument error", ret));
}



oglu::optional<oclu::oclPromise> _oclKernel::run(const uint32_t workdim, const oclCmdQue que, const size_t *worksize, bool isBlock, const size_t *workoffset, const size_t *localsize)
{
    cl_int ret;
    cl_event e;
    ret = clEnqueueNDRangeKernel(que->cmdque, kernel, workdim, workoffset, worksize, localsize, 0, NULL, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"excute kernel error", ret));
    if (isBlock)
    {
        clWaitForEvents(1, &e);
        return {};
    }
    else
        return oclPromise(e);
}


}


}
