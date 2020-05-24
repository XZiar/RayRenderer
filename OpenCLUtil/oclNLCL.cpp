#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "oclException.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"
#include "common/StrParsePack.hpp"

namespace oclu
{
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::BlockContent;
using xziar::nailang::FuncCall;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))


NLCLEvalContext::NLCLEvalContext(oclDevice dev) : Device(dev) 
{ }
NLCLEvalContext::~NLCLEvalContext() 
{ }

Arg NLCLEvalContext::LookUpCLArg(xziar::nailang::detail::VarLookup var) const
{
    if (var.Part() == U"Extension"sv)
    {
        const auto extName = common::strchset::to_string(var.Rest(), Charset::UTF8, Charset::UTF32LE);
        return Device->Extensions.Has(extName);
    }
    if (var.Part() == U"Dev"sv)
    {
        const auto propName = var.Rest();
        switch (hash_(propName))
        {
#define U_PROP(name) HashCase(propName, U ## #name) return static_cast<uint64_t>(Device->name)
        U_PROP(LocalMemSize);
        U_PROP(GlobalMemSize);
        U_PROP(GlobalCacheSize);
        U_PROP(GlobalCacheLine);
        U_PROP(ConstantBufSize);
        U_PROP(MaxMemSize);
        U_PROP(ComputeUnits);
        U_PROP(WaveSize);
        U_PROP(Version);
        U_PROP(CVersion);
#undef U_PROP
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

Arg oclu::NLCLEvalContext::LookUpArg(xziar::nailang::detail::VarHolder var) const
{
    if (IsBeginWith(var.GetFull(), U"oclu."sv))
        return LookUpCLArg(var.GetFull().substr(5));
    return CompactEvaluateContext::LookUpArg(var);
}


NLCLReplacer::NLCLReplacer(NLCLRuntime& runtime) : 
    Runtime(runtime), 
    SupportFP16(Runtime.Device->Extensions.Has("cl_khr_fp16")),
    SupportFP64(Runtime.Device->Extensions.Has("cl_khr_fp64") || Runtime.Device->Extensions.Has("cl_amd_fp64")),
    SupportNVUnroll(Runtime.Device->Extensions.Has("cl_nv_pragma_unroll")),
    SupportSubgroupKHR(Runtime.Device->Extensions.Has("cl_khr_subgroups")),
    SupportSubgroupIntel(Runtime.Device->Extensions.Has("cl_intel_subgroups")),
    SupportSubgroup8Intel(SupportSubgroupIntel && Runtime.Device->Extensions.Has("cl_intel_subgroups_char")),
    SupportSubgroup16Intel(SupportSubgroupIntel && Runtime.Device->Extensions.Has("cl_intel_subgroups_short")),
    EnableUnroll(Runtime.Device->CVersion >= 20 || SupportNVUnroll)
{ }
NLCLReplacer::~NLCLReplacer() { }

void NLCLReplacer::ThrowByArgCount(const std::u32string_view func, const common::span<std::u32string_view> args, const size_t count) const
{
    if (args.size() != count)
        Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
            fmt::format(FMT_STRING(u"Repace-func [{}] requires [{}] args, which gives [{}]."), func, args.size(), count)));
}

static std::u32string SkipDebug() noexcept
{
    return U"inline void oclu_SkipDebug() {}";
}

void NLCLReplacer::OnReplaceVariable(std::u32string& output, const std::u32string_view var)
{
    if (var.size() > 0 && var[0] == U'@')
    {
        const auto info = ParseVecType(var.substr(1));
        if (!info.has_value())
        {
            Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
                fmt::format(FMT_STRING(u"Type [{}] not found when replace-variable"sv), var)));
            return;
        }
        const auto str = GetVecTypeName(info.value());
        if (str.empty())
        {
            Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
                fmt::format(FMT_STRING(u"Type [{}] not found when replace-variable"sv), var)));
            return;
        }
        output.append(str);
    }
    else // '@' not allowed in var anyway
    {
        const auto ret = Runtime.EvalContext->LookUpArg(var);
        if (ret.IsEmpty() || ret.TypeData == Arg::InternalType::Var)
        {
            Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
                fmt::format(FMT_STRING(u"Arg [{}] not found when replace-variable"sv), var)));
            return;
        }
        output.append(ret.ToString().StrView());
    }
}

void NLCLReplacer::OnReplaceFunction(std::u32string& output, const std::u32string_view func, const common::span<std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        if (args.size() > 1)
        {
            Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
                fmt::format(FMT_STRING(u"Repace-func [unroll] requires [0,1] args, which gives [{}]."), args.size())));
        }
        else if (EnableUnroll)
        {
            if (Runtime.Device->CVersion >= 20 || !SupportNVUnroll)
            {
                if (args.empty())
                    output.append(U"__attribute__((opencl_unroll_hint))"sv);
                else
                    fmt::format_to(std::back_inserter(output), FMT_STRING(U"__attribute__((opencl_unroll_hint({})))"sv), args[0]);
            }
            else
            {
                if (args.empty())
                    output.append(U"#pragma unroll"sv);
                else
                    fmt::format_to(std::back_inserter(output), FMT_STRING(U"#pragma unroll {}"sv), args[0]);
            }
        }
        return;
    }
    if (IsBeginWith(func, U"oclu."sv))
    {
        switch (const auto subName = func.substr(5); hash_(subName))
        {
        HashCase(subName, U"SubgroupBroadcast")
        {
            output.append(GenerateSubgroupShuffle(args, false)); return;
        }
        HashCase(subName, U"SubgroupShuffle")
        {
            output.append(GenerateSubgroupShuffle(args, true)); return;
        }
        HashCase(subName, U"DebugString")
        {
            if (!Runtime.AllowDebug)
            {
                Runtime.AddPatchedBlock(U"SkipDebug"sv, SkipDebug);
                output.append(U"oclu_SkipDebug()"sv); 
            }
            else
            {
                output.append(GenerateDebugString(args));
            }
            return;
        }
        default: break;
        }
    }
    Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
        u"replace-function not ready"sv));
}

std::optional<common::simd::VecDataInfo> NLCLReplacer::ParseVecType(const std::u32string_view type) const noexcept
{
    auto [info, least] = ParseVDataType(type);
    if (info.Bit == 0)
        return {};
    if (info.Type == common::simd::VecDataInfo::DataTypes::Float) // FP ext handling
    {
        if (info.Bit == 16 && !SupportFP16) // FP16 check
        {
            if (least) // promotion
                info.Bit = 32;
            else
                Runtime.Logger.warning(u"Potential use of unsupported FP16 with [{}].\n"sv, type);
        }
        else if (info.Bit == 64 && !SupportFP64)
        {
            Runtime.Logger.warning(u"Potential use of unsupported FP64 with [{}].\n"sv, type);
        }
    }
    return info;
}

std::u32string_view NLCLReplacer::GetVecTypeName(common::simd::VecDataInfo info) noexcept
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

static constexpr uint8_t CheckVecable(const uint32_t totalBits, const uint32_t unitBits) noexcept
{
    if (totalBits % unitBits != 0) return 0;
    switch (const auto num = totalBits / unitBits; num)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 8:
    case 16:
        return static_cast<uint8_t>(num);
    default:
        return 0;
    }
}
static std::u32string SubgroupShufflePatch(const std::u32string_view funcName, const std::u32string_view base, 
    const uint8_t unitBits, const uint8_t dim, VecDataInfo::DataTypes dtype = VecDataInfo::DataTypes::Unsigned) noexcept
{
    Expects(dim > 0 && dim <= 16);
    constexpr char32_t idxNames[] = U"0123456789abcdef";
    const auto vecName = NLCLReplacer::GetVecTypeName({ dtype, unitBits, dim, 0 });
    const auto scalarName = NLCLReplacer::GetVecTypeName({ dtype, unitBits, 1, 0 });
    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{\r\n"sv),
        vecName, funcName);
    for (uint8_t i = 0; i < dim; ++i)
        fmt::format_to(std::back_inserter(func), FMT_STRING(U"    const {} ele{} = {}(val.s{}, sgId);\r\n"sv),
            scalarName, i, base, idxNames[i]);
    fmt::format_to(std::back_inserter(func), FMT_STRING(U"    return ({})("sv), vecName);
    for (uint8_t i = 0; i < dim - 1; ++i)
        fmt::format_to(std::back_inserter(func), FMT_STRING(U"ele{}, "sv), i);
    fmt::format_to(std::back_inserter(func), FMT_STRING(U"ele{});\r\n}}"sv), dim - 1);
    return func;
}
std::u32string NLCLReplacer::GenerateSubgroupShuffle(const common::span<std::u32string_view> args, const bool needShuffle) const
{
    if (needShuffle && !SupportSubgroupIntel)
    {
        Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
            fmt::format(FMT_STRING(u"Subgroup shuffle require support of intel_subgroups."sv), args[0])));
        return {};
    }
    const auto info = ParseVecType(args[0]);
    if (!info.has_value())
    {
        Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
            fmt::format(FMT_STRING(u"Type [{}] not found when replace-variable"sv), args[0])));
        return {};
    }
    const auto ReinterpretWrappedFunc = [&](const std::u32string_view func, const VecDataInfo middle)
    {
        return fmt::format(FMT_STRING(U"as_{}({}(as_{}({}), {}))"sv),
            GetVecTypeName(info.value()), func, GetVecTypeName(middle), args[1], args[2]);
    };
    const uint32_t totalBits = info->Bit * info->Dim0;
    const bool isVec = info->Dim0 > 1;
    if (!needShuffle && totalBits >= 32) // can be handled by khr_subgroups
    {
        if (totalBits == 32 || totalBits == 64) // native func
        {
            if (!isVec) // direct replace
                return fmt::format(FMT_STRING(U"sub_group_broadcast({}, {})"sv), args[1], args[2]);
            else // need type reinterpreting
                return ReinterpretWrappedFunc(U"sub_group_broadcast"sv, { VecDataInfo::DataTypes::Unsigned, static_cast<uint8_t>(totalBits), 1, 0 });
        }
        else if (!SupportSubgroupIntel) // >64, need patched func
        {
            const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_broadcast_{}"sv), totalBits);
            for (const auto targetBits : std::array<uint8_t, 2>{ 64u, 32u })
            {
                const auto vnum = CheckVecable(totalBits, targetBits);
                if (vnum == 0) continue;
                Runtime.AddPatchedBlock(funcName, SubgroupShufflePatch, 
                    funcName, U"sub_group_broadcast"sv, targetBits, vnum, VecDataInfo::DataTypes::Unsigned);
                return ReinterpretWrappedFunc(funcName, { VecDataInfo::DataTypes::Unsigned, static_cast<uint8_t>(totalBits), vnum, 0 });
            }
            // cannot handle right now
            Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
                fmt::format(FMT_STRING(u"Failed to generate patched broadcast for [{}]"sv), args[0])));
            return {};
        }
    }
    // require at least intel_subgroups
    if (!SupportSubgroupIntel)
    {
        Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
            fmt::format(FMT_STRING(u"Failed to generate broadcast for [{}] without support of intel_subgroups."sv), args[0])));
        return {};
    }
    if (info->Bit == 32) // direct replace
        return fmt::format(FMT_STRING(U"intel_sub_group_shuffle({}, {})"sv), args[1], args[2]);
    if (info->Bit == 16 && SupportSubgroup16Intel)
    {
        switch (info->Type)
        {
        case VecDataInfo::DataTypes::Float:
            if (info->Dim0 != 1 || !SupportFP16) // need type reinterpreting
                return ReinterpretWrappedFunc(U"intel_sub_group_shuffle"sv, { VecDataInfo::DataTypes::Unsigned, 16, info->Dim0, 0 });
            [[fallthrough]];
        // direct replace
        case VecDataInfo::DataTypes::Unsigned:
        case VecDataInfo::DataTypes::Signed:
            return fmt::format(FMT_STRING(U"intel_sub_group_shuffle({}, {})"sv), args[1], args[2]);
        default: Expects(false); return {};
        }
    }
    if (info->Bit == 8 && SupportSubgroup8Intel)
    {
        switch (info->Type)
        {
        case VecDataInfo::DataTypes::Unsigned:
        case VecDataInfo::DataTypes::Signed:
            return fmt::format(FMT_STRING(U"intel_sub_group_shuffle({}, {})"sv), args[1], args[2]);
        default: Expects(false); return {};
        }
    }
    if (info->Bit == 64 && info->Dim0 == 1)
    {
        switch (info->Type)
        {
        case VecDataInfo::DataTypes::Float:
            if (!SupportFP64) // need type reinterpreting
                return ReinterpretWrappedFunc(U"intel_sub_group_shuffle"sv, { VecDataInfo::DataTypes::Unsigned, 64, 1, 0 });
            [[fallthrough]];
        // direct replace
        case VecDataInfo::DataTypes::Unsigned:
        case VecDataInfo::DataTypes::Signed:
            return fmt::format(FMT_STRING(U"intel_sub_group_shuffle({}, {})"sv), args[1], args[2]);
        default: Expects(false); return {};
        }
    }
    // patched func based on intel_sub_group_shuffle
    {
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_shuffle_{}"sv), totalBits);
        for (const auto targetBits : std::array<uint8_t, 3>{ 32u, 64u, 16u })
        {
            const auto vnum = CheckVecable(totalBits, targetBits);
            if (vnum == 0) continue;
            Runtime.AddPatchedBlock(funcName, SubgroupShufflePatch, 
                funcName, U"intel_sub_group_shuffle"sv, targetBits, vnum, VecDataInfo::DataTypes::Unsigned);
            return ReinterpretWrappedFunc(funcName, { VecDataInfo::DataTypes::Unsigned, static_cast<uint8_t>(targetBits), vnum, 0 });
        }
        // cannot handle right now
        Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException,
            fmt::format(FMT_STRING(u"Failed to generate patched shuffle for [{}]"sv), args[0])));
        return {};
    }
}
std::u32string NLCLReplacer::GenerateDebugString(const common::span<std::u32string_view> args) const
{
    Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException, u"Not implemented"sv));
    return U"";
}


NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, 
    std::shared_ptr<NLCLEvalContext>&& evalCtx, const common::CLikeDefines& info) :
    NailangRuntimeBase(std::move(evalCtx)), Logger(logger), Device(dev), 
    EnabledExtensions(Device->Extensions.Size()), AllowDebug(false)
{ 
    Replacer = PrepareRepalcer();
    AllowDebug = info["debug"].has_value();
    for (const auto [key, val] : info)
    {
        const auto varName = common::strchset::to_u32string(key, Charset::UTF8);
        switch (val.index())
        {
        case 1: EvalContext->SetArg(varName, std::get<1>(val)); break;
        case 2: EvalContext->SetArg(varName, std::get<2>(val)); break;
        case 3: EvalContext->SetArg(varName, std::get<3>(val)); break;
        case 4: EvalContext->SetArg(varName, 
            common::strchset::to_u32string(std::get<4>(val), Charset::UTF8)); break;
        case 0: 
        default:
            break;
        }
    }
}
NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev,
    const common::CLikeDefines& info) : 
    NLCLRuntime(logger, dev, std::make_shared<NLCLEvalContext>(dev), info)
{ }
NLCLRuntime::~NLCLRuntime()
{ }

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
Arg NLCLRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas, const FuncTarget target)
{
    if (IsBeginWith(call.Name, U"oclu."sv))
    {
        switch (const auto subName = call.Name.substr(5); hash_(subName))
        {
        HashCase(subName, U"EnableExtension")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            const auto arg = EvaluateFuncArgs<1>(call)[0];
            if (const auto sv = arg.GetStr(); sv.has_value())
            {
                if (!EnableExtension(sv.value()))
                    Logger.warning(u"Extension [{}] not found in support list, skipped.\n", sv.value());
            }
            else
                NLRT_THROW_EX(u"Arg of [EnableExtension] should be string"sv, call);
        } return {};
        HashCase(subName, U"EnableUnroll")
        {
            if (!Replacer->EnableUnroll)
            {
                Logger.warning(u"Manually enable unroll hint.\n");
                Replacer->EnableUnroll = true;
            }
        } return {};
        HashCase(subName, U"EnableDebug")
        {
            Logger.verbose(u"Manually enable debug.\n");
            AllowDebug = true;
        } return {};
        HashCase(subName, U"Log")
        {
            const auto args2 = EvaluateFuncArgs<2, ArgLimits::AtLeast>(call);
            const auto logLevel = ParseLogLevel(args2[0]);
            if (!logLevel) 
                NLRT_THROW_EX(u"Arg[0] of [Log] should be LogLevel"sv, call);
            const auto fmtStr = args2[1].GetStr();
            if (!fmtStr)
                NLRT_THROW_EX(u"Arg[1] of [Log] should be string-able"sv, call);
            if (call.Args.size() == 2)
                InnerLog(logLevel.value(), fmtStr.value());
            else
            {
                try
                {
                    const auto str = FormatString(fmtStr.value(), call.Args.subspan(2));
                    InnerLog(logLevel.value(), str);
                }
                catch (const xziar::nailang::NailangFormatException& nfe)
                {
                    Logger.error(u"Error when formating inner log: {}\n", nfe.message);
                }
            }
        } return {};
        HashCase(subName, U"DefineDebugString")
        {
            ThrowByArgCount(call, 3, ArgLimits::AtLeast);
            const auto arg2 = EvaluateFuncArgs<2, ArgLimits::AtLeast>(call);
            if (!arg2[0].IsStr() || !arg2[1].IsStr())
                NLRT_THROW_EX(u"Arg[0][1] of [DefineDebugString] should all be string-able"sv, call);
            const auto id = arg2[0].GetStr().value();
            const auto formatter = arg2[1].GetStr().value();
            std::vector<common::simd::VecDataInfo> argInfos;
            argInfos.reserve(call.Args.size() - 2);
            size_t i = 2;
            for (const auto& rawarg : call.Args.subspan(2))
            {
                const auto arg = EvaluateArg(rawarg);
                if (!arg.IsStr())
                    NLRT_THROW_EX(fmt::format(u"Arg[{}] of [DefineDebugString] should be string, which gives [{}]"sv, i, ArgTypeName(arg.TypeData)), call);
                const auto vtype = Replacer->ParseVecType(arg.GetStr().value());
                if (!vtype.has_value())
                    NLRT_THROW_EX(fmt::format(u"Arg[{}] of [DefineDebugString], [{}] is not a recognized type"sv, i, arg.GetStr().value()), call);
                argInfos.push_back(vtype.value());
            }
            if (!AddPatchedBlock(id, [&]()
                {
                    return DebugStringPatch(id, formatter, argInfos);
                }))
                NLRT_THROW_EX(fmt::format(u"DebugString [{}] repeately defined"sv, id), call);
        } return {};
        HashCase(subName, U"AddSubgroupPatch")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            const auto args = EvaluateFuncArgs<2>(call);
            const auto isShuffle = args[0].GetBool().value();
            const auto vtype = args[1].GetStr().value();
            std::array strs{ vtype,U"x"sv,U"y"sv };
            const auto funcsrc = Replacer->GenerateSubgroupShuffle(strs, isShuffle);
            AddPatchedBlock(fmt::format(U"AddSubgroupPatch_{}_{}"sv, vtype, isShuffle),
                [&]() { return fmt::format(U"/* {} */\r\n"sv, funcsrc); });
        } return {};
        default: break;
        }
    }
    return NailangRuntimeBase::EvaluateFunc(call, metas, target);
}

void NLCLRuntime::HandleException(const NailangRuntimeException& ex) const
{
    Logger.error(u"{}\n", ex.message);
    NailangRuntimeBase::HandleException(ex);
}

void NLCLRuntime::InnerLog(common::mlog::LogLevel level, std::u32string_view str)
{
    Logger.log(level, u"[NLCL]{}\n", str);
}

void NLCLRuntime::DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const
{
    OutputConditions(metas, dst);
    common::str::StrVariant<char32_t> source(block.Source);
    if (common::linq::FromIterable(metas).ContainsIf([](const FuncCall& fcall) { return fcall.Name == U"oclu.ReplaceVariable"sv; }))
        source = Replacer->ProcessVariable(source.StrView(), U"$$!{"sv, U"}"sv);
    if (common::linq::FromIterable(metas).ContainsIf([](const FuncCall& fcall) { return fcall.Name == U"oclu.ReplaceFunction"sv; }))
        source = Replacer->ProcessFunction(source.StrView(), U"$$!"sv, U""sv);
    dst.append(source.StrView());
}

std::unique_ptr<NLCLReplacer> NLCLRuntime::PrepareRepalcer()
{
    return std::make_unique<NLCLReplacer>(*this);
}

std::u32string NLCLRuntime::DebugStringPatch(const std::u32string_view dbgId, const std::u32string_view formatter, common::span<const common::simd::VecDataInfo> args) noexcept
{
    // prepare arg layout
    const auto dbgIdx = DebugBlocks.size();
    if (dbgIdx >= 256u)
        NLRT_THROW_EX(u"Too many DebugString defined, maximum 256");
    const auto& dbgData = DebugBlocks.insert_or_assign(std::u32string(dbgId), 
        oclDebugBlock{ static_cast<uint8_t>(dbgIdx), args, formatter, static_cast<uint16_t>(4u) })
        .first->second.Layout;

    std::u32string func = fmt::format(FMT_STRING(U"inline void oglu_debug_{}("sv), dbgId);
    func.append(U"\r\n    const  uint           uid,"sv)
        .append(U"\r\n    const  uint           total,"sv)
        .append(U"\r\n    global uint* restrict counter,"sv)
        .append(U"\r\n    global uint* restrict data,"sv);
    for (size_t i = 0; i < args.size(); ++i)
    {
        fmt::format_to(std::back_inserter(func), FMT_STRING(U"\r\n    const  {:7} arg{},"sv), NLCLReplacer::GetVecTypeName(args[i]), i);
    }
    func.pop_back();
    func.append(U")\r\n{"sv);
    fmt::format_to(std::back_inserter(func), FMT_STRING(U"\r\n    const uint size = {} + 1;"), dbgData.TotalSize / 4);
    fmt::format_to(std::back_inserter(func), FMT_STRING(U"\r\n    const uint dbgId = {0:#010x}u << 24; // dbg [{0:3}]"), dbgIdx);
    func.append(U"\r\n    if (counter[0] + size > total) return;");
    func.append(U"\r\n    const uint old = atom_add (counter, size);");
    func.append(U"\r\n    if (old + size > total) return;");
    func.append(U"\r\n    data[old] = (uid & 0x00ffffffu) | dbgId;");
    func.append(U"\r\n    global uint* const restrict ptr = data + old + 1;");
    const auto WriteOne = [&](uint32_t offset, uint16_t argIdx, uint8_t vsize, VecDataInfo dtype, std::u32string_view argAccess, bool needConv)
    {
        auto dst = std::back_inserter(func);
        const VecDataInfo vtype{ dtype.Type, dtype.Bit, vsize, 0 };
        const auto dstTypeStr  = Replacer->GetVecTypeName(dtype);
        std::u32string getData = needConv ?
            fmt::format(FMT_STRING(U"as_{}(arg{}{})"sv), Replacer->GetVecTypeName(vtype), argIdx, argAccess) :
            fmt::format(FMT_STRING(U"arg{}{}"sv), argIdx, argAccess);
        if (vsize == 1)
        {
            fmt::format_to(dst, FMT_STRING(U"\r\n    ((global {}*)(ptr))[{}] = {};"sv),
                    dstTypeStr, offset / (dtype.Bit / 8), getData);
        }
        else
        {
            fmt::format_to(dst, FMT_STRING(U"\r\n    vstore{}({}, 0, (global {}*)(ptr) + {});"sv),
                vsize, getData, dstTypeStr, offset / (dtype.Bit / 8));
        }
    };
    for (const auto& [info, offset, idx] : dbgData.ByLayout())
    {
        const auto size = info.Bit * info.Dim0;
        fmt::format_to(std::back_inserter(func),
            FMT_STRING(U"\r\n    // arg[{:3}], offset[{:3}], size[{:3}], type[{}]"sv),
            idx, offset, size / 8, StringifyVDataType(info));
        for (const auto eleBit : std::array<uint8_t, 3>{ 32u,16u,8u })
        {
            const auto eleByte = eleBit / 8;
            if (size % eleBit == 0)
            {
                VecDataInfo dstType{ VecDataInfo::DataTypes::Unsigned, eleBit, 1, 0 };
                const bool needConv = info.Bit != dstType.Bit || info.Type != VecDataInfo::DataTypes::Unsigned;
                const auto cnt = gsl::narrow_cast<uint8_t>(size / eleBit);
                // For 32bit:
                // 64bit: 1,2,3,4,8,16 -> 2,4,6(4+2),8,16,32
                // 32bit: 1,2,3,4,8,16 -> 1,2,3(2+1),4,8,16
                // 16bit: 2,4,8,16     -> 1,2,4,8
                //  8bit: 4,8,16       -> 1,2,4
                // For 16bit:
                // 16bit: 1,2,3,4,8,16 -> 1,2,3(2+1),4,8,16
                //  8bit: 2,4,8,16     -> 1,2,4,8
                // For 8bit:
                //  8bit: 1,2,3,4,8,16 -> 1,2,3(2+1),4,8,16
                if (cnt == 32u)
                {
                    Expects(info.Bit == 64 && info.Dim0 == 16);
                    WriteOne(offset + eleByte * 0 , idx, 16, dstType, U".s01234567"sv, needConv);
                    WriteOne(offset + eleByte * 16, idx, 16, dstType, U".s89abcdef"sv, needConv);
                }
                else if (cnt == 6u)
                {
                    Expects(info.Bit == 64 && info.Dim0 == 3);
                    WriteOne(offset + eleByte * 0, idx, 4, dstType, U".s01"sv, needConv);
                    WriteOne(offset + eleByte * 4, idx, 2, dstType, U".s2"sv,  needConv);
                }
                else if (cnt == 3u)
                {
                    Expects(info.Bit == dstType.Bit && info.Dim0 == 3);
                    WriteOne(offset + eleByte * 0, idx, 2, dstType, U".s01"sv, needConv);
                    WriteOne(offset + eleByte * 2, idx, 1, dstType, U".s2"sv,  needConv);
                }
                else
                {
                    Expects(cnt == 1 || cnt == 2 || cnt == 4 || cnt == 8 || cnt == 16);
                    WriteOne(offset, idx, cnt, dstType, {}, needConv);
                }
                break;
            }
        }
    }
    func.append(U"\r\n}\r\n"sv);
    return func;
}

void NLCLRuntime::OutputConditions(MetaFuncs metas, std::u32string& dst) const
{
    // std::set<std::u32string_view> lateVars;
    for (const auto& meta : metas)
    {
        std::u32string_view prefix;
        if (meta.Name == U"If"sv)
            prefix = U"If "sv;
        else if (meta.Name == U"Skip"sv)
            prefix = U"Skip if "sv;
        else
            continue;
        if (meta.Args.size() == 1)
            fmt::format_to(std::back_inserter(dst),
                FMT_STRING(U"/* {:7} :  {} */\r\n"sv), prefix,
                xziar::nailang::Serializer::Stringify(meta.Args[0]));
    }
}

void NLCLRuntime::OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From GlobalBlock [{}] */\r\n"sv), block.Name);
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From StructBlock [{}] */\r\n"sv), block.Name);
    dst.append(U"typedef struct \r\n{\r\n"sv);
    DirectOutput(block, metas, dst);
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"}} {};\r\n"sv), block.Name);
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


void NLCLRuntime::OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    KernelArgStore argInfos;
    std::vector<std::u32string> argTexts;
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From KernelBlock [{}] */\r\n"sv), block.Name);
    for (const auto meta : metas)
    {
        if (!IsBeginWith(meta.Name, U"oclu.")) continue;
        switch (const auto subName = meta.Name.substr(5); hash_(subName))
        {
        HashCase(subName, U"RequestSubgroupSize")
        {
            if (Device->GetPlatform()->PlatVendor != oclu::Vendors::Intel)
            {
                Logger.verbose(u"Skip subgroup size due to non-intel platform.\n");
                break;
            }
            ThrowByArgCount(meta, 1);
            const auto sgSize = EvaluateArg(meta.Args[0]).GetUint();
            if (sgSize)
            {
                EnableExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv);
                fmt::format_to(std::back_inserter(dst), FMT_STRING(U"__attribute__((intel_reqd_sub_group_size({})))\r\n"sv), sgSize.value());
            }
            else
                Logger.verbose(u"Skip subgroup size due to empty arg.\n"sv);
        } break;
        HashCase(subName, U"RequestWorkgroupSize")
        {
            const auto args = EvaluateFuncArgs<3>(meta);
            const auto x = args[0].GetUint().value_or(1),
                       y = args[1].GetUint().value_or(1),
                       z = args[2].GetUint().value_or(1);
            fmt::format_to(std::back_inserter(dst), FMT_STRING(U"__attribute__((reqd_work_group_size({}, {}, {})))\r\n"sv),
                x, y, z);
        } break;
        HashCase(subName, U"NormalArg")
        {
            const auto args = EvaluateFuncArgs<4>(meta, { Arg::InternalType::U32Sv, Arg::InternalType::U32Sv, Arg::InternalType::U32Sv, Arg::InternalType::U32Sv });
            KernelArgInfo info;
            const auto space_  = args[0].GetStr().value();
            const auto space   = KerArgSpaceParser(space_);
            if (!space) NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgSpace [{}]"sv, space_), &meta);
            const auto argType = args[1].GetStr().value();
            const auto name    = args[2].GetStr().value();
            const auto flags   = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part) 
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        if (part == U"restrict"sv)  return flag | KerArgFlag::Restrict;
                        if (part == U"volatile"sv)  return flag | KerArgFlag::Volatile;
                        NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgFlag [{}]"sv, part),
                            &meta);
                        return flag;
                    }, KerArgFlag::None);
            argTexts.emplace_back(fmt::format(FMT_STRING(U"{:8} {:5} {} {} {} {}"sv),
                ArgFlags::ToCLString(space.value()),
                HAS_FIELD(flags, KerArgFlag::Const) ? U"const"sv : U""sv,
                HAS_FIELD(flags, KerArgFlag::Volatile) ? U"volatile"sv : U""sv,
                argType,
                HAS_FIELD(flags, KerArgFlag::Restrict) ? U"restrict"sv : U""sv,
                name));
            argInfos.AddArg(space.value(), ImgAccess::None, flags,
                common::strchset::to_string(name,    Charset::UTF8, Charset::UTF32LE),
                common::strchset::to_string(argType, Charset::UTF8, Charset::UTF32LE));
        } break;
        HashCase(subName, U"ImgArg")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::InternalType::U32Sv, Arg::InternalType::U32Sv, Arg::InternalType::U32Sv });
            KernelArgInfo info;
            const auto access_ = args[0].GetStr().value();
            const auto access = ImgArgAccessParser(access_);
            if (!access) NLRT_THROW_EX(fmt::format(u"Unrecognized ImgAccess [{}]"sv, access_), &meta);
            const auto argType = args[1].GetStr().value();
            const auto name = args[2].GetStr().value();
            argTexts.emplace_back(fmt::format(FMT_STRING(U"{:10} {} {}"sv),
                ArgFlags::ToCLString(access.value()),
                argType,
                name));
            argInfos.AddArg(KerArgSpace::Global, access.value(), KerArgFlag::None,
                common::strchset::to_string(name, Charset::UTF8, Charset::UTF32LE),
                common::strchset::to_string(argType, Charset::UTF8, Charset::UTF32LE));
        } break;
        HashCase(subName, U"DebugOutput")
        {
            if (!AllowDebug)
            {
                Logger.info(u"DebugOutput is disabled and ignored.\n");
                break;
            }
            EnableExtension("cl_khr_global_int32_base_atomics"sv, u"Use oclu-debugoutput"sv);
            EnableExtension("cl_khr_byte_addressable_store"sv, u"Use oclu-debugoutput"sv);
            argInfos.DebugBuffer = gsl::narrow_cast<uint32_t>(
                EvaluateFuncArgs<1, ArgLimits::AtMost>(meta, { Arg::InternalType::Uint })[0].GetUint().value_or(512));
        }
        default:
            break;
        }
    }
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"kernel void {} ("sv), block.Name);
    for (const auto& argText : argTexts)
    {
        dst.append(U"\r\n    "sv).append(argText).append(U","sv);
    }
    if (argInfos.DebugBuffer > 0)
    {
        dst .append(U"\r\n    const  uint           _oclu_debug_buffer_size,"sv)
            .append(U"\r\n    global uint* restrict _oclu_debug_buffer_info,"sv)
            .append(U"\r\n    global uint* restrict _oclu_debug_buffer_data,"sv);
    }
    dst.pop_back(); // remove additional ','
    dst.append(U")\r\n{\r\n"sv);
    if (argInfos.DebugBuffer > 0)
    {
        dst.append(UR"(    // below injected by oclu_debug
    const uint _oclu_thread_id = get_global_id(0) + get_global_id(1) * get_global_size(0) + get_global_id(2) * get_global_size(1);
    if (_oclu_debug_buffer_info[0] > 0)
    {
#if defined(cl_khr_subgroups) || defined(cl_intel_subgroups)
        const ushort sgid  = get_sub_group_id();
        const ushort sglid = get_sub_group_local_id();
        const uint sginfo  = sgid * 65536u + sglid;
#else
        const uint sginfo  = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1);
#endif
        _oclu_debug_buffer_info[_oclu_thread_id + 1] = sginfo;
    }


)"sv);
    }
    DirectOutput(block, metas, dst);
    dst.append(U"}\r\n"sv);
    CompiledKernels.emplace_back(common::strchset::to_string(block.Name, Charset::UTF8, Charset::UTF32LE), std::move(argInfos));
}

void NLCLRuntime::OutputTemplateKernel(const RawBlock& block, [[maybe_unused]] MetaFuncs metas, [[maybe_unused]] uint32_t extraInfo, std::u32string& dst)
{
    // TODO:
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From TempalteKernel [{}] */\r\n"sv), block.Name);
    NLRT_THROW_EX(u"TemplateKernel not supportted yet"sv,
        &block);
}

bool NLCLRuntime::EnableExtension(std::string_view ext, std::u16string_view desc)
{
    if (ext == "all"sv)
        EnabledExtensions.assign(EnabledExtensions.size(), true);
    else if (const auto idx = Device->Extensions.GetIndex(ext); idx != SIZE_MAX)
        EnabledExtensions[idx] = true;
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
        EnabledExtensions.assign(EnabledExtensions.size(), true);
    else if (const auto idx = Device->Extensions.GetIndex(TmpSv{ext}); idx != SIZE_MAX)
        EnabledExtensions[idx] = true;
    else
    {
        if (!desc.empty())
            Logger.warning(FMT_STRING(u"{} on unsupported device without [{}].\n"sv), desc, ext);
        return false;
    }
    return true;
}

bool NLCLRuntime::CheckExtensionEnabled(std::string_view ext) const
{
    if (const auto idx = Device->Extensions.GetIndex(ext); idx != SIZE_MAX)
        return EnabledExtensions[idx];
    return false;
}


void NLCLRuntime::ProcessRawBlock(const xziar::nailang::RawBlock& block, MetaFuncs metas)
{
    BlockContext ctx;
    if (IsBeginWith(block.Type, U"oclu."sv) &&
        HandleMetaFuncsBefore(metas, BlockContent::Generate(&block), ctx))
    {
        bool output = false;
        switch (const auto subName = block.Type.substr(5); hash_(subName))
        {
        HashCase(subName, U"Global")     output = true;  break;
        HashCase(subName, U"Struct")     output = true;  StructBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"Kernel")     output = true;  KernelBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"Template")   output = false; TemplateBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"KernelStub") output = false; KernelStubBlocks.emplace_back(&block, metas); break;
        default: break;
        }
        if (output)
            OutputBlocks.emplace_back(&block, metas);
    }
}

std::string NLCLRuntime::GenerateOutput()
{
    std::u32string prefix, output;

    for (const auto& item : OutputBlocks)
    {
        const MetaFuncs metas(item.MetaPtr, item.MetaCount);
        switch (hash_(item.Block->Type))
        {
        HashCase(item.Block->Type, U"oclu.Global")     OutputGlobal(*item.Block, metas, output); continue;
        HashCase(item.Block->Type, U"oclu.Struct")     OutputStruct(*item.Block, metas, output); continue;
        HashCase(item.Block->Type, U"oclu.Kernel")     OutputKernel(*item.Block, metas, output); continue;
        HashCase(item.Block->Type, U"oclu.KernelStub") OutputTemplateKernel(*item.Block, metas, item.ExtraInfo, output); continue;
        default: break;
        }
        Logger.warning(u"Unexpected RawBlock (Type[{}], Name[{}]) when generating output.\n", item.Block->Type, item.Block->Name); break;
    }

    { // Output extentions
        prefix.append(U"/* Extensions */\r\n"sv);
        if (common::linq::FromIterable(EnabledExtensions).All(true))
        {
            prefix.append(U"#pragma OPENCL EXTENSION all : enable\r\n"sv);
        }
        else
        {
            common::linq::FromIterable(EnabledExtensions).Pair(common::linq::FromIterable(Device->Extensions))
                .ForEach([&](const auto& pair)
                    {
                        if (pair.first)
                            fmt::format_to(std::back_inserter(prefix), FMT_STRING(U"#pragma OPENCL EXTENSION {} : enable\r\n"sv), pair.second);
                    });
        }
        prefix.append(U"\r\n"sv);
    }

    { // Output patched blocks
        for (const auto& [id, src] : PatchedBlocks)
        {
            fmt::format_to(std::back_inserter(prefix), FMT_STRING(U"/* Patched Block [{}] */\r\n\r\n"sv), id);
            prefix.append(src);
            prefix.append(U"\r\n"sv);
        }
    }

    return common::strchset::to_string(prefix + output, Charset::UTF8, Charset::UTF32LE);
}


NLCLBaseResult::NLCLBaseResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx) :
    EvalContext(std::move(evalCtx))
{ }
NLCLBaseResult::~NLCLBaseResult()
{ }
NLCLResult::ResultType NLCLBaseResult::QueryResult(std::u32string_view name) const
{
    auto result = EvalContext->LookUpArg(name);
    return result.Visit([](auto val) -> NLCLResult::ResultType
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>
                || std::is_same_v<T, double> || std::is_same_v<T, std::u32string_view>)
                return val;
            else if constexpr (std::is_same_v<T, std::optional<bool>>)
                return {};
            else // var
                return std::any{};
        });
}

NLCLUnBuildResult::NLCLUnBuildResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, std::string&& source) :
    NLCLBaseResult(std::move(evalCtx)), Source(std::move(source))
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

NLCLBuiltResult::NLCLBuiltResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, oclProgram prog) :
    NLCLBaseResult(std::move(evalCtx)), Prog(std::move(prog))
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

NLCLBuildFailResult::NLCLBuildFailResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, std::string&& source, 
    std::shared_ptr<common::BaseException> ex) : 
    NLCLUnBuildResult(std::move(evalCtx), std::move(source)), Exception(std::move(ex))
{ }
NLCLBuildFailResult::~NLCLBuildFailResult()
{ }
oclProgram NLCLBuildFailResult::GetProgram() const
{
    Exception->ThrowSelf();
    return {};
}


NLCLProgram::NLCLProgram(std::u32string&& source) :
    Source(std::move(source)) { }
NLCLProgram::~NLCLProgram()
{ }


NLCLProgStub::NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program,
    oclDevice dev, std::unique_ptr<NLCLRuntime>&& runtime) :
    Program(program), Device(dev), Runtime(std::move(runtime))
{ }
NLCLProgStub::~NLCLProgStub()
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

std::shared_ptr<NLCLProgram> NLCLProcessor::Parse(common::span<const std::byte> source) const
{
    auto& logger = Logger();
    const auto encoding = common::strchset::DetectEncoding(source);
    logger.info(u"Detected encoding[{}].\n", common::str::getCharsetName(encoding));
    const auto prog = std::make_shared<NLCLProgram>(common::strchset::to_u32string(source, encoding));
    logger.info(u"Translate into [utf32] for [{}] chars.\n", prog->Source.size());
    NLCLParser::GetBlock(prog->MemPool, prog->Source, prog->Program);
    logger.verbose(u"Parse done, get [{}] Blocks.\n", prog->Program.Size());
    return prog;
}

std::unique_ptr<NLCLResult> NLCLProcessor::ProcessCL(const std::shared_ptr<NLCLProgram>& prog, const oclDevice dev, const common::CLikeDefines& info) const
{
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(Logger(), dev, info));
    ConfigureCL(stub);
    return std::make_unique<NLCLUnBuildResult>(std::move(stub.Runtime->EvalContext), stub.Runtime->GenerateOutput());
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, oclDevice dev, const common::CLikeDefines& info, const oclu::CLProgConfig& config) const
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(Logger(), dev, info));
    ConfigureCL(stub);
    auto str = stub.Runtime->GenerateOutput();
    try
    {
        auto progStub = oclProgram_::Create(ctx, str, dev);
        progStub.ImportedKernelInfo = std::move(stub.Runtime->CompiledKernels);
        // if (ctx->Version < 12)
        progStub.Build(config);
        return std::make_unique<NLCLBuiltResult>(std::move(stub.Runtime->EvalContext), progStub.Finish());
    }
    catch (const common::BaseException& be)
    {
        return std::make_unique<NLCLBuildFailResult>(std::move(stub.Runtime->EvalContext), std::move(str), be.Share());
    }
}


}