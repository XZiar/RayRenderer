#include "oclPch.h"
#include "NLCLSubgroup.h"
#include "oclPlatform.h"


using namespace std::string_view_literals;

template<bool IsPrefix>
struct OptionalArg
{
    std::u32string_view Arg;
};

template<typename Char, bool IsPrefix>
struct fmt::formatter<OptionalArg<IsPrefix>, Char>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) const
    {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const OptionalArg<IsPrefix>& arg, FormatContext& ctx) const
    {
        if (arg.Arg.empty())
            return ctx.out();
        if constexpr (IsPrefix)
            return fmt::format_to(ctx.out(), FMT_STRING(U"{}, "), arg.Arg);
        else
            return fmt::format_to(ctx.out(), FMT_STRING(U", {}"), arg.Arg);
    }
};

namespace oclu
{
using namespace std::string_literals;
using xcomp::ReplaceResult;
using xcomp::U32StrSpan;
using xziar::nailang::Arg;
using xziar::nailang::ArgLimits;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;
using common::str::Encoding;


constexpr char32_t Idx16Names[] = U"0123456789abcdef";

#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define RET_FAIL(func) return {U"No proper [" STRINGIZE(func) "]"sv, false}


NLCLSubgroupExtension::NLCLSubgroupExtension(common::mlog::MiniLogger<false>& logger, NLCLContext& context) :
    NLCLExtension(context), Logger(logger)
{
    DefaultProvider = Generate(logger, {}, {});
}

void NLCLSubgroupExtension::BeginInstance(xcomp::XCNLRuntime& runtime, xcomp::InstanceContext& ctx)
{
    SubgroupSize = 0;
    Provider = DefaultProvider;
    Provider->OnBegin(static_cast<NLCLRuntime&>(runtime), *this, static_cast<KernelContext&>(ctx));
}

void NLCLSubgroupExtension::FinishInstance(xcomp::XCNLRuntime& runtime, xcomp::InstanceContext& ctx)
{
    Provider->OnFinish(static_cast<NLCLRuntime&>(runtime), *this, static_cast<KernelContext&>(ctx));
    Provider.reset();
    SubgroupSize = 0;
}

void NLCLSubgroupExtension::InstanceMeta(xcomp::XCNLExecutor& executor, const xziar::nailang::FuncPack& meta, xcomp::InstanceContext&)
{
    auto& runtime = GetRuntime(executor);
    if (meta.GetName() == U"oclu.SubgroupSize"sv || meta.GetName() == U"xcomp.SubgroupSize"sv)
    {
        executor.ThrowByParamTypes<1>(meta, { Arg::Type::Integer });
        SubgroupSize = gsl::narrow_cast<uint8_t>(meta.Params[0].GetUint().value());
    }
    else if (meta.GetName() == U"oclu.SubgroupExt"sv)
    {
        executor.ThrowByParamTypes<2, ArgLimits::AtMost>(meta, { Arg::Type::String, Arg::Type::String });
        const auto mimic = meta.TryGet(0, &Arg::GetStr).Or({});
        const auto args  = meta.TryGet(1, &Arg::GetStr).Or({});
        Provider = Generate(GetLogger(runtime), mimic, args);
    }
}

template<typename... Args>
static forceinline ReplaceResult HandleResult(ReplaceResult result, std::u32string_view msg, Args&&... args)
{
    if (!result && result.GetStr().empty())
    {
        if constexpr (sizeof...(Args) == 0)
            result.SetStr(std::move(msg));
        else
            result.SetStr(fmt::format(msg, std::forward<Args>(args)...));
    }
    return result;
}

ReplaceResult NLCLSubgroupExtension::ReplaceFunc(xcomp::XCNLRawExecutor& executor, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (common::str::IsBeginWith(func, U"oclu.Subgroup."))
        func.remove_prefix(14);
    else if(common::str::IsBeginWith(func, U"xcomp.Subgroup."))
        func.remove_prefix(15);
    else
        return {};
    auto& runtime = executor.GetRuntime();

    switch (common::DJBHash::HashC(func))
    {
    HashCase(func, U"GetSize")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->FuncGetSize(),
            U"[Subgroup.GetSize] not supported"sv);
    }
    HashCase(func, U"GetMaxSize")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->FuncGetMaxSize(),
            U"[Subgroup.GetMaxSize] not supported"sv);
    }
    HashCase(func, U"GetCount")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->FuncGetCount(),
            U"[Subgroup.GetCount] not supported"sv);
    }
    HashCase(func, U"GetId")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->FuncGetId(),
            U"[Subgroup.GetId] not supported"sv);
    }
    HashCase(func, U"GetLocalId")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->FuncGetLocalId(),
            U"[Subgroup.GetLocalId] not supported"sv);
    }
    HashCase(func, U"All")
    {
        executor.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
        return HandleResult(Provider->FuncAll(args[0]),
            U"[Subgroup.All] not supported"sv);
    }
    HashCase(func, U"Any")
    {
        executor.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
        return HandleResult(Provider->FuncAny(args[0]),
            U"[Subgroup.Any] not supported"sv);
    }
#define ShuffleCase(op) HashCase(func, U"" #op)                                                         \
    {                                                                                                   \
        executor.ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);                              \
        const auto vtype = runtime.ParseVecDataInfo(args[0], u"replace [Subgroup." #op "]"sv);          \
        return HandleResult(Provider->FuncShuffle(vtype, args.subspan(1), SubgroupShuffleOp::op, false),\
            U"[Subgroup." #op "] with Type [{}] not supported"sv, args[0]);                             \
    }
    ShuffleCase(Broadcast)
    ShuffleCase(Shuffle)
    ShuffleCase(ShuffleXor)
#undef ShuffleCase
#define ReduceCase(op) HashCase(func, U"Reduce" #op)                                                    \
    {                                                                                                   \
        executor.ThrowByReplacerArgCount(func, args, 2, ArgLimits::Exact);                              \
        const auto vtype = runtime.ParseVecDataInfo(args[0], u"replace [Subgroup.Reduce" #op "]"sv);    \
        return HandleResult(Provider->FuncReduce(vtype, args.subspan(1), SubgroupReduceOp::op, false),  \
            U"[Subgroup.Reduce" #op "] with Type [{}] not supported"sv, args[0]);                       \
    }
    ReduceCase(Add)
    ReduceCase(Mul)
    ReduceCase(Max)
    ReduceCase(Min)
    ReduceCase(And)
    ReduceCase(Or)
    ReduceCase(Xor)
#undef ReduceCase
    default: break;
    }
    return {};
}

std::optional<Arg> NLCLSubgroupExtension::ConfigFunc(xcomp::XCNLExecutor& executor, xziar::nailang::FuncEvalPack& func)
{
    auto& runtime = GetRuntime(executor);
    if (func.GetName() == U"oclu.AddSubgroupPatch"sv)
    {
        executor.ThrowIfNotFuncTarget(func, xziar::nailang::FuncName::FuncInfo::Empty);
        //Executor.ThrowByArgCount(func, 2, ArgLimits::AtLeast);
        executor.ThrowByParamTypes<2, 4>(func, { Arg::Type::BoolBit, Arg::Type::String, Arg::Type::String, Arg::Type::String });
        const auto isShuffle = func.Params[0].GetBool().value();
        const auto vstr  = func.Params[1].GetStr().value();
        const auto vtype = runtime.ParseVecDataInfo(vstr, u"call [AddSubgroupPatch]"sv);

        KernelContext kerCtx;
        SubgroupSize = 32;
        const auto mimic = func.TryGet(2, &Arg::GetStr).Or({});
        const auto args  = func.TryGet(3, &Arg::GetStr).Or({});
        const auto provider = Generate(Logger, mimic, args);
        const std::u32string_view dummyArgs[] = { U"x"sv, U"y"sv };
        const auto ret = provider->FuncShuffle(vtype, dummyArgs,
            isShuffle ? SubgroupShuffleOp::Shuffle : SubgroupShuffleOp::Broadcast, false);
        provider->OnFinish(runtime, *this, kerCtx);
        
        return Arg{};
    }
    return {};
}

NLCLSubgroupCapbility NLCLSubgroupExtension::GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr)
{
    NLCLSubgroupCapbility cap(context);
    for (auto arg : common::str::SplitStream(attr.Args, ',', false))
    {
        if (arg.empty()) continue;
        if (common::str::IsBeginWith(arg, "nv_sm") && arg.size() > 5)
        {
            uint32_t smVer = 0;
            common::StrToInt(arg.substr(5), smVer);
            cap.NvSmVer = static_cast<uint8_t>(smVer);
            continue;
        }
        const bool isDisable = arg[0] == '-';
        if (isDisable) arg.remove_prefix(1);
        switch (common::DJBHash::HashC(arg))
        {
#define Mod(dst, name) HashCase(arg, name) cap.Support##dst = !isDisable; break
        Mod(KHR,            "sg_khr");
        Mod(KHRExtType,     "sg_khrexttype");
        Mod(KHRShuffle,     "sg_khrshuf");
        Mod(KHRShuffleRel,  "sg_khrshufrel");
        Mod(KHRBallot,      "sg_khrballot");
        Mod(KHRNUArith,     "sg_khrnuarith");
        Mod(Intel,          "sg_intel");
        Mod(Intel8,         "sg_intel8");
        Mod(Intel16,        "sg_intel16");
        Mod(Intel64,        "sg_intel64");
        Mod(FP16,           "fp16");
        Mod(FP64,           "fp64");
#undef Mod
        default: break;
        }
    }
    cap.SupportBasicSubgroup = cap.SupportKHR || cap.SupportIntel;
    return cap;
}

constexpr auto SubgroupMimicParser = SWITCH_PACK(
    (U"local",  SubgroupAttributes::MimicType::Local),
    (U"khr",    SubgroupAttributes::MimicType::Khr),
    (U"intel",  SubgroupAttributes::MimicType::Intel),
    (U"ptx",    SubgroupAttributes::MimicType::Ptx),
    (U"auto",   SubgroupAttributes::MimicType::Auto),
    (U"none",   SubgroupAttributes::MimicType::None));
template<typename T, typename... Args>
inline std::shared_ptr<SubgroupProvider> NLCLSubgroupExtension::GenerateProvider(Args&&... args)
{
    auto ret = std::make_shared<T>(std::forward<Args>(args)...);
    ret->Prepare();
    return ret;
}
std::shared_ptr<SubgroupProvider> NLCLSubgroupExtension::Generate(common::mlog::MiniLogger<false>& logger,
    std::u32string_view mimic, std::u32string_view args)
{
    SubgroupAttributes attr;
    attr.Mimic = SubgroupMimicParser(mimic).value_or(SubgroupAttributes::MimicType::Auto);
    attr.Args = common::str::to_string(args, Encoding::ASCII);
    auto cap = GenerateCapabiity(Context, attr);

    SubgroupAttributes::MimicType mType = attr.Mimic;
    if (mType == SubgroupAttributes::MimicType::Auto)
    {
        if (Context.Device->PlatVendor == Vendors::NVIDIA)
            mType = SubgroupAttributes::MimicType::Ptx;
        else if (cap.SupportIntel)
            mType = SubgroupAttributes::MimicType::Intel;
        else if (cap.SupportBasicSubgroup)
            mType = SubgroupAttributes::MimicType::Khr;
        else
            mType = SubgroupAttributes::MimicType::Local;
    }
    Ensures(mType != SubgroupAttributes::MimicType::Auto);

    if (mType == SubgroupAttributes::MimicType::Intel)
        return GenerateProvider<NLCLSubgroupIntel>(logger, Context, cap);
    else if (mType == SubgroupAttributes::MimicType::Ptx)
        return GenerateProvider<NLCLSubgroupPtx>(logger, Context, cap, 
            Context.Device->GetNvidiaSMVersion().value_or(cap.NvSmVer));
    else if (mType == SubgroupAttributes::MimicType::Local)
        return GenerateProvider<NLCLSubgroupLocal>(logger, Context, cap);
    else if (mType == SubgroupAttributes::MimicType::Khr)
        return GenerateProvider<NLCLSubgroupKHR>(logger, Context, cap);
    else
        return GenerateProvider<SubgroupProvider>(logger, Context, cap);
}


static constexpr std::u32string_view StringifyReduceOp(SubgroupReduceOp op)
{
    switch (op)
    {
    case SubgroupReduceOp::Add: return U"add"sv;
    case SubgroupReduceOp::Mul: return U"mul"sv;
    case SubgroupReduceOp::Min: return U"min"sv;
    case SubgroupReduceOp::Max: return U"max"sv;
    case SubgroupReduceOp::And: return U"and"sv;
    case SubgroupReduceOp::Or:  return U"or"sv;
    case SubgroupReduceOp::Xor: return U"xor"sv;
    default: assert(false);     return U""sv;
    }
}
static constexpr bool IsReduceOpArith(SubgroupReduceOp op)
{
    switch (op)
    {
    case SubgroupReduceOp::Add:
    case SubgroupReduceOp::Mul:
    case SubgroupReduceOp::Min:
    case SubgroupReduceOp::Max:
        return true;
    default:
        return false;
    }
}


struct ArgWithType
{
    std::u32string_view Arg;
};
template<typename T>
static void PutArg(std::u32string& txt, bool& needComma, const T& arg, 
    [[maybe_unused]] const std::u32string& pfx, [[maybe_unused]] const std::u32string& sfx)
{
    if constexpr (std::is_same_v<T, ArgWithType>)
    {
        if (arg.Arg.empty())
            return;
        if (needComma)
            txt.append(U", "sv);
        txt.append(pfx).append(arg.Arg).append(sfx);
        needComma = true;
    }
    else
    {
        if (arg.empty())
            return;
        if (needComma)
            txt.append(U", "sv);
        txt.append(arg);
        needComma = true;
    }
}
template<typename... Args>
static std::u32string TypedAlgoCallText(const SubgroupProvider::TypedAlgoResult& res, VecDataInfo origType,
    const Args&... args)
{
    if (!res) return {};
    const bool step1 = origType != res.SrcType, step2 = res.SrcType != res.DstType;
    std::u32string txt, argPfx, argSfx;
    // prepare argPfx, argSfx
    if (step2)
        argPfx.append(U"as_").append(oclu::NLCLRuntime::GetCLTypeName(res.DstType)).append(U"("sv);
    if (step1)
        argPfx.append(U"convert_").append(oclu::NLCLRuntime::GetCLTypeName(res.SrcType)).append(U"("sv);
    if (step1)
        argSfx.append(U")"sv);
    if (step2)
        argSfx.append(U")"sv);
    // fill final text
    if (step1)
        txt.append(U"convert_").append(oclu::NLCLRuntime::GetCLTypeName(origType)).append(U"("sv);
    if (step2)
        txt.append(U"as_").append(oclu::NLCLRuntime::GetCLTypeName(res.SrcType)).append(U"("sv);
    txt.append(res.GetFuncName()).append(U"(");
    if (res.Extra)
        txt.append(res.Extra.Arg).append(U", ");
    // fill arg
    bool needComma = false;
    (..., PutArg(txt, needComma, args, argPfx, argSfx));
    // finish
    txt.append(U")"sv);
    if (step2)
        txt.append(U")"sv);
    if (step1)
        txt.append(U")"sv);
    return txt;
}


struct SubgroupProvider::Shuffle
{
    static constexpr uint8_t FeatureBroadcast   = 0x01;
    static constexpr uint8_t FeatureShuffle     = 0x02;
    static constexpr uint8_t FeatureShuffleRel  = 0x04;
    static constexpr uint8_t FeatureShuffleRel2 = 0x08;
    static constexpr uint8_t FeatureNonUniform  = 0x10;
    static constexpr auto SupportMap = &SubgroupProvider::ShuffleSupport;
    static constexpr auto HandleAlgo = &SubgroupProvider::HandleShuffleAlgo;
    [[nodiscard]] forceinline static std::u32string VectorPatchText(const std::u32string_view name, const TypedAlgoResult& scalarAlgo,
        VecDataInfo vtype) noexcept
    {
        return SubgroupProvider::VectorPatchText(name, scalarAlgo.GetFuncName(), vtype, scalarAlgo.DstType,
            scalarAlgo.Extra.Param, scalarAlgo.Extra.Arg, U"const uint sgId", U"sgId");
    }
    [[nodiscard]] static std::u32string GenerateFuncName(std::u32string_view suffix, VecDataInfo vtype, SubgroupShuffleOp op) noexcept
    {
        std::u32string_view opName;
        switch (op)
        {
        case SubgroupShuffleOp::Broadcast:   opName = U"broadcast"sv;    break;
        case SubgroupShuffleOp::Shuffle:     opName = U"shuffle"sv;      break;
        case SubgroupShuffleOp::ShuffleXor:  opName = U"shuffle_xor"sv;  break;
        case SubgroupShuffleOp::ShuffleDown: opName = U"shuffle_down"sv; break;
        case SubgroupShuffleOp::ShuffleUp:   opName = U"shuffle_up"sv;   break;
        default: break;
        }
        return SubgroupProvider::GenerateFuncName(opName, suffix, vtype);
    }
    [[nodiscard]] static bool IsBitwise(SubgroupShuffleOp) noexcept
    {
        return true;
    }
    [[nodiscard]] static std::u32string GenerateText(const TypedAlgoResult& result, VecDataInfo vtype, xcomp::U32StrSpan args,
        SubgroupShuffleOp op) noexcept
    {
        if (result)
        {
            if (op == SubgroupShuffleOp::ShuffleDown || op == SubgroupShuffleOp::ShuffleUp)
                return TypedAlgoCallText(result, vtype, ArgWithType{ args[0] }, ArgWithType{ args[1] }, args[2]);
            else
                return TypedAlgoCallText(result, vtype, ArgWithType{ args[0] }, args[1]);
        }
        return {};
    }
};

struct SubgroupProvider::Reduce
{
    static constexpr uint8_t FeatureBitwise    = 0x01;
    static constexpr uint8_t FeatureArith      = 0x02;
    static constexpr uint8_t FeatureArithMul   = 0x04;
    static constexpr uint8_t FeatureNonUniform = 0x08;
    static constexpr auto SupportMap = &SubgroupProvider::ReduceSupport;
    static constexpr auto HandleAlgo = &SubgroupProvider::HandleReduceAlgo;
    [[nodiscard]] forceinline static std::u32string VectorPatchText(const std::u32string_view name, const TypedAlgoResult& scalarAlgo,
        VecDataInfo vtype) noexcept
    {
        return SubgroupProvider::VectorPatchText(name, scalarAlgo.GetFuncName(), vtype, scalarAlgo.DstType,
            scalarAlgo.Extra.Param, scalarAlgo.Extra.Arg, {}, {});
    }
    [[nodiscard]] static std::u32string GenerateFuncName(std::u32string_view suffix, VecDataInfo vtype, SubgroupReduceOp op) noexcept
    {
        std::u32string_view opName;
        switch (op)
        {
        case SubgroupReduceOp::Add: opName = U"reduce_add"sv; break;
        case SubgroupReduceOp::Mul: opName = U"reduce_mul"sv; break;
        case SubgroupReduceOp::Min: opName = U"reduce_min"sv; break;
        case SubgroupReduceOp::Max: opName = U"reduce_max"sv; break;
        case SubgroupReduceOp::And: opName = U"reduce_and"sv; break;
        case SubgroupReduceOp::Or:  opName = U"reduce_or"sv;  break;
        case SubgroupReduceOp::Xor: opName = U"reduce_xor"sv; break;
        default: assert(false);
        }
        return SubgroupProvider::GenerateFuncName(opName, suffix, vtype);
    }
    [[nodiscard]] static bool IsBitwise(SubgroupReduceOp op) noexcept
    {
        switch (op)
        {
        case SubgroupReduceOp::And:
        case SubgroupReduceOp::Or:
        case SubgroupReduceOp::Xor:
            return true;
        default:
            return false;
        }
    }
    [[nodiscard]] static std::u32string GenerateText(const TypedAlgoResult& result, VecDataInfo vtype, xcomp::U32StrSpan args,
        SubgroupReduceOp) noexcept
    {
        if (result)
        {
            return TypedAlgoCallText(result, vtype, ArgWithType{ args[0] });
        }
        return {};
    }
};


SubgroupProvider::SubgroupProvider(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    Logger(logger), Context(context), Cap(cap)
{ }

void SubgroupProvider::Prepare()
{
    std::stable_sort(ShuffleSupport.begin(), ShuffleSupport.end(), [](const auto& l, const auto& r) { return l.ID.Raw < r.ID.Raw; });
    std::stable_sort(ReduceSupport.begin(), ReduceSupport.end(), [](const auto& l, const auto& r) { return l.ID.Raw < r.ID.Raw; });
}

void SubgroupProvider::EnableVecType(VecDataInfo type) noexcept 
{
    const auto stype = type.Scalar();
    switch (stype)
    {
    case VecDataInfo{ VecDataInfo::DataTypes::Float, 16, 0, 0 }: EnableFP16 = true; break;
    case VecDataInfo{ VecDataInfo::DataTypes::Float, 64, 0, 0 }: EnableFP64 = true; break;
    default: break;
    }
}

void SubgroupProvider::AddWarning(common::str::StrVariant<char16_t> msg)
{
    for (const auto& item : Warnings)
    {
        if (item == msg.StrView())
            return;
    }
    Warnings.push_back(msg.ExtractStr());
}

void SubgroupProvider::OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension&, KernelContext&)
{
    if (EnableFP16)
        runtime.EnableExtension("cl_khr_fp16"sv);
    if (EnableFP64)
        runtime.EnableExtension("cl_khr_fp64"sv) || runtime.EnableExtension("cl_amd_fp64"sv);
    for (const auto& item : Warnings)
    {
        Logger.warning(item);
    }
}

std::u32string SubgroupProvider::GenerateFuncName(std::u32string_view op, std::u32string_view suffix, common::simd::VecDataInfo vtype) noexcept
{
    return FMTSTR(U"oclu_subgroup_{}_{}_{}", op, suffix, xcomp::StringifyVDataType(vtype));
}

std::u32string SubgroupProvider::VectorPatchText(const std::u32string_view name, const std::u32string_view baseFunc,
    VecDataInfo vtype, VecDataInfo castType, std::u32string_view pfxParam, std::u32string_view pfxArg,
    std::u32string_view sfxParam, std::u32string_view sfxArg) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    Expects(vtype.Bit == castType.Bit * castType.Dim0);
    const auto scalarType = vtype.Scalar(1);
    const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
    const auto castedName = NLCLRuntime::GetCLTypeName(castType);
    const auto vectorName = NLCLRuntime::GetCLTypeName(vtype);
    std::u32string func = FMTSTR(U"inline {0} {1}({2}{3}const {0} val{4}{5})\r\n{{\r\n    {0} ret;", vectorName, name,
        pfxParam, pfxParam.empty() ? U""sv : U", "sv, sfxParam.empty() ? U""sv : U", "sv, sfxParam);
    const auto formatter = scalarType == castType ? U"\r\n    ret.s{0} = {1}({4}{5}val.s{0}{6}{7});"sv : 
        U"\r\n    ret.s{0} = as_{2}({1}({4}{5}as_{3}(val.s{0}){6}{7}));"sv;
    for (uint8_t i = 0; i < vtype.Dim0; ++i)
    {
        fmt::format_to(std::back_inserter(func), formatter, 
            Idx16Names[i], baseFunc, scalarName, castedName,
            pfxArg, pfxArg.empty() ? U""sv : U", "sv, sfxArg.empty() ? U""sv : U", "sv, sfxArg);
    }
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

template<typename Op, typename... Args>
[[nodiscard]] SubgroupProvider::TypedAlgoResult SubgroupProvider::VectorizePatch(const common::simd::VecDataInfo vtype,
    TypedAlgoResult& scalarAlgo, Args&&... args)
{
    TypedAlgoResult wrapped(Op::GenerateFuncName(scalarAlgo.AlgorithmSuffix, vtype, std::forward<Args>(args)...),
        scalarAlgo.AlgorithmSuffix, scalarAlgo.Features, vtype, true);
    Context.AddPatchedBlockEx(wrapped.GetFuncName(), [&](auto& session)
        {
            session.Commit(Op::VectorPatchText(wrapped.GetFuncName(), scalarAlgo, vtype), scalarAlgo.GetDepend());
        });
    wrapped.Extra = std::move(scalarAlgo.Extra);
    return wrapped;
}

[[nodiscard]] SubgroupProvider::TypedAlgoResult SubgroupProvider::ShufReducePatch(const VecDataInfo vtype,
    SubgroupReduceOp op, uint8_t level)
{
    Expects(vtype.Dim0 == 1);
    const auto suffix = GetAlgoSuffix(level);
    TypedAlgoResult wrapped(Reduce::GenerateFuncName(suffix, vtype, op), suffix, 0, vtype, true);
    Context.AddPatchedBlockEx(wrapped.GetFuncName(), [&](auto& session)
        {
            boost::container::small_vector<std::u32string_view, 2> depends;
            const auto sgSizeResult = FuncGetSize();
            const auto shufResult = FuncShuffle(vtype, SubgroupShuffleOp::ShuffleXor, false, level);
            for (const auto dep : sgSizeResult.GetDepends().GetPatchedBlock())
                depends.emplace_back(dep);
            if (const auto dep = shufResult.GetDepend(); !dep.empty())
                depends.emplace_back(dep);
            constexpr std::u32string_view shufArgs[] = { U"x"sv, U"mask"sv };
            const auto shufText = Shuffle::GenerateText(shufResult, vtype, shufArgs, SubgroupShuffleOp::ShuffleXor);
            const auto scalarName = NLCLRuntime::GetCLTypeName(vtype);
            std::u32string_view optxt, optail;
            switch (op)
            {
            case SubgroupReduceOp::Add: optxt = U"x + "sv; break;
            case SubgroupReduceOp::Mul: optxt = U"x * "sv; break;
            case SubgroupReduceOp::Min: optxt = U"min(x, "sv; optail = U")"sv; break;
            case SubgroupReduceOp::Max: optxt = U"max(x, "sv; optail = U")"sv; break;
            case SubgroupReduceOp::And: optxt = U"x & "sv; break;
            case SubgroupReduceOp::Or:  optxt = U"x | "sv; break;
            case SubgroupReduceOp::Xor: optxt = U"x ^ "sv; break;
            default: break;
            }
            auto func = FMTSTR(U"inline {0} {1}({2}{0} x)"sv, scalarName, wrapped.GetFuncName(), OptionalArg<true>{shufResult.Extra.Param});
            func.append(UR"(
{
    for (uint mask = )"sv).append(sgSizeResult.GetStr()).append(UR"(; mask > 0; mask /= 2)
    {
        x = )"sv)
                // x = x & intel_sub_group_shuffle_xor(val, mask);
                .append(optxt).append(shufText).append(optail).append(UR"(;
    }
    return x;
})"sv);
            session.Commit(std::move(func), depends);
        });
    // TODO: Handle Extra
    return wrapped;
}

template<typename Op, bool ExactBit, typename... Args>
SubgroupProvider::TypedAlgoResult SubgroupProvider::HandleSubgroupOperation(VecDataInfo vtype, AlgoChecker algoCheck,
    Args&&... args/*, SubgroupXXXOp op*/)
{
    const auto& supportMap = this->*Op::SupportMap;
    const bool isBitwise = Op::IsBitwise(std::forward<Args>(args)...);
    xcomp::VecDimSupport Algorithm::DTypeSupport::* dtPtr1 = nullptr;
    xcomp::VecDimSupport Algorithm::DTypeSupport::* dtPtr2 = nullptr;
    VecDataInfo::DataTypes alterType = VecDataInfo::DataTypes::Custom;
    switch (vtype.Type)
    {
    case VecDataInfo::DataTypes::Float: 
        dtPtr1 = &Algorithm::DTypeSupport::FP; 
        dtPtr2 = &Algorithm::DTypeSupport::Int; 
        alterType = VecDataInfo::DataTypes::Unsigned;
        break;
    case VecDataInfo::DataTypes::Unsigned: 
    case VecDataInfo::DataTypes::Signed: 
        dtPtr1 = &Algorithm::DTypeSupport::Int; 
        dtPtr2 = &Algorithm::DTypeSupport::FP; 
        alterType = VecDataInfo::DataTypes::Float;
        break;
    default:
        return {};
    }
    Algorithm::DTypeSupport Algorithm::* btPtr = nullptr;
    switch (vtype.Bit)
    {
    case 64: btPtr = &Algorithm::Bit64; break;
    case 32: btPtr = &Algorithm::Bit32; break;
    case 16: btPtr = &Algorithm::Bit16; break;
    case  8: btPtr = &Algorithm::Bit8;  break;
    default: return {};
    }

    // try exact match bit and dtype 
    {
        for (const auto& algo : supportMap)
        {
            if (!algoCheck.Check(algo))
                continue;
            const auto& sup = (algo.*btPtr).*dtPtr1;
            if (sup.Check(vtype.Dim0))
            {
                if (auto ret = (this->*Op::HandleAlgo)(algo, vtype, std::forward<Args>(args)...); ret)
                {
                    // ret.SrcType = vtype;
                    return ret;
                }
            }
        }
    }
    // try only bit match
    if (isBitwise)
    {
        for (const auto& algo : supportMap)
        {
            if (!algoCheck.Check(algo))
                continue;
            const auto& sup = (algo.*btPtr).*dtPtr2;
            if (sup.Check(vtype.Dim0))
            {
                VecDataInfo midType{ alterType, vtype.Bit, vtype.Dim0, 0 };
                if (auto ret = (this->*Op::HandleAlgo)(algo, midType, std::forward<Args>(args)...); ret)
                {
                    ret.SrcType = vtype;
                    return ret;
                }
            }
        }
    }
    // try loop-based
    if (vtype.Dim0 > 1)
    {
        for (const auto& algo : supportMap)
        {
            if (!algoCheck.Check(algo))
                continue;
            const bool sup1 = ((algo.*btPtr).*dtPtr1).Check(1);
            const bool sup2 = ((algo.*btPtr).*dtPtr2).Check(1);
            if (sup1 || (isBitwise && sup2))
            {
                VecDataInfo scalarUType{ sup1 ? vtype.Type : alterType, vtype.Bit, 1, 0 };
                auto ret = (this->*Op::HandleAlgo)(algo, scalarUType, std::forward<Args>(args)...);
                Expects(ret);
                const VecDataInfo vectorUType{ scalarUType.Type, vtype.Bit, vtype.Dim0, 0 };
                auto result = VectorizePatch<Op>(vectorUType, ret, std::forward<Args>(args)...);
                result.SrcType = vtype;
                return result;
            }
        }
    }

    if constexpr (!ExactBit)
    {
        if (isBitwise)
        {
            const auto totalBits = vtype.Dim0 * vtype.Bit;
            // try combine and cast to bigger datatype
            for (const auto bit : std::array<uint8_t, 3>{64, 32, 16})
            {
                if (bit <= vtype.Bit)
                    break;
                if (bit % vtype.Bit != 0 || totalBits % bit != 0)
                    continue;
                const uint8_t newDim0 = static_cast<uint8_t>(totalBits / bit);
                Expects(newDim0 <= 16);
                const VecDataInfo vectorUType{ VecDataInfo::DataTypes::Unsigned, bit, newDim0, 0 };
                if (auto ret = HandleSubgroupOperation<Op, true>(vectorUType, algoCheck, std::forward<Args>(args)...); ret)
                {
                    ret.SrcType = vtype;
                    return ret;
                }
            }
            // try split and cast to smaller datatype
            for (const auto bit : std::array<uint8_t, 3>{32, 16, 8})
            {
                if (bit >= vtype.Bit)
                    continue;
                if (vtype.Bit % bit != 0)
                    continue;
                const uint8_t newDim0 = static_cast<uint8_t>(vtype.Bit / bit);
                const VecDataInfo scalarUType{ vtype.Type, vtype.Bit, 1, 0 };
                const VecDataInfo vectorUType{ VecDataInfo::DataTypes::Unsigned, bit, newDim0, 0 };
                if (auto ret = HandleSubgroupOperation<Op, true>(vectorUType, algoCheck, std::forward<Args>(args)...); ret)
                {
                    if (vtype.Dim0 > 1)
                    {
                        auto result = VectorizePatch<Op>(vtype, ret, std::forward<Args>(args)...);
                        result.SrcType = vtype;
                        return result;
                    }
                    else
                    {
                        ret.SrcType = scalarUType;
                        return ret;
                    }
                }
            }
        }
        // try expend bits to bigger datatype
        for (const auto bit : std::array<uint8_t, 3>{16, 32, 64})
        {
            if (bit <= vtype.Bit)
                continue;
            const VecDataInfo vectorUType{ isBitwise ? VecDataInfo::DataTypes::Unsigned : vtype.Type, bit, vtype.Dim0, 0 };
            if (auto ret = HandleSubgroupOperation<Op, true>(vectorUType, algoCheck, std::forward<Args>(args)...); ret)
            {
                ret.SrcType = vectorUType;
                return ret;
            }
        }
    }

    return {};
}

SubgroupProvider::TypedAlgoResult SubgroupProvider::FuncShuffle(VecDataInfo vtype, SubgroupShuffleOp op, bool nonUniform, uint8_t level)
{
    auto feat = nonUniform ? Shuffle::FeatureNonUniform : 0;
    switch (op)
    {
    case SubgroupShuffleOp::Broadcast:
        feat |= Shuffle::FeatureBroadcast;
        break;
    case SubgroupShuffleOp::Shuffle:
    case SubgroupShuffleOp::ShuffleXor:
        feat |= Shuffle::FeatureShuffle;
        break;
    case SubgroupShuffleOp::ShuffleDown:
    case SubgroupShuffleOp::ShuffleUp:
        feat |= Shuffle::FeatureShuffleRel;
        break;
    case SubgroupShuffleOp::ShuffleDown2:
    case SubgroupShuffleOp::ShuffleUp2:
        feat |= Shuffle::FeatureShuffleRel2;
        break;
    default:
        break;
    }
    return HandleSubgroupOperation<Shuffle, false>(vtype, { level, feat }, op);
}
SubgroupProvider::TypedAlgoResult SubgroupProvider::FuncReduce(VecDataInfo vtype, SubgroupReduceOp op, bool nonUniform, uint8_t level)
{
    auto feat = nonUniform ? Reduce::FeatureNonUniform : 0;
    switch (op)
    {
    case SubgroupReduceOp::And:
    case SubgroupReduceOp::Or:
    case SubgroupReduceOp::Xor:
        feat |= Reduce::FeatureBitwise;
        break;
    case SubgroupReduceOp::Mul:
        feat |= Reduce::FeatureArithMul;
        [[fallthrough]];
    case SubgroupReduceOp::Add:
    case SubgroupReduceOp::Min:
    case SubgroupReduceOp::Max:
        feat |= Reduce::FeatureArith;
        break;
    default:
        break;
    }
    return HandleSubgroupOperation<Reduce, false>(vtype, { level, feat }, op);
}
ReplaceResult SubgroupProvider::FuncShuffle(VecDataInfo vtype, U32StrSpan args, SubgroupShuffleOp op, bool nonUniform)
{
    auto result = FuncShuffle(vtype, op, nonUniform);
    return { Shuffle::GenerateText(result, vtype, args, op), result.GetDepend() };
}
ReplaceResult SubgroupProvider::FuncReduce(VecDataInfo vtype, U32StrSpan args, SubgroupReduceOp op, bool nonUniform)
{
    auto result = FuncReduce (vtype, op, nonUniform);
    return { Reduce:: GenerateText(result, vtype, args, op), result.GetDepend() };
}


NLCLSubgroupKHR::NLCLSubgroupKHR(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    SubgroupProvider(logger, context, cap)
{
    constexpr xcomp::VecDimSupport supportAll = xcomp::VTypeDimSupport::All;
    constexpr xcomp::VecDimSupport support1   = xcomp::VTypeDimSupport::Support1;
    constexpr xcomp::VecDimSupport support0   = xcomp::VTypeDimSupport::Empty;
    if (cap.SupportKHR)
    {
        const auto basicVec = cap.SupportKHRExtType ? supportAll : support1;
        const auto extVec   = cap.SupportKHRExtType ? supportAll : support0;
        // https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_subgroups.asciidoc#add-a-new-section-613xsub-group-functions
        // !FeatureNonUniform: "These functions need not be encountered by all work items in a subgroup executing the kernel."
        auto& algo = ShuffleSupport.emplace_back(LevelKHR, AlgoKHRBroadcast, Shuffle::FeatureBroadcast);
        algo.Bit64.Set(basicVec, cap.SupportFP64 ? basicVec : support0);
        algo.Bit32.Set(basicVec);
        algo.Bit16.Set(  extVec, cap.SupportFP16 ? basicVec : support0);
        if (cap.SupportKHRExtType)
            algo.Bit8.Set(supportAll, support0);
    }
    if (cap.SupportKHRBallot)
    {
        // https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_subgroup_extensions.asciidoc#add-a-new-section-615x---subgroup-ballot-built-in-functions
        // FeatureNonUniform: "These functions need not be encountered by all work items in a subgroup executing the kernel."
        auto& algo = ShuffleSupport.emplace_back(LevelKHR, AlgoKHRBroadcastNU, static_cast<uint8_t>(Shuffle::FeatureBroadcast | Shuffle::FeatureNonUniform));
        algo.Bit64.Set(supportAll, cap.SupportFP64 ? supportAll : support0);
        algo.Bit32.Set(supportAll);
        algo.Bit16.Set(supportAll, cap.SupportFP16 ? supportAll : support0);
        algo.Bit8 .Set(supportAll, support0);
    }
    if (cap.SupportKHRShuffle)
    {
        // https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_subgroup_extensions.asciidoc#add-a-new-section-615x---subgroup-shuffle-built-in-functions
        // FeatureNonUniform: "These functions need not be encountered by all work items in a subgroup executing the kernel."
        const auto feats = static_cast<uint8_t>(Shuffle::FeatureBroadcast | Shuffle::FeatureNonUniform | Shuffle::FeatureShuffle |
            (cap.SupportKHRShuffleRel ? Shuffle::FeatureShuffleRel : 0));
        auto& algo = ShuffleSupport.emplace_back(LevelKHR, AlgoKHRShuffle, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);
    }
    if (cap.SupportKHRShuffle)
    {
        // FeatureNonUniform: Mimic ShuffleRel inherit Shuffle's feature
        const auto feats = static_cast<uint8_t>(Shuffle::FeatureNonUniform | Shuffle::FeatureShuffleRel | Shuffle::FeatureShuffleRel2);
        auto& algo = ShuffleSupport.emplace_back(LevelKHR, AlgoKHRShuffleRelM, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);
    }
    if (cap.SupportKHR)
    {
        const auto extVec = cap.SupportKHRExtType ? support1 : support0;
        // https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_subgroups.asciidoc#add-a-new-section-613xsub-group-functions
        // !FeatureNonUniform: "These functions need not be encountered by all work items in a subgroup executing the kernel."
        auto& algo = ReduceSupport.emplace_back(LevelKHR, AlgoKHRReduce, Reduce::FeatureArith);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(  extVec, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(  extVec, support0);
    }
    if (cap.SupportKHRShuffle)
    {
        // !FeatureNonUniform: Shuffle-based reduce must be uniform
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise | Reduce::FeatureArithMul);
        auto& algo = ReduceSupport.emplace_back(LevelKHR, AlgoKHRReduceShuf, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);
    }
    if (cap.SupportKHRNUArith)
    {
        // https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_subgroup_extensions.asciidoc#non-uniform-arithmetic
        // FeatureNonUniform: "These functions need not be encountered by all work items in a subgroup executing the kernel."
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise | Reduce::FeatureArithMul | Reduce::FeatureNonUniform);
        auto& algo = ReduceSupport.emplace_back(LevelKHR, AlgoKHRReduceNU, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);
    }
}

void NLCLSubgroupKHR::OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (EnableKHRBasic)
        runtime.EnableExtension("cl_khr_subgroups"sv);
    if (EnableKHRExtType)
        runtime.EnableExtension("cl_khr_subgroup_extended_types"sv);
    if (EnableKHRShuffle)
        runtime.EnableExtension("cl_khr_subgroup_shuffle"sv);
    if (EnableKHRShuffleRel)
        runtime.EnableExtension("cl_khr_subgroup_shuffle_relative"sv);
    if (EnableKHRBallot)
        runtime.EnableExtension("cl_khr_subgroup_ballot"sv); 
    if (EnableKHRNUArith)
        runtime.EnableExtension("cl_khr_subgroup_non_uniform_arithmetic"sv); 
    SubgroupProvider::OnFinish(runtime, ext, kernel);
}

void NLCLSubgroupKHR::WarnFP(VecDataInfo vtype, const std::u16string_view func)
{
    if (vtype.Type == VecDataInfo::DataTypes::Float)
    {
        std::u16string_view type;
        if (vtype.Bit == 16 && !Cap.SupportFP16)
            type = u"f16";
        else if (vtype.Bit == 64 && !Cap.SupportFP64)
            type = u"f64";
        
        if (type.empty())
            AddWarning(FMTSTR(u"Potential use of unsupported type[{}] with [{}]."sv, type, func));
    }
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupKHR::HandleShuffleAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupShuffleOp op)
{
    std::u32string_view funcname;
    const auto id = algo.ID.ID();
    if (id == AlgoKHRBroadcast)
    {
        funcname = U"sub_group_broadcast"sv;
        EnableKHRBasic = true;
        if (vtype.Dim0 > 1 || vtype.Bit == 8)
            EnableKHRExtType = true;
    }
    else if (id == AlgoKHRBroadcastNU)
    {
        funcname = U"sub_group_non_uniform_broadcast"sv;
        EnableKHRBallot = true;
    }
    else if (id == AlgoKHRShuffle)
    {
        switch (op)
        {
        case SubgroupShuffleOp::ShuffleXor:  funcname = U"sub_group_shuffle_xor"sv;  break;
        case SubgroupShuffleOp::ShuffleDown: funcname = U"sub_group_shuffle_down"sv; break;
        case SubgroupShuffleOp::ShuffleUp:   funcname = U"sub_group_shuffle_up"sv;   break;
        default:                             funcname = U"sub_group_shuffle"sv;      break;
        }
        EnableKHRShuffle = true;
    }
    else
        return {};
    EnableVecType(vtype);
    return { funcname, U"khr"sv, algo.Features, vtype, false };
}
SubgroupProvider::TypedAlgoResult NLCLSubgroupKHR::HandleReduceAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupReduceOp op)
{
    std::u32string_view funcname;
    const auto id = algo.ID.ID();
    if (id == AlgoKHRReduceShuf)
    {
        if (!IsReduceOpArith(op))
            vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
        EnableVecType(vtype);
        return ShufReducePatch(vtype, op, LevelKHR);
    }
    else if (id == AlgoKHRReduce)
    {
        switch (op)
        {
        case SubgroupReduceOp::Add: funcname = U"sub_group_reduce_add"sv; break;
        case SubgroupReduceOp::Min: funcname = U"sub_group_reduce_min"sv; break;
        case SubgroupReduceOp::Max: funcname = U"sub_group_reduce_max"sv; break;
        default: assert(false); break;
        }
        EnableKHRBasic = true;
        if ((vtype.Bit == 16 && (vtype.Type == VecDataInfo::DataTypes::Unsigned || vtype.Type == VecDataInfo::DataTypes::Signed)) ||
            vtype.Bit == 8)
            EnableKHRExtType = true;
    }
    else if (id == AlgoKHRReduceNU)
    {
        switch (op)
        {
        case SubgroupReduceOp::Add: funcname = U"sub_group_non_uniform_reduce_add"sv; break;
        case SubgroupReduceOp::Mul: funcname = U"sub_group_non_uniform_reduce_mul"sv; break;
        case SubgroupReduceOp::Min: funcname = U"sub_group_non_uniform_reduce_min"sv; break;
        case SubgroupReduceOp::Max: funcname = U"sub_group_non_uniform_reduce_max"sv; break;
        case SubgroupReduceOp::And: funcname = U"sub_group_non_uniform_reduce_and"sv; break;
        case SubgroupReduceOp::Or:  funcname = U"sub_group_non_uniform_reduce_or"sv;  break;
        case SubgroupReduceOp::Xor: funcname = U"sub_group_non_uniform_reduce_xor"sv; break;
        default: assert(false); break;
        }
        EnableKHRNUArith = true;
    }
    else
        return {};
    EnableVecType(vtype);
    return { funcname, U"khr"sv, algo.Features, vtype, false };
}
std::u32string_view NLCLSubgroupKHR::GetAlgoSuffix(uint8_t level) noexcept
{
    if (level == LevelKHR) return U"khr"sv;
    return SubgroupProvider::GetAlgoSuffix(level);
}


ReplaceResult NLCLSubgroupKHR::FuncGetSize()
{
    EnableKHRBasic = true;
    return U"get_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::FuncGetMaxSize()
{
    EnableKHRBasic = true;
    return U"get_max_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::FuncGetCount()
{
    EnableKHRBasic = true;
    return U"get_num_sub_groups()"sv;
}
ReplaceResult NLCLSubgroupKHR::FuncGetId()
{
    EnableKHRBasic = true;
    return U"get_sub_group_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::FuncGetLocalId()
{
    EnableKHRBasic = true;
    return U"get_sub_group_local_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::FuncAll(const std::u32string_view predicate)
{
    EnableKHRBasic = true;
    return FMTSTR(U"sub_group_all({})"sv, predicate);
}
ReplaceResult NLCLSubgroupKHR::FuncAny(const std::u32string_view predicate)
{
    EnableKHRBasic = true;
    return FMTSTR(U"sub_group_any({})"sv, predicate);
}


NLCLSubgroupIntel::NLCLSubgroupIntel(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    NLCLSubgroupKHR(logger, context, cap)
{
    constexpr xcomp::VecDimSupport supportAll = xcomp::VTypeDimSupport::All;
    constexpr xcomp::VecDimSupport support1   = xcomp::VTypeDimSupport::Support1;
    constexpr xcomp::VecDimSupport support0   = xcomp::VTypeDimSupport::Empty;
    if (cap.SupportIntel)
    {
        // https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups.html#_add_a_new_section_6_13_x_sub_group_shuffle_functions
        // FeatureNonUniform: "These built-in functions need not be encountered by all work items in a subgroup executing the kernel"
        const auto feats = static_cast<uint8_t>(Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle | Shuffle::FeatureShuffleRel | Shuffle::FeatureNonUniform);
        auto& algo = ShuffleSupport.emplace_back(LevelIntel, AlgoIntelShuffle, feats);
        const auto int64 = context.Device->Platform->BeignetFix ? support0 : support1;
        algo.Bit64.Set(int64, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(supportAll);
        const auto int16 = Cap.SupportIntel16 ? (context.Device->Platform->BeignetFix ? support1 : supportAll) : support0;
        algo.Bit16.Set(int16, cap.SupportFP16 ? support1 : support0);
        if (cap.SupportIntel8)
            algo.Bit8.Set(REMOVE_MASK(xcomp::VTypeDimSupport::All, xcomp::VTypeDimSupport::Support3), support0);
    }
    if (cap.SupportIntel)
    {
        // https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups.html#_additions_to_section_6_13_15_work_group_functions
        // !FeatureNonUniform: "These built-in functions must be encountered by all work items in a subgroup executing the kernel."
        auto& algo = ReduceSupport.emplace_back(LevelIntel, AlgoKHRReduce, Reduce::FeatureArith);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(cap.SupportIntel16 ? support1 : support0, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(cap.SupportIntel8  ? support1 : support0, support0);
    }
    if (cap.SupportIntel)
    {
        // !FeatureNonUniform: Shuffle-based reduce must be uniform
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise | Reduce::FeatureArithMul);
        auto& algo = ReduceSupport.emplace_back(LevelIntel, AlgoIntelReduceShuf, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(cap.SupportIntel16 ? support1 : support0, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(cap.SupportIntel8  ? support1 : support0, support0);
    }
}

void NLCLSubgroupIntel::EnableExtraVecType(VecDataInfo type) noexcept
{
    if (type.Bit == 8)
        EnableIntel8 = EnableIntel = true;
    else if (type.Bit == 16)
    {
        if (type.Type == VecDataInfo::DataTypes::Signed || type.Type == VecDataInfo::DataTypes::Unsigned)
            EnableIntel16 = EnableIntel = true;
    }
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupIntel::HandleShuffleAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupShuffleOp op)
{
    std::u32string_view funcname;
    const auto id = algo.ID.ID();
    if (id == AlgoIntelShuffle)
    {
        switch (op)
        {
        case SubgroupShuffleOp::ShuffleXor:  funcname = U"intel_sub_group_shuffle_xor"sv;  break;
        case SubgroupShuffleOp::ShuffleDown: funcname = U"intel_sub_group_shuffle_down"sv; break;
        case SubgroupShuffleOp::ShuffleUp:   funcname = U"intel_sub_group_shuffle_up"sv;   break;
        default:                             funcname = U"intel_sub_group_shuffle"sv;      break;
        }
    }
    else
        return NLCLSubgroupKHR::HandleShuffleAlgo(algo, vtype, op);
    EnableExtraVecType(vtype);
    EnableVecType(vtype);
    return { funcname, U"intel"sv, algo.Features, vtype, false };
}
SubgroupProvider::TypedAlgoResult NLCLSubgroupIntel::HandleReduceAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupReduceOp op)
{
    const auto id = algo.ID.ID();
    if (id == AlgoIntelReduceShuf)
    {
        if (!IsReduceOpArith(op))
            vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
        EnableVecType(vtype);
        return ShufReducePatch(vtype, op, LevelIntel);
    }
    auto ret = NLCLSubgroupKHR::HandleReduceAlgo(algo, vtype, op);
    if (id == AlgoKHRReduce)
    {
        if (!Cap.SupportKHRExtType)
            EnableExtraVecType(vtype);
    }
    return ret;
}
std::u32string_view NLCLSubgroupIntel::GetAlgoSuffix(uint8_t level) noexcept
{
    if (level == LevelIntel) return U"intel"sv;
    return NLCLSubgroupKHR::GetAlgoSuffix(level);
}

// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_short.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html

void NLCLSubgroupIntel::OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (EnableIntel)
        runtime.EnableExtension("cl_intel_subgroups"sv);
    if (EnableIntel16)
        runtime.EnableExtension("cl_intel_subgroups_short"sv);
    if (EnableIntel8)
        runtime.EnableExtension("cl_intel_subgroups_char"sv);
    if (EnableKHRBasic)
        EnableKHRBasic = !runtime.EnableExtension("cl_intel_subgroups"sv);
    if (ext.SubgroupSize > 0)
    {
        if (runtime.EnableExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv))
            kernel.AddAttribute(U"reqd_sub_group_size",
                FMTSTR(U"__attribute__((intel_reqd_sub_group_size({})))", ext.SubgroupSize));
    }
    NLCLSubgroupKHR::OnFinish(runtime, ext, kernel);
}


const ExtraParam NLCLSubgroupLocal::CommonExtra = { {}, U"local ulong* _oclu_subgroup_local"sv, U"_oclu_subgroup_local"sv };

NLCLSubgroupLocal::NLCLSubgroupLocal(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    NLCLSubgroupKHR(logger, context, cap)
{
    static constexpr xcomp::VecDimSupport supportAll = xcomp::VTypeDimSupport::All;
    static constexpr xcomp::VecDimSupport support1   = xcomp::VTypeDimSupport::Support1;
    static constexpr xcomp::VecDimSupport support0   = xcomp::VTypeDimSupport::Empty;
    constexpr auto setSupport = [](auto& algo) 
    {
        algo.Bit64.Set(supportAll, support0);
        algo.Bit32.Set(supportAll, support0);
        algo.Bit16.Set(supportAll, support0);
        algo.Bit8 .Set(supportAll, support0);
    };
    {
        auto& algo = ShuffleSupport.emplace_back(LevelLocal, AlgoLocalBroadcast, Shuffle::FeatureBroadcast);
        setSupport(algo);
    }
    {
        const auto feats = static_cast<uint8_t>(Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle);
        auto& algo = ShuffleSupport.emplace_back(LevelLocal, AlgoLocalShuffle, feats);
        setSupport(algo);
    }
    {
        // !FeatureNonUniform: Shuffle-based reduce must be uniform
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise | Reduce::FeatureArithMul);
        auto& algo = ReduceSupport.emplace_back(LevelLocal, AlgoLocalReduceShuf, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);
    }
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupLocal::HandleShuffleAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupShuffleOp op)
{
    TypedAlgoResult(NLCLSubgroupLocal::*patchFunc)(const VecDataInfo) noexcept;
    const auto id = algo.ID.ID();
    if (id == AlgoLocalBroadcast)
    {
        patchFunc = &NLCLSubgroupLocal::BroadcastPatch;
    }
    else if (id == AlgoLocalShuffle)
    {
        patchFunc = op == SubgroupShuffleOp::Shuffle ? &NLCLSubgroupLocal::ShufflePatch : &NLCLSubgroupLocal::ShuffleXorPatch;
    }
    else
        return NLCLSubgroupKHR::HandleShuffleAlgo(algo, vtype, op);
    vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
    EnableVecType(vtype);
    auto ret = (this->*patchFunc)(vtype);
    NeedSubgroupSize = NeedLocalTemp = true;
    ret.Features = algo.Features;
    return ret;
}
SubgroupProvider::TypedAlgoResult NLCLSubgroupLocal::HandleReduceAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupReduceOp op)
{
    const auto id = algo.ID.ID();
    if (id == AlgoLocalReduceShuf)
    {
        if (!IsReduceOpArith(op))
            vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
        EnableVecType(vtype);
        auto ret = ShufReducePatch(vtype, op, LevelLocal);
        ret.Extra = CommonExtra;
        return ret;
    }
    return NLCLSubgroupKHR::HandleReduceAlgo(algo, vtype, op);
}
std::u32string_view NLCLSubgroupLocal::GetAlgoSuffix(uint8_t level) noexcept
{
    if (level == LevelLocal) return U"local"sv;
    return NLCLSubgroupKHR::GetAlgoSuffix(level);
}

void NLCLSubgroupLocal::OnBegin(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    KernelName = kernel.InsatnceName;
    NLCLSubgroupKHR::OnBegin(runtime, ext, kernel);
}
void NLCLSubgroupLocal::OnFinish(NLCLRuntime& runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (NeedSubgroupSize || NeedLocalTemp)
    {
        Expects(kernel.InsatnceName == KernelName);
        const auto sgSize = kernel.GetWorkgroupSize();
        if (sgSize == 0)
            runtime.NLRT_THROW_EX(u"Subgroup mimic [local] need to define GetWorkgroupSize() when under fully mimic");
        if (ext.SubgroupSize && kernel.GetWorkgroupSize() != ext.SubgroupSize)
            runtime.NLRT_THROW_EX(FMTSTR(u"Subgroup mimic [local] requires only 1 subgroup for the whole workgroup, now have subgroup[{}] and workgroup[{}].",
                ext.SubgroupSize, kernel.GetWorkgroupSize()));

        const auto blkId = GenerateKID(U"sgsize"sv);
        Context.AddPatchedBlock(blkId, [&]()
            {
                return FMTSTR(U"#define _{} {}"sv, blkId, sgSize);
            });
    }
    if (NeedLocalTemp)
    {
        const auto sgSize = kernel.GetWorkgroupSize();
        kernel.AddBodyPrefix(U"oclu_subgroup_mimic_local_slm"sv,
            FMTSTR(U"    local ulong _oclu_subgroup_local[{}];\r\n", std::max<uint32_t>(sgSize, 16)));
        Logger.debug(u"Subgroup mimic [local] is enabled with size [{}].\n", sgSize);
    }
    KernelName = {};
    NLCLSubgroupKHR::OnFinish(runtime, ext, kernel);
}

std::u32string NLCLSubgroupLocal::GenerateKID(std::u32string_view type) const noexcept
{
    return FMTSTR(U"oclu_local_{}_{}"sv, type, KernelName);
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupLocal::BroadcastPatch(const VecDataInfo vtype) noexcept
{
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    TypedAlgoResult wrapped(Shuffle::GenerateFuncName(U"local"sv, vtype, SubgroupShuffleOp::Broadcast),
        U"local"sv, Shuffle::FeatureBroadcast, vtype, true);
    //auto funcName = FMTSTR(U"oclu_subgroup_broadcast_local_{}"sv, xcomp::StringifyVDataType(vtype));
    Context.AddPatchedBlock(wrapped.GetFuncName(), [&]()
        {
            const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
            const auto scalarName = NLCLRuntime::GetCLTypeName(vtype.Scalar(1));
            std::u32string func = FMTSTR(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)", vecName, wrapped.GetFuncName());
            func.append(UR"(
{
    barrier(CLK_LOCAL_MEM_FENCE);
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
)"sv);
            APPEND_FMT(func, U"    local {0}* restrict ptr = (local {0}*)(tmp);\r\n", scalarName);

            if (vtype.Dim0 == 1)
            {
                func.append(UR"(
    if (lid == sgId) ptr[0] = val;
    barrier(CLK_LOCAL_MEM_FENCE);
    return ptr[0];
)"sv);
            }
            else
            {
                // at least 16 x ulong, need only one store/load
                static constexpr auto temp = UR"(
    if (lid == sgId) vstore{0}(val, 0, ptr);
    barrier(CLK_LOCAL_MEM_FENCE);
    return vload{0}(0, ptr);
)"sv;
                APPEND_FMT(func, temp, vtype.Dim0);
            }

            func.append(U"}"sv);
            return func;
        });
    wrapped.Extra = CommonExtra;
    return wrapped;
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupLocal::ShufflePatch(const VecDataInfo vtype) noexcept
{
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    TypedAlgoResult wrapped(Shuffle::GenerateFuncName(U"local"sv, vtype, SubgroupShuffleOp::Shuffle),
        U"local"sv, Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle, vtype, true);
    //auto funcName = FMTSTR(U"oclu_subgroup_shuffle_local_{}"sv, xcomp::StringifyVDataType(vtype));
    Context.AddPatchedBlock(wrapped.GetFuncName(), [&]()
        {
            const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
            const auto scalarName = NLCLRuntime::GetCLTypeName(vtype.Scalar(1));
            std::u32string func = FMTSTR(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)"sv, vecName, wrapped.GetFuncName());
            func.append(UR"(
{
    barrier(CLK_LOCAL_MEM_FENCE);
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
)"sv);
            APPEND_FMT(func, U"    local {0}* restrict ptr = (local {0}*)(tmp);\r\n", scalarName);

            if (vtype.Dim0 == 1)
            {
                func.append(UR"(
    ptr[lid] = val;
    barrier(CLK_LOCAL_MEM_FENCE);
    return ptr[sgId];
)"sv);
            }
            else
            {
                APPEND_FMT(func, U"\r\n    {0} ret;", vecName); 
                static constexpr auto temp = UR"(
    ptr[lid] = val.s{0};
    barrier(CLK_LOCAL_MEM_FENCE);
    ret.s{0} = ptr[sgId];)"sv;
                for (uint8_t i = 0; i < vtype.Dim0; ++i)
                    APPEND_FMT(func, temp, Idx16Names[i]);
                func.append(U"\r\n    return ret;\r\n"sv);
            }

            func.append(U"}"sv);
            return func;
        });
    wrapped.Extra = CommonExtra;
    return wrapped;
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupLocal::ShuffleXorPatch(const VecDataInfo vtype) noexcept
{
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    TypedAlgoResult wrapped(Shuffle::GenerateFuncName(U"local"sv, vtype, SubgroupShuffleOp::ShuffleXor),
        U"local"sv, Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle, vtype, true);
    Context.AddPatchedBlockEx(wrapped.GetFuncName(), [&](auto& session)
        {
            const auto lidResult = FuncGetLocalId();
            auto depends = lidResult.GetDepends().GetDependSpan(true);
            const auto shufResult = ShufflePatch(vtype);
            if (const auto dep = shufResult.GetDepend(); !dep.empty())
                depends.emplace_back(dep);
            auto lidText = U" ^ mask"s; lidText.insert(0, lidResult.GetStr());
            const std::u32string_view shufArgs[] = { U"val"sv, lidText };
            const auto shufText = Shuffle::GenerateText(shufResult, vtype, shufArgs, SubgroupShuffleOp::ShuffleXor);
            const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
            std::u32string func = FMTSTR(U"inline {0} {1}({2}const {0} val, const uint mask)\r\n{{\r\n    return {3};\r\n}}"sv, vecName, wrapped.GetFuncName(),
                OptionalArg<true>{shufResult.Extra.Param}, shufText);
            session.Commit(std::move(func), depends);
        });
    wrapped.Extra = CommonExtra;
    return wrapped;
}

ReplaceResult NLCLSubgroupLocal::FuncGetSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncGetSize();

    static constexpr auto id = U"oclu_subgroup_local_get_size"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_local_get_size()
{
    return get_local_size(0) * get_local_size(1) * get_local_size(2);
})"sv;
        });
    return { U"oclu_subgroup_local_get_size()"sv, id };
}
ReplaceResult NLCLSubgroupLocal::FuncGetMaxSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncGetMaxSize();
    
    NeedSubgroupSize = true;
    const auto kid = GenerateKID(U"sgsize"sv);
    return { FMTSTR(U"_{}"sv, kid), kid };
}
ReplaceResult NLCLSubgroupLocal::FuncGetCount()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncGetCount();
    return U"(/*local mimic SubgroupCount*/ 1)"sv;
}
ReplaceResult NLCLSubgroupLocal::FuncGetId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncGetId();
    return U"(/*local mimic SubgroupId*/ 0)"sv;
}
ReplaceResult NLCLSubgroupLocal::FuncGetLocalId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncGetLocalId();

    static constexpr auto id = U"oclu_subgroup_local_get_local_id"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_local_get_local_id()
{
    return get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
})"s;
        });
    return { U"oclu_subgroup_local_get_local_id()"sv, id };
}
ReplaceResult NLCLSubgroupLocal::FuncAll(const std::u32string_view predicate)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncAll(predicate);
    RET_FAIL(SubgroupAll);
}
ReplaceResult NLCLSubgroupLocal::FuncAny(const std::u32string_view predicate)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::FuncAny(predicate);
    RET_FAIL(SubgroupAny);
}


// https://intel.github.io/llvm-docs/cuda/opencl-subgroup-vs-cuda-crosslane-op.html
NLCLSubgroupPtx::NLCLSubgroupPtx(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap, const uint32_t smVersion) :
    SubgroupProvider(logger, context, cap), SMVersion(smVersion)
{
    if (smVersion == 0)
        Logger.warning(u"Trying to use PtxSubgroup on non-NV platform.\n");
    else if (smVersion < 30)
        Logger.warning(u"Trying to use PtxSubgroup on sm{}, shuf is introduced since sm30.\n", smVersion);
    if (smVersion < 70)
        ExtraSync = {};
    else
        ExtraSync = U".sync"sv, ExtraMask = U", 0xffffffff"sv; // assume 32 as warp size

    constexpr xcomp::VecDimSupport support1 = xcomp::VTypeDimSupport::Support1;
    constexpr xcomp::VecDimSupport support0 = xcomp::VTypeDimSupport::Empty;
    if (SMVersion >= 30)
    {
        // FeatureNonUniform: "Note that results are undefined in divergent control flow within a warp, if an active thread sources a register from an inactive thread."
        const auto feats = static_cast<uint8_t>(Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle | Shuffle::FeatureNonUniform);
        auto& algo = ShuffleSupport.emplace_back(LevelPtx, AlgoPtxShuffle, feats);
        algo.Bit64.Set(support1, support0);
        algo.Bit32.Set(support1, support0);
        algo.Bit16.Set(support0);
        algo.Bit8 .Set(support0);
    }
    if (SMVersion >= 80)
    {
        // https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#parallel-synchronization-and-communication-instructions-redux-sync
        // https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#warp-reduce-functions
        // TODO: FeatureNonUniform: Need to handle mask explicitly 
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise);
        auto& algo = ReduceSupport.emplace_back(LevelPtx, AlgoPtxReduce80, feats);
        algo.Bit64.Set(support0);
        algo.Bit32.Set(support1, support0);
        algo.Bit16.Set(support0);
        algo.Bit8 .Set(support0);
    }
    if (SMVersion >= 30)
    {
        // !FeatureNonUniform: Shuffle-based reduce must be uniform
        const auto feats = static_cast<uint8_t>(Reduce::FeatureArith | Reduce::FeatureBitwise | Reduce::FeatureArithMul);
        auto& algo = ReduceSupport.emplace_back(LevelPtx, AlgoPtxReduceShuf, feats);
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(support1);
        algo.Bit16.Set(support0);
        algo.Bit8 .Set(support0);
        /*algo.Bit16.Set(support1, cap.SupportFP16 ? support1 : support0);
        algo.Bit8 .Set(support1, support0);*/
    }
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupPtx::HandleShuffleAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupShuffleOp op)
{
    if (const auto id = algo.ID.ID(); id != AlgoPtxShuffle)
        return SubgroupProvider::HandleShuffleAlgo(algo, vtype, op);
    vtype.Type = VecDataInfo::DataTypes::Unsigned;
    auto ret = ShufflePatch(vtype, op);
    EnableVecType(vtype);
    ret.Features = algo.Features;
    return ret;
}
SubgroupProvider::TypedAlgoResult NLCLSubgroupPtx::HandleReduceAlgo(const Algorithm& algo, VecDataInfo vtype, SubgroupReduceOp op)
{
    const auto id = algo.ID.ID();
    if (id == AlgoPtxReduce80)
    {
        if (!IsReduceOpArith(op))
            vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
        return ReduceSM80Patch(op, vtype);
    }
    else if (id == AlgoPtxReduceShuf)
    {
        return ShufReducePatch(vtype, op, LevelPtx);
    }
    return SubgroupProvider::HandleReduceAlgo(algo, vtype, op);
}
std::u32string_view NLCLSubgroupPtx::GetAlgoSuffix(uint8_t level) noexcept
{
    if (level == LevelPtx) return U"ptx"sv;
    return SubgroupProvider::GetAlgoSuffix(level);
}

// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#data-movement-and-conversion-instructions-shfl
SubgroupProvider::TypedAlgoResult NLCLSubgroupPtx::ShufflePatch(const VecDataInfo vtype, SubgroupShuffleOp op) noexcept
{
    Expects(vtype.Bit == 32 || vtype.Bit == 64);
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    TypedAlgoResult wrapped(Shuffle::GenerateFuncName(U"ptx"sv, vtype, op),
        U"ptx"sv, Shuffle::FeatureBroadcast | Shuffle::FeatureShuffle, vtype, true);
    Context.AddPatchedBlock(wrapped.GetFuncName(), [&]()
        {
            const auto shflMode = op == SubgroupShuffleOp::ShuffleXor ? U"bfly"sv : U"idx"sv;
            auto func = FMTSTR(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{"sv,
                NLCLRuntime::GetCLTypeName(vtype), wrapped.GetFuncName());
            if (vtype.Bit == 32)
            {
                func.append(U"\r\n    uint ret;\r\n    "sv);
                APPEND_FMT(func, UR"(asm volatile("shfl{1}.{0}.b32 %0, %1, %2, 0x1f{2};" : "=r"(ret) : "r"(val), "r"(sgId));)"sv,
                    shflMode, ExtraSync, ExtraMask);
                func.append(U"\r\n    return ret;\r\n}"sv);
            }
            else if (vtype.Bit == 64)
            {
                func.append(U"\r\n    uint2 tmp = as_uint2(val), ret;\r\n    "sv);
                static constexpr auto temp = UR"(asm volatile("shfl{1}.{0}.b32 %0, %2, %4, 0x1f{2}; shfl{1}.{0}.b32 %1, %3, %4, 0x1f{2};"
        : "=r"(ret.x), "=r"(ret.y) : "r"(tmp.x), "r"(tmp.y), "r"(sgId));)"sv;
                APPEND_FMT(func, temp, shflMode, ExtraSync, ExtraMask);
                func.append(U"\r\n    return as_ulong(ret);\r\n}"sv);
            }
            else
            {
                assert(false);
            }
            return func;
        });
    return wrapped;
}

SubgroupProvider::TypedAlgoResult NLCLSubgroupPtx::ReduceSM80Patch(SubgroupReduceOp op, VecDataInfo vtype)
{
    const bool isArith = IsReduceOpArith(op);
    Expects(isArith || vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Bit == 32 && vtype.Dim0 == 1);

    TypedAlgoResult wrapped(Reduce::GenerateFuncName(U"ptx"sv, vtype, op),
        U"ptx"sv, Reduce::FeatureArith | Reduce::FeatureBitwise, vtype, true);
    Context.AddPatchedBlock(wrapped.GetFuncName(), [&]()
        {
            const auto opname = StringifyReduceOp(op);
            const auto scalarType = VecDataInfo{ vtype.Type, 32, 1, 0 };
            const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
            const auto regType = isArith ? (vtype.Type == VecDataInfo::DataTypes::Unsigned ? U'u' : U's') : U'b';
            static constexpr auto syntax = UR"(inline {0} {1}({0} x)
{{
    {0} ret;
    asm volatile("redux.sync.{2}.{3}32 %0, %1{4};" : "=r"(ret) : "r"(x));
    return ret;
}})"sv;
            return FMTSTR(syntax, scalarName, wrapped.GetFuncName(), opname, regType, ExtraMask);
        });
    return wrapped;
}

ReplaceResult NLCLSubgroupPtx::FuncGetSize()
{
    static constexpr auto id = U"oclu_subgroup_ptx_get_size"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_size()
{
    uint ret;
    asm volatile("mov.u32 %0, WARP_SZ;" : "=r"(ret));
    return ret;
})"s;
        });
    return { U"oclu_subgroup_ptx_get_size()"sv, id };
}
ReplaceResult NLCLSubgroupPtx::FuncGetMaxSize()
{
    return U"(/*nv_warp*/32u)"sv;
}
ReplaceResult NLCLSubgroupPtx::FuncGetCount()
{
    if (SMVersion < 20)
        return {};
    static constexpr auto id = U"oclu_subgroup_ptx_count"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_count()
{
    uint ret;
    asm volatile("mov.u32 %0, %%nwarpid;" : "=r"(ret));
    return ret;
})"s;
        });
    return { U"oclu_subgroup_ptx_count()"sv, id };
}
ReplaceResult NLCLSubgroupPtx::FuncGetId()
{
    static constexpr auto id = U"oclu_subgroup_ptx_get_id"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_id()
{
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
    return lid / 32;
})"s;
        });
    return { U"oclu_subgroup_ptx_get_id()"sv, id };
}
ReplaceResult NLCLSubgroupPtx::FuncGetLocalId()
{
    static constexpr auto id = U"oclu_subgroup_ptx_get_local_id"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_local_id()
{
    uint ret;
    asm volatile("mov.u32 %0, %%laneid;" : "=r"(ret));
    return ret;
})"s;
        });
    return { U"oclu_subgroup_ptx_get_local_id()"sv, id };
}
ReplaceResult NLCLSubgroupPtx::FuncAll(const std::u32string_view predicate)
{
    static constexpr auto id = U"oclu_subgroup_ptx_all"sv;
    Context.AddPatchedBlock(id, [&]()
        {
            auto ret = UR"(inline int oclu_subgroup_ptx_all(int predicate)
{
    int ret;
    asm volatile("{\n\t\t"
                 ".reg .pred        %%p1, %%p2;     \n\t\t"
                 "setp.ne.u32       %%p1, %1, 0;    \n\t\t"
                 )"s;
            APPEND_FMT(ret, UR"("vote{0}.all.pred  %%p2, %%p1{1};  \n\t\t")"sv, ExtraSync, ExtraMask);
            ret.append(UR"(
                 "selp.s32          %0, 1, 0, %%p2; \n\t"
                 "}" : "=r"(ret) : "r"(predicate));
    return ret;
})"sv);
            return ret;
        });
    return { FMTSTR(U"oclu_subgroup_ptx_all({})", predicate), id };
}
ReplaceResult NLCLSubgroupPtx::FuncAny(const std::u32string_view predicate)
{
    static constexpr auto id = U"oclu_subgroup_ptx_any"sv;
    Context.AddPatchedBlock(id, [&]()
        {
            auto ret = UR"(inline int oclu_subgroup_ptx_any(int predicate)
{
    int ret;
    asm volatile("{\n\t\t"
                 ".reg .pred        %%p1, %%p2;     \n\t\t"
                 "setp.ne.u32       %%p1, %1, 0;    \n\t\t"
                 )"s;
            APPEND_FMT(ret, UR"("vote{0}.any.pred  %%p2, %%p1{1};  \n\t\t")"sv, ExtraSync, ExtraMask);
            ret.append(UR"(
                 "selp.s32          %0, 1, 0, %%p2; \n\t"
                 "}" : "=r"(ret) : "r"(predicate));
    return ret;
})"sv);
            return ret;
        });
    return { FMTSTR(U"oclu_subgroup_ptx_any({})", predicate), id };
}



}