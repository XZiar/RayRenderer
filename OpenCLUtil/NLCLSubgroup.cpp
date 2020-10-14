#include "oclPch.h"
#include "NLCLSubgroup.h"



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
    auto format(OptionalArg<IsPrefix> arg, FormatContext& ctx)
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
using namespace std::string_view_literals;
using namespace std::string_literals;
using xcomp::ReplaceResult;
using xziar::nailang::Arg;
using xziar::nailang::ArgLimits;
using xziar::nailang::FuncCall;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;
using common::str::Charset;


constexpr char32_t Idx16Names[] = U"0123456789abcdef";

#define NLRT_THROW_EX(...) Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define RET_FAIL(func) return {U"No proper [" STRINGIZE(func) "]"sv, false}


class SubgroupException : public NailangRuntimeException
{
    friend class NailangRuntimeBase;
    PREPARE_EXCEPTION(SubgroupException, NailangRuntimeException,
        ExceptionInfo(const std::u16string_view msg) : TPInfo(TYPENAME, msg, {}, {}) { }
    );
public:
    SubgroupException(const std::u16string_view msg) : NailangRuntimeException(T_<ExceptionInfo>{}, msg) { }
};


void NLCLSubgroupExtension::BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext&)
{
    Provider = DefaultProvider;
    SubgroupSize = 0;
}

void NLCLSubgroupExtension::FinishInstance(xcomp::XCNLRuntime& runtime, xcomp::InstanceContext& ctx)
{
    Provider->OnFinish(static_cast<NLCLRuntime_&>(runtime), *this, static_cast<KernelContext&>(ctx));
    Provider.reset();
    SubgroupSize = 0;
}

void NLCLSubgroupExtension::InstanceMeta(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& meta, xcomp::InstanceContext&)
{
    auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
    using namespace xziar::nailang;
    if (*meta.Name == U"oclu.SubgroupSize"sv || *meta.Name == U"xcomp.SubgroupSize"sv)
    {
        const auto sgSize = Runtime.EvaluateFuncArgs<1>(meta, { Arg::Type::Integer })[0].GetUint().value();
        SubgroupSize = gsl::narrow_cast<uint8_t>(sgSize);
    }
    else if (*meta.Name == U"oclu.SubgroupExt"sv)
    {
        const auto args = Runtime.EvaluateFuncArgs<2, ArgLimits::AtMost>(meta, { Arg::Type::String, Arg::Type::String });
        Provider = NLCLSubgroupExtension::Generate(Runtime.Logger, Runtime.Context,
            args[0].GetStr().value_or(std::u32string_view{}),
            args[1].GetStr().value_or(std::u32string_view{}));
    }
}

ReplaceResult NLCLSubgroupExtension::ReplaceFunc(xcomp::XCNLRuntime& runtime, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (common::str::IsBeginWith(func, U"oclu."))
        func.remove_prefix(5);
    else if(common::str::IsBeginWith(func, U"xcomp."))
        func.remove_prefix(6);
    else
        return {};
    auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
    constexpr auto HandleResult = [](ReplaceResult result, common::str::StrVariant<char32_t> msg)
    {
        if (!result && result.GetStr().empty())
            result.SetStr(std::move(msg));
        return result;
    };
    try
    {
        const auto SubgroupReduce = [&](std::u32string_view name, SubgroupReduceOp op)
        {
            Runtime.ThrowByReplacerArgCount(func, args, 2, ArgLimits::Exact);
            const auto vtype = Runtime.ParseVecType(args[0], FMTSTR(u"replace [{}]"sv, name));
            return Provider->SubgroupReduce(op, vtype, args[1]);
        };

        switch (hash_(func))
        {
        HashCase(func, U"GetSubgroupSize")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
            return HandleResult(Provider->GetSubgroupSize(),
                U"[GetSubgroupSize] not supported"sv);
        }
        HashCase(func, U"GetMaxSubgroupSize")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
            return HandleResult(Provider->GetMaxSubgroupSize(),
                U"[GetMaxSubgroupSize] not supported"sv);
        }
        HashCase(func, U"GetSubgroupCount")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
            return HandleResult(Provider->GetSubgroupCount(),
                U"[GetSubgroupCount] not supported"sv);
        }
        HashCase(func, U"GetSubgroupLocalId")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
            return HandleResult(Provider->GetSubgroupLocalId(),
                U"[GetSubgroupLocalId] not supported"sv);
        }
        HashCase(func, U"SubgroupAll")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
            return HandleResult(Provider->SubgroupAll(args[0]),
                U"[SubgroupAll] not supported"sv);
        }
        HashCase(func, U"SubgroupAny")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 1, ArgLimits::Exact);
            return HandleResult(Provider->SubgroupAny(args[0]),
                U"[SubgroupAny] not supported"sv);
        }
        HashCase(func, U"SubgroupBroadcast")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
            const auto vtype = Runtime.ParseVecType(args[0], u"replace [SubgroupBroadcast]"sv);
            return HandleResult(Provider->SubgroupBroadcast(vtype, args[1], args[2]),
                FMTSTR(U"SubgroupBroadcast with Type [{}] not supported", args[0]));
        }
        HashCase(func, U"SubgroupShuffle")
        {
            Runtime.ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
            const auto vtype = Runtime.ParseVecType(args[0], u"replace [SubgroupShuffle]"sv);
            return HandleResult(Provider->SubgroupShuffle(vtype, args[1], args[2]),
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
    }
    catch (const SubgroupException& ex)
    {
        Runtime.HandleException(ex);
    }
    return {};
}

std::optional<Arg> NLCLSubgroupExtension::XCNLFunc(xcomp::XCNLRuntime& runtime, const FuncCall& call,
    common::span<const FuncCall>)
{
    auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
    using namespace xziar::nailang;
    if (*call.Name == U"oclu.AddSubgroupPatch"sv)
    {
        Runtime.ThrowIfNotFuncTarget(call, xziar::nailang::FuncName::FuncInfo::Empty);
        Runtime.ThrowByArgCount(call, 2, ArgLimits::AtLeast);
        const auto args = Runtime.EvaluateFuncArgs<4, ArgLimits::AtMost>(call, { Arg::Type::Boolable, Arg::Type::String, Arg::Type::String, Arg::Type::String });
        const auto isShuffle = args[0].GetBool().value();
        const auto vstr  = args[1].GetStr().value();
        const auto vtype = Runtime.ParseVecType(vstr, u"call [AddSubgroupPatch]"sv);

        KernelContext kerCtx;
        SubgroupSize = 32;
        const auto provider = Generate(Logger, Runtime.Context,
            args[2].GetStr().value_or(std::u32string_view{}),
            args[3].GetStr().value_or(std::u32string_view{}));
        const auto ret = isShuffle ?
            provider->SubgroupShuffle(vtype, U"x"sv, U"y"sv) :
            provider->SubgroupBroadcast(vtype, U"x"sv, U"y"sv);
        provider->OnFinish(Runtime, *this, kerCtx);
        
        return Arg{};
    }
    return {};
}

SubgroupCapbility NLCLSubgroupExtension::GenerateCapabiity(NLCLContext& context, const SubgroupAttributes& attr)
{
    SubgroupCapbility cap;
    cap.SupportSubgroupKHR      = context.SupportSubgroupKHR;
    cap.SupportSubgroupIntel    = context.SupportSubgroupIntel;
    cap.SupportSubgroup8Intel   = context.SupportSubgroup8Intel;
    cap.SupportSubgroup16Intel  = context.SupportSubgroup16Intel;
    cap.SupportFP16             = context.SupportFP16;
    cap.SupportFP64             = context.SupportFP64;
    for (auto arg : common::str::SplitStream(attr.Args, ',', false))
    {
        if (arg.empty()) continue;
        const bool isDisable = arg[0] == '-';
        if (isDisable) arg.remove_prefix(1);
        switch (hash_(arg))
        {
#define Mod(dst, name) HashCase(arg, name) cap.dst = !isDisable; break
        Mod(SupportSubgroupKHR,     "sg_khr");
        Mod(SupportSubgroupIntel,   "sg_intel");
        Mod(SupportSubgroup8Intel,  "sg_intel8");
        Mod(SupportSubgroup16Intel, "sg_intel16");
        Mod(SupportFP16,            "fp16");
        Mod(SupportFP64,            "fp64");
#undef Mod
        default: break;
        }
    }
    cap.SupportBasicSubgroup = cap.SupportSubgroupKHR || cap.SupportSubgroupIntel;
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

    if (cap.SupportSubgroupIntel)
        return std::make_shared<NLCLSubgroupIntel>(context, cap);
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
        return std::make_shared<NLCLSubgroupLocal>(context, cap);
    else if (cap.SupportBasicSubgroup)
        return std::make_shared<NLCLSubgroupKHR>(context, cap);
    else
        return std::make_shared<SubgroupProvider>(context, cap);
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
    const auto vecName = NLCLContext::GetCLTypeName(vtype);
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
    const auto vecName = NLCLContext::GetCLTypeName(vtype);
    const auto midName = NLCLContext::GetCLTypeName(midType);
    const auto halfName = NLCLContext::GetCLTypeName(halfType);
    std::u32string func = FMTSTR(U"inline {0} {1}(const {0} val{2})\r\n{{\r\n    {0} ret;", vecName, name, extraParam);
    APPEND_FMT(func, U"\r\n    ret.hi = as_{0}({1}(as_{2}(val.hi){3});\r\n    ret.lo = as_{0}({1}(as_{2}(val.lo){3});",
        halfName, baseFunc, midName, extraArg);
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

SubgroupProvider::SubgroupProvider(NLCLContext& context, SubgroupCapbility cap) :
    Context(context), Cap(cap)
{ }
void SubgroupProvider::AddWarning(common::str::StrVariant<char16_t> msg)
{
    for (const auto& item : Warnings)
    {
        if (item == msg.StrView())
            return;
    }
    Warnings.push_back(msg.ExtractStr());
}
void SubgroupProvider::OnFinish(NLCLRuntime_& runtime, const NLCLSubgroupExtension&, KernelContext&)
{
    for (const auto& item : Warnings)
    {
        runtime.Logger.warning(item);
    }
}


struct SingleArgFunc
{
    std::u32string_view Func;
    std::u32string_view Element;
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
    constexpr SingleArgFunc& Attach(const std::u32string_view prefix, const std::u32string_view suffix) noexcept
    {
        Prefix.Arg = prefix, Suffix.Arg = suffix;
        return *this;
    }
    operator std::u32string() const
    {
        switch (TypeCast.index())
        {
        case 1:
        {
            const auto [dst, mid, convert] = std::get<1>(TypeCast);
            if (dst != mid)
                return FMTSTR(U"{0}_{1}({3}({4}{0}_{2}({5}){6}))"sv,
                    convert ? U"convert"sv : U"as"sv, NLCLContext::GetCLTypeName(dst), NLCLContext::GetCLTypeName(mid), 
                    Func, Prefix, Element, Suffix);
        } break;
        case 2:
        {
            const auto [dst, first, then, convFirst] = std::get<2>(TypeCast);
            if (convFirst)
                return FMTSTR(U"convert_{0}(as_{1}({3}({4}as_{2}(convert_{1}({5})){6})))"sv,
                    NLCLContext::GetCLTypeName(dst), NLCLContext::GetCLTypeName(first), NLCLContext::GetCLTypeName(then),
                    Func, Prefix, Element, Suffix);
            else
                return FMTSTR(U"as_{0}(convert_{1}({3}({4}convert_{2}(as_{1}({5})){6})))"sv,
                    NLCLContext::GetCLTypeName(dst), NLCLContext::GetCLTypeName(first), NLCLContext::GetCLTypeName(then),
                    Func, Prefix, Element, Suffix);
        } break;
        default:
            break;
        }
        return FMTSTR(U"{}({}{}{})", Func, Prefix, Element, Suffix);
    }
};
#define BSRetDirect(func)               return SingleArgFunc(func, ele).Attach(U""sv, idx)
#define BSRetCastT(func, mid)           return SingleArgFunc(func, ele, vtype, mid, false).Attach(U""sv, idx)
#define BSRetCast(func, type, bit, dim) return SingleArgFunc(func, ele, vtype, { type, bit, static_cast<uint8_t>(dim), 0 }, false).Attach(U""sv, idx)



void NLCLSubgroupKHR::OnFinish(NLCLRuntime_& Runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (EnableSubgroup)
        Runtime.EnableExtension("cl_khr_subgroups"sv);
    if (EnableFP16)
        Runtime.EnableExtension("cl_khr_fp16"sv);
    if (EnableFP64)
        Runtime.EnableExtension("cl_khr_fp64"sv) || Runtime.EnableExtension("cl_amd_fp64"sv);
    SubgroupProvider::OnFinish(Runtime, ext, kernel);
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
    EnableSubgroup = true;
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
    Context.AddPatchedBlock(funcName, [&]()
        {
            return NLCLSubgroupKHR::ScalarPatch(funcName, func, vtype);
        });
    return SingleArgFunc(funcName, ele, realType, vtype, true);
}

ReplaceResult NLCLSubgroupKHR::GetSubgroupSize()
{
    EnableSubgroup = true;
    return U"get_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetMaxSubgroupSize()
{
    EnableSubgroup = true;
    return U"get_max_sub_group_size()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupCount()
{
    EnableSubgroup = true;
    return U"get_num_sub_groups()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupId()
{
    EnableSubgroup = true;
    return U"get_sub_group_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::GetSubgroupLocalId()
{
    EnableSubgroup = true;
    return U"get_sub_group_local_id()"sv;
}
ReplaceResult NLCLSubgroupKHR::SubgroupAll(const std::u32string_view predicate)
{
    EnableSubgroup = true;
    return FMTSTR(U"sub_group_all({})"sv, predicate);
}
ReplaceResult NLCLSubgroupKHR::SubgroupAny(const std::u32string_view predicate)
{
    EnableSubgroup = true;
    return FMTSTR(U"sub_group_any({})"sv, predicate);
}
ReplaceResult NLCLSubgroupKHR::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    EnableSubgroup = true;
    // https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/sub_group_broadcast.html
    constexpr auto func = U"sub_group_broadcast"sv;

    // prioritize supported width to avoid patch
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    switch (totalBits)
    {
    case 64:
        if (vtype.Type == VecDataInfo::DataTypes::Float && vtype.Dim0 == 1 && Cap.SupportFP64)
        { // fp64, consider extension
            EnableFP64 = true;
            BSRetDirect(func);
        }
        BSRetCast(func, VecDataInfo::DataTypes::Unsigned, 64, 1);
    case 32:
        BSRetCast(func, vtype.Type, 32, 1);
    case 16:
        if (Cap.SupportFP16) // only when FP16 is supported
        {
            EnableFP16 = true;
            BSRetCast(func, VecDataInfo::DataTypes::Float, 16, 1);
        }
        break;
    default:
        break;
    }

    // need patch
    constexpr VecDataInfo supportScalars[]
    {
        { VecDataInfo::DataTypes::Unsigned, 64u, 1u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 1u, 0u },
        { VecDataInfo::DataTypes::Float,    16u, 1u, 0u },
    };
    common::span<const VecDataInfo> supports(supportScalars);
    if (!Cap.SupportFP16)
        supports = supports.subspan(0, 2);
    if (const auto mid = DecideVtype(vtype, supports); mid.has_value())
    {
        if (mid->Bit == 16)
            EnableFP16 = true;
        const auto funcName = FMTSTR(U"oclu_subgroup_broadcast_{}", totalBits);
        Context.AddPatchedBlock(funcName, [&]()
            {
                return NLCLSubgroupKHR::ScalarPatchBcastShuf(funcName, U"sub_group_broadcast"sv, mid.value());
            });
        BSRetCastT(funcName, *mid);
    }
    RET_FAIL(SubgroupBroadcast);
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


NLCLSubgroupIntel::NLCLSubgroupIntel(NLCLContext& context, SubgroupCapbility cap) : NLCLSubgroupKHR(context, cap)
{
    CommonSupport =
    {
        { VecDataInfo::DataTypes::Unsigned, 64u,  1u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 16u, 0u },
    };
    if (Cap.SupportFP16)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Float,    16u,  1u, 0u });
    if (Cap.SupportSubgroup16Intel)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 16u, 0u });
    if (Cap.SupportSubgroup8Intel)
        CommonSupport.push_back({ VecDataInfo::DataTypes::Unsigned, 8u,  16u, 0u });
}

// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_short.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html

std::u32string NLCLSubgroupIntel::VectorPatch(const std::u32string_view funcName, const std::u32string_view baseFunc, VecDataInfo vtype, 
    VecDataInfo mid, const std::u32string_view extraArg, const std::u32string_view extraParam) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    Expects(vtype.Bit == mid.Bit * mid.Dim0);
    const auto vecName = NLCLContext::GetCLTypeName(vtype);
    const auto scalarType = VecDataInfo{ vtype.Type,vtype.Bit,1,0 };
    std::u32string func = FMTSTR(U"inline {0} {1}(const {0} val{2})\r\n{{\r\n    {0} ret;", vecName, funcName, extraParam);
    if (scalarType == mid)
    {
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
            APPEND_FMT(func, U"\r\n    ret.s{0} = {1}(val.s{0}{2});"sv, Idx16Names[i], baseFunc, extraArg);
    }
    else
    {
        const auto scalarName = NLCLContext::GetCLTypeName(scalarType);
        const auto midName    = NLCLContext::GetCLTypeName(mid);
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
            APPEND_FMT(func, U"\r\n    ret.s{0} = as_{1}({2}(as_{3}(val.s{0}){4}));"sv,
                Idx16Names[i], scalarName, baseFunc, midName, extraArg);
    }
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

void NLCLSubgroupIntel::OnFinish(NLCLRuntime_& Runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (EnableSubgroupIntel)
        Runtime.EnableExtension("cl_intel_subgroups"sv);
    if (EnableSubgroup16Intel)
        Runtime.EnableExtension("cl_intel_subgroups_short"sv);
    if (EnableSubgroup8Intel)
        Runtime.EnableExtension("cl_intel_subgroups_char"sv);
    if (EnableSubgroup)
        EnableSubgroup = !Runtime.EnableExtension("cl_intel_subgroups"sv);
    if (ext.SubgroupSize > 0)
    {
        if (Runtime.EnableExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv))
            kernel.AddAttribute(U"reqd_sub_group_size",
                FMTSTR(U"__attribute__((intel_reqd_sub_group_size({})))", ext.SubgroupSize));
    }
    NLCLSubgroupKHR::OnFinish(Runtime, ext, kernel);
}

ReplaceResult NLCLSubgroupIntel::DirectShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    constexpr auto func = U"intel_sub_group_shuffle"sv;
    EnableSubgroupIntel = true;

    // match scalar bit first
    switch (vtype.Bit)
    {
    case 64: // only scalar supported
        if (vtype.Dim0 == 1)
        {
            if (Cap.SupportFP64) // all is fine
            {
                EnableFP64 = true;
                BSRetDirect(func);
            }
            else
                BSRetCast(func, vtype.Type == VecDataInfo::DataTypes::Float ? VecDataInfo::DataTypes::Unsigned : vtype.Type, 64, 1);
        }
        break;
    case 32: // all vector supported
        BSRetDirect(func);
    case 16:
        if ((vtype.Dim0 != 1 || vtype.Type != VecDataInfo::DataTypes::Float) && Cap.SupportSubgroup16Intel)
        {
            // vector need intel16, scalar int prefer intel16
            EnableSubgroup16Intel = true;
            if (vtype.Type != VecDataInfo::DataTypes::Float)
                BSRetDirect(func);
            else
                BSRetCast(func, VecDataInfo::DataTypes::Unsigned, 16, vtype.Dim0);
        }
        if (vtype.Dim0 == 1 && Cap.SupportFP16)
        {
            EnableFP16 = true;
            BSRetCast(func, VecDataInfo::DataTypes::Float, 16, 1);
        }
        break;
    case 8:
        if (Cap.SupportSubgroup8Intel)
        {
            EnableSubgroup8Intel = true;
            const auto newType = vtype.Type == VecDataInfo::DataTypes::Float ? VecDataInfo::DataTypes::Unsigned : vtype.Type;
            BSRetCast(func, newType, 8, vtype.Dim0);
        }
        break;
    default:
        break;
    }

    // try match smaller datatype
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    for (const auto& mid : CommonSupport)
    {
        if (vtype.Bit <= mid.Bit /*|| vtype.Bit % mid.Bit != 0*/)
            continue;
        const auto vnum = TryCombine(totalBits, mid.Bit, mid.Dim0);
        if (!vnum)
            continue;
        if (mid.Bit == 16)
        {
            if (mid.Type == VecDataInfo::DataTypes::Float)
                EnableFP16 = true;
            else
                EnableSubgroup16Intel = true;
        }
        else if (mid.Bit == 8)
            EnableSubgroup8Intel = true;
        BSRetCast(func, mid.Type, mid.Bit, vnum);
    }
    RET_FAIL(SubgroupShuffle);
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
            if (Cap.SupportSubgroup16Intel)
                EnableSubgroup16Intel = true;
            else
                mid.Bit = 32;
            break;
        case 8:
            // https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html
            if (Cap.SupportSubgroup8Intel)
                EnableSubgroup8Intel = true;
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
        DecideMethod(totalBits, 16, Cap.SupportSubgroup16Intel) :
        (vtype.Bit == 8 ?
            DecideMethod(totalBits, 8, Cap.SupportSubgroup8Intel) :
            std::pair<Method, uint8_t>{ Method::Direct, vtype.Bit });
    const auto vnum = method == Method::Combine ? static_cast<uint8_t>(totalBits / scalarBit) : vtype.Dim0;
    Expects(vnum >= 1 && vnum <= 16);

    const auto scalarType = VecDataInfo{ VecDataInfo::DataTypes::Unsigned, scalarBit, 1, 0 };
    const auto scalarName = NLCLContext::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_intel_{}_{}"sv, opstr, xcomp::StringifyVDataType(scalarType));

    EnableSubgroupIntel = true;
    Context.AddPatchedBlock(scalarFunc, [&]()
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
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, false);
        case Method::Convert:
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, true);
        }
    }

    const auto vectorType = VecDataInfo { scalarType.Type, scalarBit, vnum, 0 };
    const auto vectorFunc = FMTSTR(U"{}v{}", scalarFunc, vnum);
    Context.AddPatchedBlock(vectorFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });
    switch (method)
    {
    case Method::Direct:
    case Method::Combine:
        return SingleArgFunc(vectorFunc, ele, vtype, vectorType, false);
    case Method::Convert:
        return SingleArgFunc(vectorFunc, ele, vtype, ToUintVec(vtype), vectorType, false);
    default: 
        assert(false); return {};
    }

}

ReplaceResult NLCLSubgroupIntel::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    constexpr auto bcKHR = U"sub_group_broadcast"sv;
    constexpr auto bcIntel = U"intel_sub_group_broadcast"sv;
    
    // prioritize scalar type due to reg layout
    if (vtype.Dim0 == 1)
    {
        switch (vtype.Bit)
        {
        case 64:
            if (vtype.Type == VecDataInfo::DataTypes::Float && Cap.SupportFP64)
            { // fp64, consider extension
                EnableSubgroup = true;
                EnableFP64 = true;
                BSRetDirect(bcKHR);
            }
            BSRetCast(bcKHR, VecDataInfo::DataTypes::Unsigned, 64, 1);
        case 32: // all supported
            EnableSubgroup = true;
            BSRetDirect(bcKHR);
        case 16:
            if (vtype.Type == VecDataInfo::DataTypes::Float && Cap.SupportFP16)
            {
                EnableFP16 = true;
                EnableSubgroup = true;
                BSRetDirect(bcKHR);
            }
            if (vtype.Type != VecDataInfo::DataTypes::Float && Cap.SupportSubgroup16Intel)
            {
                EnableSubgroup16Intel = true;
                BSRetDirect(bcIntel);
            }
            if (Cap.SupportFP16)
            {
                EnableSubgroup = true;
                EnableFP16 = true;
                BSRetCast(bcKHR, VecDataInfo::DataTypes::Float, 16, 1);
            }
            if (Cap.SupportSubgroup16Intel)
            {
                EnableSubgroup16Intel = true;
                BSRetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 16, 1);
            }
            break;
        case 8:
            if (Cap.SupportSubgroup8Intel)
            {
                EnableSubgroup8Intel = true;
                BSRetDirect(bcIntel);
            }
            break;
        default:
            break;
        }
    }
    
    // fallback to subgroup_shuffle due to reg layout
    if (auto ret = DirectShuffle(vtype, ele, idx); ret)
        return ret;
    
    // fallback to reinterpret to avoid patch
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    switch (totalBits)
    {
    case 64:
        // no need to consider FP64
        EnableSubgroup = true;
        BSRetCast(bcKHR, VecDataInfo::DataTypes::Unsigned, 64, 1);
    case 32:
        EnableSubgroup = true;
        BSRetCast(bcKHR, vtype.Type, 32, 1);
    case 16:
        if (Cap.SupportFP16)
        {
            EnableSubgroup = true;
            EnableFP16 = true;
            BSRetCast(bcKHR, VecDataInfo::DataTypes::Float, 16, 1);
        }
        if (Cap.SupportSubgroup16Intel)
        {
            EnableSubgroup16Intel = true;
            BSRetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 16, 1);
        }
        break;
    case 8:
        if (Cap.SupportSubgroup8Intel)
        {
            EnableSubgroup8Intel = true;
            BSRetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 8, 1);
        }
        break;
    default:
        break;
    }

    // need patch
    if (const auto mid = DecideVtype(vtype, CommonSupport); mid.has_value())
    {
        bool useIntel = false;
        if (mid->Bit == 16)
        {
            if (mid->Type == VecDataInfo::DataTypes::Float)
                EnableFP16 = true, useIntel = false;
            else
                EnableSubgroup16Intel = true, useIntel = true;
        }
        else if (mid->Bit == 8)
            EnableSubgroup8Intel = true, useIntel = true;
        else
            useIntel = false;
        const auto funcName = FMTSTR(U"oclu_subgroup_broadcast{}_{}", useIntel? U"_intel"sv : U""sv, totalBits);
        Context.AddPatchedBlock(funcName, [&]()
            {
                return NLCLSubgroupIntel::ScalarPatchBcastShuf(funcName, useIntel ? bcIntel : bcKHR, mid.value());
            });
        BSRetCastT(funcName, mid.value());
    }
    RET_FAIL(SubgroupBroadcast);
}

ReplaceResult NLCLSubgroupIntel::SubgroupShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    constexpr auto func = U"intel_sub_group_shuffle"sv;
    // fallback to subgroup_shuffle to due to reg layout
    if (auto ret = DirectShuffle(vtype, ele, idx); ret)
        return ret;

    // need patch, prefer smaller type due to reg layout
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const VecDataInfo newType = ToUintVec(vtype);
    for (const auto& mid : CommonSupport)
    {
        if (vtype.Bit <= mid.Bit || mid.Bit > 32 /*|| vtype.Bit % mid.Bit != 0*/)
            continue;
        const auto vnum = vtype.Bit / mid.Bit;
        if (!CheckVNum(vnum))
            continue;
        if (mid.Bit == 16)
        {
            if (mid.Type == VecDataInfo::DataTypes::Float)
                EnableFP16 = true;
            else
                EnableSubgroup16Intel = true;
        }
        else if (mid.Bit == 8)
            EnableSubgroup8Intel = true;
        const auto funcName = FMTSTR(U"oclu_subgroup_shuffle_intel_{}", totalBits); 
        Context.AddPatchedBlock(funcName, [&]()
            {
                return NLCLSubgroupIntel::VectorPatchBcastShuf(funcName, func, newType, ToScalar(mid));
            });
        BSRetCastT(funcName, newType);
    }

    // need patch, ignore reg size
    if (const auto mid = DecideVtype(vtype, CommonSupport); mid.has_value())
    {
        if (mid->Bit == 16)
        {
            if (mid->Type == VecDataInfo::DataTypes::Float)
                EnableFP16 = true;
            else
                EnableSubgroup16Intel = true;
        }
        else if (mid->Bit == 8)
            EnableSubgroup8Intel = true;
        const auto funcName = FMTSTR(U"oclu_subgroup_shuffle_intel_{}", totalBits); 
        Context.AddPatchedBlock(funcName, [&]()
            {
                return NLCLSubgroupIntel::ScalarPatchBcastShuf(funcName, func, mid.value());
            });
        BSRetCastT(funcName, mid.value());
    }

    RET_FAIL(SubgroupShuffle);
}

ReplaceResult NLCLSubgroupIntel::SubgroupReduce(SubgroupReduceOp op, common::simd::VecDataInfo vtype, const std::u32string_view ele)
{
    if (IsReduceOpArith(op))
        return SubgroupReduceArith(op, vtype, ele);
    else
        return SubgroupReduceBitwise(op, vtype, ele);
}


void NLCLSubgroupLocal::OnFinish(NLCLRuntime_& Runtime, const NLCLSubgroupExtension& ext, KernelContext& kernel)
{
    if (NeedCommonInfo)
    {
        kernel.AddBodyPrefix(U"oclu_subgroup_mimic_local_common"sv, UR"(
    const uint _oclu_sglid  = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
    const uint _oclu_sgsize = get_local_size(0) * get_local_size(1) * get_local_size(2);
)"sv);
    }
    if (NeedLocalTemp)
    {
        if (kernel.GetWorkgroupSize() == 0)
            NLRT_THROW_EX(u"Subgroup mimic [local] need to define GetWorkgroupSize() when under fully mimic");
        if (ext.SubgroupSize && kernel.GetWorkgroupSize() != ext.SubgroupSize)
            NLRT_THROW_EX(FMTSTR(u"Subgroup mimic [local] requires only 1 subgroup for the whole workgroup, now have subgroup[{}] and workgroup[{}].",
                ext.SubgroupSize, kernel.GetWorkgroupSize()));
        const auto sgSize = kernel.GetWorkgroupSize();
        kernel.AddBodyPrefix(U"oclu_subgroup_mimic_local_slm",
            FMTSTR(U"    local ulong _oclu_subgroup_local[{}];\r\n", std::max<uint32_t>(sgSize, 16)));
        Runtime.Logger.debug(u"Subgroup mimic [local] is enabled with size [{}].\n", sgSize);
    }
    NLCLSubgroupKHR::OnFinish(Runtime, ext, kernel);
}

std::u32string NLCLSubgroupLocal::BroadcastPatch(const std::u32string_view funcName, const VecDataInfo vtype) noexcept
{
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    const auto vecName = NLCLContext::GetCLTypeName(vtype);
    const auto scalarName = NLCLContext::GetCLTypeName(ToScalar(vtype));
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
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    const auto vecName = NLCLContext::GetCLTypeName(vtype);
    const auto scalarName = NLCLContext::GetCLTypeName(ToScalar(vtype));
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
    NeedCommonInfo = true;
    return U"_oclu_sgsize"sv;
}
ReplaceResult NLCLSubgroupLocal::GetMaxSubgroupSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetMaxSubgroupSize();
    NeedCommonInfo = true;
    return U"_oclu_sgsize"sv;
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupCount()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupCount();
    NeedCommonInfo = true;
    return U"(/*local mimic SubgroupCount*/ 1)"sv;
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupId();
    NeedCommonInfo = true;
    return U"(/*local mimic SubgroupId*/ 0)"sv;
}
ReplaceResult NLCLSubgroupLocal::GetSubgroupLocalId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupLocalId();
    NeedCommonInfo = true;
    return U"_oclu_sglid"sv;
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

ReplaceResult NLCLSubgroupLocal::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupBroadcast(vtype, ele, idx);
    
    NeedCommonInfo = NeedLocalTemp = true;
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const auto funcName = FMTSTR(U"oclu_subgroup_local_broadcast_{}", totalBits);
    for (const auto targetBits : std::array<uint8_t, 4>{ 64u, 32u, 16u, 8u })
    {
        const auto vnum = TryCombine(totalBits, targetBits);
        if (!vnum) continue;
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, targetBits, vnum, 0 };
        Context.AddPatchedBlock(*this, funcName, &NLCLSubgroupLocal::BroadcastPatch, funcName, mid);
        return SingleArgFunc(funcName, ele, vtype, mid, false).Attach(U"_oclu_subgroup_local"sv, idx);
    }
    RET_FAIL(SubgroupBroadcast);
}

ReplaceResult NLCLSubgroupLocal::SubgroupShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupShuffle(vtype, ele, idx);
    
    NeedCommonInfo = NeedLocalTemp = true;
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const auto funcName = FMTSTR(U"oclu_subgroup_local_shuffle_{}", totalBits);
    for (const auto targetBits : std::array<uint8_t, 4>{ 64u, 32u, 16u, 8u })
    {
        const auto vnum = TryCombine(totalBits, targetBits);
        if (!vnum) continue;
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, targetBits, vnum, 0 };
        Context.AddPatchedBlock(*this, funcName, &NLCLSubgroupLocal::ShufflePatch, funcName, mid);
        return SingleArgFunc(funcName, ele, vtype, mid, false).Attach(U"_oclu_subgroup_local"sv, idx);
    }
    RET_FAIL(SubgroupShuffle);
}


// https://intel.github.io/llvm-docs/cuda/opencl-subgroup-vs-cuda-crosslane-op.html
NLCLSubgroupPtx::NLCLSubgroupPtx(common::mlog::MiniLogger<false>& logger, NLCLContext& context, SubgroupCapbility cap, const uint32_t smVersion) :
    SubgroupProvider(context, cap), SMVersion(smVersion)
{
    if (smVersion == 0)
        logger.warning(u"Trying to use PtxSubgroup on non-NV platform.\n");
    else if (smVersion < 30)
        logger.warning(u"Trying to use PtxSubgroup on sm{}, shuf is introduced since sm30.\n", smVersion);
    if (smVersion < 70)
        ExtraSync = {};
    else
        ExtraSync = U".sync"sv, ExtraMask = U", 0xffffffff"sv; // assum 32 as warp size
}

// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#data-movement-and-conversion-instructions-shfl
std::u32string NLCLSubgroupPtx::ShufflePatch(const uint8_t bit) noexcept
{
    Expects(bit == 32 || bit == 64);
    VecDataInfo vtype{ VecDataInfo::DataTypes::Unsigned,bit,1,0 };
    auto func = FMTSTR(U"inline {0} oclu_subgroup_ptx_shuffle_u{1}(const {0} val, const uint sgId)\r\n{{",
        NLCLContext::GetCLTypeName(vtype), bit);
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
    const auto scalarName = NLCLContext::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_ptx_{}_{}32"sv, opstr, isArith ? (isUnsigned ? U"u" : U"i") : U"");
    Context.AddPatchedBlock(scalarFunc, [&]()
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
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, true);
    }
    else if (totalBits == 32)
    {
        return SingleArgFunc(scalarFunc, ele, vtype, scalarType, false);
    }
    else if (totalBits < 32)
    {
        return FMTSTR(U"as_{0}(convert_{1}({2}(convert_uint(as_{1}({3})))))"sv,
            NLCLContext::GetCLTypeName(vtype), NLCLContext::GetCLTypeName({ VecDataInfo::DataTypes::Unsigned,static_cast<uint8_t>(totalBits),1,0 }),
            scalarFunc, ele);
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
    Context.AddPatchedBlock(vectorFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });

    return SingleArgFunc(vectorFunc, ele, vtype, vectorType, isArith);
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
    const auto scalarName = NLCLContext::GetCLTypeName(scalarType);
    const auto scalarFunc = FMTSTR(U"oclu_subgroup_ptx_{}_{}"sv, opstr, xcomp::StringifyVDataType(scalarType));
    Context.AddPatchedBlock(scalarFunc, [&]()
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
            return SingleArgFunc(scalarFunc, ele);
        }
        else
        {
            return SingleArgFunc(scalarFunc, ele, vtype, scalarType, totalBits != scalarBit);
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
    Context.AddPatchedBlock(vectorFunc, [&]()
        {
            return NLCLSubgroupPtx::ScalarPatch(vectorFunc, scalarFunc, vectorType);
        });

    if (isArith)
    {
        Expects(vectorType == vtype);
        return SingleArgFunc(vectorFunc, ele);
    }
    else
    {
        if (totalBits != static_cast<uint32_t>(scalarBit * vnum)) // need convert
        {
            return SingleArgFunc(vectorFunc, ele, vtype, ToUintVec(vtype), vectorType, false);
        }
        else
        {
            return SingleArgFunc(vectorFunc, ele, vtype, vectorType, false);
        }
    }
}

ReplaceResult NLCLSubgroupPtx::GetSubgroupSize()
{
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_get_size"sv, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_size()
{
    uint ret;
    asm volatile("mov.u32 %0, WARP_SZ;" : "=r"(ret));
    return ret;
})"s;
        });
    return U"oclu_subgroup_ptx_get_size()"sv;
}
ReplaceResult NLCLSubgroupPtx::GetMaxSubgroupSize()
{
    return U"(/*nv_warp*/32u)"sv;
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupCount()
{
    if (SMVersion < 20)
        return {};
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_count"sv, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_count()
{
    uint ret;
    asm volatile("mov.u32 %0, %%nwarpid;" : "=r"(ret));
    return ret;
})"s;
        });
    return U"oclu_subgroup_ptx_count()"sv;
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupId()
{
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_get_id"sv, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_id()
{
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
    return lid / 32;
})"s;
        });
    /*Context.AddPatchedBlock(U"oclu_subgroup_ptx_get_id"sv, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_id()
{
    uint ret;
    asm volatile("mov.u32 %0, %%warpid;" : "=r"(ret));
    return ret;
})"s;
        });*/
    return U"oclu_subgroup_ptx_get_id()"sv;
}
ReplaceResult NLCLSubgroupPtx::GetSubgroupLocalId()
{
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_get_local_id"sv, []()
        {
            return UR"(inline uint oclu_subgroup_ptx_get_local_id()
{
    uint ret;
    asm volatile("mov.u32 %0, %%laneid;" : "=r"(ret));
    return ret;
})"s;
        });
    return U"oclu_subgroup_ptx_get_local_id()"sv;
}
ReplaceResult NLCLSubgroupPtx::SubgroupAll(const std::u32string_view predicate)
{
    Context.AddPatchedBlock(U"oclu_subgroup_ptx_all"sv, [&]()
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
    return FMTSTR(U"oclu_subgroup_ptx_all({})", predicate);
}
ReplaceResult NLCLSubgroupPtx::SubgroupAny(const std::u32string_view predicate)
{
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
    return FMTSTR(U"oclu_subgroup_ptx_any({})", predicate);
}

ReplaceResult NLCLSubgroupPtx::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    return SubgroupShuffle(vtype, ele, idx);
}
ReplaceResult NLCLSubgroupPtx::SubgroupShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    if (vtype.Bit == 32 || vtype.Bit == 64)
    {
        const VecDataInfo scalarUType{ VecDataInfo::DataTypes::Unsigned, vtype.Bit, 1, 0 };
        const auto scalarFunc = FMTSTR(U"oclu_subgroup_ptx_shuffle_u{}", vtype.Bit);
        Context.AddPatchedBlock(*this, scalarFunc, &NLCLSubgroupPtx::ShufflePatch, vtype.Bit);
        if (vtype.Dim0 == 1)
        {
            BSRetCastT(scalarFunc, scalarUType);
        }

        const auto funcName = FMTSTR(U"{}v{}", scalarFunc, vtype.Dim0);
        const VecDataInfo vectorUType = ToUintVec(vtype);
        Context.AddPatchedBlock(funcName, [&]()
            {
                return NLCLSubgroupPtx::ScalarPatchBcastShuf(funcName, scalarFunc, vectorUType);
            });
        BSRetCastT(funcName, vectorUType);
    }

    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    if (const auto vnum = TryCombine(totalBits, 32); vnum)
    {
        const VecDataInfo midType{ VecDataInfo::DataTypes::Unsigned, 32, vnum, 0 };
        [[maybe_unused]] const auto dummy = SubgroupShuffle(midType, U""sv, U""sv);
        const auto funcName = vnum > 1 ? FMTSTR(U"oclu_subgroup_ptx_shuffle_u32v{}", vnum) : U"oclu_subgroup_ptx_shuffle_u32"s;
        BSRetCastT(funcName, midType);
    }
    RET_FAIL(SubgroupShuffle);
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

    if (SMVersion >= 30)
    {
        auto ret = SubgroupReduceSM30(op, vtype, ele);
        return ret;
    }

    RET_FAIL(SubgroupReduce);
}



}