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
using xziar::nailang::RawArg;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::BlockContent;
using xziar::nailang::FuncCall;
using xziar::nailang::ArgLimits;
using xziar::nailang::FrameFlags;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::mlog::LogLevel;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


TemplateBlockInfo::TemplateBlockInfo(common::span<const RawArg> args)
{
    ReplaceArgs.reserve(args.size());
    for (const auto& arg : args)
    {
        ReplaceArgs.emplace_back(*arg.GetVar<RawArg::Type::Var>());
    }
}


bool KernelContext::Add(std::vector<NamedText>& container, const std::u32string_view id, std::u32string_view content)
{
    for (const auto& item : container)
        if (item.ID == id)
            return false;
    container.push_back(NamedText{ std::u32string(id), std::u32string(content) });
    return true;
}


KernelExtension::KernelExtension(NLCLContext& context) : Context(context), ID(UINT32_MAX)
{ }
KernelExtension::~KernelExtension() { }


NLCLExtension::~NLCLExtension() { }


static uint32_t RegId() noexcept
{
    static std::atomic<uint32_t> ID = 0;
    return ID++;
}
static auto& KerExtGenVector() noexcept
{
    static std::vector<std::pair<NLCLExtension::KerExtGen, uint32_t>> container;
    return container;
}
static auto& NLCLExtGenVector() noexcept
{
    static std::vector< std::pair<NLCLExtension::NLCLExtGen, uint32_t>> container;
    return container;
}
uint32_t NLCLExtension::RegisterKernelExtenstion(KerExtGen creator) noexcept
{
    const auto id = RegId();
    KerExtGenVector().emplace_back(std::move(creator), id);
    return id;
}
uint32_t NLCLExtension::RegisterNLCLExtenstion(NLCLExtGen creator) noexcept
{
    const auto id = RegId();
    NLCLExtGenVector().emplace_back(std::move(creator), id);
    return id;
}


NLCLContext::NLCLContext(oclDevice dev, const common::CLikeDefines& info) :
    Device(dev),
    SupportFP16(Device->Extensions.Has("cl_khr_fp16")),
    SupportFP64(Device->Extensions.Has("cl_khr_fp64") || Device->Extensions.Has("cl_amd_fp64")),
    SupportNVUnroll(Device->Extensions.Has("cl_nv_pragma_unroll")),
    SupportSubgroupKHR(Device->Extensions.Has("cl_khr_subgroups")),
    SupportSubgroupIntel(Device->Extensions.Has("cl_intel_subgroups")),
    SupportSubgroup8Intel(SupportSubgroupIntel&& Device->Extensions.Has("cl_intel_subgroups_char")),
    SupportSubgroup16Intel(SupportSubgroupIntel&& Device->Extensions.Has("cl_intel_subgroups_short")),
    SupportBasicSubgroup(SupportSubgroupKHR || SupportSubgroupIntel),
    EnabledExtensions(Device->Extensions.Size()),
    EnableUnroll(Device->CVersion >= 20 || SupportNVUnroll), AllowDebug(false)
{
    AllowDebug = info["debug"].has_value();
    for (const auto [key, val] : info)
    {
        const auto varName = common::str::to_u32string(key, Charset::UTF8);
        const auto var = xziar::nailang::LateBindVar::CreateSimple(varName);
        switch (val.index())
        {
        case 1: SetArg(var, std::get<1>(val), true); break;
        case 2: SetArg(var, std::get<2>(val), true); break;
        case 3: SetArg(var, std::get<3>(val), true); break;
        case 4: SetArg(var, common::str::to_u32string(std::get<4>(val), Charset::UTF8), true); break;
        case 0:
        default:
            break;
        }
    }
}
NLCLContext::~NLCLContext()
{ }

Arg NLCLContext::LookUpCLArg(const xziar::nailang::LateBindVar& var) const
{
    Expects(var.PartCount > 1);
    if (var[1] == U"Extension"sv)
    {
        if (var.PartCount != 3)
            return {};
        const auto extName = common::str::to_string(var[2], Charset::UTF8, Charset::UTF32);
        return Device->Extensions.Has(extName);
    }
    if (var[1] == U"Dev"sv)
    {
        if (var.PartCount != 3)
            return {};
        const auto propName = var[2];
        switch (hash_(propName))
        {
#define UINT_PROP(name) HashCase(propName, U ## #name) return static_cast<uint64_t>(Device->name)
        UINT_PROP(LocalMemSize);
        UINT_PROP(GlobalMemSize);
        UINT_PROP(GlobalCacheSize);
        UINT_PROP(GlobalCacheLine);
        UINT_PROP(ConstantBufSize);
        UINT_PROP(MaxMemAllocSize);
        UINT_PROP(ComputeUnits);
        UINT_PROP(WaveSize);
        UINT_PROP(Version);
        UINT_PROP(CVersion);
#undef UINT_PROP
#define BOOL_PROP(name) HashCase(propName, U ## #name) return static_cast<uint64_t>(Device->name)
        BOOL_PROP(SupportImage);
        BOOL_PROP(LittleEndian);
#undef BOOL_PROP
        HashCase(propName, U"Type")
        {
            switch (Device->Type)
            {
            case DeviceType::Accelerator:   return U"accelerator"sv;
            case DeviceType::CPU:           return U"cpu"sv;
            case DeviceType::GPU:           return U"gpu"sv;
            case DeviceType::Custom:        return U"custom"sv;
            default:                        return U"other"sv;
            }
        }
        HashCase(propName, U"Vendor")
        {
            switch (Device->PlatVendor)
            {
#define U_VENDOR(name) case Vendors::name: return PPCAT(PPCAT(U, STRINGIZE(name)), sv)
            U_VENDOR(AMD);
            U_VENDOR(ARM);
            U_VENDOR(Intel);
            U_VENDOR(NVIDIA);
            U_VENDOR(Qualcomm);
#undef U_VENDOR
            default:    return U"Other"sv;
            }
        }
        default: return {};
        }
    }
    return {};
}

Arg NLCLContext::LookUpArg(const xziar::nailang::LateBindVar& var) const
{
    if (var[0] == U"oclu"sv && var.PartCount > 1)
        return LookUpCLArg(var);
    return CompactEvaluateContext::LookUpArg(var);
}

NLCLContext::VecTypeResult NLCLContext::ParseVecType(const std::u32string_view type) const noexcept
{
    auto [info, least] = ParseVDataType(type);
    if (info.Bit == 0)
        return { info, false };
    bool typeSupport = true;
    if (info.Type == common::simd::VecDataInfo::DataTypes::Float) // FP ext handling
    {
        if (info.Bit == 16 && !SupportFP16) // FP16 check
        {
            if (least) // promotion
                info.Bit = 32;
            else
                typeSupport = false;
        }
        else if (info.Bit == 64 && !SupportFP64)
        {
            typeSupport = false;
        }
    }
    return { info, typeSupport };
}

std::u32string_view NLCLContext::GetCLTypeName(common::simd::VecDataInfo info) noexcept
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



NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx) :
    NailangRuntimeBase(evalCtx), Context(*evalCtx), Logger(logger)
{ }
NLCLRuntime::~NLCLRuntime()
{ }

void NLCLRuntime::InnerLog(common::mlog::LogLevel level, std::u32string_view str)
{
    Logger.log(level, u"[NLCL]{}\n", str);
}

void NLCLRuntime::HandleException(const xziar::nailang::NailangParseException& ex) const
{
    Logger.error(u"{}\n", ex.Message());
    ReplaceEngine::HandleException(ex);
}

void NLCLRuntime::HandleException(const NailangRuntimeException& ex) const
{
    Logger.error(u"{}\n", ex.Message());
    NailangRuntimeBase::HandleException(ex);
}

xziar::nailang::Arg NLCLRuntime::LookUpArg(const xziar::nailang::LateBindVar& var) const
{
    if (var[0] == U"oclu"sv && var.PartCount > 1)
        return Context.LookUpCLArg(var);
    return NailangRuntimeBase::LookUpArg(var);
}

void NLCLRuntime::ThrowByReplacerArgCount(const std::u32string_view call, const common::span<const std::u32string_view> args,
    const size_t count, const ArgLimits limit) const
{
    std::u32string_view prefix;
    switch (limit)
    {
    case ArgLimits::Exact:
        if (args.size() == count) return;
        prefix = U""; break;
    case ArgLimits::AtMost:
        if (args.size() <= count) return;
        prefix = U"at most"; break;
    case ArgLimits::AtLeast:
        if (args.size() >= count) return;
        prefix = U"at least"; break;
    }
    NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [{}] requires {} [{}] args, which gives [{}]."sv, call, prefix, count, args.size()));
}

common::simd::VecDataInfo NLCLRuntime::ParseVecType(const std::u32string_view var,
    std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo) const
{
    const auto vtype = Context.ParseVecType(var);
    if (!vtype)
    {
        switch (extraInfo.index())
        {
        case 0:
        {
            const auto str = std::get<0>(extraInfo);
            if (str.size() == 0)
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType."sv, var));
            else
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType when {}."sv, var, str));
        } break;
        case 1:
        {
            const auto str = std::get<1>(extraInfo)();
            NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType, {}."sv, var, str));
        } break;
        }
    }
    if (!vtype.TypeSupport)
        Logger.warning(u"Potential use of unsupported type[{}] with [{}].\n", StringifyVDataType(vtype.Info), var);

    return vtype.Info;
}

void NLCLRuntime::OnReplaceOptBlock(std::u32string& output, void*, const std::u32string_view cond, const std::u32string_view content)
{
    if (cond.empty())
    {
        NLRT_THROW_EX(u"Empty condition occurred when replace-opt-block"sv);
        return;
    }
    Arg ret;
    if (cond.back() == U';')
    {
        ret = EvaluateRawStatement(cond);
    }
    else
    {
        std::u32string str(cond);
        str.push_back(U';');
        ret = EvaluateRawStatement(str);
    }
    if (ret.IsEmpty())
    {
        NLRT_THROW_EX(u"No value returned as condition when replace-opt-block"sv);
        return;
    }
    else if (!HAS_FIELD(ret.TypeData, Arg::Type::Boolable))
    {
        NLRT_THROW_EX(u"Evaluated condition is not boolable when replace-opt-block"sv);
        return;
    }
    else if (ret.GetBool().value())
    {
        output.append(content);
    }
}

void NLCLRuntime::OnReplaceVariable(std::u32string& output, [[maybe_unused]] void* cookie, const std::u32string_view var)
{
    if (var.size() > 0 && var[0] == U'@')
    {
        const auto vtype = ParseVecType(var.substr(1), u"replace-variable"sv);
        const auto str = NLCLContext::GetCLTypeName(vtype);
        if (!str.empty())
            output.append(str);
        else
            NLRT_THROW_EX(FMTSTR(u"Type [{}] not supported when replace-variable"sv, var));
    }
    else // '@' not allowed in var anyway
    {
        const auto var_ = DecideDynamicVar(var, u"ReplaceVariable"sv);
        const auto ret = LookUpArg(var_);
        if (ret.IsEmpty() || ret.TypeData == Arg::Type::Var)
        {
            NLRT_THROW_EX(FMTSTR(u"Arg [{}] not found when replace-variable"sv, var));
            return;
        }
        output.append(ret.ToString().StrView());
    }
}

void NLCLRuntime::OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtMost);
        if (Context.EnableUnroll)
        {
            if (Context.Device->CVersion >= 20 || !Context.SupportNVUnroll)
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
    const auto kerCk = BlockCookie::Get<KernelCookie>(cookie);
    if (IsBeginWith(func, U"oclu."sv))
    {
        const auto subName = func.substr(5);
        switch (hash_(subName))
        {
        HashCase(subName, U"GetVecTypeName")
        {
            ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
            const auto vname = args[0];
            const auto vtype = ParseVecType(vname, u"call [GetVecTypeName]"sv);
            const auto str = NLCLContext::GetCLTypeName(vtype);
            if (!str.empty())
                output.append(str);
            else
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not supported"sv, vname));
        } return;
        HashCase(subName, U"CodeBlock")
        {
            ThrowByReplacerArgCount(func, args, 1, ArgLimits::AtLeast);
            const OutputBlock* block = nullptr;
            for (const auto& blk : Context.TemplateBlocks)
                if (blk.Block->Name == args[0])
                {
                    block = &blk; break;
                }
            if (block)
            {
                const auto& extra = static_cast<const TemplateBlockInfo&>(*block->ExtraInfo);
                ThrowByReplacerArgCount(func, args, extra.ReplaceArgs.size() + 1, ArgLimits::AtLeast);
                auto frame = PushFrame(extra.ReplaceArgs.size() > 0, FrameFlags::FlowScope);
                for (size_t i = 0; i < extra.ReplaceArgs.size(); ++i)
                {
                    const auto var_ = DecideDynamicVar(extra.ReplaceArgs[i], u"CodeBlock"sv);
                    {
                        using xziar::nailang::LateBindVar;
                        auto& var2 = const_cast<LateBindVar&>(var_.Get());
                        var2.Info() |= LateBindVar::VarInfo::Local;
                    }
                    const auto ret = LookUpArg(var_);
                    SetArg(var_, args[i + 1]);
                }
                APPEND_FMT(output, U"// template block [{}]\r\n"sv, args[0]);
                DirectOutput(*block->Block, block->MetaFunc, output, reinterpret_cast<BlockCookie*>(cookie));
            }
        } return;
        default: break;
        }
    }
    
    const auto CheckExtension = [&, runtime=this](KernelExtension& ext)
    {
        const auto ret = ext.ReplaceFunc(*runtime, func, args);
        if (ret)
        {
            output.append(ret.GetStr());
            return true;
        }
        else
        {
            const auto str = ret.GetStr();
            if (!ret.CheckAllowFallback())
                NLRT_THROW_EX(FMTSTR(u"replace-func [{}] with [{}]args error: {}", func, args.size(), str));
            if (!str.empty())
                Logger.warning(FMT_STRING(u"when replace-func [{}]: {}"), func, str);
        }
        return false;
    };
    for (const auto& ext : Context.NLCLExts)
    {
        if (!ext) continue;
        if (CheckExtension(*ext))
            return;
    }
    if (kerCk)
    {
        for (const auto& ext : kerCk->Extensions)
        {
            if (!ext) continue;
            if (CheckExtension(*ext))
                return;
        }
    }
    NLRT_THROW_EX(FMTSTR(u"replace-func [{}] with [{}]args is not resolved", func, args.size()));
}

void NLCLRuntime::OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas)
{
    ProcessRawBlock(block, metas);
}

constexpr auto LogLevelParser = SWITCH_PACK(Hash, 
    (U"error",   common::mlog::LogLevel::Error), 
    (U"success", common::mlog::LogLevel::Success),
    (U"warning", common::mlog::LogLevel::Warning),
    (U"info",    common::mlog::LogLevel::Info),
    (U"verbose", common::mlog::LogLevel::Verbose), 
    (U"debug",   common::mlog::LogLevel::Debug));
std::optional<common::mlog::LogLevel> ParseLogLevel(const Arg& arg) noexcept
{
    const auto str = arg.GetStr();
    if (str)
        return LogLevelParser(str.value());
    return {};
}
Arg NLCLRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas, const FuncTargetType target)
{
    if (call.Name->PartCount == 2 && (*call.Name)[0] == U"oclu"sv)
    {
        switch (const auto subName = (*call.Name)[1]; hash_(subName))
        {
        HashCase(subName, U"CompilerFlag")
        {
            const auto arg  = EvaluateFuncArgs<1>(call, { Arg::Type::String })[0];
            const auto flag = common::str::to_string(arg.GetStr().value(), Charset::UTF8, Charset::UTF32);
            for (const auto& item : Context.CompilerFlags)
            {
                if (item == flag)
                    return {};
            }
            Context.CompilerFlags.push_back(flag);
        } return {};
        HashCase(subName, U"GetVecTypeName")
        {
            const auto arg = EvaluateFuncArgs<1>(call, { Arg::Type::String })[0];
            const auto vname = arg.GetStr().value();
            const auto vtype = ParseVecType(vname, u"call [GetVecTypeName]"sv);
            const auto str = NLCLContext::GetCLTypeName(vtype);
            if (!str.empty())
                return str;
            else
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not supported"sv, vname));
        } return {};
        HashCase(subName, U"EnableExtension")
        {
            // ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            const auto arg = EvaluateFuncArgs<1>(call, { Arg::Type::String })[0];
            if (const auto sv = arg.GetStr(); sv.has_value())
            {
                if (EnableExtension(sv.value()))
                    return true;
                else
                    Logger.warning(u"Extension [{}] not found in support list, skipped.\n", sv.value());
            }
            else
                NLRT_THROW_EX(u"Arg of [EnableExtension] should be string"sv, call);
        } return false;
        HashCase(subName, U"EnableUnroll")
        {
            ThrowIfNotFuncTarget(*call.Name, target, FuncTargetType::Plain);
            if (!Context.EnableUnroll)
            {
                Logger.warning(u"Manually enable unroll hint.\n");
                Context.EnableUnroll = true;
            }
        } return {};
        HashCase(subName, U"Log")
        {
            ThrowIfNotFuncTarget(*call.Name, target, FuncTargetType::Plain);
            const auto args2 = EvaluateFuncArgs<2, ArgLimits::AtLeast>(call, { Arg::Type::String, Arg::Type::String });
            const auto logLevel = ParseLogLevel(args2[0]);
            if (!logLevel) 
                NLRT_THROW_EX(u"Arg[0] of [Log] should be LogLevel"sv, call);
            const auto fmtStr = args2[1].GetStr().value();
            if (call.Args.size() == 2)
                InnerLog(logLevel.value(), fmtStr);
            else
            {
                try
                {
                    const auto str = FormatString(fmtStr, call.Args.subspan(2));
                    InnerLog(logLevel.value(), str);
                }
                catch (const xziar::nailang::NailangFormatException& nfe)
                {
                    Logger.error(u"Error when formating inner log: {}\n", nfe.Message());
                }
            }
        } return {};
        default: break;
        }
        for (const auto& ext : Context.NLCLExts)
        {
            auto ret = ext->NLCLFunc(*this, call, metas, target);
            if (ret)
                return std::move(ret.value());
        }
    }
    return NailangRuntimeBase::EvaluateFunc(call, metas, target);
}

void NLCLRuntime::DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst, BlockCookie* cookie)
{
    Expects(CurFrame != nullptr);
    OutputConditions(metas, dst);
    common::str::StrVariant<char32_t> source(block.Source);
    bool repVar = false, repFunc = false;
    for (const auto& fcall : metas)
    {
        if (*fcall.Name == U"oclu.ReplaceVariable"sv)
            repVar = true;
        else if (*fcall.Name == U"oclu.ReplaceFunction"sv)
            repFunc = true;
        else if (*fcall.Name == U"oclu.Replace"sv)
            repVar = repFunc = true;
        else if (*fcall.Name == U"oclu.PreAssign"sv)
        {
            ThrowByArgCount(fcall, 2, ArgLimits::Exact);
            ThrowByArgType(fcall, RawArg::Type::Var, 0);
            SetArg(*fcall.Args[0].GetVar<RawArg::Type::Var>(), EvaluateArg(fcall.Args[1]));
        }
    }
    if (repVar || repFunc)
        source = ProcessOptBlock(source.StrView(), U"$$@"sv, U"@$$"sv);
    if (repVar)
        source = ProcessVariable(source.StrView(), U"$$!{"sv, U"}"sv);
    if (repFunc)
        source = ProcessFunction(source.StrView(), U"$$!"sv, U""sv, cookie);
    dst.append(source.StrView());
}

void NLCLRuntime::OutputConditions(MetaFuncs metas, std::u32string& dst) const
{
    // std::set<std::u32string_view> lateVars;
    for (const auto& meta : metas)
    {
        std::u32string_view prefix;
        if (*meta.Name == U"If"sv)
            prefix = U"If "sv;
        else if (*meta.Name == U"Skip"sv)
            prefix = U"Skip if "sv;
        else
            continue;
        if (meta.Args.size() == 1)
            APPEND_FMT(dst, U"    /* {:7} :  {} */\r\n"sv, prefix, xziar::nailang::Serializer::Stringify(meta.Args[0]));
    }
}

void NLCLRuntime::OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    APPEND_FMT(dst, U"\r\n/* From GlobalBlock [{}] */\r\n"sv, block.Name);
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    APPEND_FMT(dst, U"\r\n/* From StructBlock [{}] */\r\n"sv, block.Name);
    dst.append(U"typedef struct \r\n{\r\n"sv);
    DirectOutput(block, metas, dst);
    APPEND_FMT(dst, U"}} {};\r\n"sv, block.Name);
}


constexpr auto KerArgSpaceParser = SWITCH_PACK(Hash, 
    (U"global",     KerArgSpace::Global), 
    (U"__global",   KerArgSpace::Global), 
    (U"constant",   KerArgSpace::Constant),
    (U"__constant", KerArgSpace::Constant),
    (U"local",      KerArgSpace::Local), 
    (U"__local",    KerArgSpace::Local), 
    (U"private",    KerArgSpace::Private),
    (U"__private",  KerArgSpace::Private),
    (U"",           KerArgSpace::Private));
constexpr auto ImgArgAccessParser = SWITCH_PACK(Hash, 
    (U"read_only",    ImgAccess::ReadOnly),
    (U"__read_only",  ImgAccess::ReadOnly),
    (U"write_only",   ImgAccess::WriteOnly),
    (U"__write_only", ImgAccess::WriteOnly),
    (U"read_write",   ImgAccess::ReadWrite),
    (U"",             ImgAccess::ReadWrite));


void NLCLRuntime::HandleKernelMeta(const FuncCall& meta, KernelContext& kerCtx, const std::vector<std::unique_ptr<KernelExtension>>& kerExts)
{
    if (meta.Name->PartCount == 2 && (*meta.Name)[0] == U"oclu"sv)
    {
        switch (const auto subName = (*meta.Name)[1]; hash_(subName))
        {
        HashCase(subName, U"RequestWorkgroupSize")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::Integer, Arg::Type::Integer, Arg::Type::Integer });
            const auto x = args[0].GetUint().value_or(1),
                y = args[1].GetUint().value_or(1),
                z = args[2].GetUint().value_or(1);
            kerCtx.WorkgroupSize = gsl::narrow_cast<uint32_t>(x * y * z);
            kerCtx.AddAttribute(U"ReqWGSize"sv, FMTSTR(U"__attribute__((reqd_work_group_size({}, {}, {})))"sv, x, y, z));
        } return;
        HashCase(subName, U"SimpleArg")
        {
            const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto space_ = args[0].GetStr().value();
            const auto space = KerArgSpaceParser(space_);
            if (!space) NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgSpace [{}]"sv, space_), &meta);
            const auto argType = args[1].GetStr().value();
            // const bool isPointer = !argType.empty() && argType.back() == U'*';
            const auto name = args[2].GetStr().value();
            const auto flags = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part)
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgFlag [{}]"sv, part),
                            &meta);
                        return flag;
                    }, KerArgFlag::None);
            kerCtx.AddArg(KerArgType::Simple, space.value(), ImgAccess::None, flags,
                common::str::to_string(name, Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return;
        HashCase(subName, U"BufArg")
        {
            const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto space_ = args[0].GetStr().value();
            const auto space = KerArgSpaceParser(space_);
            if (!space)
                NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgSpace [{}]"sv, space_), &meta);
            const auto argType = std::u32string(args[1].GetStr().value()) + U'*';
            const auto name = args[2].GetStr().value();
            const auto flags = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part)
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        if (part == U"restrict"sv)  return flag | KerArgFlag::Restrict;
                        if (part == U"volatile"sv)  return flag | KerArgFlag::Volatile;
                        NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgFlag [{}]"sv, part),
                            &meta);
                        return flag;
                    }, KerArgFlag::None);
            if (space.value() == KerArgSpace::Private)
                NLRT_THROW_EX(FMTSTR(u"BufArg [{}] cannot be in private space: [{}]"sv, name, space_), &meta);
            kerCtx.AddArg(KerArgType::Buffer, space.value(), ImgAccess::None, flags,
                common::str::to_string(name, Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return;
        HashCase(subName, U"ImgArg")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto access_ = args[0].GetStr().value();
            const auto access = ImgArgAccessParser(access_);
            if (!access) NLRT_THROW_EX(fmt::format(u"Unrecognized ImgAccess [{}]"sv, access_), &meta);
            const auto argType = args[1].GetStr().value();
            const auto name = args[2].GetStr().value();
            kerCtx.AddArg(KerArgType::Image, KerArgSpace::Global, access.value(), KerArgFlag::None,
                common::str::to_string(name, Charset::UTF8, Charset::UTF32),
                common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        } return;
        default:
            break;
        }
    }
    for (const auto& ext : Context.NLCLExts)
    {
        if (ext->KernelMeta(*this, meta, kerCtx))
            return;
    }
    for (const auto& ext : kerExts)
    {
        if (ext->KernelMeta(*this, meta, kerCtx))
            return;
    }
}

void NLCLRuntime::StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg)
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

void NLCLRuntime::OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    KernelCookie cookie;
    KernelContext& kerCtx = cookie.Context;
    for (const auto& [creator, id] : KerExtGenVector())
    {
        auto ext = creator(Logger, Context, kerCtx);
        if (ext)
        {
            ext->ID = id;
            cookie.Extensions.push_back(std::move(ext));
        }
    }

    for (const auto& meta : metas)
    {
        switch (const auto newMeta = HandleMetaIf(meta); newMeta.index())
        {
        case 0:
            HandleKernelMeta(meta, kerCtx, cookie.Extensions); break;
        case 2:
            HandleKernelMeta({ std::get<2>(newMeta).Ptr(), meta.Args.subspan(2) }, kerCtx, cookie.Extensions); break;
        default:
            break;
        }
    }
    std::u32string content;
    DirectOutput(block, metas, content, &cookie);
    for (const auto& ext : Context.NLCLExts)
    {
        ext->FinishKernel(*this, kerCtx);
    }
    for (const auto& ext : cookie.Extensions)
    {
        ext->FinishKernel(*this, kerCtx);
    }

    APPEND_FMT(dst, U"\r\n/* From KernelBlock [{}] */\r\n"sv, block.Name);
    // attributes
    for (const auto& item : kerCtx.Attributes)
    {
        dst.append(item.Content).append(U"\r\n"sv);
    }
    APPEND_FMT(dst, U"kernel void {} ("sv, block.Name);
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
    dst.pop_back(); // remove additional ','
    dst.append(U")\r\n{\r\n"sv);
    // prefixes
    for (const auto& item : kerCtx.BodyPrefixes)
    {
        APPEND_FMT(dst, U"    //vvvvvvvv below injected by {}  vvvvvvvv\r\n"sv, item.ID);
        dst.append(item.Content).append(U"\r\n"sv);
        APPEND_FMT(dst, U"    //^^^^^^^^ above injected by {}  ^^^^^^^^\r\n\r\n"sv, item.ID);
    }
    // content
    dst.append(content);
    // suffixes
    for (const auto& item : kerCtx.BodySuffixes)
    {
        APPEND_FMT(dst, U"    //vvvvvvvv below injected by {}  vvvvvvvv\r\n"sv, item.ID);
        dst.append(item.Content).append(U"\r\n"sv);
        APPEND_FMT(dst, U"    //^^^^^^^^ above injected by {}  ^^^^^^^^\r\n\r\n"sv, item.ID);
    }
    dst.append(U"}\r\n"sv);

    for (auto& arg : kerCtx.Args.ArgsInfo)
    {
        if (arg.ArgType == KerArgType::Simple)
            arg.Qualifier = REMOVE_MASK(arg.Qualifier, KerArgFlag::Const); // const will be ignored for simple arg
    }
    Context.CompiledKernels.emplace_back(
        common::str::to_string(block.Name, Charset::UTF8, Charset::UTF32), 
        std::move(kerCtx.Args));
}

void NLCLRuntime::OutputTemplateKernel(const RawBlock& block, [[maybe_unused]] MetaFuncs metas, 
    [[maybe_unused]] const OutputBlock::BlockInfo& extraInfo, std::u32string& dst)
{
    // TODO:
    APPEND_FMT(dst, U"\r\n/* From TempalteKernel [{}] */\r\n"sv, block.Name);
    NLRT_THROW_EX(u"TemplateKernel not supportted yet"sv,
        &block);
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
    struct TmpSv : public common::container::PreHashedStringView<>
    {
        using Hasher = common::container::DJBHash;
        std::u32string_view View;
        constexpr TmpSv(std::u32string_view sv) noexcept : 
            PreHashedStringView(Hasher()(sv)), View(sv) { }
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

void NLCLRuntime::ProcessRawBlock(const xziar::nailang::RawBlock& block, MetaFuncs metas)
{
    auto frame = PushFrame(FrameFlags::FlowScope);
    if (IsBeginWith(block.Type, U"oclu."sv) &&
        HandleMetaFuncs(metas, BlockContent::Generate(&block)))
    {
        bool output = false;
        switch (const auto subName = block.Type.substr(5); hash_(subName))
        {
        HashCase(subName, U"Global")     output = true;  break;
        HashCase(subName, U"Struct")     output = true;  Context.StructBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"Kernel")     output = true;  Context.KernelBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"KernelStub") output = false; Context.KernelStubBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"Template")
        {
            output = false;
            common::span<const RawArg> tpArgs;
            for (const auto& meta : metas)
            {
                if (*meta.Name == U"oclu.TemplateArgs")
                {
                    for (uint32_t i = 0; i < meta.Args.size(); ++i)
                    {
                        if (meta.Args[i].TypeData != RawArg::Type::Var)
                            NLRT_THROW_EX(FMTSTR(u"TemplateArgs's arg[{}] is [{}]. not [Var]"sv,
                                i, ArgTypeName(meta.Args[i].TypeData)), meta, &block);
                    }
                    tpArgs = meta.Args;
                    break;
                }
            }
            Context.TemplateBlocks.emplace_back(&block, metas, std::make_unique<TemplateBlockInfo>(tpArgs));
        } break;
        default: break;
        }
        if (output)
            Context.OutputBlocks.emplace_back(&block, metas);
    }
}

std::string NLCLRuntime::GenerateOutput()
{
    std::u32string prefix, output;

    for (const auto& item : Context.OutputBlocks)
    {
        auto frame = PushFrame(FrameFlags::FlowScope);
        switch (hash_(item.Block->Type))
        {
        HashCase(item.Block->Type, U"oclu.Global")     OutputGlobal(*item.Block, item.MetaFunc, output); continue;
        HashCase(item.Block->Type, U"oclu.Struct")     OutputStruct(*item.Block, item.MetaFunc, output); continue;
        HashCase(item.Block->Type, U"oclu.Kernel")     OutputKernel(*item.Block, item.MetaFunc, output); continue;
        HashCase(item.Block->Type, U"oclu.KernelStub") OutputTemplateKernel(*item.Block, item.MetaFunc, *item.ExtraInfo, output); continue;
        default: break;
        }
        Logger.warning(u"Unexpected RawBlock (Type[{}], Name[{}]) when generating output.\n", item.Block->Type, item.Block->Name); break;
    }
    for (const auto& ext : Context.NLCLExts)
    {
        ext->FinishNLCL(*this);
    }

    { // Output extentions
        prefix.append(U"/* Extensions */\r\n"sv);
        if (common::linq::FromIterable(Context.EnabledExtensions).All(true))
        {
            prefix.append(U"#pragma OPENCL EXTENSION all : enable\r\n"sv);
        }
        else
        {
            common::linq::FromIterable(Context.EnabledExtensions).Pair(common::linq::FromIterable(Context.Device->Extensions))
                .ForEach([&](const auto& pair)
                    {
                        if (pair.first)
                            APPEND_FMT(prefix, U"#pragma OPENCL EXTENSION {} : enable\r\n"sv, pair.second);
                    });
        }
        prefix.append(U"\r\n"sv);
    }

    { // Output patched blocks
        for (const auto& [id, src] : Context.PatchedBlocks)
        {
            APPEND_FMT(prefix, U"/* Patched Block [{}] */\r\n"sv, id);
            prefix.append(src);
            prefix.append(U"\r\n\r\n"sv);
        }
    }

    return common::str::to_string(prefix + output, Charset::UTF8, Charset::UTF32);
}



NLCLBaseResult::NLCLBaseResult(const std::shared_ptr<NLCLContext>& context) : Context(context)
{ }
NLCLBaseResult::~NLCLBaseResult()
{ }
NLCLResult::ResultType NLCLBaseResult::QueryResult(std::u32string_view name) const
{
    const auto var = xziar::nailang::LateBindVar::CreateTemp(name);
    auto result = Context->LookUpArg(var);
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


NLCLProgram::NLCLProgram(std::u32string&& source) :
    Source(std::move(source)) { }
NLCLProgram::~NLCLProgram()
{ }


NLCLProgStub::NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program, std::shared_ptr<NLCLContext>&& context, 
    std::unique_ptr<NLCLRuntime>&& runtime) : Program(program), Context(std::move(context)), Runtime(std::move(runtime))
{
    Prepare();
}
NLCLProgStub::NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program, const std::shared_ptr<NLCLContext> context, 
    common::mlog::MiniLogger<false>& logger) : Program(program), Context(context), 
    Runtime(std::make_unique<NLCLRuntime>(logger, context))
{ 
    Prepare();
}
NLCLProgStub::~NLCLProgStub()
{ }
void NLCLProgStub::Prepare()
{
    Context->NLCLExts.clear();
    for (const auto& [creator, id] : NLCLExtGenVector())
    {
        auto ext = creator(oclLog(), *Context, Program->Program);
        if (ext)
        {
            ext->ID = id;
            Context->NLCLExts.push_back(std::move(ext));
        }
    }
}


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
    // Prepare
    stub.Program->ForEachBlockType<true, true>(U"oclu.Prepare"sv,
        [&](const Block& block, common::span<const FuncCall> metas) 
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
    // Collect
    stub.Program->ForEachBlockType<false, false>(U"oclu."sv,
        [&](const RawBlock& block, common::span<const FuncCall> metas)
        {
            stub.Runtime->ProcessRawBlock(block, metas);
        });
    // PostAct
    stub.Program->ForEachBlockType<true, true>(U"oclu.PostAct"sv,
        [&](const Block& block, common::span<const FuncCall> metas)
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileIntoProgram(NLCLProgStub& stub, const oclContext& ctx, CLProgConfig config) const
{
    auto str = stub.Runtime->GenerateOutput();
    try
    {
        auto progStub = oclProgram_::Create(ctx, str, stub.Context->Device);
        for (const auto& ext : stub.Context->NLCLExts)
        {
            ext->OnCompile(progStub);
        }
        progStub.ImportedKernelInfo = std::move(stub.Context->CompiledKernels);
        for (const auto flag : stub.Context->CompilerFlags)
        {
            config.Flags.insert(flag);
        }
        progStub.Build(config);
        return std::make_unique<NLCLBuiltResult>(stub.Context, progStub.Finish());
    }
    catch (const common::BaseException& be)
    {
        return std::make_unique<NLCLBuildFailResult>(stub.Context, std::move(str), be.InnerInfo());
    }
}

std::shared_ptr<NLCLProgram> NLCLProcessor::Parse(common::span<const std::byte> source) const
{
    auto& logger = Logger();
    const auto encoding = common::str::DetectEncoding(source);
    logger.info(u"Detected encoding[{}].\n", common::str::getCharsetName(encoding));
    const auto prog = std::make_shared<NLCLProgram>(common::str::to_u32string(source, encoding));
    logger.info(u"Translate into [utf32] for [{}] chars.\n", prog->Source.size());
    NLCLParser::GetBlock(prog->MemPool, prog->Source, prog->Program);
    logger.verbose(u"Parse done, get [{}] Blocks.\n", prog->Program.Size());
    return prog;
}

std::unique_ptr<NLCLResult> NLCLProcessor::ProcessCL(const std::shared_ptr<NLCLProgram>& prog, const oclDevice dev, const common::CLikeDefines& info) const
{
    NLCLProgStub stub(prog, std::make_shared<NLCLContext>(dev, info), Logger());
    ConfigureCL(stub);
    return std::make_unique<NLCLUnBuildResult>(stub.Context, stub.Runtime->GenerateOutput());
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, oclDevice dev, const common::CLikeDefines& info, const oclu::CLProgConfig& config) const
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    NLCLProgStub stub(prog, std::make_shared<NLCLContext>(dev, info), Logger());
    ConfigureCL(stub);
    return CompileIntoProgram(stub, ctx, config);
}


}