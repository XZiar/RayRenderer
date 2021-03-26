#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "oclException.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Detect.h"
#include "common/StrParsePack.hpp"

namespace oclu
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::ArgLocator;
using xziar::nailang::Expr;
using xziar::nailang::CustomVar;
using xziar::nailang::SubQuery;
using xziar::nailang::LateBindVar;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::Statement;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncPack;
using xziar::nailang::MetaSet;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::ArgLimits;
using xziar::nailang::AutoVarHandler;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::mlog::LogLevel;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;
using FuncInfo = xziar::nailang::FuncName::FuncInfo;


#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)



NLCLExtension::NLCLExtension(NLCLContext& context) : xcomp::XCNLExtension(context), Context(context)
{ }
NLCLExtension::~NLCLExtension() { }


struct NLCLContext::OCLUVar : public AutoVarHandler<NLCLContext>
{
    struct CLExtVar : public CustomVar::Handler
    {
        Arg SubfieldGetter(const CustomVar& var, std::u32string_view name) override
        {
            const auto& exts = reinterpret_cast<const oclDevice_*>(var.Meta0)->Extensions;
            if (name == U"Length"sv)
                return exts.Size();
            const auto extName = common::str::to_string(name, Charset::UTF8);
            return exts.Has(extName);
        }
        Arg IndexerGetter(const CustomVar& var, const Arg& idx, const Expr& src) override
        {
            const auto& exts = reinterpret_cast<const oclDevice_*>(var.Meta0)->Extensions;
            const auto tidx = xziar::nailang::NailangHelper::BiDirIndexCheck(exts.Size(), idx, &src);
            return common::str::to_u32string(exts[tidx]);
        }
        std::u32string_view GetTypeName() noexcept override { return U"OCLUDeviceExtensions"sv; }
    };
    OCLUVar() : AutoVarHandler<NLCLContext>(U"NLCL"sv)
    {
        static CLExtVar CLExtVarHandler;
        AddCustomMember(U"Extension"sv, [](NLCLContext& ctx) -> CustomVar
            {
                return { &CLExtVarHandler, reinterpret_cast<uintptr_t>(ctx.Device), 0, 0 };
            });
        AddAutoMember<oclDevice_>(U"Dev"sv, U"oclDevice"sv, [](NLCLContext& ctx)
            {
                return ctx.Device;
            }, [](auto& argHandler)
            {
#define MEMBER(name) argHandler.AddSimpleMember(U"" STRINGIZE(name) ""sv, [](oclDevice_& dev) { return dev.name; })
                MEMBER(LocalMemSize);
                MEMBER(GlobalMemSize);
                MEMBER(GlobalCacheSize);
                MEMBER(GlobalCacheLine);
                MEMBER(ConstantBufSize);
                MEMBER(MaxMemAllocSize);
                MEMBER(ComputeUnits);
                MEMBER(WaveSize);
                MEMBER(Version);
                MEMBER(CVersion);
                MEMBER(SupportImage);
                MEMBER(LittleEndian);
#undef MEMBER
                argHandler.AddSimpleMember(U"Type"sv, [](oclDevice_& dev) 
                    { 
                        switch (dev.Type)
                        {
                        case DeviceType::Accelerator:   return U"accelerator"sv;
                        case DeviceType::CPU:           return U"cpu"sv;
                        case DeviceType::GPU:           return U"gpu"sv;
                        case DeviceType::Custom:        return U"custom"sv;
                        default:                        return U"other"sv;
                        }
                    });
                argHandler.AddSimpleMember(U"Vendor"sv, [](oclDevice_& dev)
                    {
                        switch (dev.PlatVendor)
                        {
#define U_VENDOR(name) case Vendors::name: return U"" STRINGIZE(name) ""sv
                        U_VENDOR(AMD);
                        U_VENDOR(ARM);
                        U_VENDOR(Intel);
                        U_VENDOR(NVIDIA);
                        U_VENDOR(Qualcomm);
#undef U_VENDOR
                        default: return U"Other"sv;
                        }
                    });
            });
    }
    Arg ConvertToCommon(const CustomVar&, Arg::Type type) noexcept override
    {
        if (type == Arg::Type::Bool)
            return true;
        return {};
    }
};


NLCLContext::NLCLContext(oclDevice dev, const common::CLikeDefines& info) :
    xcomp::XCNLContext(info), Device(dev),
    SupportFP16(Device->Extensions.Has("cl_khr_fp16")),
    SupportFP64(Device->Extensions.Has("cl_khr_fp64") || Device->Extensions.Has("cl_amd_fp64")),
    SupportNVUnroll(Device->Extensions.Has("cl_nv_pragma_unroll")),
    SupportSubgroupKHR(Device->Extensions.Has("cl_khr_subgroups")),
    SupportSubgroupIntel(Device->Extensions.Has("cl_intel_subgroups")),
    SupportSubgroup8Intel(SupportSubgroupIntel && Device->Extensions.Has("cl_intel_subgroups_char")),
    SupportSubgroup16Intel(SupportSubgroupIntel && Device->Extensions.Has("cl_intel_subgroups_short")),
    SupportBasicSubgroup(SupportSubgroupKHR || SupportSubgroupIntel),
    EnabledExtensions(Device->Extensions.Size()),
    EnableUnroll(Device->CVersion >= 20 || SupportNVUnroll), AllowDebug(false)
{
    AllowDebug = info["debug"].has_value();
    static OCLUVar OCLUVarHandler;
    OCLUArg = OCLUVarHandler.CreateVar(*this);
}
NLCLContext::~NLCLContext()
{ }

ArgLocator NLCLContext::LocateArg(const LateBindVar& var, bool create) noexcept
{
    if (var == U"oclu"sv)
        return { &OCLUArg, 0 };
    return XCNLContext::LocateArg(var, create);
}


NLCLExecutor::NLCLExecutor(NLCLRuntime* runtime) : XCNLExecutor(runtime)
{ }


NLCLConfigurator::NLCLConfigurator(NLCLRuntime* runtime) : NLCLExecutor(runtime)
{ }
NLCLConfigurator::~NLCLConfigurator()
{ }
xcomp::XCNLExecutor& NLCLConfigurator::GetExecutor() noexcept
{
    return *this;
}

NLCLConfigurator::MetaFuncResult NLCLConfigurator::HandleMetaFunc(const FuncCall& meta, MetaSet& allMetas)
{
    const auto ret = NLCLExecutor::HandleMetaFunc(meta, allMetas);
    if (ret != MetaFuncResult::Unhandled)
        return ret;
    if (const auto frame = TryGetOutputFrame(); frame)
        return XCNLConfigurator::HandleRawBlockMeta(meta, frame->Block, allMetas) ? MetaFuncResult::Next : MetaFuncResult::Unhandled;
    return MetaFuncResult::Unhandled;
}

Arg NLCLConfigurator::EvaluateFunc(FuncEvalPack& func)
{
    auto& runtime = GetRuntime();
    const auto& fname = func.GetName();
    if (func.Name->PartCount == 2 && fname[0] == U"oclu"sv)
    {
        const auto subName = fname[1];
        switch (common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"CompilerFlag")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            const auto flag = common::str::to_string(func.Params[0].GetStr().value(), Charset::UTF8, Charset::UTF32);
            for (const auto& item : runtime.Context.CompilerFlags)
            {
                if (item == flag)
                    return {};
            }
            runtime.Context.CompilerFlags.push_back(flag);
        } return {};
        HashCase(subName, U"EnableExtension")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            if (const auto sv = func.Params[0].GetStr(); sv.has_value())
            {
                if (runtime.EnableExtension(sv.value()))
                    return true;
                else
                    runtime.Logger.warning(u"Extension [{}] not found in support list, skipped.\n", sv.value());
            }
            else
                NLRT_THROW_EX(u"Arg of [EnableExtension] should be string"sv, func);
        } return false;
        HashCase(subName, U"EnableUnroll")
        {
            ThrowIfNotFuncTarget(func, FuncInfo::Empty);
            if (!runtime.Context.EnableUnroll)
            {
                runtime.Logger.warning(u"Manually enable unroll hint.\n");
                runtime.Context.EnableUnroll = true;
            }
        } return {};
        default: break;
        }
        auto ret = runtime.CommonFunc(subName, func);
        if (ret.has_value())
            return std::move(ret.value());
    }
    return XCNLExecutor::EvaluateFunc(func);
}

void NLCLConfigurator::EvaluateRawBlock(const RawBlock& block, xcomp::MetaFuncs metas)
{
    GetRuntime().CollectRawBlock(block, metas);
}


NLCLRawExecutor::NLCLRawExecutor(NLCLRuntime* runtime) : NLCLExecutor(runtime)
{ }
NLCLRawExecutor::~NLCLRawExecutor()
{ }
xcomp::XCNLExecutor& NLCLRawExecutor::GetExecutor() noexcept
{
    return *this;
}
const xcomp::XCNLExecutor& NLCLRawExecutor::GetExecutor() const noexcept
{
    return *this;
}

void NLCLRawExecutor::StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg) const
{
    switch (arg.ArgType)
    {
    case KerArgType::Simple:
        APPEND_FMT(out, U"{:8} {:5}  {} {}", ArgFlags::ToCLString(arg.Space),
            HAS_FIELD(arg.Qualifier, KerArgFlag::Const) ? U"const"sv : U""sv,
            common::str::to_u32string(arg.Type, Charset::UTF8),
            common::str::to_u32string(arg.Name, Charset::UTF8));
        break;
    case KerArgType::Buffer:
        APPEND_FMT(out, U"{:8} {:5} {} {} {} {}", ArgFlags::ToCLString(arg.Space),
            HAS_FIELD(arg.Qualifier, KerArgFlag::Const) ? U"const"sv : U""sv,
            HAS_FIELD(arg.Qualifier, KerArgFlag::Volatile) ? U"volatile"sv : U""sv,
            common::str::to_u32string(arg.Type, Charset::UTF8),
            HAS_FIELD(arg.Qualifier, KerArgFlag::Restrict) ? U"restrict"sv : U""sv,
            common::str::to_u32string(arg.Name, Charset::UTF8));
        break;
    case KerArgType::Image:
        APPEND_FMT(out, U"{:10} {} {}", ArgFlags::ToCLString(arg.Access),
            common::str::to_u32string(arg.Type, Charset::UTF8),
            common::str::to_u32string(arg.Name, Charset::UTF8));
        break;
    default:
        NLRT_THROW_EX(u"Does not support KerArg of type[Any]");
        break;
    }
}

std::unique_ptr<xcomp::InstanceContext> NLCLRawExecutor::PrepareInstance(const xcomp::OutputBlock&)
{
    return std::make_unique<KernelContext>();
}

void NLCLRawExecutor::OutputInstance(const xcomp::OutputBlock& block, std::u32string& dst)
{
    auto& kerCtx = GetCurInstance();
    // attributes
    for (const auto& item : kerCtx.Attributes)
    {
        dst.append(item.Content).append(U"\r\n"sv);
    }
    APPEND_FMT(dst, U"kernel void {} ("sv, block.Name());
    // arguments
    for (const auto& arg : kerCtx.Args)
    {
        dst.append(U"\r\n    "sv);
        StringifyKernelArg(dst, arg);
        dst.append(U","sv);
    }
    for (const auto& arg : kerCtx.TailArgs)
    {
        dst.append(U"\r\n    "sv);
        StringifyKernelArg(dst, arg);
        dst.append(U","sv);
    }
    if (dst.back() == U',')
        dst.pop_back(); // remove additional ','
    dst.append(U")\r\n{\r\n"sv);
    // prefixes
    kerCtx.WritePrefixes(dst);
    // content
    dst.append(kerCtx.Content);
    // suffixes
    kerCtx.WriteSuffixes(dst);
    dst.append(U"}\r\n"sv);

    for (auto& arg : kerCtx.Args.ArgsInfo)
    {
        if (arg.ArgType == KerArgType::Simple)
            arg.Qualifier = REMOVE_MASK(arg.Qualifier, KerArgFlag::Const); // const will be ignored for simple arg
    }
    GetRuntime().Context.CompiledKernels.emplace_back(
        common::str::to_string(block.Name(), Charset::UTF8, Charset::UTF32),
        std::move(kerCtx.Args));
}

void NLCLRawExecutor::ProcessStruct(const xcomp::OutputBlock& block, std::u32string& dst)
{
    auto frame = XCNLRawExecutor::PushFrame(block, CreateContext(), nullptr);
    dst.append(U"typedef struct \r\n{\r\n"sv);
    DirectOutput(block, dst);
    APPEND_FMT(dst, U"}} {};\r\n"sv, block.Block->Name);
}

bool NLCLRawExecutor::HandleInstanceMeta(FuncPack& meta)
{
    constexpr auto KerArgSpaceParser = PARSE_PACK(
        U"global"sv,     KerArgSpace::Global, 
        U"__global"sv,   KerArgSpace::Global, 
        U"constant"sv,   KerArgSpace::Constant,
        U"__constant"sv, KerArgSpace::Constant,
        U"local"sv,      KerArgSpace::Local, 
        U"__local"sv,    KerArgSpace::Local, 
        U"private"sv,    KerArgSpace::Private,
        U"__private"sv,  KerArgSpace::Private,
        U""sv,           KerArgSpace::Private);
    constexpr auto ImgArgAccessParser = PARSE_PACK(
        U"read_only"sv,    ImgAccess::ReadOnly,
        U"__read_only"sv,  ImgAccess::ReadOnly,
        U"write_only"sv,   ImgAccess::WriteOnly,
        U"__write_only"sv, ImgAccess::WriteOnly,
        U"read_write"sv,   ImgAccess::ReadWrite,
        U""sv,             ImgAccess::ReadWrite);
    auto& kerCtx = GetCurInstance();
    const auto& fname = meta.GetName();
    if (meta.Name->PartCount == 2 && fname[0] == U"oclu"sv)
    {
        switch (const auto subName = fname[1]; common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"RequestWorkgroupSize")
        {
            ThrowByParamTypes<3>(meta, { Arg::Type::Integer, Arg::Type::Integer, Arg::Type::Integer });
            const auto x = meta.Params[0].GetUint().value_or(1),
                y = meta.Params[1].GetUint().value_or(1),
                z = meta.Params[2].GetUint().value_or(1);
            kerCtx.WorkgroupSize = gsl::narrow_cast<uint32_t>(x * y * z);
            kerCtx.AddAttribute(U"ReqWGSize"sv, FMTSTR(U"__attribute__((reqd_work_group_size({}, {}, {})))"sv, x, y, z));
        } return true;
        HashCase(subName, U"SimpleArg")
        {
            ThrowByParamTypes<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto space_ = meta.Params[0].GetStr().value();
            const auto space = KerArgSpaceParser(space_);
            if (!space) NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgSpace [{}]"sv, space_), meta);
            const auto argType = meta.Params[1].GetStr().value();
            // const bool isPointer = !argType.empty() && argType.back() == U'*';
            const auto name = meta.Params[2].GetStr().value();
            const auto flags = common::str::SplitStream(meta.Params[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part)
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgFlag [{}]"sv, part), meta);
                        return flag;
                    }, KerArgFlag::None);
            kerCtx.AddArg(KerArgType::Simple, space.value(), ImgAccess::None, flags,
                common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return true;
        HashCase(subName, U"BufArg")
        {
            ThrowByParamTypes<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto space_ = meta.Params[0].GetStr().value();
            const auto space = KerArgSpaceParser(space_);
            if (!space)
                NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgSpace [{}]"sv, space_), meta);
            const auto argType = std::u32string(meta.Params[1].GetStr().value()) + U'*';
            const auto name = meta.Params[2].GetStr().value();
            const auto flags = common::str::SplitStream(meta.Params[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part)
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        if (part == U"restrict"sv)  return flag | KerArgFlag::Restrict;
                        if (part == U"volatile"sv)  return flag | KerArgFlag::Volatile;
                        NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgFlag [{}]"sv, part), meta);
                        return flag;
                    }, KerArgFlag::None);
            if (space.value() == KerArgSpace::Private)
                NLRT_THROW_EX(FMTSTR(u"BufArg [{}] cannot be in private space: [{}]"sv, name, space_), meta);
            kerCtx.AddArg(KerArgType::Buffer, space.value(), ImgAccess::None, flags,
                common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return true;
        HashCase(subName, U"ImgArg")
        {
            ThrowByParamTypes<3>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto access_ = meta.Params[0].GetStr().value();
            const auto access = ImgArgAccessParser(access_);
            if (!access) NLRT_THROW_EX(fmt::format(u"Unrecognized ImgAccess [{}]"sv, access_), meta);
            const auto argType = meta.Params[1].GetStr().value();
            const auto name = meta.Params[2].GetStr().value();
            kerCtx.AddArg(KerArgType::Image, KerArgSpace::Global, access.value(), KerArgFlag::None,
                common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return true;
        default:
            break;
        }
    }
    return XCNLRawExecutor::HandleInstanceMeta(meta);
}

NLCLRawExecutor::MetaFuncResult NLCLRawExecutor::HandleMetaFunc(FuncPack& meta, MetaSet& allMetas)
{
    if (HandleInstanceMeta(meta))
        return MetaFuncResult::Next;
    return NLCLExecutor::HandleMetaFunc(meta, allMetas);
}

void NLCLRawExecutor::OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtMost);
        auto& ctx = GetRuntime().Context;
        if (ctx.EnableUnroll)
        {
            if (ctx.Device->CVersion >= 20 || !ctx.SupportNVUnroll)
            {
                if (args.empty())
                    output.append(U"__attribute__((opencl_unroll_hint))"sv);
                else
                    APPEND_FMT(output, U"__attribute__((opencl_unroll_hint({})))"sv, args[0]);
            }
            else
            {
                if (args.empty())
                    output.append(U"#pragma unroll"sv);
                else
                    APPEND_FMT(output, U"#pragma unroll {}"sv, args[0]);
            }
        }
        return;
    }
    return XCNLRawExecutor::OnReplaceFunction(output, cookie, func, args);
}


NLCLStructHandler::NLCLStructHandler(NLCLRuntime* runtime) : NLCLExecutor(runtime)
{ }
NLCLStructHandler::~NLCLStructHandler()
{ }
xcomp::XCNLExecutor& NLCLStructHandler::GetExecutor() noexcept
{
    return *this;
}
const xcomp::XCNLExecutor& NLCLStructHandler::GetExecutor() const noexcept
{
    return *this;
}

void NLCLStructHandler::OnNewField(const xcomp::XCNLStruct& target, xcomp::XCNLStruct::Field& field, xziar::nailang::MetaSet& allMetas)
{

}

NLCLStructHandler::MetaFuncResult NLCLStructHandler::HandleMetaFunc(const FuncCall& meta, MetaSet& allMetas)
{
    const auto ret = NLCLExecutor::HandleMetaFunc(meta, allMetas);
    if (ret != MetaFuncResult::Unhandled)
        return ret;
    if (TryGetStructFrame())
    {

    }
    return MetaFuncResult::Unhandled;
}

Arg NLCLStructHandler::EvaluateFunc(FuncEvalPack& func)
{
    auto& runtime = GetRuntime();
    const auto& fname = func.GetName();
    if (EvaluateStructFunc(func))
        return {};
    return XCNLExecutor::EvaluateFunc(func);
}



NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx) :
    XCNLRuntime(logger, evalCtx), Context(*evalCtx), Configurator(this), RawExecutor(this), StructHandler(this)
{ }
NLCLRuntime::~NLCLRuntime()
{ }

xcomp::XCNLConfigurator& NLCLRuntime::GetConfigurator() noexcept { return Configurator; }
xcomp::XCNLRawExecutor& NLCLRuntime::GetRawExecutor() noexcept { return RawExecutor; }
xcomp::XCNLStructHandler& NLCLRuntime::GetStructHandler() noexcept { return StructHandler; }

std::u32string_view NLCLRuntime::GetCLTypeName(common::simd::VecDataInfo info) noexcept
{
#define CASE(s, type, bit, n) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): return PPCAT(PPCAT(U,s),sv);
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),            type, bit, 1)  \
    CASE(STRINGIZE(PPCAT(pfx, 2)),  type, bit, 2)  \
    CASE(STRINGIZE(PPCAT(pfx, 3)),  type, bit, 3)  \
    CASE(STRINGIZE(PPCAT(pfx, 4)),  type, bit, 4)  \
    CASE(STRINGIZE(PPCAT(pfx, 8)),  type, bit, 8)  \
    CASE(STRINGIZE(PPCAT(pfx, 16)), type, bit, 16) \

    switch (static_cast<uint32_t>(info))
    {
    CASEV(uchar,  Unsigned, 8)
    CASEV(ushort, Unsigned, 16)
    CASEV(uint,   Unsigned, 32)
    CASEV(ulong,  Unsigned, 64)
    CASEV(char,   Signed,   8)
    CASEV(short,  Signed,   16)
    CASEV(int,    Signed,   32)
    CASEV(long,   Signed,   64)
    CASEV(half,   Float,    16)
    CASEV(float,  Float,    32)
    CASEV(double, Float,    64)
    default: return {};
    }

#undef CASEV
#undef CASE
}

xcomp::OutputBlock::BlockType NLCLRuntime::GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept
{
    switch (common::DJBHash::HashC(block.Type))
    {
    HashCase(block.Type, U"oclu.Global")    return xcomp::OutputBlock::BlockType::Global;
    HashCase(block.Type, U"oclu.Struct")    return xcomp::OutputBlock::BlockType::Struct;
    HashCase(block.Type, U"oclu.Kernel")    return xcomp::OutputBlock::BlockType::Instance;
    HashCase(block.Type, U"oclu.Template")  return xcomp::OutputBlock::BlockType::Template;
    default:                                break;
    }
    return XCNLRuntime::GetBlockType(block, metas);
}

void NLCLRuntime::HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const FuncPack& meta, const Arg*)
{
    using xcomp::InstanceArgInfo;
    auto& kerCtx = static_cast<KernelContext&>(ctx);
    const auto PrepareImgAccess = [&](std::u16string_view dst) 
    {
        switch (arg.Flag & (InstanceArgInfo::Flags::Read | InstanceArgInfo::Flags::Write))
        {
        case InstanceArgInfo::Flags::Read | InstanceArgInfo::Flags::Write:
            if (Context.Device->Version < 20)
                oclLog().warning(u"{} [{}]: image object does not support readwrite before CL2.0.\n", dst, arg.Name);
            return ImgAccess::ReadWrite;
        case InstanceArgInfo::Flags::Read:
            return ImgAccess::ReadOnly;
        case InstanceArgInfo::Flags::Write:
            return ImgAccess::WriteOnly;
        case InstanceArgInfo::Flags::Empty:
            if (Context.Device->Version < 20)
            {
                oclLog().warning(u"{} [{}]: image object does not support readwrite before CL2.0, defaults to readonly.\n", dst, arg.Name);
                return ImgAccess::ReadOnly;
            }
            else
                return ImgAccess::ReadWrite;
        default:
            return ImgAccess::None;
        }
    };
    switch (arg.Type)
    {
    case InstanceArgInfo::Types::RawBuf:
    {
        KerArgSpace space = KerArgSpace::Global;
        KerArgFlag flag = KerArgFlag::None;
        if (!HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write))
            flag |= KerArgFlag::Const;
        if (HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Restrict))
            flag |= KerArgFlag::Restrict;
        std::u32string unknwonExtra;
        for (const auto extra : arg.Extra)
        {
            switch (common::DJBHash::HashC(extra))
            {
            HashCase(extra, U"volatile")    flag |= KerArgFlag::Volatile; break;
            HashCase(extra, U"global")      space = KerArgSpace::Global; break;
            HashCase(extra, U"local")       space = KerArgSpace::Local; break;
            HashCase(extra, U"private")     NLRT_THROW_EX(FMTSTR(u"BufArg [{}] cannot be in private space"sv, arg.Name), meta); break;
            default:                        unknwonExtra.append(extra).append(U"], ["); break;
            }
        }
        if (!unknwonExtra.empty())
        {
            Expects(unknwonExtra.size() > 4);
            unknwonExtra.resize(unknwonExtra.size() - 4);
            oclLog().warning(u"BufArg [{}] has unrecoginzed flags: [{}].\n", arg.Name, unknwonExtra);
        }
        kerCtx.AddArg(KerArgType::Buffer, space, ImgAccess::None, flag,
            common::str::to_string(arg.Name,     Charset::UTF8, Charset::UTF32),
            common::str::to_string(arg.DataType, Charset::UTF8, Charset::UTF32) + '*');
    } return;
    case InstanceArgInfo::Types::TypedBuf:
    {
        if (!Context.Device->SupportImage)
            NLRT_THROW_EX(FMTSTR(u"Device [{}] does not support TypedArg [{}]"sv, Context.Device->Name, arg.Name), meta);
        const auto access = PrepareImgAccess(u"TypedArg");
        std::u32string unknwonExtra;
        for (const auto extra : arg.Extra)
        {
            unknwonExtra.append(extra).append(U"], [");
        }
        if (!unknwonExtra.empty())
        {
            Expects(unknwonExtra.size() > 4);
            unknwonExtra.resize(unknwonExtra.size() - 4);
            oclLog().warning(u"TypedArg [{}] has unrecoginzed flags: [{}].\n", arg.Name, unknwonExtra);
        }
        kerCtx.AddArg(KerArgType::Image, KerArgSpace::Global, access, KerArgFlag::None,
            common::str::to_string(arg.Name, Charset::UTF8, Charset::UTF32), "image1d_buffer_t"sv);
    } return;
    case InstanceArgInfo::Types::Simple:
    {
        KerArgSpace space = KerArgSpace::Global;
        KerArgFlag flag = KerArgFlag::None;
        if (!HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write))
            flag |= KerArgFlag::Const;
        std::u32string unknwonExtra;
        for (const auto extra : arg.Extra)
        {
            switch (common::DJBHash::HashC(extra))
            {
            HashCase(extra, U"global")      space = KerArgSpace::Global; break;
            HashCase(extra, U"local")       space = KerArgSpace::Local; break;
            HashCase(extra, U"private")     space = KerArgSpace::Private; break;
            default:                        unknwonExtra.append(extra).append(U"], ["); break;
            }
        }
        if (!unknwonExtra.empty())
        {
            Expects(unknwonExtra.size() > 4);
            unknwonExtra.resize(unknwonExtra.size() - 4);
            oclLog().warning(u"SimpleArg [{}] has unrecoginzed flags: [{}].\n", arg.Name, unknwonExtra);
        }
        kerCtx.AddArg(KerArgType::Simple, space, ImgAccess::None, flag,
            common::str::to_string(arg.Name,     Charset::UTF8, Charset::UTF32),
            common::str::to_string(arg.DataType, Charset::UTF8, Charset::UTF32));
    } return;
    case InstanceArgInfo::Types::Texture:
    {
        if (!Context.Device->SupportImage)
            NLRT_THROW_EX(FMTSTR(u"Device [{}] does not support ImgArg [{}]"sv, Context.Device->Name, arg.Name), meta);
        const auto access = PrepareImgAccess(u"ImgArg");
        std::string_view tname;
        switch (arg.TexType)
        {
        case InstanceArgInfo::TexTypes::Tex1D:      tname = "image1d_t"sv; break;
        case InstanceArgInfo::TexTypes::Tex2D:      tname = "image2d_t"sv; break;
        case InstanceArgInfo::TexTypes::Tex3D:      tname = "image3d_t"sv; break;
        case InstanceArgInfo::TexTypes::Tex1DArray: tname = "image1d_array_t"sv; break;
        case InstanceArgInfo::TexTypes::Tex2DArray: tname = "image2d_array_t"sv; break;
        default: NLRT_THROW_EX(FMTSTR(u"ImgArg [{}] has unsupported type"sv, arg.Name), meta); break;
        }
        std::u32string unknwonExtra;
        for (const auto extra : arg.Extra)
        {
            unknwonExtra.append(extra).append(U"], [");
        }
        if (!unknwonExtra.empty())
        {
            Expects(unknwonExtra.size() > 4);
            unknwonExtra.resize(unknwonExtra.size() - 4);
            oclLog().warning(u"ImgArg [{}] has unrecoginzed flags: [{}].\n", arg.Name, unknwonExtra);
        }
        kerCtx.AddArg(KerArgType::Image, KerArgSpace::Global, access, KerArgFlag::None,
            common::str::to_string(arg.Name,     Charset::UTF8, Charset::UTF32), tname);
    } return;
    default:
        return;
    }
}

void NLCLRuntime::BeforeFinishOutput(std::u32string& prefix, std::u32string&)
{
    std::u32string exts = U"/* Extensions */\r\n"s;
    // Output extensions
    if (common::linq::FromIterable(Context.EnabledExtensions).All(true))
    {
        exts.append(U"#pragma OPENCL EXTENSION all : enable\r\n"sv);
    }
    else
    {
        common::linq::FromIterable(Context.EnabledExtensions).Pair(common::linq::FromIterable(Context.Device->Extensions))
            .ForEach([&](const auto& pair)
                {
                    if (pair.first)
                        APPEND_FMT(exts, U"#pragma OPENCL EXTENSION {} : enable\r\n"sv, pair.second);
                });
    }
    exts.append(U"\r\n"sv);
    prefix.insert(prefix.begin(), exts.begin(), exts.end());
}

NLCLRuntime::VecTypeResult NLCLRuntime::TryParseVecType(const std::u32string_view type) const noexcept
{
    auto [info, least] = xcomp::ParseVDataType(type);
    if (info.Bit == 0)
        return { info, false };
    bool typeSupport = true;
    if (info.Type == common::simd::VecDataInfo::DataTypes::Float) // FP ext handling
    {
        if (info.Bit == 16 && !Context.SupportFP16) // FP16 check
        {
            if (least) // promotion
                info.Bit = 32;
            else
                typeSupport = false;
        }
        else if (info.Bit == 64 && !Context.SupportFP64)
        {
            typeSupport = false;
        }
    }
    return { info, typeSupport };
}
std::u32string_view NLCLRuntime::GetVecTypeName(common::simd::VecDataInfo info) const noexcept
{
    return NLCLRuntime::GetCLTypeName(info);
}

bool NLCLRuntime::EnableExtension(std::string_view ext, std::u16string_view desc)
{
    if (ext == "all"sv)
        Context.EnabledExtensions.assign(Context.EnabledExtensions.size(), true);
    else if (const auto idx = Context.Device->Extensions.GetIndex(ext); idx != SIZE_MAX)
        Context.EnabledExtensions[idx] = true;
    else
    {
        if (!desc.empty())
            Logger.warning(FMT_STRING(u"{} on unsupported device without [{}].\n"sv), desc, ext);
        return false;
    }
    return true;
}

bool NLCLRuntime::EnableExtension(std::u32string_view ext, std::u16string_view desc)
{
    struct TmpSv : public common::str::HashedStrView<char32_t>
    {
        using common::str::HashedStrView<char32_t>::operator==;
        constexpr bool operator==(std::string_view other) const noexcept
        {
            if (View.size() != other.size()) return false;
            for (size_t i = 0; i < View.size(); ++i)
                if (View[i] != static_cast<char32_t>(other[i]))
                    return false;
            return true;
        }
    };
    if (ext == U"all"sv)
        Context.EnabledExtensions.assign(Context.EnabledExtensions.size(), true);
    else if (const auto idx = Context.Device->Extensions.GetIndex(TmpSv{ext}); idx != SIZE_MAX)
        Context.EnabledExtensions[idx] = true;
    else
    {
        if (!desc.empty())
            Logger.warning(FMT_STRING(u"{} on unsupported device without [{}].\n"sv), desc, ext);
        return false;
    }
    return true;
}



NLCLBaseResult::NLCLBaseResult(const std::shared_ptr<NLCLContext>& context) :
    Runtime(context)
{ }
NLCLBaseResult::~NLCLBaseResult()
{ }
NLCLResult::ResultType NLCLBaseResult::QueryResult(std::u32string_view name) const
{
    const auto result = Runtime.EvaluateRawStatement(name, false);
    return result.Visit([](auto val) -> NLCLResult::ResultType
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>
                || std::is_same_v<T, double> || std::is_same_v<T, std::u32string_view>)
                return val;
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
                return {};
            else // var
                return std::any{};
        });
}

NLCLUnBuildResult::NLCLUnBuildResult(const std::shared_ptr<NLCLContext>& context, std::string&& source) :
    NLCLBaseResult(context), Source(std::move(source))
{ }
NLCLUnBuildResult::~NLCLUnBuildResult()
{ }
std::string_view NLCLUnBuildResult::GetNewSource() const noexcept
{
    return Source;
}
oclProgram NLCLUnBuildResult::GetProgram() const
{
    COMMON_THROW(common::BaseException, u"Program has not been built!");
}

NLCLBuiltResult::NLCLBuiltResult(const std::shared_ptr<NLCLContext>& context, oclProgram prog) :
    NLCLBaseResult(context), Prog(std::move(prog))
{ }
NLCLBuiltResult::~NLCLBuiltResult()
{ }
std::string_view NLCLBuiltResult::GetNewSource() const noexcept
{
    return Prog->GetSource();
}
oclProgram NLCLBuiltResult::GetProgram() const
{
    return Prog;
}

NLCLBuildFailResult::NLCLBuildFailResult(const std::shared_ptr<NLCLContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex) :
    NLCLUnBuildResult(context, std::move(source)), Exception(std::move(ex))
{ }
NLCLBuildFailResult::~NLCLBuildFailResult()
{ }
oclProgram NLCLBuildFailResult::GetProgram() const
{
    Exception->ThrowReal();
    return {};
}


NLCLProgStub::NLCLProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLCLContext> context,
    std::unique_ptr<NLCLRuntime>&& runtime) : XCNLProgStub(program, std::move(context), std::move(runtime))
{ }
NLCLProgStub::NLCLProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLCLContext> context,
    common::mlog::MiniLogger<false>& logger) : NLCLProgStub(program, std::move(context), std::make_unique<NLCLRuntime>(logger, context))
{ }


NLCLResult::~NLCLResult()
{ }


NLCLProcessor::NLCLProcessor() : TheLogger(&oclLog())
{ }
NLCLProcessor::NLCLProcessor(common::mlog::MiniLogger<false>&& logger) : TheLogger(std::move(logger)) 
{ }
NLCLProcessor::~NLCLProcessor()
{ }

void NLCLProcessor::ConfigureCL(NLCLProgStub& stub) const
{
    constexpr std::u32string_view preapres[] = { U"xcomp.Prepare"sv, U"oclu.Prepare"sv };
    stub.Prepare(preapres);
    constexpr std::u32string_view collects[] = { U"oclu."sv, U"xcomp."sv };
    stub.Collect(collects);
}
std::string NLCLProcessor::GenerateCL(NLCLProgStub& stub) const
{
    auto str = stub.Runtime->GenerateOutput();
    constexpr std::u32string_view postacts[] = { U"xcomp.PostAct"sv, U"oclu.PostAct"sv };
    stub.PostAct(postacts);
    return str;
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileIntoProgram(NLCLProgStub& stub, const oclContext& ctx, CLProgConfig config) const
{
    auto str = GenerateCL(stub);
    try
    {
        auto& nlclCtx = *stub.GetContext().get();
        auto progStub = oclProgram_::Create(ctx, str, nlclCtx.Device);
        if (const auto dbgMan = debug::ExtractDebugManager(nlclCtx); dbgMan)
        {
            progStub.DebugMan = std::make_shared<xcomp::debug::DebugManager>(std::move(*dbgMan));
        }
        progStub.ImportedKernelInfo = std::move(stub.GetContext()->CompiledKernels);
        for (const auto& flag : nlclCtx.CompilerFlags)
        {
            config.Flags.insert(flag);
        }
        progStub.Build(config);
        return std::make_unique<NLCLBuiltResult>(stub.GetContext(), progStub.Finish());
    }
    catch (const common::BaseException& be)
    {
        return std::make_unique<NLCLBuildFailResult>(stub.GetContext(), std::move(str), be.InnerInfo());
    }
}

std::shared_ptr<xcomp::XCNLProgram> NLCLProcessor::Parse(common::span<const std::byte> source, std::u16string fileName) const
{
    auto& logger = Logger();
    const auto encoding = common::str::DetectEncoding(source);
    logger.info(u"File[{}], detected encoding[{}].\n", fileName, common::str::getCharsetName(encoding));
    auto src = common::str::to_u32string(source, encoding);
    logger.info(u"Translate into [utf32] for [{}] chars.\n", src.size());
    const auto prog = xcomp::XCNLProgram::Create(std::move(src), std::move(fileName));
    logger.verbose(u"Parse finished, get [{}] Blocks.\n", prog->GetProgram().Size());
    return prog;
}

std::unique_ptr<NLCLResult> NLCLProcessor::ProcessCL(const std::shared_ptr<xcomp::XCNLProgram>& prog, const oclDevice dev, const common::CLikeDefines& info) const
{
    NLCLProgStub stub(prog, std::make_shared<NLCLContext>(dev, info), Logger());
    ConfigureCL(stub);
    return std::make_unique<NLCLUnBuildResult>(stub.GetContext(), GenerateCL(stub));
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileProgram(const std::shared_ptr<xcomp::XCNLProgram>& prog, const oclContext& ctx, oclDevice dev, const common::CLikeDefines& info, const oclu::CLProgConfig& config) const
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    NLCLProgStub stub(prog, std::make_shared<NLCLContext>(dev, info), Logger());
    ConfigureCL(stub);
    return CompileIntoProgram(stub, ctx, config);
}


}