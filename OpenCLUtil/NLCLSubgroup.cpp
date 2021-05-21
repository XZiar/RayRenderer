#include "oclPch.h"
#include "NLCLSubgroup.h"


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
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const OptionalArg<IsPrefix>& arg, FormatContext& ctx)
    {
        if (arg.Arg.empty())
            return ctx.out();
        if constexpr (IsPrefix)
            return fmt::format_to(ctx.out(), FMT_STRING(U"{}, "), arg.Arg);
        else
            return fmt::format_to(ctx.out(), FMT_STRING(U", {}"), arg.Arg);
    }
};

struct CastTypeArg
{
    using VecDataInfo = common::simd::VecDataInfo;
    VecDataInfo VecType;
    bool IsConvert;
    constexpr CastTypeArg(VecDataInfo from, VecDataInfo to, bool isConvert) noexcept :
        VecType(from == to ? to : VecDataInfo{ VecDataInfo::DataTypes::Empty,0,0,0 }), IsConvert(isConvert) { }
    constexpr bool NeedCast() const noexcept { return VecType.Type != VecDataInfo::DataTypes::Empty; }
    const std::u32string_view EndChar() const noexcept { return NeedCast() ? U")"sv : std::u32string_view{}; }
};

template<typename Char>
struct fmt::formatter<CastTypeArg, Char>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const CastTypeArg& arg, FormatContext& ctx)
    {
        if (!arg.NeedCast())
            return ctx.out();
        return fmt::format_to(ctx.out(), FMT_STRING(U"{}_{}("), 
            arg.IsConvert ? U"convert"sv : U"as"sv, oclu::NLCLRuntime::GetCLTypeName(arg.VecType));
    }
};


namespace oclu
{
using namespace std::string_literals;
using xcomp::ReplaceResult;
using xziar::nailang::Arg;
using xziar::nailang::ArgLimits;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;
using common::str::Charset;
using DepGen = std::function<std::shared_ptr<const xcomp::ReplaceDepend>()>;


constexpr char32_t Idx16Names[] = U"0123456789abcdef";

#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define RET_FAIL(func) return {U"No proper [" STRINGIZE(func) "]"sv, false}
#define GEN_DEPEND(...) [=](){ return xcomp::ReplaceDepend::CreateFrom(__VA_ARGS__); }


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
        Provider = NLCLSubgroupExtension::Generate(GetLogger(runtime), Context, mimic, args);
    }
}

ReplaceResult NLCLSubgroupExtension::ReplaceFunc(xcomp::XCNLRawExecutor& executor, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (common::str::IsBeginWith(func, U"oclu."))
        func.remove_prefix(5);
    else if(common::str::IsBeginWith(func, U"xcomp."))
        func.remove_prefix(6);
    else
        return {};
    //auto& Executor = static_cast<NLCLRawExecutor&>(executor);
    auto& runtime = executor.GetRuntime();
    constexpr auto HandleResult = [](ReplaceResult result, common::str::StrVariant<char32_t> msg)
    {
        if (!result && result.GetStr().empty())
            result.SetStr(std::move(msg));
        return result;
    };
    const auto SubgroupReduce = [&](std::u32string_view name, SubgroupReduceOp op)
    {
        executor.ThrowByReplacerArgCount(func, args, 2, ArgLimits::Exact);
        const auto vtype = runtime.ParseVecDataInfo(args[0], FMTSTR(u"replace [{}]"sv, name));
        return Provider->SubgroupReduce(op, vtype, args[1]);
    };

    switch (common::DJBHash::HashC(func))
    {
    HashCase(func, U"GetSubgroupSize")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->GetSubgroupSize(),
            U"[GetSubgroupSize] not supported"sv);
    }
    HashCase(func, U"GetMaxSubgroupSize")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->GetMaxSubgroupSize(),
            U"[GetMaxSubgroupSize] not supported"sv);
    }
    HashCase(func, U"GetSubgroupCount")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->GetSubgroupCount(),
            U"[GetSubgroupCount] not supported"sv);
    }
    HashCase(func, U"GetSubgroupId")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->GetSubgroupId(),
            U"[GetSubgroupId] not supported"sv);
    }
    HashCase(func, U"GetSubgroupLocalId")
    {
        executor.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        return HandleResult(Provider->GetSubgroupLocalId(),
            U"[GetSubgroupLocalId] not supported"sv);
    }
    HashCase(func, U"SubgroupAll")
    {
        executor.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
        return HandleResult(Provider->SubgroupAll(args[0]),
            U"[SubgroupAll] not supported"sv);
    }
    HashCase(func, U"SubgroupAny")
    {
        executor.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
        return HandleResult(Provider->SubgroupAny(args[0]),
            U"[SubgroupAny] not supported"sv);
    }
    HashCase(func, U"SubgroupBroadcast")
    {
        executor.ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
        const auto vtype = runtime.ParseVecDataInfo(args[0], u"replace [SubgroupBroadcast]"sv);
        return HandleResult(
            //Provider->SubgroupBroadcast(vtype, args[1], args[2]),
            Provider->SubgroupBShuf(vtype, args[1], args[2], false),
            FMTSTR(U"SubgroupBroadcast with Type [{}] not supported", args[0]));
    }
    HashCase(func, U"SubgroupShuffle")
    {
        executor.ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
        const auto vtype = runtime.ParseVecDataInfo(args[0], u"replace [SubgroupShuffle]"sv);
        return HandleResult(
            //Provider->SubgroupShuffle(vtype, args[1], args[2]),
            Provider->SubgroupBShuf(vtype, args[1], args[2], true),
            FMTSTR(U"SubgroupShuffle with Type [{}] not supported", args[0]));
    }
    HashCase(func, U"SubgroupSum")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::Sum), U"[SubgroupSum] not supported"sv);
    }
    HashCase(func, U"SubgroupMin")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::Min), U"[SubgroupMin] not supported"sv);
    }
    HashCase(func, U"SubgroupMax")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::Max), U"[SubgroupMax] not supported"sv);
    }
    HashCase(func, U"SubgroupAnd")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::And), U"[SubgroupAnd] not supported"sv);
    }
    HashCase(func, U"SubgroupOr")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::Or),  U"[SubgroupOr] not supported"sv);
    }
    HashCase(func, U"SubgroupXor")
    {
        return HandleResult(SubgroupReduce(func, SubgroupReduceOp::Xor), U"[SubgroupXor] not supported"sv);
    }
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
        const auto provider = Generate(Logger, Context, mimic, args);
        const auto ret = isShuffle ?
            provider->SubgroupShuffle(vtype, U"x"sv, U"y"sv) :
            provider->SubgroupBroadcast(vtype, U"x"sv, U"y"sv);
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
        const bool isDisable = arg[0] == '-';
        if (isDisable) arg.remove_prefix(1);
        switch (common::DJBHash::HashC(arg))
        {
#define Mod(dst, name) HashCase(arg, name) cap.Support##dst = !isDisable; break
        Mod(KHR,            "sg_khr");
        Mod(KHRExtType,     "sg_khrexttype");
        Mod(KHRShuffle,     "sg_khrshuf");
        Mod(KHRShuffleRel,  "sg_khrshufrel");
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

constexpr auto SubgroupMimicParser = SWITCH_PACK(Hash,
    (U"local",  SubgroupAttributes::MimicType::Local),
    (U"ptx",    SubgroupAttributes::MimicType::Ptx),
    (U"auto",   SubgroupAttributes::MimicType::Auto),
    (U"none",   SubgroupAttributes::MimicType::None));
std::shared_ptr<SubgroupProvider> NLCLSubgroupExtension::Generate(common::mlog::MiniLogger<false>& logger, NLCLContext& context,
    std::u32string_view mimic, std::u32string_view args)
{
    SubgroupAttributes attr;
    attr.Mimic = SubgroupMimicParser(mimic).value_or(SubgroupAttributes::MimicType::Auto);
    attr.Args = common::str::to_string(args, Charset::ASCII);
    auto cap = GenerateCapabiity(context, attr);

    SubgroupAttributes::MimicType mType = attr.Mimic;
    if (mType == SubgroupAttributes::MimicType::Auto)
    {
        if (context.Device->PlatVendor == Vendors::NVIDIA)
            mType = SubgroupAttributes::MimicType::Ptx;
        else
            mType = SubgroupAttributes::MimicType::Local;
    }
    Ensures(mType != SubgroupAttributes::MimicType::Auto);

    if (cap.SupportIntel)
        return std::make_shared<NLCLSubgroupIntel>(logger, context, cap);
    else if (mType == SubgroupAttributes::MimicType::Ptx)
    {
        uint32_t smVer = 0;
        if (context.Device->Extensions.Has("cl_nv_device_attribute_query"sv))
        {
            uint32_t major = 0, minor = 0;
            clGetDeviceInfo(*context.Device, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(uint32_t), &major, nullptr);
            clGetDeviceInfo(*context.Device, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(uint32_t), &minor, nullptr);
            smVer = major * 10 + minor;
        }
        return std::make_shared<NLCLSubgroupPtx>(logger, context, cap, smVer);
    }
    else if (mType == SubgroupAttributes::MimicType::Local)
        return std::make_shared<NLCLSubgroupLocal>(logger, context, cap);
    else if (cap.SupportBasicSubgroup)
        return std::make_shared<NLCLSubgroupKHR>(logger, context, cap);
    else
        return std::make_shared<SubgroupProvider>(logger, context, cap);
}

forceinline constexpr static bool CheckVNum(const uint32_t num) noexcept
{
    switch (num)
    {
    case 1: case 2: case 3: case 4: case 8: case 16: return true;
    default: return false;
    }
}
forceinline constexpr static uint8_t TryCombine(const uint32_t total, const uint8_t bit, const uint8_t vmax = 16) noexcept
{
    if (total % bit == 0)
    {
        const auto vnum = total / bit;
        if (vnum <= vmax || CheckVNum(vnum))
            return static_cast<uint8_t>(vnum);
    }
    return 0;
}
forceinline constexpr static VecDataInfo ToUintVec(VecDataInfo vtype) noexcept
{
    return { VecDataInfo::DataTypes::Unsigned, vtype.Bit, vtype.Dim0, 0 };
}
forceinline constexpr static VecDataInfo ToScalar(VecDataInfo vtype) noexcept
{
    return { vtype.Type, vtype.Bit, 1, 0 };
}

static constexpr std::u32string_view StringifyReduceOp(SubgroupReduceOp op)
{
    switch (op)
    {
    case SubgroupReduceOp::Sum: return U"sum"sv;
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
    case SubgroupReduceOp::Sum:
    case SubgroupReduceOp::Min:
    case SubgroupReduceOp::Max:
        return true;
    default:
        return false;
    }
}


std::optional<VecDataInfo> SubgroupProvider::DecideVtype(VecDataInfo vtype, common::span<const VecDataInfo> scalars) noexcept
{
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    for (const auto scalar : scalars)
    {
        if (const auto vnum = TryCombine(totalBits, scalar.Bit); vnum)
            return VecDataInfo{ scalar.Type, scalar.Bit, vnum, 0 };
    }
    return {};
}
std::u32string SubgroupProvider::ScalarPatch(const std::u32string_view name, const std::u32string_view baseFunc, VecDataInfo vtype,
    const std::u32string_view extraArg, const std::u32string_view extraParam) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
    std::u32string func = FMTSTR(U"inline {0} {1}(const {0} val{2})\r\n{{\r\n    {0} ret;", vecName, name, extraParam);
    for (uint8_t i = 0; i < vtype.Dim0; ++i)
        APPEND_FMT(func, U"\r\n    ret.s{0} = {1}(val.s{0}{2});", Idx16Names[i], baseFunc, extraArg);
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}
std::u32string SubgroupProvider::HiLoPatch(const std::u32string_view name, const std::u32string_view baseFunc, VecDataInfo vtype,
    const std::u32string_view extraArg, const std::u32string_view extraParam) noexcept
{
    Expects(vtype.Dim0 % 2 == 0 && vtype.Bit % 2 == 0);
    const VecDataInfo midType{ vtype.Type, static_cast<uint8_t>(vtype.Bit / 2), vtype.Dim0, 0 };
    const VecDataInfo halfType{ vtype.Type, vtype.Bit, static_cast<uint8_t>(vtype.Dim0 / 2), 0 };
    const auto  vecName = NLCLRuntime::GetCLTypeName(vtype);
    const auto  midName = NLCLRuntime::GetCLTypeName(midType);
    const auto halfName = NLCLRuntime::GetCLTypeName(halfType);
    std::u32string func = FMTSTR(U"inline {0} {1}(const {0} val{2})\r\n{{\r\n    {0} ret;", vecName, name, extraParam);
    APPEND_FMT(func, U"\r\n    ret.hi = as_{0}({1}(as_{2}(val.hi){3});\r\n    ret.lo = as_{0}({1}(as_{2}(val.lo){3});",
        halfName, baseFunc, midName, extraArg);
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

SubgroupProvider::SubgroupProvider(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    Logger(logger), Context(context), Cap(cap)
{ }
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


struct SingleArgFunc
{
    std::u32string_view Func;
    std::u32string_view Element;
    std::shared_ptr<const xcomp::ReplaceDepend> Depends;
    OptionalArg<true>  Prefix;
    OptionalArg<false> Suffix;
    std::variant<std::monostate, std::tuple<VecDataInfo, VecDataInfo, bool>, std::tuple<VecDataInfo, VecDataInfo, VecDataInfo, bool>> TypeCast;
    
    constexpr SingleArgFunc(const std::u32string_view func, const std::u32string_view ele) noexcept :
        Func(func), Element(ele) { }
    constexpr SingleArgFunc(const std::u32string_view func, const std::u32string_view ele, const VecDataInfo dst, const VecDataInfo mid, const bool convert) noexcept :
        Func(func), Element(ele), TypeCast(std::tuple{ dst, mid, convert }) { }
    constexpr SingleArgFunc(const std::u32string_view func, const std::u32string_view ele, 
        const VecDataInfo dst, const VecDataInfo first, const VecDataInfo then, const bool convertFirst) noexcept :
        Func(func), Element(ele), TypeCast(std::tuple{ dst, first, then, convertFirst }) { }
    
    SingleArgFunc& Depend(std::shared_ptr<const xcomp::ReplaceDepend> depends) noexcept
    {
        Depends = std::move(depends);
        return *this;
    }
    SingleArgFunc& Depend(std::pair<std::shared_ptr<const xcomp::ReplaceDepend>, bool>& depends) noexcept
    {
        Depends = std::move(depends.first);
        return *this;
    }
    SingleArgFunc& Depend() noexcept
    {
        Depends = GEN_DEPEND(Func)();
        return *this;
    }
    constexpr SingleArgFunc& Attach(const std::u32string_view prefix, const std::u32string_view suffix) noexcept
    {
        Prefix.Arg = prefix, Suffix.Arg = suffix;
        return *this;
    }

    std::u32string ToStr() const
    {
        switch (TypeCast.index())
        {
        case 1:
        {
            const auto [dst, mid, convert] = std::get<1>(TypeCast);
            if (dst != mid)
                return FMTSTR(U"{0}_{1}({3}({4}{0}_{2}({5}){6}))"sv,
                    convert ? U"convert"sv : U"as"sv, NLCLRuntime::GetCLTypeName(dst), NLCLRuntime::GetCLTypeName(mid), 
                    Func, Prefix, Element, Suffix);
        } break;
        case 2:
        {
            const auto [dst, first, then, convFirst] = std::get<2>(TypeCast);
            if (convFirst)
                return FMTSTR(U"convert_{0}(as_{1}({3}({4}as_{2}(convert_{1}({5})){6})))"sv,
                    NLCLRuntime::GetCLTypeName(dst), NLCLRuntime::GetCLTypeName(first), NLCLRuntime::GetCLTypeName(then),
                    Func, Prefix, Element, Suffix);
            else
                return FMTSTR(U"as_{0}(convert_{1}({3}({4}convert_{2}(as_{1}({5})){6})))"sv,
                    NLCLRuntime::GetCLTypeName(dst), NLCLRuntime::GetCLTypeName(first), NLCLRuntime::GetCLTypeName(then),
                    Func, Prefix, Element, Suffix);
        } break;
        default:
            break;
        }
        return FMTSTR(U"{}({}{}{})", Func, Prefix, Element, Suffix);
    }
    operator xcomp::ReplaceResult() const
    {
        if (Depends)
        {
            return { ToStr(), Depends };
        }
        else
        {
            return ToStr();
        }
    }

};
#define BSRetDirect(func)               return SingleArgFunc(func, ele).Attach(U""sv, idx)
#define BSRetCastT(func, mid)           return SingleArgFunc(func, ele, vtype, mid, false).Attach(U""sv, idx)
#define BSRetCast(func, type, bit, dim) return SingleArgFunc(func, ele, vtype, { type, bit, static_cast<uint8_t>(dim), 0 }, false).Attach(U""sv, idx)


struct TypedArgFunc
{
    static constexpr VecDataInfo Dummy{ VecDataInfo::DataTypes::Empty, 0, 0, 0 };
    common::StringPool<char32_t> StrPool;
    std::shared_ptr<const xcomp::ReplaceDepend> Depends;
    common::StringPiece<char32_t> Func;
    std::u32string_view Element;
    common::StringPiece<char32_t> Prefix;
    common::StringPiece<char32_t> Suffix;
    VecDataInfo SrcType;
    VecDataInfo MidType;
    VecDataInfo DstType;

    TypedArgFunc() noexcept : SrcType(Dummy), MidType(Dummy), DstType(Dummy) { }
    TypedArgFunc(const std::u32string_view func, const VecDataInfo src, const VecDataInfo mid, const VecDataInfo dst) noexcept :
        SrcType(src), MidType(mid), DstType(dst) 
    {
        Func = StrPool.AllocateString(func);
    }
    TypedArgFunc(const SubgroupProvider::AlgorithmResult algo, const VecDataInfo src, const VecDataInfo mid, const VecDataInfo dst) noexcept :
        TypedArgFunc(algo.FuncName.StrView(), src, mid, dst)
    {
        Prefix = StrPool.AllocateString(algo.ExtraPrefixArg);
        Depends = algo.Depends;
    }

    std::u32string_view FuncName() const noexcept
    {
        return StrPool.GetStringView(Func);
    }

    TypedArgFunc& Depend(std::shared_ptr<const xcomp::ReplaceDepend> depends) noexcept
    {
        Depends = std::move(depends);
        return *this;
    }
    TypedArgFunc& Depend(std::pair<std::shared_ptr<const xcomp::ReplaceDepend>, bool>& depends) noexcept
    {
        Depends = std::move(depends.first);
        return *this;
    }
    TypedArgFunc& Depend() noexcept
    {
        Depends = GEN_DEPEND(FuncName())();
        return *this;
    }
    constexpr TypedArgFunc& Content(const std::u32string_view element) noexcept
    {
        Element = element;
        return *this;
    }
    TypedArgFunc& Attach(const std::u32string_view prefix, const std::u32string_view suffix) noexcept
    {
        if (Prefix.GetLength() > 0)
        {
            if (!prefix.empty())
            {
                const std::u32string pfx{ StrPool.GetStringView(Prefix) };
                Prefix = StrPool.AllocateConcatString(pfx, U", "sv, prefix);
            }
        }
        else
            Prefix = StrPool.AllocateString(prefix);
        if (Suffix.GetLength() > 0)
        {
            if (!suffix.empty())
            {
                const std::u32string sfx{ StrPool.GetStringView(Suffix) };
                Suffix = StrPool.AllocateConcatString(suffix, U", "sv, sfx);
            }
        }
        else
            Suffix = StrPool.AllocateString(suffix);
        return *this;
    }

    explicit operator bool() const noexcept { return Func.GetLength() > 0; }
    std::u32string ToStr() const
    {
        if (!static_cast<bool>(*this)) return {};
        /*CastTypeArg src2mid(SrcType, MidType, true), mid2dst(MidType, DstType, false),
            dst2mid(DstType, MidType, false), mid2src(MidType, SrcType, true);*/
        const bool step1 = SrcType != MidType, step2 = MidType != DstType;
        std::u32string txt;
        if (step1)
            txt.append(U"convert_").append(oclu::NLCLRuntime::GetCLTypeName(SrcType)).append(U"("sv);
        if (step2)
            txt.append(U"as_").append(oclu::NLCLRuntime::GetCLTypeName(MidType)).append(U"("sv);
        txt.append(StrPool.GetStringView(Func)).append(U"(");
        if (Prefix.GetLength() > 0)
            txt.append(StrPool.GetStringView(Prefix)).append(U", ");
        if (step2)
            txt.append(U"as_").append(oclu::NLCLRuntime::GetCLTypeName(DstType)).append(U"("sv);
        if (step1)
            txt.append(U"convert_").append(oclu::NLCLRuntime::GetCLTypeName(MidType)).append(U"("sv);
        txt.append(Element);
        if (step1)
            txt.append(U")"sv);
        if (step2)
            txt.append(U")"sv);
        if (Suffix.GetLength() > 0)
            txt.append(U", ").append(StrPool.GetStringView(Suffix));
        txt.append(U")"sv);
        if (step2)
            txt.append(U")"sv);
        if (step1)
            txt.append(U")"sv);
        return txt;
    }
    operator xcomp::ReplaceResult() const
    {
        if (Depends)
        {
            return { ToStr(), Depends };
        }
        else
        {
            return ToStr();
        }
    }

};


TypedArgFunc SubgroupProvider::SubgroupBShuf(common::simd::VecDataInfo origType, common::simd::VecDataInfo srcType, 
    bool isConvert, uint8_t featMask)
{
    xcomp::VecDimSupport Algorithm::DTypeSupport::* dtPtr1 = nullptr;
    xcomp::VecDimSupport Algorithm::DTypeSupport::* dtPtr2 = nullptr;
    VecDataInfo::DataTypes alterType = VecDataInfo::DataTypes::Custom;
    switch (srcType.Type)
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
    switch (srcType.Bit)
    {
    case 64: btPtr = &Algorithm::Bit64; break;
    case 32: btPtr = &Algorithm::Bit32; break;
    case 16: btPtr = &Algorithm::Bit16; break;
    case  8: btPtr = &Algorithm::Bit8;  break;
    default: return {};
    }

    const auto convedType = isConvert ? srcType : origType /*bypass prev cast*/;
    // try exact match bit and dtype 
    {
        for (const auto& algo : BShufSupport)
        {
            if ((algo.Features & featMask) == 0)
                continue;
            const auto& sup = (algo.*btPtr).*dtPtr1;
            if (sup.Check(srcType.Dim0))
            {
                auto dstType = srcType;
                const auto ret = SubgroupBShufAlgo(algo.ID, dstType);
                if (!ret.FuncName.empty())
                    return TypedArgFunc(ret, origType, convedType, dstType);
            }
        }
    }
    // try only bit match
    {
        for (const auto& algo : BShufSupport)
        {
            if ((algo.Features & featMask) == 0)
                continue;
            const auto& sup = (algo.*btPtr).*dtPtr2;
            if (sup.Check(srcType.Dim0))
            {
                VecDataInfo midType{ alterType, srcType.Bit, srcType.Dim0, 0 };
                const auto ret = SubgroupBShufAlgo(algo.ID, midType);
                if (!ret.FuncName.empty())
                    return TypedArgFunc(ret, origType, convedType, midType);
            }
        }
    }
    // try loop-based
    {
        for (const auto& algo : BShufSupport)
        {
            if ((algo.Features & featMask) == 0)
                continue;
            const auto& sup = (algo.*btPtr).Int;
            if (sup.Check(1))
            {
                VecDataInfo scalarUType{ VecDataInfo::DataTypes::Unsigned, srcType.Bit, 1, 0 };
                const auto ret = SubgroupBShufAlgo(algo.ID, scalarUType);
                Expects(!ret.FuncName.empty());
                const VecDataInfo vectorUType{ scalarUType.Type, srcType.Bit, srcType.Dim0, 0 };
                const auto funcName = FMTSTR(U"oclu_subgroup_{}_{}_{}", 
                    (algo.Features & Algorithm::FeatureShuffle) ? U"shuffle"sv : U"broadcast"sv, ret.Suffix, xcomp::StringifyVDataType(vectorUType));
                auto dep = Context.AddPatchedBlockD(funcName, ret.GetPatchedBlockDepend(), [&]()
                    {
                        return NLCLSubgroupIntel::ScalarPatchBcastShuf(funcName, ret.FuncName.StrView(), vectorUType);
                    });
                return TypedArgFunc(funcName, origType, convedType, vectorUType).Attach(ret.ExtraPrefixArg, {}).Depend(dep);
            }
        }
    }
    return {};
}
xcomp::ReplaceResult SubgroupProvider::SubgroupBShuf(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx, bool isShuf)
{
    const uint8_t featMask = isShuf ? Algorithm::FeatureShuffle : Algorithm::FeatureBroadcast;

    // try direct type
    if (auto ret = SubgroupBShuf(vtype, vtype, false, featMask); ret)
        return ret.Content(ele).Attach({}, idx);

    // try cast to other datatype
    for (const auto bit : std::array<uint8_t, 4>{64, 32, 16, 8})
    {
        if (bit > vtype.Bit)
        {
            const auto scale = bit / vtype.Bit;
            if (bit % vtype.Bit != 0 || vtype.Dim0 % scale != 0)
                continue;
        }
        else if (bit < vtype.Bit)
        {
            const auto scale = vtype.Bit / bit;
            if (vtype.Bit % bit != 0)
                continue;
        }
        else
            continue;
        const uint8_t newDim0 = static_cast<uint8_t>(vtype.Dim0 * vtype.Bit / bit);
        // TODO: handle vdim0 > 16
        const VecDataInfo vectorUType{ VecDataInfo::DataTypes::Unsigned, bit, newDim0, 0 };
        if (auto ret = SubgroupBShuf(vtype, vectorUType, false, featMask); ret)
            return ret.Content(ele).Attach({}, idx);
    }

    // try expend bits to bigger datatype
    for (const auto bit : std::array<uint8_t, 3>{16, 32, 64})
    {
        uint8_t newDim0 = 0;
        if (bit <= vtype.Bit)
            continue;
        const VecDataInfo vectorUType{ VecDataInfo::DataTypes::Unsigned, bit, vtype.Dim0, 0 };
        if (auto ret = SubgroupBShuf(vtype, vectorUType, true, featMask); ret)
            return ret.Content(ele).Attach({}, idx);
    }
    return {};
}

// algo 0: sub_group_broadcast
// algo 1: sub_group_shuffle | relative
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
        auto& algo = BShufSupport.emplace_back(AlgoKHRBroadcast, Algorithm::FeatureBroadcast);
        algo.Bit64.Set(basicVec, cap.SupportFP64 ? basicVec : support0);
        algo.Bit32.Set(basicVec);
        algo.Bit16.Set(  extVec, cap.SupportFP16 ? basicVec : support0);
        if (cap.SupportKHRExtType)
            algo.Bit8.Set(supportAll, support0);
    }
    if (cap.SupportKHRShuffle)
    {
        const auto feats = static_cast<uint8_t>(Algorithm::FeatureBroadcast | 
            (cap.SupportKHRShuffleRel ? Algorithm::FeatureShuffleRel : Algorithm::FeatureShuffle));
        auto& algo = BShufSupport.emplace_back(AlgoKHRShuffle, feats);
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
    SubgroupProvider::OnFinish(runtime, ext, kernel);
}

void NLCLSubgroupKHR::EnableVecExtType(VecDataInfo type) noexcept
{
    if (type.Dim0 > 1 || type.Bit == 8)
        EnableKHRExtType = true;
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

ReplaceResult NLCLSubgroupKHR::SubgroupReduceArith(SubgroupReduceOp op, VecDataInfo vtype, VecDataInfo realType, const std::u32string_view ele)
{
    Expects(IsReduceOpArith(op));
    EnableKHRBasic = true;
    // https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/sub_group_broadcast.html
    const auto func = [](SubgroupReduceOp op)
    {
        switch (op)
        {
        case SubgroupReduceOp::Sum: return U"sub_group_reduce_add"sv;
        case SubgroupReduceOp::Min: return U"sub_group_reduce_min"sv;
        case SubgroupReduceOp::Max: return U"sub_group_reduce_max"sv;
        default: assert(false);     return U""sv;
        }
    }(op);

    if (vtype.Dim0 == 1)
    {
        return SingleArgFunc(func, ele, realType, vtype, true);
    }

    const auto funcName = FMTSTR(U"oclu_subgroup_{}_{}", StringifyReduceOp(op), xcomp::StringifyVDataType(vtype));
    auto dep = Context.AddPatchedBlock(funcName, [&]()
        {
            return NLCLSubgroupKHR::ScalarPatch(funcName, func, vtype);
        });
    return SingleArgFunc(funcName, ele, realType, vtype, true).Depend(dep);
}

ReplaceResult NLCLSubgroupKHR::GetSubgroupSize()
{
    EnableKHRBasic = true;
    return U"get_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetMaxSubgroupSize()
{
    EnableKHRBasic = true;
    return U"get_max_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupCount()
{
    EnableKHRBasic = true;
    return U"get_num_sub_groups()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupId()
{
    EnableKHRBasic = true;
    return U"get_sub_group_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupLocalId()
{
    EnableKHRBasic = true;
    return U"get_sub_group_local_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::SubgroupAll(const std::u32string_view predicate)
{
    EnableKHRBasic = true;
    return FMTSTR(U"sub_group_all({})"sv, predicate);
}
ReplaceResult NLCLSubgroupKHR::SubgroupAny(const std::u32string_view predicate)
{
    EnableKHRBasic = true;
    return FMTSTR(U"sub_group_any({})"sv, predicate);
}
SubgroupProvider::AlgorithmResult NLCLSubgroupKHR::SubgroupBShufAlgo(uint8_t algoId, VecDataInfo& vtype)
{
    std::u32string_view funcname;
    if (algoId == AlgoKHRBroadcast)
    {
        funcname = U"sub_group_broadcast"sv;
        EnableKHRBasic = true;
        EnableVecExtType(vtype); 
    }
    else if (algoId == AlgoKHRShuffle)
    {
        funcname = U"sub_group_shuffle"sv;
        EnableKHRShuffle = true;
    }
    else
        return {};
    EnableVecType(vtype);
    return { funcname, {}, U"khr"sv, {} };
}

ReplaceResult NLCLSubgroupKHR::SubgroupReduce(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    if (!IsReduceOpArith(op))
        return {U"khr's [SubgroupReduce] not not supported bitwise op"sv, false};
    VecDataInfo mid = vtype;
    if (vtype.Type == VecDataInfo::DataTypes::Float)
    {
        WarnFP(vtype, u"SubgroupReduce"sv);
    }
    else if (vtype.Type == VecDataInfo::DataTypes::Unsigned || vtype.Type == VecDataInfo::DataTypes::Signed)
    {
        if (vtype.Bit < 32)
        {
            mid.Bit = 32;
        }
    }
    else
    {
        Expects(false);
    }
    return SubgroupReduceArith(op, mid, vtype, ele);
}


// algo 8 : intel_sub_group_broadcast
NLCLSubgroupIntel::NLCLSubgroupIntel(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    NLCLSubgroupKHR(logger, context, cap)
{
    constexpr xcomp::VecDimSupport supportAll = xcomp::VTypeDimSupport::All;
    constexpr xcomp::VecDimSupport support1   = xcomp::VTypeDimSupport::Support1;
    constexpr xcomp::VecDimSupport support0   = xcomp::VTypeDimSupport::Empty;
    if (cap.SupportIntel)
    {
        auto& algo = BShufSupport.emplace_back(AlgoIntelShuffle, static_cast<uint8_t>(
            Algorithm::FeatureBroadcast | Algorithm::FeatureShuffle | Algorithm::FeatureShuffleRel));
        algo.Bit64.Set(support1, cap.SupportFP64 ? support1 : support0);
        algo.Bit32.Set(supportAll);
        algo.Bit16.Set(Cap.SupportIntel16 ? supportAll : support0, cap.SupportFP16 ? support1 : support0);
        if (cap.SupportIntel8)
            algo.Bit8.Set(REMOVE_MASK(xcomp::VTypeDimSupport::All, xcomp::VTypeDimSupport::Support3), support0);
    }

    CommonSupport =
    {
        { VecDataInfo::DataTypes::Unsigned, 64u,  1u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 16u, 0u },
    };
    if (Cap.SupportFP16)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Float,    16u,  1u, 0u });
    if (Cap.SupportIntel16)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 16u, 0u });
    if (Cap.SupportIntel8)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Unsigned, 8u,  16u, 0u });
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

SubgroupProvider::AlgorithmResult NLCLSubgroupIntel::SubgroupBShufAlgo(uint8_t algoId, VecDataInfo& vtype)
{
    std::u32string_view funcname;
    if (algoId == AlgoIntelShuffle)
        funcname = U"intel_sub_group_shuffle"sv;
    else
        return NLCLSubgroupKHR::SubgroupBShufAlgo(algoId, vtype);
    EnableVecType(vtype);
    return { funcname, {}, U"intel"sv, {} };
}

// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_short.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html

std::u32string NLCLSubgroupIntel::VectorPatch(const std::u32string_view funcName, const std::u32string_view baseFunc, VecDataInfo vtype, 
    VecDataInfo mid, const std::u32string_view extraArg, const std::u32string_view extraParam) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    Expects(vtype.Bit == mid.Bit * mid.Dim0);
    const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
    const auto scalarType = VecDataInfo{ vtype.Type,vtype.Bit,1,0 };
    std::u32string func = FMTSTR(U"inline {0} {1}(const {0} val{2})\r\n{{\r\n    {0} ret;", vecName, funcName, extraParam);
    if (scalarType == mid)
    {
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
            APPEND_FMT(func, U"\r\n    ret.s{0} = {1}(val.s{0}{2});"sv, Idx16Names[i], baseFunc, extraArg);
    }
    else
    {
        const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
        const auto midName    = NLCLRuntime::GetCLTypeName(mid);
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
            APPEND_FMT(func, U"\r\n    ret.s{0} = as_{1}({2}(as_{3}(val.s{0}){4}));"sv,
                Idx16Names[i], scalarName, baseFunc, midName, extraArg);
    }
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

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

ReplaceResult NLCLSubgroupIntel::SubgroupReduceArith(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    Expects(IsReduceOpArith(op)); 

    VecDataInfo mid = vtype;
    if (vtype.Type == VecDataInfo::DataTypes::Float)
    {
        WarnFP(vtype, u"SubgroupReduce"sv);
    }
    else if (vtype.Type == VecDataInfo::DataTypes::Unsigned || vtype.Type == VecDataInfo::DataTypes::Signed)
    {
        switch (vtype.Bit)
        {
        case 16:
            // https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_short.html
            if (Cap.SupportIntel16)
                EnableIntel16 = true;
            else
                mid.Bit = 32;
            break;
        case 8:
            // https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html
            if (Cap.SupportIntel8)
                EnableIntel8 = true;
            else
                mid.Bit = 32;
            break;
        default:
            break;
        }
    }
    else
        Expects(false);
    return NLCLSubgroupKHR::SubgroupReduceArith(op, mid, vtype, ele);
}

ReplaceResult NLCLSubgroupIntel::SubgroupReduceBitwise(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    Expects(!IsReduceOpArith(op));
    const auto opstr = StringifyReduceOp(op);
    enum class Method { Direct, Combine, Convert };

    constexpr auto DecideMethod = [](const uint32_t total, const uint8_t bit, const bool support) -> std::pair<Method, uint8_t>
    {
        if (support)
            return { Method::Direct, bit };
        if (total % 32 == 0 && CheckVNum(total / 32))
            return { Method::Combine, (uint8_t)32 };
        if (total % 64 == 0 && CheckVNum(total / 64))
            return { Method::Combine, (uint8_t)64 };
        return { Method::Convert, (uint8_t)32 };
    };
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const auto [method, scalarBit] = vtype.Bit == 16 ?
        DecideMethod(totalBits, 16, Cap.SupportIntel16) :
        (vtype.Bit == 8 ?
            DecideMethod(totalBits, 8, Cap.SupportIntel8) :
            std::pair<Method, uint8_t>{ Method::Direct, vtype.Bit });
    const auto vnum = method == Method::Combine ? static_cast<uint8_t>(totalBits / scalarBit) : vtype.Dim0;
    Expects(vnum >= 1 && vnum <= 16);

    const auto scalarType = VecDataInfo{ VecDataInfo::DataTypes::Unsigned, scalarBit, 1, 0 };
    const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_intel_{}_{}"sv, opstr, xcomp::StringifyVDataType(scalarType));

    EnableIntel = true;
    auto dep0 = Context.AddPatchedBlock(scalarFunc, [&]()
        {
            auto func = FMTSTR(U"inline {0} {1}({0} x)"sv, scalarName, scalarFunc);
            const auto opinst = [](SubgroupReduceOp op)
            {
                switch (op)
                {
                case SubgroupReduceOp::And: return U"x = x & intel_sub_group_shuffle_xor(x, mask);"sv;
                case SubgroupReduceOp::Or:  return U"x = x | intel_sub_group_shuffle_xor(x, mask);"sv;
                case SubgroupReduceOp::Xor: return U"x = x ^ intel_sub_group_shuffle_xor(x, mask);"sv;
                default: assert(false);     return U""sv;
                }
            }(op);
            func.append(UR"(
{
    for (uint mask = get_sub_group_size(); mask > 0; mask /= 2) 
    {
        )"sv);
            // x = x & intel_sub_group_shuffle_xor(val, mask);
            constexpr auto tail = UR"(
    }
    return x;
})"sv;
            func.append(opinst);
            func.append(tail);
            return func;
        });

    if (vnum == 1)
    {
        switch (method)
        {
        case Method::Direct:
        case Method::Combine:
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, false).Depend(dep0);
        case Method::Convert:
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, true).Depend(dep0);
        }
    }

    const auto vectorType = VecDataInfo { scalarType.Type, scalarBit, vnum, 0 };
    const auto vectorFunc = FMTSTR(U"{}v{}", scalarFunc, vnum);
    auto dep1 = Context.AddPatchedBlockD(vectorFunc, scalarFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });
    switch (method)
    {
    case Method::Direct:
    case Method::Combine:
        return SingleArgFunc(vectorFunc, ele, vtype, vectorType, false).Depend(dep1);
    case Method::Convert:
        return SingleArgFunc(vectorFunc, ele, vtype, ToUintVec(vtype), vectorType, false).Depend(dep1);
    default: 
        assert(false); return {};
    }

}

ReplaceResult NLCLSubgroupIntel::SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele)
{
    if (IsReduceOpArith(op))
        return SubgroupReduceArith(op, vtype, ele);
    else
        return SubgroupReduceBitwise(op, vtype, ele);
}


// algo 8 : intel_sub_group_broadcast
NLCLSubgroupLocal::NLCLSubgroupLocal(common::mlog::MiniLogger<false>& logger, NLCLContext& context, NLCLSubgroupCapbility cap) :
    NLCLSubgroupKHR(logger, context, cap)
{
    constexpr auto setSupport = [](auto& algo) 
    {
        constexpr xcomp::VecDimSupport supportAll = xcomp::VTypeDimSupport::All;
        constexpr xcomp::VecDimSupport support0   = xcomp::VTypeDimSupport::Empty;
        algo.Bit64.Set(supportAll, support0);
        algo.Bit32.Set(supportAll, support0);
        algo.Bit16.Set(supportAll, support0);
        algo.Bit8 .Set(supportAll, support0);
    };
    {
        auto& algo = BShufSupport.emplace_back(AlgoLocalBroadcast, Algorithm::FeatureBroadcast);
        setSupport(algo);
    }
    {
        auto& algo = BShufSupport.emplace_back(AlgoLocalShuffle, static_cast<uint8_t>(
            Algorithm::FeatureBroadcast | Algorithm::FeatureShuffle | Algorithm::FeatureShuffleRel));
        setSupport(algo);
    }
}

SubgroupProvider::AlgorithmResult NLCLSubgroupLocal::SubgroupBShufAlgo(uint8_t algoId, VecDataInfo& vtype)
{
    AlgorithmResult ret{ {}, U"_oclu_subgroup_local"sv, U"local"sv, {} };
    typename decltype(&BroadcastPatch) patchFunc = nullptr;
    std::u32string_view funcTypeName;
    if (algoId == AlgoLocalBroadcast)
    {
        funcTypeName = U"broadcast"sv;
        patchFunc = &NLCLSubgroupLocal::BroadcastPatch;
    }
    else if (algoId == AlgoLocalShuffle)
    {
        funcTypeName = U"shuffle"sv;
        patchFunc = &NLCLSubgroupLocal::ShufflePatch;
    }
    else
        return NLCLSubgroupKHR::SubgroupBShufAlgo(algoId, vtype);
    vtype.Type = VecDataInfo::DataTypes::Unsigned; // force use unsigned even for signed
    ret.FuncName = FMTSTR(U"oclu_subgroup_{}_local_{}"sv, funcTypeName, xcomp::StringifyVDataType(vtype));
    ret.Depends = Context.AddPatchedBlock(ret.FuncName.StrView(), [&]() { return patchFunc(ret.FuncName.StrView(), vtype); }).first;
    EnableVecType(vtype);
    NeedSubgroupSize = NeedLocalTemp = true;
    return ret;
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

std::u32string NLCLSubgroupLocal::BroadcastPatch(const std::u32string_view funcName, const VecDataInfo vtype) noexcept
{
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
    const auto scalarName = NLCLRuntime::GetCLTypeName(ToScalar(vtype));
    std::u32string func = FMTSTR(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)", vecName, funcName);
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
        // at least 16 x ulong, nedd only one store/load
        static constexpr auto temp = UR"(
    if (lid == sgId) vstore{0}(val, 0, ptr);
    barrier(CLK_LOCAL_MEM_FENCE);
    return vload{0}(0, ptr);
)"sv;
        APPEND_FMT(func, temp, vtype.Dim0);
    }

    func.append(U"}"sv);
    return func;
}

std::u32string NLCLSubgroupLocal::ShufflePatch(const std::u32string_view funcName, const VecDataInfo vtype) noexcept
{
    Expects(vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    const auto vecName = NLCLRuntime::GetCLTypeName(vtype);
    const auto scalarName = NLCLRuntime::GetCLTypeName(ToScalar(vtype));
    std::u32string func = FMTSTR(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)"sv, vecName, funcName);
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
}

ReplaceResult NLCLSubgroupLocal::GetSubgroupSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupSize();

    static constexpr auto id = U"oclu_subgroup_local_get_size"sv;
    Context.AddPatchedBlock(U"oclu_subgroup_local_get_size"sv, []()
        {
            return UR"(inline uint oclu_subgroup_local_get_size()
{
    return get_local_size(0) * get_local_size(1) * get_local_size(2);
})"s;
        });
    return { U"oclu_subgroup_local_get_size()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupLocal::GetMaxSubgroupSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetMaxSubgroupSize();
    
    NeedSubgroupSize = true;
    const auto kid = GenerateKID(U"sgsize"sv);
    return { FMTSTR(U"_{}"sv, kid), GEN_DEPEND(kid) };
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupCount()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupCount();
    return U"(/*local mimic SubgroupCount*/ 1)"sv;
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupId();
    return U"(/*local mimic SubgroupId*/ 0)"sv;
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupLocalId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupLocalId();

    static constexpr auto id = U"oclu_subgroup_local_get_local_id"sv;
    Context.AddPatchedBlock(id, []()
        {
            return UR"(inline uint oclu_subgroup_local_get_local_id()
{
    return get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
})"s;
        });
    return { U"oclu_subgroup_local_get_local_id()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupLocal::SubgroupAll(const std::u32string_view predicate)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupAll(predicate);
    RET_FAIL(SubgroupAll);
}
ReplaceResult NLCLSubgroupLocal::SubgroupAny(const std::u32string_view predicate)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupAny(predicate);
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
        ExtraSync = U".sync"sv, ExtraMask = U", 0xffffffff"sv; // assum 32 as warp size

    auto& algo = BShufSupport.emplace_back(AlgoPtxShuffle, static_cast<uint8_t>(
        Algorithm::FeatureBroadcast | Algorithm::FeatureShuffle | Algorithm::FeatureShuffleRel));
    constexpr xcomp::VecDimSupport support1 = xcomp::VTypeDimSupport::Support1;
    constexpr xcomp::VecDimSupport support0 = xcomp::VTypeDimSupport::Empty;
    algo.Bit64.Set(support1, support0);
    algo.Bit32.Set(support1, support0);
    algo.Bit16.Set(support0);
    algo.Bit8 .Set(support0);
}

SubgroupProvider::AlgorithmResult NLCLSubgroupPtx::SubgroupBShufAlgo(uint8_t algoId, VecDataInfo& vtype)
{
    AlgorithmResult ret{ {}, {}, U"ptx"sv, {} };
    if (algoId == AlgoPtxShuffle)
    {
        vtype.Type = VecDataInfo::DataTypes::Unsigned;
        ret.FuncName = FMTSTR(U"oclu_subgroup_shuffle_ptx_u{}"sv, vtype.Bit);
        ret.Depends = Context.AddPatchedBlock(ret.FuncName.StrView(), [&]() { return ShufflePatch(vtype.Bit); }).first;
    }
    else
        return SubgroupProvider::SubgroupBShufAlgo(algoId, vtype);
    EnableVecType(vtype);
    return ret;
}

// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#data-movement-and-conversion-instructions-shfl
std::u32string NLCLSubgroupPtx::ShufflePatch(const uint8_t bit) noexcept
{
    Expects(bit == 32 || bit == 64);
    VecDataInfo vtype{ VecDataInfo::DataTypes::Unsigned,bit,1,0 };
    auto func = FMTSTR(U"inline {0} oclu_subgroup_shuffle_ptx_u{1}(const {0} val, const uint sgId)\r\n{{",
        NLCLRuntime::GetCLTypeName(vtype), bit);
    if (bit == 32)
    {
        func.append(UR"(
    uint ret;
    )"sv);
        APPEND_FMT(func, UR"(asm volatile("shfl{0}.idx.b32 %0, %1, %2, 0x1f{1};" : "=r"(ret) : "r"(val), "r"(sgId));)"sv, ExtraSync, ExtraMask);
        func.append(UR"(
    return ret;
})"sv);
    }
    else if (bit == 64)
    {
        func.append(UR"(
    uint2 tmp = as_uint2(val), ret;
    )"sv);
        static constexpr auto temp = UR"(asm volatile("shfl{0}.idx.b32 %0, %2, %4, 0x1f{1}; shfl{0}.idx.b32 %1, %3, %4, 0x1f{1};" 
        : "=r"(ret.x), "=r"(ret.y) : "r"(tmp.x), "r"(tmp.y), "r"(sgId));)"sv;
        APPEND_FMT(func, temp, ExtraSync, ExtraMask);
        func.append(UR"(
    return as_ulong(ret);
})"sv);
    }
    else
    {
        assert(false);
    }
    return func;
}

ReplaceResult NLCLSubgroupPtx::SubgroupReduceSM80(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    const bool isArith = IsReduceOpArith(op);
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    if (isArith)
    {
        if ((vtype.Type != VecDataInfo::DataTypes::Unsigned && vtype.Type != VecDataInfo::DataTypes::Signed) || vtype.Bit > 32)
            return { U"ptx's sm80 [SubgroupReduce] support only int-32 for arith op"sv, false };
    }
    else
    {
        if (totalBits > 32 && !TryCombine(totalBits, 32)) // b32v16 at most
            return {U"ptx's sm80 [SubgroupReduce] support at most b32v16 for bitwise op"sv, false};
    }

    const auto opstr      = StringifyReduceOp(op);
    const auto opname     = op == SubgroupReduceOp::Sum ? U"add"sv : opstr;
    const bool isUnsigned = vtype.Type == VecDataInfo::DataTypes::Unsigned;
    const auto scalarType = VecDataInfo{ isArith ? vtype.Type : VecDataInfo::DataTypes::Unsigned, 32, 1, 0 };
    const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_ptx_{}_{}32"sv, opstr, isArith ? (isUnsigned ? U"u" : U"i") : U"");
    auto dep0 = Context.AddPatchedBlock(scalarFunc, [&]()
        {
            const auto regType = isArith ? (isUnsigned ? U'u' : U's') : U'b';
            // https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#warp-reduce-functions
            static constexpr auto syntax = UR"(inline {0} {1}({0} x)
{{
    {0} ret;
    asm volatile("redux.sync.{2}.{3}32 %0, %1{4};" : "=r"(ret) : "r"(x));
    return ret;
}})"sv;
            return FMTSTR(syntax, scalarName, scalarFunc, opname, regType, ExtraMask);
        });

    // direct
    if (isArith)
    {
        if (vtype.Dim0 == 1)
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, true).Depend(dep0);
    }
    else if (totalBits == 32)
    {
        return SingleArgFunc(scalarFunc, ele, vtype, scalarType, false).Depend(dep0);
    }
    else if (totalBits < 32)
    {
        auto str = FMTSTR(U"as_{0}(convert_{1}({2}(convert_uint(as_{1}({3})))))"sv,
            NLCLRuntime::GetCLTypeName(vtype),
            NLCLRuntime::GetCLTypeName({ VecDataInfo::DataTypes::Unsigned,static_cast<uint8_t>(totalBits),1,0 }),
            scalarFunc, ele);
        return ReplaceResult(std::move(str), dep0.first);
    }

    const auto vectorType = VecDataInfo
    { 
        scalarType.Type,
        32, 
        isArith ? vtype.Dim0 : static_cast<uint8_t>(totalBits / 32),
        0
    };
    Expects(vectorType.Dim0 > 1 && vectorType.Dim0 <= 16);
    const auto vectorFunc = FMTSTR(U"{}v{}", scalarFunc, vectorType.Dim0);
    auto dep1 = Context.AddPatchedBlockD(vectorFunc, scalarFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });

    return SingleArgFunc(vectorFunc, ele, vtype, vectorType, isArith).Depend(dep1);
}

ReplaceResult NLCLSubgroupPtx::SubgroupReduceSM30(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    const bool isArith = IsReduceOpArith(op);
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    uint8_t scalarBit = vtype.Bit, vnum = vtype.Dim0;
    if (isArith)
    {
        if (vtype.Bit != 32 && vtype.Bit != 64)
            return {U"ptx's [SubgroupReduce] only support 32/64bit on arith op"sv, false};
    }
    else
    {
        if (vtype.Bit < 32)
        {
            if (const auto vnum_ = TryCombine(totalBits, 32); vnum_)
                scalarBit = 32, vnum = vnum_;
            else
                scalarBit = 32;
        }
    }
    
    const auto opstr      = StringifyReduceOp(op);
    const auto scalarType = VecDataInfo{ isArith ? vtype.Type : VecDataInfo::DataTypes::Unsigned, scalarBit, 1, 0 };
    const auto scalarName = NLCLRuntime::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_ptx_{}_{}"sv, opstr, xcomp::StringifyVDataType(scalarType));
    auto dep0 = Context.AddPatchedBlock(scalarFunc, [&]()
        {
            auto func = FMTSTR(U"inline {0} {1}({0} x)"sv, scalarName, scalarFunc);
            const auto opinst = [](SubgroupReduceOp op)
            {
                switch (op)
                {
                case SubgroupReduceOp::Sum: return U"\r\n        x = x + tmp;"sv;
                case SubgroupReduceOp::Min: return U"\r\n        x = min(x, tmp);"sv;
                case SubgroupReduceOp::Max: return U"\r\n        x = max(x, tmp);"sv;
                case SubgroupReduceOp::And: return U"\r\n        x = x & tmp;"sv;
                case SubgroupReduceOp::Or:  return U"\r\n        x = x | tmp;"sv;
                case SubgroupReduceOp::Xor: return U"\r\n        x = x ^ tmp;"sv;
                default: assert(false);     return U""sv;
                }
            }(op);
            // https://developer.nvidia.com/blog/faster-parallel-reductions-kepler/
            func.append(UR"(
{
    for (uint mask = 32/2; mask > 0; mask /= 2) 
    {
        )"sv);
        // shfl.bfly.b32 %r1, %r2, %r3, %r4; // tmp = __shfl_xor(val, mask);
        // x = x + tmp;
            constexpr auto tail = UR"(
    }
    return x;
})"sv;

            if (scalarBit == 64)
            {
                func.append(UR"(uint2 tmpi = as_uint2(x), tmpo;
        )"sv);
                static constexpr auto syntax = UR"(asm volatile("shfl{0}.bfly.b32 %0, %2, %4, 0x1f{1}; shfl{0}.bfly.b32 %1, %3, %4, 0x1f{1};" 
            : "=r"(tmpo.x), "=r"(tmpo.y) : "r"(tmpi.x), "r"(tmpi.y), "r"(mask));
        )"sv;
                APPEND_FMT(func, syntax, ExtraSync, ExtraMask);
                APPEND_FMT(func, U"{0} tmp = as_{0}(tmpo);"sv, scalarName);
            }
            else //if (scalarBit == 32)
            {
                static constexpr auto syntax = UR"({0} tmp;
        asm volatile("shfl{1}.bfly.b32 %0, %1, %2, 0x1f{2};" : "=r"(tmp) : "r"(x), "r"(mask));)"sv;
                APPEND_FMT(func, syntax, scalarName, ExtraSync, ExtraMask);
            }
            func.append(opinst);
            func.append(tail);
            return func;
        });

    // direct
    if (vnum == 1)
    {
        if (isArith)
        {
            Expects(scalarType == vtype);
            return SingleArgFunc(scalarFunc, ele).Depend(dep0);
        }
        else
        {
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, totalBits != scalarBit).Depend(dep0);
        }
    }

    const auto vectorType = VecDataInfo
    {
        scalarType.Type,
        scalarBit,
        vnum,
        0
    };
    Expects(CheckVNum(vnum));
    const auto vectorFunc = FMTSTR(U"{}v{}", scalarFunc, vectorType.Dim0);
    auto dep1 = Context.AddPatchedBlockD(vectorFunc, scalarFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });

    if (isArith)
    {
        Expects(vectorType == vtype);
        return SingleArgFunc(vectorFunc, ele).Depend(dep1);
    }
    else
    {
        if (totalBits != static_cast<uint32_t>(scalarBit * vnum)) // need convert
        {
            return SingleArgFunc(vectorFunc, ele, vtype, ToUintVec(vtype), vectorType, false).Depend(dep1);
        }
        else
        {
            return SingleArgFunc(vectorFunc, ele, vtype, vectorType, false).Depend(dep1);
        }
    }
}

ReplaceResult NLCLSubgroupPtx::GetSubgroupSize()
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
    return { U"oclu_subgroup_ptx_get_size()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupPtx::GetMaxSubgroupSize()
{
    return U"(/*nv_warp*/32u)"sv;
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupCount()
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
    return { U"oclu_subgroup_ptx_count()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupId()
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
    return { U"oclu_subgroup_ptx_get_id()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupLocalId()
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
    return { U"oclu_subgroup_ptx_get_local_id()"sv, GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupPtx::SubgroupAll(const std::u32string_view predicate)
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
            constexpr auto tail = UR"(
                 "selp.s32          %0, 1, 0, %%p2; \n\t"
                 "}" : "=r"(ret) : "r"(predicate));
    return ret;
})"sv;
            APPEND_FMT(ret, UR"("vote{0}.all.pred  %%p2, %%p1{1};  \n\t\t")"sv, ExtraSync, ExtraMask);
            ret.append(tail);
            return ret;
        });
    return { FMTSTR(U"oclu_subgroup_ptx_all({})", predicate), GEN_DEPEND(id) };
}
ReplaceResult NLCLSubgroupPtx::SubgroupAny(const std::u32string_view predicate)
{
    static constexpr auto id = U"oclu_subgroup_ptx_any"sv;
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_any"sv, [&]()
        {
            auto ret = UR"(inline int oclu_subgroup_ptx_any(int predicate)
{
    int ret;
    asm volatile("{\n\t\t"
                 ".reg .pred        %%p1, %%p2;     \n\t\t"
                 "setp.ne.u32       %%p1, %1, 0;    \n\t\t"
                 )"s;
            constexpr auto tail = UR"(
                 "selp.s32          %0, 1, 0, %%p2; \n\t"
                 "}" : "=r"(ret) : "r"(predicate));
    return ret;
})"sv;
            APPEND_FMT(ret, UR"("vote{0}.any.pred  %%p2, %%p1{1};  \n\t\t")"sv, ExtraSync, ExtraMask);
            ret.append(tail);
            return ret;
        });
    return { FMTSTR(U"oclu_subgroup_ptx_any({})", predicate), GEN_DEPEND(id) };
}

ReplaceResult NLCLSubgroupPtx::SubgroupReduce(SubgroupReduceOp op, VecDataInfo vtype, const std::u32string_view ele)
{
    if (SMVersion >= 80)
    {
        // fast path 
        auto ret = SubgroupReduceSM80(op, vtype, ele);
        if (ret)
            return ret;
    }

    if (SMVersion >= 0)
    {
        auto ret = SubgroupReduceSM30(op, vtype, ele);
        return ret;
    }

    RET_FAIL(SubgroupReduce);
}



}