#include "oclPch.h"
#include "oclPlatform.h"
#include "oclProgram.h"
#include "oclUtil.h"

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


string_view ArgFlags::GetArgTypeName() const noexcept
{
    switch (ArgType)
    {
    case KerArgType::Buffer:    return "Buffer"sv;
    case KerArgType::Image:     return "Image"sv;
    case KerArgType::Simple:    return "Simple"sv;
    default:                    return "Any"sv;
    }
}
string_view ArgFlags::GetSpaceName() const noexcept
{
    switch (Space)
    {
    case KerArgSpace::Global:   return "Global"sv;
    case KerArgSpace::Constant: return "Constant"sv;
    case KerArgSpace::Local:    return "Local"sv;
    default:                    return "Private"sv;
    }
}
string_view ArgFlags::GetImgAccessName() const noexcept
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
#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4063)
#elif COMMON_COMPILER_GCC | COMMON_COMPILER_CLANG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
string_view ArgFlags::GetQualifierName() const noexcept
{
    switch (Qualifier)
    {
    case KerArgFlag::None:
        return ""sv;
    case KerArgFlag::Const:
        return "Const"sv;
    case KerArgFlag::Volatile:
        return "Volatile"sv;
    case KerArgFlag::Restrict:
        return "Restrict"sv;
    case KerArgFlag::Pipe:
        return "Pipe"sv;
    case KerArgFlag::Const    | KerArgFlag::Volatile:
        return "Const Volatile"sv;
    case KerArgFlag::Const    | KerArgFlag::Restrict:
        return "Const Restrict"sv;
    case KerArgFlag::Const    | KerArgFlag::Pipe:
        return "Const Pipe"sv;
    case KerArgFlag::Volatile | KerArgFlag::Restrict:
        return "Volatile Restrict"sv;
    case KerArgFlag::Volatile | KerArgFlag::Pipe:
        return "Volatile Pipe"sv;
    case KerArgFlag::Restrict | KerArgFlag::Pipe:
        return "Restrict Pipe"sv;
    case KerArgFlag::Const    | KerArgFlag::Volatile | KerArgFlag::Restrict:
        return "Const Volatile Restrict"sv;
    case KerArgFlag::Const    | KerArgFlag::Volatile | KerArgFlag::Pipe:
        return "Const Volatile Pipe"sv;
    case KerArgFlag::Const    | KerArgFlag::Restrict | KerArgFlag::Pipe:
        return "Const Restrict Pipe"sv;
    case KerArgFlag::Volatile | KerArgFlag::Restrict | KerArgFlag::Pipe:
        return "Volatile Restrict Pipe"sv;
    case KerArgFlag::Const    | KerArgFlag::Volatile | KerArgFlag::Restrict | KerArgFlag::Pipe:
        return "Const Volatile Restrict Pipe"sv;
    default:
        return "Unknown"sv;
    }
}
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#elif COMMON_COMPILER_GCC | COMMON_COMPILER_CLANG
#pragma GCC diagnostic pop
#endif

KernelArgStore::KernelArgStore(const detail::PlatFuncs* funcs, CLHandle<detail::CLKernel> kernel, const KernelArgStore& reference) :
    InfoProv(reference.InfoProv), DebugBuffer(0), HasInfo(true), HasDebug(false)
{
    uint32_t size = 0;
    {
        size_t dummy = 0;
        funcs->clGetKernelInfo(*kernel, CL_KERNEL_NUM_ARGS, sizeof(uint32_t), &size, &dummy);
        ArgsInfo.reserve(size);
    }
    auto GetKernelArgStr = [&, tmp = std::string{}](auto kernel, auto idx, cl_kernel_arg_info info) mutable
    {
        tmp.resize(255, '\0'); // in case not returning correct length
        size_t length = 0;
        LogCLError(funcs->clGetKernelArgInfo(*kernel, idx, info, 0, nullptr, &length),
            u"error when get length");
        if (length > 255) tmp.resize(length, '\0');
        // Mali require size to be at least sizeof(char[]), although it does not return length when query.
        LogCLError(funcs->clGetKernelArgInfo(*kernel, idx, info, tmp.size(), tmp.data(), &length),
            u"error when get str");
        return std::string_view(tmp.data(), length > 0 ? length - 1 : 0);
    };
    for (uint32_t i = 0; i < size; ++i)
    {
        ArgInfo info;

        size_t dummy = 0;
        cl_kernel_arg_address_qualifier space;
        
        LogCLError(funcs->clGetKernelArgInfo(*kernel, i, CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(space), &space, &dummy),
            u"error when get arg addr qualifier");
        switch (space)
        {
        case CL_KERNEL_ARG_ADDRESS_GLOBAL:      info.Space = KerArgSpace::Global;   break;
        case CL_KERNEL_ARG_ADDRESS_CONSTANT:    info.Space = KerArgSpace::Constant; break;
        case CL_KERNEL_ARG_ADDRESS_LOCAL:       info.Space = KerArgSpace::Local;    break;
        default:                                info.Space = KerArgSpace::Private;  break;
        }

        cl_kernel_arg_access_qualifier access;
        LogCLError(funcs->clGetKernelArgInfo(*kernel, i, CL_KERNEL_ARG_ACCESS_QUALIFIER, sizeof(access), &access, &dummy),
            u"error when get arg access qualifier");
        switch (access)
        {
        case CL_KERNEL_ARG_ACCESS_READ_ONLY:    info.Access = ImgAccess::ReadOnly;  break;
        case CL_KERNEL_ARG_ACCESS_WRITE_ONLY:   info.Access = ImgAccess::WriteOnly; break;
        case CL_KERNEL_ARG_ACCESS_READ_WRITE:   info.Access = ImgAccess::ReadWrite; break;
        default:                                info.Access = ImgAccess::None;      break;
        }
        if (info.Access != ImgAccess::None)
            info.ArgType = KerArgType::Image;

        cl_kernel_arg_type_qualifier qualifier;
        LogCLError(funcs->clGetKernelArgInfo(*kernel, i, CL_KERNEL_ARG_TYPE_QUALIFIER, sizeof(qualifier), &qualifier, &dummy),
            u"error when get arg type qualifier");
        info.Qualifier = static_cast<KerArgFlag>(qualifier);

        info.Name = ArgTexts.AllocateString(GetKernelArgStr(kernel, i, CL_KERNEL_ARG_NAME));
        info.Type = ArgTexts.AllocateString(GetKernelArgStr(kernel, i, CL_KERNEL_ARG_TYPE_NAME));

        ArgsInfo.emplace_back(info);
    }
    // post-process
    if (ArgsInfo.size() >= 3)
    {
        const auto& arg0 = ArgsInfo[ArgsInfo.size() - 3];
        const auto& arg1 = ArgsInfo[ArgsInfo.size() - 2];
        const auto& arg2 = ArgsInfo[ArgsInfo.size() - 1];
        if (ArgTexts.GetStringView(arg0.Name) == "_oclu_debug_buffer_size" && ArgTexts.GetStringView(arg0.Type) == "uint"  &&
            ArgTexts.GetStringView(arg1.Name) == "_oclu_debug_buffer_info" && ArgTexts.GetStringView(arg1.Type) == "uint*" &&
            ArgTexts.GetStringView(arg2.Name) == "_oclu_debug_buffer_data" && ArgTexts.GetStringView(arg2.Type) == "uint*")
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
                {
                    if (cur.Name.empty())
                        ArgsInfo[i].Name = ArgTexts.AllocateString(ref.Name);
                    else
                        oclLog().debug(u"KerArgStore, external reports arg[{}] is [{}] while provided [{}].\n",
                            i, ref.Name, cur.Name);
                }
                if (ref.Type != cur.Type)
                {
                    if (cur.Type.empty())
                        ArgsInfo[i].Type = ArgTexts.AllocateString(ref.Type);
                    /*else
                        oclLog().debug(u"KerArgStore, external reports arg[{}] is [{}] while provided [{}].\n",
                            i, ref.Type, cur.Type);*/
                }
                if (ref.Space != cur.Space)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n", 
                        i, ref.Name, ref.GetSpaceName(), cur.GetSpaceName());
                if (ref.Access != cur.Access)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n",
                        i, ref.Name, ref.GetImgAccessName(), cur.GetImgAccessName());
                if (ref.Qualifier != cur.Qualifier)
                    oclLog().debug(u"KerArgStore, external reports arg[{}]({}) is [{}] while provided [{}].\n",
                        i, ref.Name, ref.GetQualifierName(), cur.GetQualifierName());
                if (ref.ArgType != cur.ArgType)
                {
                    if (cur.ArgType == KerArgType::Any)
                        ArgsInfo[i].ArgType = ref.ArgType;
                    else if (ref.ArgType != KerArgType::Any)
                        oclLog().debug(u"KerArgStore, external reports arg[{}] is [{}] while provided [{}].\n",
                            i, ref.GetArgTypeName(), cur.GetArgTypeName());
                }
            }
        }
    }
}

KernelArgInfo KernelArgStore::GetArgInfo(const size_t idx) const noexcept
{
    const auto info = GetArg(idx);
    return { { info->ArgType, info->Space, info->Access, info->Qualifier }, ArgTexts.GetStringView(info->Name), ArgTexts.GetStringView(info->Type) };
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

void KernelArgStore::AddArg(const KerArgType argType, const KerArgSpace space, const ImgAccess access, const KerArgFlag qualifier,
    const std::string_view name, const std::string_view type)
{
    ArgInfo info;
    info.ArgType   = argType;
    info.Space     = space;
    info.Access    = access;
    info.Qualifier = qualifier;
    info.Name      = ArgTexts.AllocateString(name);
    info.Type      = ArgTexts.AllocateString(type);
    ArgsInfo.emplace_back(info);
    HasInfo = true;
}


class KernelDebugPackage : public xcomp::debug::DebugPackage
{
public:
    using xcomp::debug::DebugPackage::DebugPackage;
    void ReleaseRuntime() override
    {
        if (oclMapPtr::CheckIsCLBuffer(DataBuffer))
            DataBuffer = common::AlignedBuffer(DataBuffer);
        if (oclMapPtr::CheckIsCLBuffer(InfoBuffer))
            InfoBuffer = common::AlignedBuffer(InfoBuffer);
    }
};

std::unique_ptr<xcomp::debug::DebugPackage> CallResult::GetDebugData(const bool releaseRuntime) const
{
    if (!DebugMan) return {};
    const auto exeName = common::str::to_u32string(Kernel->Name, common::str::Encoding::UTF8);
    const auto info = InfoBuf->Map(Queue, oclu::MapFlag::Read);
    const auto infoData = info.AsType<uint32_t>();
    const auto dbgSize = std::min(infoData[0] * sizeof(uint32_t), DebugBuf->Size);
    if (releaseRuntime)
    {
        common::AlignedBuffer infoBuf(InfoBuf->Size), dataBuf(dbgSize);
        memcpy_s(infoBuf.GetRawPtr(), InfoBuf->Size, &infoData[0], InfoBuf->Size);
        if (dbgSize > 0)
            DebugBuf->ReadSpan(Queue, dataBuf.AsSpan())->WaitFinish();
        return std::make_unique<xcomp::debug::DebugPackage>(
            exeName, DebugMan, InfoProv,
            std::move(infoBuf),
            std::move(dataBuf));
    }
    else
    {
        return std::make_unique<KernelDebugPackage>(
            exeName, DebugMan, InfoProv,
            info.AsBuffer(), 
            DebugBuf->Map(Queue, oclu::MapFlag::Read).AsBuffer().CreateSubBuffer(0, dbgSize));
    }
}


static void GetWorkGroupInfo(const detail::PlatFuncs* funcs, WorkGroupInfo& info, const cl_kernel kerId, const oclDevice dev)
{
#define GetInfo(name, field) funcs->clGetKernelWorkGroupInfo(kerId, *dev->DeviceID, name, sizeof(info.field), &info.field, nullptr)
    GetInfo(CL_KERNEL_LOCAL_MEM_SIZE,                       LocalMemorySize);
    GetInfo(CL_KERNEL_PRIVATE_MEM_SIZE,                     PrivateMemorySize);
    GetInfo(CL_KERNEL_WORK_GROUP_SIZE,                      WorkGroupSize);
    GetInfo(CL_KERNEL_COMPILE_WORK_GROUP_SIZE,              CompiledWorkGroupSize);
    GetInfo(CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,   PreferredWorkGroupSizeMultiple);
    if (dev->Extensions.Has("cl_intel_required_subgroup_size"sv))
        GetInfo(CL_KERNEL_SPILL_MEM_SIZE_INTEL,             SpillMemSize);
    else
        info.SpillMemSize = 0;
}

oclKernel_::oclKernel_(const oclProgram_* prog, void* kerID, string name, KernelArgStore&& argStore) :
    detail::oclCommon(*prog), Kernel(kerID), Program(*prog), ReqDbgBufSize(0), Name(std::move(name))
{
    GetWorkGroupInfo(Funcs, WgInfo, *Kernel, Program.Device);
    if (Program.Context->Version >= 12)
        ArgStore = KernelArgStore(Funcs, *Kernel, argStore);
    else if (argStore.HasInfo)
    {
        oclLog().verbose(u"use external arg-info for kernel [{}].\n", Name);
        ArgStore = std::move(argStore);
    }
    ReqDbgBufSize = ArgStore.DebugBuffer == 0 ? 512 : ArgStore.DebugBuffer;
}
oclKernel_::~oclKernel_()
{
    clReleaseKernel(*Kernel);
}

std::optional<SubgroupInfo> oclKernel_::GetSubgroupInfo(const uint8_t dim, const size_t* localsize) const
{
    if (!Funcs->clGetKernelSubGroupInfoKHR)
        return {};
    if (!Program.Device->Extensions.Has("cl_khr_subgroups"sv) && !Program.Device->Extensions.Has("cl_intel_subgroups"sv))
        return {};
    SubgroupInfo info;
    const cl_device_id devid = *Program.Device->DeviceID;
    Funcs->clGetKernelSubGroupInfoKHR(*Kernel, devid, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupSize, nullptr);
    Funcs->clGetKernelSubGroupInfoKHR(*Kernel, devid, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR, sizeof(size_t) * dim, localsize, sizeof(size_t), &info.SubgroupCount, nullptr);
    if (Program.Device->Extensions.Has("cl_intel_required_subgroup_size"sv))
        Funcs->clGetKernelSubGroupInfoKHR(*Kernel, devid, CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL, 0, nullptr, sizeof(size_t), &info.CompiledSubgroupSize, nullptr);
    else
        info.CompiledSubgroupSize = 0;
    return info;
}


static cl_kernel CloneKernel(const detail::PlatFuncs* funcs, cl_program prog, cl_kernel ker, const std::string& name, const oclDevice dev)
{
    if (dev->Version < 22)
    {
        if (funcs->IsICD || !funcs->clCloneKernel)
        {
            return funcs->clCreateKernel(prog, name.c_str(), nullptr);
        }
    }
    return funcs->clCloneKernel(ker, nullptr);
}

oclKernel_::CallSiteInternal::CallSiteInternal(const oclKernel_* kernel) : 
    KernelHost(kernel->Program.shared_from_this(), kernel), 
    Kernel(CloneKernel(kernel->Funcs, *kernel->Program.Program, *kernel->Kernel, kernel->Name, kernel->Program.Device))
{ }

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const oclSubBuffer_& buf) const
{
    if (const auto info = KernelHost->ArgStore.GetArg(idx); info && !info->IsType(KerArgType::Buffer))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"buffer is set to a non-buffer kernel argument slot");
    auto ret = KernelHost->Funcs->clSetKernelArg(*Kernel, idx, sizeof(cl_mem), &buf.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const oclImage_ & img) const
{
    if (const auto info = KernelHost->ArgStore.GetArg(idx); info && !info->IsType(KerArgType::Image))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"image is set to an non-image kernel argument slot");
    auto ret = KernelHost->Funcs->clSetKernelArg(*Kernel, idx, sizeof(cl_mem), &img.MemID);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

void oclKernel_::CallSiteInternal::SetArg(const uint32_t idx, const void* dat, const size_t size) const
{
    if (const auto info = KernelHost->ArgStore.GetArg(idx); info && !info->IsType(KerArgType::Simple))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"simple is set to an non-simple kernel argument slot");
    KernelHost->ArgStore.GetArg(idx);
    auto ret = KernelHost->Funcs->clSetKernelArg(*Kernel, idx, size, dat);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"set kernel argument error");
}

PromiseResult<CallResult> oclKernel_::CallSiteInternal::Run(const uint8_t dim, DependEvents depend,
    const oclCmdQue& que, const size_t* worksize, const size_t* workoffset, const size_t* localsize)
{
    if (KernelHost->Program.Device != que->Device)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"queue's device is not the same as this kernel");

    CallResult result;
    if (KernelHost->ArgStore.HasInfo && KernelHost->ArgStore.HasDebug) // inject debug buffer
    {
        result.DebugMan = KernelHost->Program.DebugMan;
        result.InfoProv = KernelHost->ArgStore.InfoProv;
        result.Kernel = this->KernelHost;
        result.Queue = que;
        const auto infosize = result.InfoProv->GetInfoBufferSize(worksize, dim);
        const auto startIdx = static_cast<uint32_t>(KernelHost->ArgStore.GetSize());
        std::vector<std::byte> tmp(infosize);
        const uint32_t dbgBufCnt = KernelHost->ReqDbgBufSize * 1024u / sizeof(uint32_t);
        result.InfoBuf  = oclBuffer_::Create(que->Context, MemFlag::ReadWrite, tmp.size(), tmp.data());
        result.DebugBuf = oclBuffer_::Create(que->Context, MemFlag::HostReadOnly | MemFlag::WriteOnly, dbgBufCnt * sizeof(uint32_t));
        KernelHost->Funcs->clSetKernelArg(*Kernel, startIdx + 0, sizeof(uint32_t), &dbgBufCnt);
        KernelHost->Funcs->clSetKernelArg(*Kernel, startIdx + 1, sizeof(cl_mem),   &result.InfoBuf->MemID);
        KernelHost->Funcs->clSetKernelArg(*Kernel, startIdx + 2, sizeof(cl_mem),   &result.DebugBuf->MemID);
    }

    cl_int ret;
    cl_event e;
    const auto [evtPtr, evtCnt] = depend.GetWaitList();
    if (localsize == nullptr && KernelHost->WgInfo.HasCompiledWGSize())
    {
        localsize = KernelHost->WgInfo.CompiledWorkGroupSize;
        oclLog().warning(FMT_STRING(u"passing nullptr to LocalSize, while attributes shows [{},{},{}]\n"), 
            localsize[0], localsize[1], localsize[2]);
    }
    ret = KernelHost->Funcs->clEnqueueNDRangeKernel(*que->CmdQue, *Kernel, dim, workoffset, worksize, localsize,
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"execute kernel error");
    return oclPromise<CallResult>::Create(std::move(depend), e, que, std::move(result));
}

oclKernel_::KernelDynCallSiteInternal::KernelDynCallSiteInternal(const oclKernel_* kernel, CallArgs&& args) :
    CallSiteInternal(kernel), Args(std::move(args))
{
    using ArgType = std::variant<oclSubBuffer, oclImage, std::vector<std::byte>, std::array<std::byte, 32>, common::span<const std::byte>>;
    uint32_t idx = 0;
    for (const auto& arg : Args.Args)
    {
        switch (arg.index())
        {
        case 0: this->SetArg(idx, *std::get<0>(arg)); break;
        case 1: this->SetArg(idx, *std::get<1>(arg)); break;
        case 2: this->SetArg(idx, std::get<2>(arg).data(), std::get<2>(arg).size()); break;
        case 3: this->SetArg(idx, &std::get<3>(arg)[1], (size_t)std::get<3>(arg)[0]); break;
        case 4: this->SetArg(idx, std::get<4>(arg).data(), std::get<4>(arg).size()); break;
        default: assert(false); break;
        }
        idx++;
    }
}


oclProgStub::oclProgStub(const oclContext& ctx, const oclDevice& dev, string&& str)
    : Context(ctx), Device(dev), Source(std::move(str))
{
    cl_int errcode;
    auto* ptr = Source.c_str();
    size_t size = Source.length();
    Program = Context->Funcs->clCreateProgramWithSource(*Context->Context, 1, &ptr, &size, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create program");
}

oclProgStub::~oclProgStub()
{
    if (*Program)
    {
        oclLog().warning(u"oclProg released before finished.\n");
        Context->Funcs->clReleaseProgram(*Program);
    }
}

void oclProgStub::Build(const CLProgConfig& config)
{
    const auto minVer = std::min(Device->CVersion, Device->Version);
    const auto cver = config.Version == 0 ? minVer : config.Version;
    if (cver > minVer)
        oclLog().warning(u"request cversion [{}] on device [{}] (Ver[{}], CVer[{}])\n", cver, Device->Name, Device->Version, Device->CVersion);
    string options;
    switch (Context->GetVendor())
    {
    case Vendors::NVIDIA:    options += "-DOCLU_NVIDIA "; break;
    case Vendors::AMD:       options += "-DOCLU_AMD "; break;
    case Vendors::Intel:     options += "-DOCLU_INTEL "; break;
    default:                break;
    }
    if (Device->Extensions.Has("cl_nv_compiler_options"))
        options += "-cl-nv-verbose ";
    if (cver >= 12)
        options.append("-cl-kernel-arg-info ");
    for (const auto& def : config.Defines)
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
        lSize = (lSize == UINT64_MAX) ? 0 : lSize; //default as zero
        fmt::format_to(std::back_inserter(options), "-DOCLU_LOCAL_MEM_SIZE={} ", lSize);
    }
    for (const auto& flag : config.Flags)
        options.append(flag).append(" "sv);
    fmt::format_to(std::back_inserter(options), "-cl-std=CL{}.{} ", cver / 10, cver % 10);
    
    const cl_device_id devid = *Device->DeviceID;
    cl_int ret = Context->Funcs->clBuildProgram(*Program, 1, &devid, options.c_str(), nullptr, nullptr);

    std::vector<oclDevice> devs2;
    u16string buildlog = Device->Name + u":\n" + GetBuildLog();
    if (ret == CL_SUCCESS)
    {
        oclLog().success(u"build program {:p} success:\n{}\n", (void*)*Program, buildlog);
    }
    else
    {
        oclLog().error(u"build program {:p} failed:\nwith option:\t{}\n{}\n", (void*)*Program, options, buildlog);
        common::SharedString<char16_t> log(buildlog);
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"Build Program failed")
            .Attach("dev", Device)
            .Attach("source", Source)
            .Attach("detail", log)
            .Attach("buildlog", log);
    }

}

std::u16string oclProgStub::GetBuildLog() const
{ 
    return oclProgram_::GetProgBuildLog(Context->Funcs, Program, Device->DeviceID); 
}

oclProgram oclProgStub::Finish()
{
    return MAKE_ENABLER_SHARED(const oclProgram_, (this));
}


u16string oclProgram_::GetProgBuildLog(const detail::PlatFuncs* funcs, CLHandle<detail::CLProgram> progID, CLHandle<detail::CLDevice> dev)
{
    u16string result;
    cl_build_status status = CL_BUILD_NONE;
    funcs->clGetProgramBuildInfo(*progID, *dev, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
    switch (status)
    {
    case CL_BUILD_NONE:     return u"Not been built yet";
    case CL_BUILD_SUCCESS:  result = u"Build successfully:\n"; break;
    case CL_BUILD_ERROR:    result = u"Build error:\n"; break;
    default:                return u"";
    }
    size_t logsize;
    funcs->clGetProgramBuildInfo(*progID, *dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logsize);
    if (logsize > 0)
    {
        string logstr(logsize, '\0');
        funcs->clGetProgramBuildInfo(*progID, *dev, CL_PROGRAM_BUILD_LOG, logstr.size(), logstr.data(), &logsize);
        logstr.pop_back();
        result.append(common::str::to_u16string(logstr, common::str::Encoding::UTF8));
    }
    return result;
}

oclProgram_::oclProgram_(oclProgStub* stub) : detail::oclCommon(*stub->Context), Program(stub->Program),
    Context(std::move(stub->Context)), Device(std::move(stub->Device)), Source(std::move(stub->Source)),
    DebugMan(std::move(stub->DebugMan))
{
    stub->Program = nullptr;

    cl_uint numKernels;
    Funcs->clCreateKernelsInProgram(*Program, 0, nullptr, &numKernels);
    std::vector<cl_kernel> kernelIDs;
    kernelIDs.resize(numKernels);
    Funcs->clCreateKernelsInProgram(*Program, numKernels, kernelIDs.data(), &numKernels);
    KernelNames.reserve(numKernels);

    for (const auto kerID : kernelIDs)
    {
        size_t strSize = 0;
        Funcs->clGetKernelInfo(kerID, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &strSize);
        std::string name; name.resize(strSize, '\0');
        Funcs->clGetKernelInfo(kerID, CL_KERNEL_FUNCTION_NAME, strSize, name.data(), &strSize);
        if (strSize > 0 && name.back() == '\0') 
            name.pop_back();

        KernelArgStore argInfo;
        for (const auto& [name_, info] : stub->ImportedKernelInfo)
        {
            if (name == name_)
            {
                argInfo = info;
                break;
            }
        }
        Kernels.emplace_back(MAKE_ENABLER_UNIQUE(oclKernel_, (this, kerID, name, std::move(argInfo))));
        KernelNames.emplace_back(std::move(name));
    }
}

oclProgram_::~oclProgram_()
{
    Funcs->clReleaseProgram(*Program);
}

oclKernel oclProgram_::GetKernel(const string_view& name) const
{
    for (const auto& ker : Kernels)
        if (ker->Name == name)
            return oclKernel(shared_from_this(), ker.get());
    return {};
}

std::vector<std::byte> oclProgram_::GetBinary() const
{
    std::vector<std::byte> ret;
    uint32_t devCnt = 0;
#define CHK_SUC(x) if (x != CL_SUCCESS) return ret
    CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &devCnt, nullptr));
    if (devCnt == 1)
    {
        size_t size = 0;
        CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_BINARY_SIZES, sizeof(size), &size, nullptr));
        ret.resize(size);
        auto ptr = ret.data();
        CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_BINARIES, sizeof(ptr), &ptr, nullptr));
        return ret;
    }
    else if (devCnt > 1) 
    {
        std::vector<size_t> sizes; sizes.resize(devCnt, 0);
        CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t) * devCnt, sizes.data(), nullptr));
        std::vector<cl_device_id> devs; devs.resize(devCnt);
        CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * devCnt, devs.data(), nullptr));
        std::vector<std::byte*> ptrs; ptrs.resize(devCnt, nullptr);
        size_t idx = 0;
        for (const auto dev : devs)
        {
            if (dev == *Device->DeviceID)
            {
                ret.resize(sizes[idx]);
                ptrs[idx] = ret.data();
                break;
            }
            idx++;
        }
        CHK_SUC(Funcs->clGetProgramInfo(*Program, CL_PROGRAM_BINARIES, sizeof(std::byte*) * devCnt, ptrs.data(), nullptr));
        return ret;
    }
    return ret;
}

oclProgStub oclProgram_::Create(const oclContext& ctx, string str, const oclDevice& dev)
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
