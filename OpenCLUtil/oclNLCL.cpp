#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "NLCLSubgroup.h"
#include "oclException.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"
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
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


NLCLEvalContext::NLCLEvalContext(oclDevice dev) : Device(dev) 
{ }
NLCLEvalContext::~NLCLEvalContext() 
{ }

Arg NLCLEvalContext::LookUpCLArg(xziar::nailang::detail::VarLookup var) const
{
    if (var.Part() == U"Extension"sv)
    {
        const auto extName = common::strchset::to_string(var.Rest(), Charset::UTF8, Charset::UTF32);
        return Device->Extensions.Has(extName);
    }
    if (var.Part() == U"Dev"sv)
    {
        const auto propName = var.Rest();
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

Arg oclu::NLCLEvalContext::LookUpArgInside(xziar::nailang::detail::VarLookup var) const
{
    if (var.Part() == U"oclu"sv)
        return LookUpCLArg(var.Rest());
    return CompactEvaluateContext::LookUpArgInside(var);
}


TemplateBlockInfo::TemplateBlockInfo(common::span<const RawArg> args)
{
    ReplaceArgs.reserve(args.size());
    for (const auto& arg : args)
    {
        ReplaceArgs.emplace_back(arg.GetVar<RawArg::Type::Var>().Name);
    }
}


NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, 
    std::shared_ptr<NLCLEvalContext>&& evalCtx, const common::CLikeDefines& info) :
    NailangRuntimeBase(std::move(evalCtx)), Logger(logger), Device(dev), 
    EnabledExtensions(Device->Extensions.Size()),
    EnableUnroll(Device->CVersion >= 20 || SupportNVUnroll), AllowDebug(false),
    SupportFP16(Device->Extensions.Has("cl_khr_fp16")),
    SupportFP64(Device->Extensions.Has("cl_khr_fp64") || Device->Extensions.Has("cl_amd_fp64")),
    SupportNVUnroll(Device->Extensions.Has("cl_nv_pragma_unroll")),
    SupportSubgroupKHR(Device->Extensions.Has("cl_khr_subgroups")),
    SupportSubgroupIntel(Device->Extensions.Has("cl_intel_subgroups")),
    SupportSubgroup8Intel(SupportSubgroupIntel && Device->Extensions.Has("cl_intel_subgroups_char")),
    SupportSubgroup16Intel(SupportSubgroupIntel && Device->Extensions.Has("cl_intel_subgroups_short")),
    SupportBasicSubgroup(SupportSubgroupKHR || SupportSubgroupIntel)
{ 
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

void NLCLRuntime::InnerLog(common::mlog::LogLevel level, std::u32string_view str)
{
    Logger.log(level, u"[NLCL]{}\n", str);
}

void NLCLRuntime::HandleException(const NailangRuntimeException& ex) const
{
    Logger.error(u"{}\n", ex.message);
    NailangRuntimeBase::HandleException(ex);
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
    NLRT_THROW_EX(fmt::format(FMT_STRING(u"Repalcer-Func [{}] requires {} [{}] args, which gives [{}]."), call, prefix, count, args.size()));
}

std::optional<common::simd::VecDataInfo> NLCLRuntime::ParseVecType(const std::u32string_view type) const noexcept
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
                Logger.warning(u"Potential use of unsupported FP16 with [{}].\n"sv, type);
        }
        else if (info.Bit == 64 && !SupportFP64)
        {
            Logger.warning(u"Potential use of unsupported FP64 with [{}].\n"sv, type);
        }
    }
    return info;
}

std::u32string_view NLCLRuntime::GetVecTypeName(common::simd::VecDataInfo info) noexcept
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

std::u32string NLCLRuntime::SkipDebugPatch() const noexcept
{
    return U"inline void oclu_SkipDebug() {}\r\n";
}

std::u32string NLCLRuntime::DebugStringPatch(const std::u32string_view dbgId, const std::u32string_view formatter, common::span<const common::simd::VecDataInfo> args) noexcept
{
    // prepare arg layout
    const auto& dbgBlock = DebugManager.AppendBlock(dbgId, formatter, args);
    const auto& dbgData = dbgBlock.Layout;
    // test format
    try
    {
        std::vector<std::byte> test(dbgData.TotalSize);
        Logger.debug(FMT_STRING(u"DebugString:[{}]\n{}\ntest output:\n{}\n"sv), dbgId, formatter, dbgBlock.GetString(test));
    }
    catch (const fmt::format_error& fe)
    {
        HandleException(CREATE_EXCEPTION(xziar::nailang::NailangFormatException, formatter, fe));
        return U"// Formatter not match the datatype provided\r\n";
    }

    std::u32string func = fmt::format(FMT_STRING(U"inline void oglu_debug_{}("sv), dbgId);
    func.append(U"\r\n    const  uint           uid,"sv)
        .append(U"\r\n    const  uint           total,"sv)
        .append(U"\r\n    global uint* restrict counter,"sv)
        .append(U"\r\n    global uint* restrict data,"sv);
    for (size_t i = 0; i < args.size(); ++i)
    {
        APPEND_FMT(func, U"\r\n    const  {:7} arg{},"sv, GetVecTypeName(args[i]), i);
    }
    func.pop_back();
    func.append(U")\r\n{"sv);
    APPEND_FMT(func, U"\r\n    const uint size = {} + 1;", dbgData.TotalSize / 4);
    APPEND_FMT(func, U"\r\n    const uint dbgId = {0:#010x}u << 24; // dbg [{0:3}]", dbgBlock.DebugId);
    func.append(U"\r\n    if (total == 0) return;");
    func.append(U"\r\n    if (counter[0] + size > total) return;");
    func.append(U"\r\n    const uint old = atom_add (counter, size);");
    func.append(U"\r\n    if (old >= total) return;");
    func.append(U"\r\n    if (old + size > total) { data[old] = 0xff000000u; return; }");
    func.append(U"\r\n    data[old] = (uid & 0x00ffffffu) | dbgId;");
    func.append(U"\r\n    global uint* const restrict ptr = data + old + 1;");
    const auto WriteOne = [&](uint32_t offset, uint16_t argIdx, uint8_t vsize, VecDataInfo dtype, std::u32string_view argAccess, bool needConv)
    {
        const VecDataInfo vtype{ dtype.Type, dtype.Bit, vsize, 0 };
        const auto dstTypeStr  = GetVecTypeName(dtype);
        std::u32string getData = needConv ?
            fmt::format(FMT_STRING(U"as_{}(arg{}{})"sv), GetVecTypeName(vtype), argIdx, argAccess) :
            fmt::format(FMT_STRING(U"arg{}{}"sv), argIdx, argAccess);
        if (vsize == 1)
        {
            APPEND_FMT(func, U"\r\n    ((global {}*)(ptr))[{}] = {};"sv, dstTypeStr, offset / (dtype.Bit / 8), getData);
        }
        else
        {
            APPEND_FMT(func, U"\r\n    vstore{}({}, 0, (global {}*)(ptr) + {});"sv, vsize, getData, dstTypeStr, offset / (dtype.Bit / 8));
        }
    };
    for (const auto& [info, offset, idx] : dbgData.ByLayout())
    {
        const auto size = info.Bit * info.Dim0;
        APPEND_FMT(func, U"\r\n    // arg[{:3}], offset[{:3}], size[{:3}], type[{}]"sv,
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

std::u32string NLCLRuntime::GenerateDebugString(const common::span<const std::u32string_view> args) const
{
    Expects(args.size() >= 1);

    std::u32string func = fmt::format(FMT_STRING(U"oglu_debug_{}(_oclu_thread_id, _oclu_debug_buffer_size, _oclu_debug_buffer_info, _oclu_debug_buffer_data, "sv), args[0]);
    for (size_t i = 1; i < args.size(); ++i)
    {
        APPEND_FMT(func, U"{}, "sv, args[i]);
    }
    func.pop_back();
    func.back() = U')';

    return func;
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
        const auto info = ParseVecType(var.substr(1));
        if (info.has_value())
        {
            const auto str = GetVecTypeName(info.value());
            if (!str.empty())
                output.append(str);
            else
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not supported when replace-variable"sv), var));
        }
        else
            NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not recognized as VecType when replace-variable"sv), var));
    }
    else // '@' not allowed in var anyway
    {
        const auto ret = EvalContext->LookUpArg(var);
        if (ret.IsEmpty() || ret.TypeData == Arg::Type::Var)
        {
            NLRT_THROW_EX(fmt::format(FMT_STRING(u"Arg [{}] not found when replace-variable"sv), var));
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
        if (EnableUnroll)
        {
            if (Device->CVersion >= 20 || !SupportNVUnroll)
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
    if (IsBeginWith(func, U"oclu."sv))
    {
        const auto subName = func.substr(5);
        const auto kerCk = BlockCookie::Get<KernelCookie>(cookie);
        if (kerCk && kerCk->SubgroupSolver)
        {
            const auto ret = kerCk->SubgroupSolver->ReplaceFunc(subName, args, kerCk);
            if (!ret.empty())
            {
                output.append(ret);
                return;
            }
        }

        switch (hash_(subName))
        {
        HashCase(subName, U"GetVecTypeName")
        {
            ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
            const auto vname = args[0];
            const auto info = ParseVecType(vname);
            if (info.has_value())
            {
                const auto str = GetVecTypeName(info.value());
                if (!str.empty())
                    output.append(str);
                else
                    NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not supported"sv), vname));
            }
            else
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not recognized as VecType"sv), vname));
        } return;
        HashCase(subName, U"CodeBlock")
        {
            ThrowByReplacerArgCount(func, args, 1, ArgLimits::AtLeast);
            const OutputBlock* block = nullptr;
            for (const auto& blk : TemplateBlocks)
                if (blk.Block->Name == args[0])
                {
                    block = &blk; break;
                }
            if (block)
            {
                const auto& extra = static_cast<const TemplateBlockInfo&>(*block->ExtraInfo);
                ThrowByReplacerArgCount(func, args, extra.ReplaceArgs.size() + 1, ArgLimits::AtLeast);
                auto scope = InnerScope(extra.ReplaceArgs.size() > 0);
                for (size_t i = 0; i < extra.ReplaceArgs.size(); ++i)
                {
                    std::u32string name = U":"; name += extra.ReplaceArgs[i];
                    EvalContext->SetArg(name, args[i + 1]);
                }
                APPEND_FMT(output, U"// template block [{}]\r\n"sv, args[0]);
                DirectOutput(*block->Block, block->MetaFunc, output, reinterpret_cast<BlockCookie*>(cookie));
            }
        } return;
        HashCase(subName, U"DebugString")
        {
            ThrowByReplacerArgCount(func, args, 1, ArgLimits::AtLeast);
            if (!AllowDebug)
            {
                AddPatchedBlock(*this, U"SkipDebug"sv, &NLCLRuntime::SkipDebugPatch);
                output.append(U"oclu_SkipDebug()"sv); 
            }
            else if (kerCk)
            {
                output.append(GenerateDebugString(args));
                kerCk->Injects.insert_or_assign(U"oclu_debug", UR"(
    const uint _oclu_thread_id = get_global_id(0) + get_global_id(1) * get_global_size(0) + get_global_id(2) * get_global_size(1) * get_global_size(0);
    if (_oclu_debug_buffer_size > 0)
    {
        if (_oclu_thread_id == 0)
        {
            _oclu_debug_buffer_info[1] = get_global_size(0);
            _oclu_debug_buffer_info[2] = get_global_size(1);
            _oclu_debug_buffer_info[3] = get_global_size(2);
            _oclu_debug_buffer_info[4] = get_local_size(0);
            _oclu_debug_buffer_info[5] = get_local_size(1);
            _oclu_debug_buffer_info[6] = get_local_size(2);
        }

#if defined(cl_khr_subgroups) || defined(cl_intel_subgroups)
        const ushort sgid  = get_sub_group_id();
        const ushort sglid = get_sub_group_local_id();
        const uint sginfo  = sgid * 65536u + sglid;
#else
        const uint sginfo  = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
#endif
        _oclu_debug_buffer_info[_oclu_thread_id + 7] = sginfo;
    }
)"sv);
            }
            else
                NLRT_THROW_EX(u"Repalcer-Func [DebugString] only support inside kernel.");
        } return;
        default: break;
        }
    }
    NLRT_THROW_EX(u"replace-function not ready"sv);
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
Arg NLCLRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas, const FuncTarget target)
{
    if (IsBeginWith(call.Name, U"oclu."sv))
    {
        switch (const auto subName = call.Name.substr(5); hash_(subName))
        {
        HashCase(subName, U"GetVecTypeName")
        {
            const auto arg = EvaluateFuncArgs<1>(call, { Arg::Type::String })[0];
            const auto vname = arg.GetStr().value();
            const auto info = ParseVecType(vname);
            if (info.has_value())
            {
                const auto str = GetVecTypeName(info.value());
                if (!str.empty())
                    return str;
                else
                    NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not supported"sv), vname));
            }
            else
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not recognized as VecType"sv), vname));
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
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            if (!EnableUnroll)
            {
                Logger.warning(u"Manually enable unroll hint.\n");
                EnableUnroll = true;
            }
        } return {};
        HashCase(subName, U"EnableDebug")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            Logger.verbose(u"Manually enable debug.\n");
            AllowDebug = true;
        } return {};
        HashCase(subName, U"Log")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
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
                    Logger.error(u"Error when formating inner log: {}\n", nfe.message);
                }
            }
        } return {};
        HashCase(subName, U"DefineDebugString")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            ThrowByArgCount(call, 3, ArgLimits::AtLeast);
            const auto arg2 = EvaluateFuncArgs<2, ArgLimits::AtLeast>(call, { Arg::Type::String, Arg::Type::String });
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
                const auto vtype = ParseVecType(arg.GetStr().value());
                if (!vtype.has_value())
                    NLRT_THROW_EX(fmt::format(u"Arg[{}] of [DefineDebugString], [{}] is not a recognized type"sv, i, arg.GetStr().value()), call);
                argInfos.push_back(vtype.value());
            }
            if (!AddPatchedBlock(*this, id, &NLCLRuntime::DebugStringPatch, id, formatter, argInfos))
                NLRT_THROW_EX(fmt::format(u"DebugString [{}] repeately defined"sv, id), call);
        } return {};
        HashCase(subName, U"AddSubgroupPatch")
        {
            ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Block);
            ThrowByArgCount(call, 2, ArgLimits::AtLeast);
            const auto args = EvaluateFuncArgs<4, ArgLimits::AtMost>(call, { Arg::Type::Boolable, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            const auto isShuffle = args[0].GetBool().value();
            const auto vtype = args[1].GetStr().value();

            SubgroupAttributes sgAttr;
            sgAttr.Mimic = SubgroupMimicParser(args[2].GetStr().value_or(std::u32string_view{}))
                .value_or(SubgroupAttributes::MimicType::Auto);
            sgAttr.Args = common::strchset::to_string(args[3].GetStr().value_or(std::u32string_view{}), Charset::ASCII);
            const auto solver = NLCLSubgroup::Generate(this, sgAttr);
            std::array strs{ vtype, U"x"sv, U"y"sv };

            solver->ReplaceFunc(isShuffle ? U"SubgroupShuffle"sv : U"SubgroupBroadcast"sv, strs, nullptr);
            solver->Finish(nullptr);
        } return {};
        default: break;
        }
    }
    return NailangRuntimeBase::EvaluateFunc(call, metas, target);
}

void NLCLRuntime::DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst, BlockCookie* cookie, std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx)
{
    OutputConditions(metas, dst);
    common::str::StrVariant<char32_t> source(block.Source);
    bool repVar = false, repFunc = false;
    for (const auto& fcall : metas)
    {
        if (fcall.Name == U"oclu.ReplaceVariable"sv)
            repVar = true;
        else if (fcall.Name == U"oclu.ReplaceFunction"sv)
            repFunc = true;
        else if (fcall.Name == U"oclu.PreAssign"sv)
        {
            ThrowByArgCount(fcall, 2, ArgLimits::Exact);
            ThrowByArgType(fcall, RawArg::Type::Var, 0);
            if (!evalCtx)
                evalCtx = ConstructEvalContext();
            evalCtx->SetArg(fcall.Args[0].GetVar<RawArg::Type::Var>().Name, EvaluateArg(fcall.Args[1]));
        }
    }
    auto scope = InnerScope(evalCtx);
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
        if (meta.Name == U"If"sv)
            prefix = U"If "sv;
        else if (meta.Name == U"Skip"sv)
            prefix = U"Skip if "sv;
        else
            continue;
        if (meta.Args.size() == 1)
            APPEND_FMT(dst, U"/* {:7} :  {} */\r\n"sv, prefix, xziar::nailang::Serializer::Stringify(meta.Args[0]));
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


void NLCLRuntime::OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    KernelCookie cookie;
    KernelArgStore argInfos;
    SubgroupAttributes sgAttr;
    std::vector<std::u32string> argTexts;
    APPEND_FMT(dst, U"\r\n/* From KernelBlock [{}] */\r\n"sv, block.Name);
    for (auto meta : metas)
    {
        if (meta.Name == U"MetaIf")
        {
            const auto args = EvaluateFuncArgs<2, ArgLimits::AtLeast>(meta, { Arg::Type::Boolable, Arg::Type::String });
            if (!args[0].GetBool().value())
                continue;
            meta.Name = args[1].GetStr().value();
            meta.Args = meta.Args.subspan(2);
        }
        if (!IsBeginWith(meta.Name, U"oclu.")) continue;
        switch (const auto subName = meta.Name.substr(5); hash_(subName))
        {
        HashCase(subName, U"RequestWorkgroupSize")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::Integer, Arg::Type::Integer, Arg::Type::Integer });
            const auto x = args[0].GetUint().value_or(1),
                       y = args[1].GetUint().value_or(1),
                       z = args[2].GetUint().value_or(1);
            cookie.WorkgroupSize = gsl::narrow_cast<uint32_t>(x * y * z);
            APPEND_FMT(dst, U"__attribute__((reqd_work_group_size({}, {}, {})))\r\n"sv, x, y, z);
        } break;
        HashCase(subName, U"SubgroupSize")
        {
            const auto sgSize = EvaluateFuncArgs<1>(meta, { Arg::Type::Integer })[0].GetUint().value();
            cookie.SubgroupSize = gsl::narrow_cast<uint8_t>(sgSize);
        } break;
        HashCase(subName, U"SubgroupExt")
        {
            const auto args = EvaluateFuncArgs<2, ArgLimits::AtMost>(meta, { Arg::Type::String, Arg::Type::String });
            sgAttr.Mimic = SubgroupMimicParser(args[0].GetStr().value_or(std::u32string_view{}))
                .value_or(SubgroupAttributes::MimicType::Auto);
            sgAttr.Args = common::strchset::to_string(args[1].GetStr().value_or(std::u32string_view{}), Charset::ASCII);
        } break;
        HashCase(subName, U"SimpleArg")
        {
            const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            KernelArgInfo info;
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
            argTexts.emplace_back(fmt::format(FMT_STRING(U"{:8} {:5} {} {}"sv),
                ArgFlags::ToCLString(space.value()),
                HAS_FIELD(flags, KerArgFlag::Const) ? U"const"sv : U""sv,
                argType,
                name));
            argInfos.AddArg(KerArgType::Simple, space.value(), ImgAccess::None, KerArgFlag::None, // const will be ignored for simple arg
                common::strchset::to_string(name, Charset::UTF8, Charset::UTF32),
                common::strchset::to_string(argType, Charset::UTF8, Charset::UTF32));
        } break;
        HashCase(subName, U"BufArg")
        {
            const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
            KernelArgInfo info;
            const auto space_  = args[0].GetStr().value();
            const auto space   = KerArgSpaceParser(space_);
            if (!space) 
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"Unrecognized KerArgSpace [{}]"sv), space_), &meta);
            const auto argType = std::u32string(args[1].GetStr().value()) + U'*';
            const auto name    = args[2].GetStr().value();
            const auto flags   = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
                .Reduce([&](KerArgFlag flag, std::u32string_view part) 
                    {
                        if (part == U"const"sv)     return flag | KerArgFlag::Const;
                        if (part == U"restrict"sv)  return flag | KerArgFlag::Restrict;
                        if (part == U"volatile"sv)  return flag | KerArgFlag::Volatile;
                        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Unrecognized KerArgFlag [{}]"sv), part),
                            &meta);
                        return flag;
                    }, KerArgFlag::None);
            if (space.value() == KerArgSpace::Private)
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"BufArg [{}] cannot be in private space: [{}]"sv), name, space_), &meta);
            argTexts.emplace_back(fmt::format(FMT_STRING(U"{:8} {:5} {} {} {} {}"sv),
                ArgFlags::ToCLString(space.value()),
                HAS_FIELD(flags, KerArgFlag::Const)    ? U"const"sv    : U""sv,
                HAS_FIELD(flags, KerArgFlag::Volatile) ? U"volatile"sv : U""sv,
                argType,
                HAS_FIELD(flags, KerArgFlag::Restrict) ? U"restrict"sv : U""sv,
                name));
            argInfos.AddArg(KerArgType::Buffer, space.value(), ImgAccess::None, flags,
                common::strchset::to_string(name,    Charset::UTF8, Charset::UTF32),
                common::strchset::to_string(argType, Charset::UTF8, Charset::UTF32));
        } break;
        HashCase(subName, U"ImgArg")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
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
            argInfos.AddArg(KerArgType::Image, KerArgSpace::Global, access.value(), KerArgFlag::None,
                common::strchset::to_string(name, Charset::UTF8, Charset::UTF32),
                common::strchset::to_string(argType, Charset::UTF8, Charset::UTF32));
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
            argInfos.HasDebug = true;
            argInfos.DebugBuffer = gsl::narrow_cast<uint32_t>(
                EvaluateFuncArgs<1, ArgLimits::AtMost>(meta, { Arg::Type::Integer })[0].GetUint().value_or(512));
        }
        default:
            break;
        }
    }
    if (sgAttr.Mimic != SubgroupAttributes::MimicType::None)
    {
        const bool specifiedSgSize = cookie.SubgroupSize != 0;
        if (cookie.SubgroupSize == 0)
            cookie.SubgroupSize = gsl::narrow_cast<uint8_t>(cookie.WorkgroupSize);
        if (cookie.WorkgroupSize != cookie.SubgroupSize && cookie.WorkgroupSize != 0)
            NLRT_THROW_EX(fmt::format(u"MimicSubgroup requires only 1 subgroup for an workgroup, now have subgroup[{}] and workgroup [{}]."sv,
                cookie.SubgroupSize, cookie.WorkgroupSize));
        else if (cookie.SubgroupSize == 0) // means SubgroupSize == 0, WorkgroupSize == 0
            NLRT_THROW_EX(u"MimicSubgroup without knowing subgroupSize."sv);
        if (!specifiedSgSize && Device->GetPlatform()->PlatVendor == oclu::Vendors::Intel)
        {
            EnableExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv);
            APPEND_FMT(dst, U"__attribute__((intel_reqd_sub_group_size({})))\r\n"sv, cookie.SubgroupSize);
        }
    }
    if (cookie.SubgroupSize > 0)
    {
        if (EnableExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv))
            APPEND_FMT(dst, U"__attribute__((intel_reqd_sub_group_size({})))\r\n"sv, cookie.SubgroupSize);
        else
            Logger.verbose(u"Skip subgroup size due to non-intel platform.\n");
    }
    APPEND_FMT(dst, U"kernel void {} ("sv, block.Name);
    for (const auto& argText : argTexts)
    {
        dst.append(U"\r\n    "sv).append(argText).append(U","sv);
    }
    if (argInfos.HasDebug)
    {
        dst .append(U"\r\n    const  uint           _oclu_debug_buffer_size,"sv)
            .append(U"\r\n    global uint* restrict _oclu_debug_buffer_info,"sv)
            .append(U"\r\n    global uint* restrict _oclu_debug_buffer_data,"sv);
    }
    dst.pop_back(); // remove additional ','
    dst.append(U")\r\n{\r\n"sv);
    std::u32string content;

    sgAttr.SubgroupSize = cookie.SubgroupSize;
    cookie.SubgroupSolver = NLCLSubgroup::Generate(this, sgAttr);

    DirectOutput(block, metas, content, &cookie);
    cookie.SubgroupSolver->Finish(&cookie);

    for (const auto& [key, val] : cookie.Injects)
    {
        APPEND_FMT(dst, U"    // below injected by {}\r\n"sv, key);
        dst.append(val);
        dst.append(U"\r\n"sv);
    }
    dst.append(content);
    dst.append(U"}\r\n"sv);
    CompiledKernels.emplace_back(common::strchset::to_string(block.Name, Charset::UTF8, Charset::UTF32), std::move(argInfos));
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
        HashCase(subName, U"KernelStub") output = false; KernelStubBlocks.emplace_back(&block, metas); break;
        HashCase(subName, U"Template")
        {
            output = false;
            common::span<const RawArg> tpArgs;
            for (const auto& meta : metas)
            {
                if (meta.Name == U"oclu.TemplateArgs")
                {
                    for (uint32_t i = 0; i < meta.Args.size(); ++i)
                    {
                        if (meta.Args[i].TypeData != RawArg::Type::Var)
                            NLRT_THROW_EX(fmt::format(FMT_STRING(u"TemplateArgs's arg[{}] is [{}]. not [Var]"sv),
                                i, ArgTypeName(meta.Args[i].TypeData)), meta, &block);
                    }
                    tpArgs = meta.Args;
                    break;
                }
            }
            TemplateBlocks.emplace_back(&block, metas, std::make_unique<TemplateBlockInfo>(tpArgs));
        } break;
        default: break;
        }
        if (output)
            OutputBlocks.emplace_back(&block, metas);
    }
}

std::string NLCLRuntime::GenerateOutput()
{
    DebugManager.InfoMan = SupportBasicSubgroup ?
        static_cast<std::unique_ptr<oclDebugInfoMan>>(std::make_unique<SubgroupInfoMan>()) : 
        static_cast<std::unique_ptr<oclDebugInfoMan>>(std::make_unique<NonSubgroupInfoMan>());
    std::u32string prefix, output;

    for (const auto& item : OutputBlocks)
    {
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
                            APPEND_FMT(prefix, U"#pragma OPENCL EXTENSION {} : enable\r\n"sv, pair.second);
                    });
        }
        prefix.append(U"\r\n"sv);
    }

    { // Output patched blocks
        for (const auto& [id, src] : PatchedBlocks)
        {
            APPEND_FMT(prefix, U"/* Patched Block [{}] */\r\n"sv, id);
            prefix.append(src);
            prefix.append(U"\r\n\r\n"sv);
        }
    }

    return common::strchset::to_string(prefix + output, Charset::UTF8, Charset::UTF32);
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
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
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

std::unique_ptr<NLCLResult> NLCLProcessor::CompileIntoProgram(NLCLProgStub& stub, const oclContext& ctx, const oclu::CLProgConfig& config) const
{
    auto str = stub.Runtime->GenerateOutput();
    try
    {
        auto progStub = oclProgram_::Create(ctx, str, stub.Device);
        progStub.ImportedKernelInfo = std::move(stub.Runtime->CompiledKernels);
        progStub.DebugManager = std::move(stub.Runtime->DebugManager);
        progStub.Build(config);
        return std::make_unique<NLCLBuiltResult>(stub.Runtime->EvalContext, progStub.Finish());
    }
    catch (const common::BaseException& be)
    {
        return std::make_unique<NLCLBuildFailResult>(stub.Runtime->EvalContext, std::move(str), be.Share());
    }
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
    return std::make_unique<NLCLUnBuildResult>(stub.Runtime->EvalContext, stub.Runtime->GenerateOutput());
}

std::unique_ptr<NLCLResult> NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, oclDevice dev, const common::CLikeDefines& info, const oclu::CLProgConfig& config) const
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(Logger(), dev, info));
    ConfigureCL(stub);
    return CompileIntoProgram(stub, ctx, config);
}


}