#include "oclRely.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu
{


namespace detail
{
using common::container::FindInMap;
using common::container::ContainInVec;

static cl_program LoadProgram(const string& src, const cl_context& ctx)
{
    cl_int errcode;
    auto *str = src.c_str();
    size_t size = src.length();
    const auto prog = clCreateProgramWithSource(ctx, 1, &str, &size, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create program", errcode));
    return prog;
}

_oclProgram::_oclProgram(const oclContext& ctx, const string& str) : Context(ctx), src(str), progID(LoadProgram(src, Context->context))
{
}

_oclProgram::~_oclProgram()
{
    clReleaseProgram(progID);
}

vector<cl_device_id> _oclProgram::getDevs() const
{
    cl_int ret;
    cl_uint devCount = 0;
    ret = clGetProgramInfo(progID, CL_PROGRAM_NUM_DEVICES, sizeof(devCount), &devCount, nullptr);
    vector<cl_device_id> devids(devCount);
    ret = clGetProgramInfo(progID, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*devCount, devids.data(), nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"ANY ERROR in get devices from program", ret));
    return devids;
}

u16string _oclProgram::GetBuildLog(const cl_device_id dev) const
{
    cl_build_status status;
    clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
    switch (status)
    {
    case CL_BUILD_NONE:
        return u"Not been built yet";
    case CL_BUILD_SUCCESS:
        return u"Build successfully";
    case CL_BUILD_ERROR:
        {
            size_t logsize;
            clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logsize);
            string logstr(logsize, '\0');
            clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_LOG, logstr.size(), logstr.data(), &logsize);
            return str::to_u16string(logstr.c_str());
        }
    default:
        return u"";
    }
}


void _oclProgram::Build(const string& options, const oclDevice dev)
{
    cl_int ret;
    vector<oclDevice> devices;
    if (dev)
    {
        devices.push_back(dev);
        ret = clBuildProgram(progID, 1, &dev->deviceID, options.c_str(), nullptr, nullptr);
    }
    else
    {
        ret = clBuildProgram(progID, 0, nullptr, options.c_str(), nullptr, nullptr);
        const auto dids = getDevs();
        for (const auto& d : Context->Devices)
            if (ContainInVec(dids, d->deviceID))
                devices.push_back(d);
    }
    u16string buildlog;
    for (auto& dev : devices)
        buildlog += dev->Name + u":\n" + GetBuildLog(dev) + u"\n";
    if (ret == CL_SUCCESS)
    {
        oclLog().success(u"build program {:p} success:\n{}\n", (void*)progID, buildlog);
    }
    else
    {
        oclLog().error(u"build program {:p} failed:\n{}\n", (void*)progID, buildlog);
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"Build Program failed", ret), buildlog);
    }

    char buf[8192];
    size_t len = 0;
    ret = clGetProgramInfo(progID, CL_PROGRAM_KERNEL_NAMES, sizeof(buf) - 2, &buf, &len);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot find kernels", ret));
    buf[len] = '\0';
    const auto names = str::Split<char>(string_view(buf), ';', false);
    KernelNames.assign(names.cbegin(), names.cend());

    Kernels.clear();
    const auto self = shared_from_this();
    for (const auto& kername : KernelNames)
    {
        Kernels.insert_or_assign(kername, oclKernel(new _oclKernel(self, kername)));
    }
}

oclKernel _oclProgram::GetKernel(const string& name)
{
    if (const auto it = FindInMap(Kernels, name); it)
        return *it;
    return {};
}



static cl_kernel CreateKernel(const cl_program prog, const std::string& name)
{
    cl_int errorcode;
    auto kid = clCreateKernel(prog, name.data(), &errorcode);
    if (errorcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"canot create kernel from program", errorcode));
    return kid;
}

_oclKernel::_oclKernel(const oclProgram& prog, const string& name) : Prog(prog), Kernel(CreateKernel(Prog->progID, name)), Name(name)
{
}

_oclKernel::~_oclKernel()
{
    clReleaseKernel(Kernel);
}

WorkGroupInfo _oclKernel::GetWorkGroupInfo(const oclDevice& dev)
{
    const cl_device_id devid = dev->deviceID;
    WorkGroupInfo info;
    clGetKernelWorkGroupInfo(Kernel, devid, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(uint64_t), &info.LocalMemorySize, nullptr);
    clGetKernelWorkGroupInfo(Kernel, devid, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(uint64_t), &info.PrivateMemorySize, nullptr);
    clGetKernelWorkGroupInfo(Kernel, devid, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &info.WorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(Kernel, devid, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(size_t) * 3, &info.CompiledWorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(Kernel, devid, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &info.PreferredWorkGroupSizeMultiple, nullptr);
    return info;
}


void _oclKernel::SetArg(const uint32_t idx, const oclBuffer& buf)
{
    auto ret = clSetKernelArg(Kernel, idx, sizeof(cl_mem), &buf->memID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"set kernel argument error", ret));
}

void _oclKernel::SetArg(const uint32_t idx, const oclImage& img)
{
    auto ret = clSetKernelArg(Kernel, idx, sizeof(cl_mem), &img->memID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"set kernel argument error", ret));
}

void _oclKernel::SetArg(const uint32_t idx, const void *dat, const size_t size)
{
    auto ret = clSetKernelArg(Kernel, idx, size, dat);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"set kernel argument error", ret));
}



oclu::oclPromise _oclKernel::Run(const uint32_t workdim, const oclCmdQue que, const size_t *worksize, bool isBlock, const size_t *workoffset, const size_t *localsize)
{
    cl_int ret;
    cl_event e;
    ret = clEnqueueNDRangeKernel(que->cmdque, Kernel, workdim, workoffset, worksize, localsize, 0, NULL, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"excute kernel error", ret));
    if (isBlock)
    {
        clWaitForEvents(1, &e);
        return {};
    }
    else
        return std::make_shared<oclPromise_>(oclPromise_(e, que->cmdque));
}


}


}
