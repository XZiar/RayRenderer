#include "oclPch.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.h"

namespace oclu
{
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::vector;
using common::PromiseResult;
using namespace std::literals::string_view_literals;


MAKE_ENABLER_IMPL(oclProgram_)
MAKE_ENABLER_IMPL(oclKernel_)


string_view ArgFlags::GetSpace() const noexcept
{
    switch (Space)
    {
    case KerArgSpace::Global:   return "Global"sv;
    case KerArgSpace::Constant: return "Constant"sv;
    case KerArgSpace::Local:    return "Local"sv;
    default:                    return "Private"sv;
    }
}
string_view ArgFlags::GetImgAccess() const noexcept
{
    switch (Access)
    {
    case ImgAccess::ReadOnly:   return "ReadOnly"sv;
    case ImgAccess::WriteOnly:  return "WriteOnly"sv;
    case ImgAccess::ReadWrite:  return "ReadWrite"sv;
    default:                    return "NotImage"sv;
    }
}
string_view ArgFlags::ToCLString(const KerArgSpace space) noexcept
{
    switch (space)
    {
    case KerArgSpace::Global:   return "global"sv;
    case KerArgSpace::Constant: return "constant"sv;
    case KerArgSpace::Local:    return "local"sv;
    default:                    return "private"sv;
    }
}
string_view ArgFlags::ToCLString(const ImgAccess access) noexcept
{
    switch (access)
    {
    case ImgAccess::ReadOnly:   return "read_only"sv;
    case ImgAccess::WriteOnly:  return "write_only"sv;
    default:                    return ""sv;
    }
}
string ArgFlags::GetQualifier() const noexcept
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

KernelArgStore::KernelArgStore(cl_kernel kernel, const KernelArgStore& reference) : DebugBuffer(0), HasInfo(true)
{
    uint32_t size = 0;
    {
        size_t dummy;
        clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(uint32_t), &size, &dummy);
        ArgsInfo.reserve(size);
    }
    std::string tmp;
    for (uint32_t i = 0; i < size; ++i)
    {
        ArgInfo info;

        size_t dummy = 0;
        cl_kernel_arg_address_qualifier space;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(space), &space, &dummy);
        switch (space)
        {
        case CL_KERNEL_ARG_ADDRESS_GLOBAL:      info.Space = KerArgSpace::Global;   break;
        case CL_KERNEL_ARG_ADDRESS_CONSTANT:    info.Space = KerArgSpace::Constant; break;
        case CL_KERNEL_ARG_ADDRESS_LOCAL:       info.Space = KerArgSpace::Local;    break;
        default:                                info.Space = KerArgSpace::Private;  break;
        }

        cl_kernel_arg_access_qualifier access;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_ACCESS_QUALIFIER, sizeof(access), &access, &dummy);
        switch (access)
        {
        case CL_KERNEL_ARG_ACCESS_READ_ONLY:    info.Access = ImgAccess::ReadOnly;  break;
        case CL_KERNEL_ARG_ACCESS_WRITE_ONLY:   info.Access = ImgAccess::WriteOnly; break;
        case CL_KERNEL_ARG_ACCESS_READ_WRITE:   info.Access = ImgAccess::ReadWrite; break;
        default:                                info.Access = ImgAccess::None;      break;
        }

        cl_kernel_arg_type_qualifier qualifier;
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_QUALIFIER, sizeof(qualifier), &qualifier, &dummy);
        info.Qualifier = static_cast<KerArgFlag>(qualifier);

        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, 0, nullptr, &dummy);
        tmp.resize(dummy, '\0');
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, dummy, tmp.data(), &dummy);
        if (dummy > 0) tmp.pop_back();
        info.Name = AllocateString(tmp);

        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_NAME, 0, nullptr, &dummy);
        tmp.resize(dummy, '\0');
        clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_NAME, dummy, tmp.data(), &dummy);
        if (dummy > 0) tmp.pop_back();
        info.Type = AllocateString(tmp);

        ArgsInfo.emplace_back(info);
    }
    // post-process
    if (ArgsInfo.size() >= 3)
    {
        const auto& arg0 = ArgsInfo[ArgsInfo.size() - 3];
        const auto& arg1 = ArgsInfo[ArgsInfo.size() - 2];
        const auto& arg2 = ArgsInfo[ArgsInfo.size() - 1];
        if (GetStringView(arg0.Name) == "_oclu_debug_buffer_size" && GetStringView(arg0.Type) == "uint"  &&
            GetStringView(arg1.Name) == "_oclu_debug_buffer_info" && GetStringView(arg1.Type) == "uint*" &&
            GetStringView(arg2.Name) == "_oclu_debug_buffer_data" && GetStringView(arg2.Type) == "uint*")
        {
            HasDebug = true;
            ArgsInfo.resize(ArgsInfo.size() - 3);
            DebugBuffer = reference.HasInfo ? reference.DebugBuffer : 512;
        }
    }
    // check with reference
    if (reference.HasInfo)
    {
        if (reference.GetSize() != GetSize())
            oclLog().debug(u"KerArgStore, clAPI reports [{}] arg while provided [{}].\n", GetSize(), reference.GetSize());
        else
        {
            for (size_t i = 0; i < GetSize(); ++i)
            {
                const auto& ref = reference.GetArgInfo(i);
                const auto& cur = GetArgInfo(i);
                if (ref.Name != cur.Name)
                    oclLog().debug(u"KerArgStore, external reports arg[{}] is [{}] while provided [{}].\n",
                        i, ref.Name, cur.Name);
                if (ref.Space != cur.Space)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n", 
                        i, ref.Name, ref.GetSpace(), cur.GetSpace());
                if (ref.Access != cur.Access)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n",
                        i, ref.Name, ref.GetImgAccess(), cur.GetImgAccess());
                if (ref.Qualifier != cur.Qualifier)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n",
                        i, ref.Name, ref.GetQualifier(), cur.GetQualifier());
            }
        }
    }
}

KernelArgInfo KernelArgStore::GetArgInfo(const size_t idx) const noexcept
{
    const auto info = GetArg(idx);
    return { info->Space, info->Access, info->Qualifier, GetStringView(info->Name), GetStringView(info->Type) };
}

const KernelArgStore::ArgInfo* KernelArgStore::GetArg(const size_t idx, const bool check) const
{
    if (HasInfo)
    {
        if (idx >= ArgsInfo.size())
        {
            if (check)
                COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"kernel argument index exceed limit");
        }
        else
            return &ArgsInfo[idx];
    }
    return nullptr;
}

void KernelArgStore::AddArg(const KerArgSpace space, const ImgAccess access, const KerArgFlag qualifier,
    const std::string_view name, const std::string_view type)
{
    ArgInfo info;
    info.Space     = space;
    info.Access    = access;
    info.Qualifier = qualifier;
    info.Name      = AllocateString(name);
    info.Type      = AllocateString(type);
    ArgsInfo.emplace_back(info);
    HasInfo = true;
}


oclKernel_::oclKernel_(const oclPlatform_* plat, const oclProgram_* prog, string name, KernelArgStore&& argStore) :
    Plat(*plat), Prog(*prog), KernelID(nullptr), Name(std::move(name))
{
    cl_int errcode;
    KernelID = clCreateKernel(Prog.ProgID, Name.data(), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"canot create kernel from program");

    if (Prog.Context->Version >= 12)
        ArgStore = KernelArgStore(KernelID, argStore);
    else if (argStore.HasInfo)
    {
        oclLog().verbose(u"use external arg-info for kernel [{}].\n", Name);
        ArgStore = std::move(argStore);
    }
    ReqDbgBufSize = ArgStore.DebugBuffer == 0 ? 512 : ArgStore.DebugBuffer;
}

oclKernel_::~oclKernel_()
{
    clReleaseKernel(KernelID);
}

WorkGroupInfo oclKernel_::GetWorkGroupInfo() const
{
    const cl_device_id devid = Prog.Device->DeviceID;
    WorkGroupInfo info;
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(uint64_t), &info.LocalMemorySize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(uint64_t), &info.PrivateMemorySize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &info.WorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(size_t) * 3, &info.CompiledWorkGroupSize, nullptr);
    clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &info.PreferredWorkGroupSizeMultiple, nullptr);
    if (Prog.Device->Extensions.Has("cl_intel_required_subgroup_size"sv))
        clGetKernelWorkGroupInfo(KernelID, devid, CL_KERNEL_SPILL_MEM_SIZE_INTEL, sizeof(uint64_t), &info.SpillMemSize, nullptr);
    else
        info.SpillMemSize = 0;
    return info;
}

std::optional<SubgroupInfo> oclKernel_::GetSubgroupInfo(const uint8_t dim, const size_t* localsize) const
{
    if (!Plat.FuncClGetKernelSubGroupInfo)
        return {};
    if (!Prog.Device->Extensions.Has("cl_khr_subgroups"sv) && !Prog.Device->Extensions.Has("cl_intel_subgroups"sv))
        return {};
    SubgroupInfo info;
    const auto devid = Prog.Device->DeviceID;
    Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupSize, nullptr);
    Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupCount, nullptr);
    if (Prog.Device->Extensions.Has("cl_intel_required_subgroup_size"sv))
        Plat.FuncClGetKernelSubGroupInfo(KernelID, devid, CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL, 0, nullptr, sizeof(size_t), &info.CompiledSubgroupSize, nullptr);
    else
        info.CompiledSubgroupSize = 0;
    return info;
}



oclKernel_::CallSiteInternal::CallSiteInternal(const oclKernel_* kernel) :
    Kernel(kernel->Prog.shared_from_this(), kernel), KernelLock(Kernel->ArgLock.LockScope())
{
}

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const oclBuffer_ & buf) const
{
    if (const auto info = Kernel->ArgStore.GetArg(idx); info && info->IsImage())
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"non-image is set to an image kernel argument slot");
    auto ret = clSetKernelArg(Kernel->KernelID, idx, sizeof(cl_mem), &buf.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const oclImage_ & img) const
{
    if (const auto info = Kernel->ArgStore.GetArg(idx); info && !info->IsImage())
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"image is set to an non-image kernel argument slot");
    auto ret = clSetKernelArg(Kernel->KernelID, idx, sizeof(cl_mem), &img.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const void* dat, const size_t size) const
{
    Kernel->ArgStore.GetArg(idx);
    auto ret = clSetKernelArg(Kernel->KernelID, idx, size, dat);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

PromiseResult<CallResult> oclKernel_::CallSiteInternal::Run(const uint8_t dim, const common::PromiseStub& pmss,
    const oclCmdQue& que, const size_t* worksize, const size_t* workoffset, const size_t* localsize)
{
    if (Kernel->Prog.Device != que->Device)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"queue's device is not the same as this kernel");

    CallResult result;
    if (Kernel->ArgStore.HasInfo && Kernel->ArgStore.HasDebug) // inject debug buffer
    {
        result.DebugManager = Kernel->Prog.DebugManager;
        result.Queue = que;
        const auto infosize = Kernel->Prog.DebugManager->GetInfoMan().GetInfoBufferSize(worksize, dim);
        const auto startIdx = static_cast<uint32_t>(Kernel->ArgStore.GetSize());
        std::vector<std::byte> tmp(infosize);
        const uint32_t dbgBufSize = Kernel->ReqDbgBufSize * 1024u;
        result.InfoBuf  = oclBuffer_::Create(que->Context, MemFlag::ReadWrite, tmp.size(), tmp.data());
        result.DebugBuf = oclBuffer_::Create(que->Context, MemFlag::HostReadOnly | MemFlag::WriteOnly, dbgBufSize);
        clSetKernelArg(Kernel->KernelID, startIdx + 0, sizeof(uint32_t), &dbgBufSize);
        clSetKernelArg(Kernel->KernelID, startIdx + 1, sizeof(cl_mem),   &result.InfoBuf->MemID);
        clSetKernelArg(Kernel->KernelID, startIdx + 2, sizeof(cl_mem),   &result.DebugBuf->MemID);
    }

    cl_int ret;
    cl_event e;
    auto [clpmss, evts] = oclPromiseCore::ParsePms(pmss);
    const auto [evtPtr, evtCnt] = evts.Get();
    ret = clEnqueueNDRangeKernel(que->CmdQue, Kernel->KernelID, dim, workoffset, worksize, localsize, evtCnt, evtPtr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"execute kernel error");
    return oclPromise<CallResult>::Create(std::move(clpmss), e, que, std::move(result));
}


oclProgram_::oclProgStub::oclProgStub(const oclContext& ctx, const oclDevice& dev, string&& str)
    : Context(ctx), Device(dev), Source(std::move(str)), ProgID(nullptr)
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

void oclProgram_::oclProgStub::Build(const CLProgConfig& config)
{
    const auto cver = config.Version == 0 ? Device->CVersion : config.Version;
    if (cver > Device->CVersion)
        oclLog().warning(u"request cversion [{}] on [{}] device [{}]\n", cver, Device->CVersion, Device->Name);
    string options;
    switch (Context->GetVendor())
    {
    case Vendors::NVIDIA:    options = "-DOCLU_NVIDIA -cl-nv-verbose "; break;
    case Vendors::AMD:       options = "-DOCLU_AMD "; break;
    case Vendors::Intel:     options = "-DOCLU_INTEL "; break;
    default:                break;
    }
    for (const auto def : config.Defines)
    {
        options.append("-D"sv).append(def.Key);
        std::visit([&](const auto val)
            {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::string_view>)
                    options.append("="sv).append(val);
                else if constexpr (!std::is_same_v<T, std::monostate>)
                    options.append("="sv).append(std::to_string(val));
            }, def.Val);
        options.append(" "sv);
    }
    {
        uint64_t lSize = Device->LocalMemSize;
        lSize = lSize == UINT64_MAX ? 0 : lSize; //default as zero
        options.append("-DOCLU_LOCAL_MEM_SIZE=").append(std::to_string(lSize));
    }
    for (const auto& flag : config.Flags)
        options.append(flag).append(" "sv);
    fmt::format_to(std::back_inserter(options), "-cl-std=CL{}.{} ", cver / 10, cver % 10);
    
    cl_int ret = clBuildProgram(ProgID, 1, &Device->DeviceID, options.c_str(), nullptr, nullptr);

    std::vector<oclDevice> devs2;
    u16string buildlog = Device->Name + u":\n" + GetBuildLog();
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
    return MAKE_ENABLER_SHARED(const oclProgram_, (this));
}


u16string oclProgram_::GetProgBuildLog(cl_program progID, const cl_device_id dev)
{
    u16string result;
    cl_build_status status;
    clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
    switch (status)
    {
    case CL_BUILD_NONE:     return u"Not been built yet";
    case CL_BUILD_SUCCESS:  result = u"Build successfully:\n"; break;
    case CL_BUILD_ERROR:    result = u"Build error:\n"; break;
    default:                return u"";
    }
    size_t logsize;
    clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logsize);
    if (logsize > 0)
    {
        string logstr(logsize, '\0');
        clGetProgramBuildInfo(progID, dev, CL_PROGRAM_BUILD_LOG, logstr.size(), logstr.data(), &logsize);
        logstr.pop_back();
        result.append(common::strchset::to_u16string(logstr, common::strchset::Charset::UTF8));
    }
    return result;
}

oclProgram_::oclProgram_(oclProgStub* stub) : 
    Context(std::move(stub->Context)), Device(std::move(stub->Device)), Source(std::move(stub->Source)), ProgID(stub->ProgID),
    DebugManager(std::make_shared<oclDebugManager>(std::move(stub->DebugManager)))
{
    stub->ProgID = nullptr;

    if (Context->Version >= 12)
    {
        cl_int ret;
        size_t len = 0;
        ret = clGetProgramInfo(ProgID, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &len);
        if (ret != CL_SUCCESS)
            COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot find kernels");
        vector<char> buf(len, '\0');
        clGetProgramInfo(ProgID, CL_PROGRAM_KERNEL_NAMES, len, buf.data(), &len);
        if (len > 0)
            buf.pop_back(); //null-terminated
        const auto names = common::str::Split(buf, ';', false);
        KernelNames.assign(names.cbegin(), names.cend());
    }
    else
    {
        for (const auto& [name, info] : stub->ImportedKernelInfo)
            KernelNames.push_back(name);
    }

    Kernels = common::linq::FromIterable(KernelNames)
        .Select([&](const auto& name)
            {
                KernelArgStore argInfo;
                for (const auto& [name_, info] : stub->ImportedKernelInfo)
                    if (name == name_)
                        argInfo = info;
                return MAKE_ENABLER_UNIQUE(oclKernel_, (Context->Plat.get(), this, name, std::move(argInfo)));
            })
        .ToVector();
}

oclProgram_::~oclProgram_()
{
    clReleaseProgram(ProgID);
}

oclKernel oclProgram_::GetKernel(const string_view& name) const
{
    for (const auto& ker : Kernels)
        if (ker->Name == name)
            return oclKernel(shared_from_this(), ker.get());
    return {};
}

oclProgram_::oclProgStub oclProgram_::Create(const oclContext& ctx, string str, const oclDevice& dev)
{
    return oclProgStub(ctx, dev ? dev : ctx->Devices[0], std::move(str));
}

oclProgram oclProgram_::CreateAndBuild(const oclContext& ctx, string str, const CLProgConfig& config, const oclDevice& dev)
{
    auto stub = Create(ctx, std::move(str), dev);
    stub.Build(config);
    return stub.Finish();
}



}
