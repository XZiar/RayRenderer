#include "oclPch.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"

namespace oclu
{
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::vector;
using common::linq::Linq;
using common::PromiseResult;
using namespace std::literals::string_view_literals;


MAKE_ENABLER_IMPL(oclProgram_)
//MAKE_ENABLER_IMPL(oclKernel_)


string_view KernelArgInfo::GetSpace() const
{
    switch (Space)
    {
    case KerArgSpace::Global:   return "Global"sv;
    case KerArgSpace::Constant: return "Constant"sv;
    case KerArgSpace::Local:    return "Local"sv;
    default:                    return "Private"sv;
    }
}
string_view KernelArgInfo::GetImgAccess() const
{
    switch (Access)
    {
    case ImgAccess::ReadOnly:   return "ReadOnly"sv;
    case ImgAccess::WriteOnly:  return "WriteOnly"sv;
    case ImgAccess::ReadWrite:  return "ReadWrite"sv;
    default:                    return "NotImage"sv;
    }
}
string KernelArgInfo::GetQualifier() const
{
    string ret;
    if (HAS_FIELD(Qualifier, KerArgFlag::Const))
        ret.append("Const "sv);
    if (HAS_FIELD(Qualifier, KerArgFlag::Volatile))
        ret.append("Volatile "sv);
    if (HAS_FIELD(Qualifier, KerArgFlag::Restrict))
        ret.append("Restrict "sv);
    if (HAS_FIELD(Qualifier, KerArgFlag::Pipe))
        ret.append("Pipe "sv);
    if (!ret.empty())
        ret.pop_back();
    return ret;
}

static uint32_t GetKernelInfoInt(cl_kernel kernel, cl_kernel_info param)
{
    uint32_t ret = 0;
    size_t dummy;
    clGetKernelInfo(kernel, param, sizeof(uint32_t), &ret, &dummy);
    return ret;
}
static vector<KernelArgInfo> GerKernelArgsInfo(cl_kernel kernel)
{
    vector<KernelArgInfo> infos;
    const uint32_t count = GetKernelInfoInt(kernel, CL_KERNEL_NUM_ARGS);
    for (uint32_t i = 0; i < count; ++i)
    {
        size_t size = 0;
        KernelArgInfo info;

        cl_kernel_arg_address_qualifier space;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(space), &space, &size);
        switch (space)
        {
        case CL_KERNEL_ARG_ADDRESS_GLOBAL:      info.Space = KerArgSpace::Global; break;
        case CL_KERNEL_ARG_ADDRESS_CONSTANT:    info.Space = KerArgSpace::Constant; break;
        case CL_KERNEL_ARG_ADDRESS_LOCAL:       info.Space = KerArgSpace::Local; break;
        default:                                info.Space = KerArgSpace::Private; break;
        }

        cl_kernel_arg_access_qualifier access;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_ACCESS_QUALIFIER, sizeof(access), &access, &size);
        switch (access)
        {
        case CL_KERNEL_ARG_ACCESS_READ_ONLY:    info.Access = ImgAccess::ReadOnly; break;
        case CL_KERNEL_ARG_ACCESS_WRITE_ONLY:   info.Access = ImgAccess::WriteOnly; break;
        case CL_KERNEL_ARG_ACCESS_READ_WRITE:   info.Access = ImgAccess::ReadWrite; break;
        default:                                info.Access = ImgAccess::None; break;
        }

        cl_kernel_arg_type_qualifier qualifier;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_QUALIFIER, sizeof(qualifier), &qualifier, &size);
        info.Qualifier = static_cast<KerArgFlag>(qualifier);

        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, 0, nullptr, &size);
        info.Name.resize(size, '\0');
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, size, info.Name.data(), &size);
        if (size > 0) info.Name.pop_back();

        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_NAME, 0, nullptr, &size);
        info.Type.resize(size, '\0');
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_NAME, size, info.Type.data(), &size);
        if (size > 0) info.Type.pop_back();

        infos.push_back(info);
    }
    return infos;
}
oclKernel_::oclKernel_(const oclPlatform_* plat, const oclProgram_* prog, string name) :
    Plat(*plat), Prog(*prog), KernelID(nullptr), Name(std::move(name))
{
    cl_int errcode;
    KernelID = clCreateKernel(Prog.ProgID, Name.data(), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"canot create kernel from program");
    ArgsInfo = GerKernelArgsInfo(KernelID);
}

oclKernel_::~oclKernel_()
{
    clReleaseKernel(KernelID);
}

WorkGroupInfo oclKernel_::GetWorkGroupInfo(const oclDevice& dev) const
{
    const cl_device_id devid = dev->DeviceID;
    WorkGroupInfo info;
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(uint64_t), &info.LocalMemorySize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(uint64_t), &info.PrivateMemorySize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &info.WorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(size_t) * 3, &info.CompiledWorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &info.PreferredWorkGroupSizeMultiple, nullptr);
    if (dev->Extensions.Has("cl_intel_required_subgroup_size"))
        clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_SPILL_MEM_SIZE_INTEL, sizeof(uint64_t), &info.SpillMemSize, nullptr);
    else
        info.SpillMemSize = 0;
    return info;
}

std::optional<SubgroupInfo> oclKernel_::GetSubgroupInfo(const oclDevice& dev, const uint8_t dim, const size_t* localsize) const
{
    const cl_device_id devid = dev->DeviceID;
    if (!Prog.DeviceIDs.Has(devid))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"target device is not related to this kernel");
    if (!Plat.FuncClGetKernelSubGroupInfo)
        return {};
    if (!dev->Extensions.Has("cl_khr_subgroups") && !dev->Extensions.Has("cl_intel_subgroups"))
        return {};
    SubgroupInfo info;
    Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupSize, nullptr);
    Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupCount, nullptr);
    if (dev->Extensions.Has("cl_intel_required_subgroup_size"))
        Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL, 0, nullptr, sizeof(size_t), &info.CompiledSubgroupSize, nullptr);
    else
        info.CompiledSubgroupSize = 0;
    return info;
}

void oclKernel_::CheckArgIdx(const uint32_t idx) const
{
    if (idx >= ArgsInfo.size())
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"kernel argument index exceed limit");
}

void oclKernel_::SetArg(const uint32_t idx, const oclBuffer_& buf) const
{
    CheckArgIdx(idx);
    auto ret = clSetKernelArg(KernelID, idx, sizeof(cl_mem), &buf.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::SetArg(const uint32_t idx, const oclImage_& img) const
{
    CheckArgIdx(idx);
    auto ret = clSetKernelArg(KernelID, idx, sizeof(cl_mem), &img.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::SetArg(const uint32_t idx, const void* dat, const size_t size) const
{
    CheckArgIdx(idx);
    auto ret = clSetKernelArg(KernelID, idx, size, dat);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

PromiseResult<void> oclKernel_::Run(const PromiseResult<void>& pms, const uint32_t workdim, const oclCmdQue& que, const size_t* worksize, bool isBlock, const size_t* workoffset, const size_t* localsize) const
{
    cl_int ret;
    cl_event e;
    cl_uint ecount = 0;
    const cl_event* depend = nullptr;
    auto clpms = std::dynamic_pointer_cast<oclPromise<void>>(pms);
    if (clpms)
        depend = &clpms->GetEvent(), ecount = 1;
    ret = clEnqueueNDRangeKernel(que->CmdQue, KernelID, workdim, workoffset, worksize, localsize, ecount, depend, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"execute kernel error");
    if (isBlock)
    {
        clWaitForEvents(1, &e);
        return {};
    }
    else
        return std::make_shared<oclPromise<void>>(e, que, 0, clpms);
}


static vector<cl_device_id> GetProgDevs(cl_program progID)
{
    cl_int ret;
    cl_uint devCount = 0;
    ret = clGetProgramInfo(progID, CL_PROGRAM_NUM_DEVICES, sizeof(devCount), &devCount, nullptr);
    vector<cl_device_id> devids(devCount);
    ret = clGetProgramInfo(progID, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * devCount, devids.data(), nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"ANY ERROR in get devices from program");
    return devids;
}


oclProgram_::oclProgStub::oclProgStub(const oclContext& ctx, const string& str)
    : Context(ctx), Source(str), ProgID(nullptr)
{
    cl_int errcode;
    auto* ptr = Source.c_str();
    size_t size = Source.length();
    ProgID = clCreateProgramWithSource(Context->Context, 1, &ptr, &size, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create program");
}

oclProgram_::oclProgStub::~oclProgStub()
{
    if (ProgID)
    {
        oclLog().warning(u"oclProg released before finished.\n");
        clReleaseProgram(ProgID);
    }
}

void oclProgram_::oclProgStub::Build(const CLProgConfig& config, const std::vector<oclDevice>& devs)
{
    string options;
    switch (Context->GetVendor())
    {
    case Vendors::NVIDIA:    options = "-DOCLU_NVIDIA -cl-nv-verbose "; break;
    case Vendors::AMD:       options = "-DOCLU_AMD "; break;
    case Vendors::Intel:     options = "-DOCLU_INTEL "; break;
    default:                break;
    }
    for (const auto& def : config.Defines)
    {
        options.append("-D"sv).append(def.first);
        std::visit([&](auto&& val)
            {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, string>)
                    options.append("="sv).append(val);
                else if constexpr (!std::is_same_v<T, std::monostate>)
                    options.append("="sv).append(std::to_string(val));
            }, def.second);
        options.append(" "sv);
    }
    for (const auto& flag : config.Flags)
        options.append(flag).append(" "sv);

    {
        uint64_t lSize = UINT64_MAX;
        for (const auto& dev : devs)
            lSize = std::min(lSize, dev->LocalMemSize);
        lSize = lSize == UINT64_MAX ? 0 : lSize; //default as zero
        options.append("-DOCLU_LOCAL_MEM_SIZE=").append(std::to_string(lSize));
    }
    
    auto devids = Linq::FromIterable(devs)
        .Select([](const auto& dev) { return dev->DeviceID; })
        .ToVector();
    cl_int ret = clBuildProgram(ProgID, static_cast<cl_uint>(devids.size()), 
        devids.empty() ? nullptr : devids.data(), options.c_str(), nullptr, nullptr);

    std::vector<oclDevice> devs2;
    if (devs.empty())
    {
        common::container::FrozenDenseSet<cl_device_id> devidset = GetProgDevs(ProgID);
        devs2 = Linq::FromIterable(Context->Devices)
            .Where([&](const auto& dev) { return devidset.Has(dev->DeviceID); })
            .ToVector();
    }
    u16string buildlog;
    for (auto& thedev : devs.empty() ? devs2 : devs)
        buildlog += thedev->Name + u":\n" + GetBuildLog(thedev) + u"\n";
    if (ret == CL_SUCCESS)
    {
        oclLog().success(u"build program {:p} success:\n{}\n", (void*)ProgID, buildlog);
    }
    else
    {
        oclLog().error(u"build program {:p} failed:\n{}\n", (void*)ProgID, buildlog);
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"Build Program failed", buildlog);
    }

}

oclProgram oclProgram_::oclProgStub::Finish()
{
    return MAKE_ENABLER_SHARED_CONST(oclProgram_, this);
}


u16string oclProgram_::GetProgBuildLog(cl_program progID, const cl_device_id dev)
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
        if (logsize > 0)
        {
            string logstr(logsize, '\0');
            clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_LOG, logstr.size(), logstr.data(), &logsize);
            logstr.pop_back();
            return common::strchset::to_u16string(logstr, common::strchset::Charset::UTF8);
        }
        return u"";
    }
    default:
        return u"";
    }
}

u16string oclProgram_::GetProgBuildLog(cl_program progID, const std::vector<oclDevice>& devs)
{
    u16string buildlog;
    for (auto& thedev : devs)
        buildlog += thedev->Name + u":\n" + GetProgBuildLog(progID, thedev->DeviceID) + u"\n";
    return buildlog;
}

u16string oclProgram_::GetProgBuildLog(cl_program progID, const oclContext_& ctx,
    const common::container::FrozenDenseSet<cl_device_id>& dids)
{
    const auto devs = Linq::FromIterable(ctx.Devices)
        .Where([&](const auto& dev) { return dids.Has(dev->DeviceID); })
        .ToVector();
    return GetProgBuildLog(progID, devs);
}

oclProgram_::oclProgram_(oclProgStub* stub)
    : Context(std::move(stub->Context)), Source(std::move(stub->Source)), ProgID(stub->ProgID)
{
    stub->ProgID = nullptr;

    cl_int ret;
    size_t len = 0;
    ret = clGetProgramInfo(ProgID, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &len);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot find kernels");
    vector<char> buf(len, '\0');
    clGetProgramInfo(ProgID, CL_PROGRAM_KERNEL_NAMES, len, buf.data(), &len);
    if (len > 0)
        buf.pop_back(); //null-terminated
    const auto names = common::str::Split<char>(buf, ';', false);
    KernelNames.assign(names.cbegin(), names.cend());

    Kernels = Linq::FromIterable(KernelNames)
        .Select([&](const auto& name) 
            { return std::unique_ptr<oclKernel_>(new oclKernel_(Context->Plat.get(), this, name)); })
        .ToVector();
    /*for (const auto& name : KernelNames)
        Kernels.emplace_back(Context->Plat.get(), this, name);*/

    DeviceIDs = GetProgDevs(ProgID);
}

oclProgram_::~oclProgram_()
{
    clReleaseProgram(ProgID);
}

oclKernel oclProgram_::GetKernel(const string& name) const
{
    for (const auto& ker : Kernels)
        if (ker->Name == name)
            return oclKernel(shared_from_this(), ker.get());
    return {};
}

oclProgram_::oclProgStub oclProgram_::Create(const oclContext& ctx, const string& str)
{
    return oclProgStub(ctx, str);
}

oclProgram oclProgram_::CreateAndBuild(const oclContext& ctx, const string& str, const CLProgConfig& config, const std::vector<oclDevice>& devs)
{
    auto stub = Create(ctx, str);
    stub.Build(config, devs);
    return stub.Finish();
}


}
