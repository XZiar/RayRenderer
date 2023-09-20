#include "oclPch.h"
#include "NLCLDp4a.h"
#include "SystemCommon/FormatExtra.h"


using namespace std::string_view_literals;

struct ConvB4Arg
{
    std::u32string_view Arg;
    bool IsUnsigned;
    bool IsToPacked;
    bool NeedConv;
    constexpr ConvB4Arg(std::u32string_view arg, char32_t pfx, bool isPacked, bool wantPacked) :
        Arg(arg), IsUnsigned(pfx == U'u'), IsToPacked(wantPacked), NeedConv(wantPacked != isPacked) { }

    void FormatWith(common::str::FormatterExecutor& executor, common::str::FormatterExecutor::Context& context, const common::str::FormatSpec*) const
    {
        if (!NeedConv)
            executor.PutString(context, Arg, nullptr);
        else
            executor.PutString(context, common::str::Formatter<char32_t>{}.FormatStatic(FmtString(U"as_{}({})"),
                IsToPacked ? (IsUnsigned ? U"uint"sv : U"int"sv) : (IsUnsigned ? U"uchar4"sv : U"char4"sv), Arg),
            nullptr);
    }
};

//template<typename Char>
//struct fmt::formatter<ConvB4Arg, Char>
//{
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx) const
//    {
//        return ctx.begin();
//    }
//    template<typename FormatContext>
//    auto format(const ConvB4Arg& arg, FormatContext& ctx) const
//    {
//        if (!arg.NeedConv)
//            return fmt::format_to(ctx.out(), FMT_STRING(U"{}"), arg.Arg);
//        else
//            return fmt::format_to(ctx.out(), FMT_STRING(U"as_{}({})"), 
//                arg.IsToPacked ? (arg.IsUnsigned ? U"uint"sv : U"int"sv) : (arg.IsUnsigned ? U"uchar4"sv : U"char4"sv), 
//                arg.Arg);
//    }
//};

namespace oclu
{
using namespace std::string_literals;
using xcomp::ReplaceResult;
using xziar::nailang::ArgLimits;
using xziar::nailang::Arg;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncPack;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;
using common::str::Encoding;



#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define FMTSTR4(syntax, ...) Fmter.FormatStatic(FmtString(syntax), __VA_ARGS__)
#define RET_FAIL(func) return {U"No proper ["## #func ##"]"sv, false}



void NLCLDp4aExtension::BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext&)
{
    Provider = GetDefaultProvider();
}

void NLCLDp4aExtension::FinishInstance(xcomp::XCNLRuntime& runtime, xcomp::InstanceContext&)
{
    Provider->OnFinish(static_cast<NLCLRuntime&>(runtime));
    Provider.reset();
}

void NLCLDp4aExtension::InstanceMeta(xcomp::XCNLExecutor& executor, const FuncPack& meta, xcomp::InstanceContext&)
{
    if (meta.GetName() == U"oclu.Dp4aExt"sv)
    {
        executor.ThrowByParamTypes<2, ArgLimits::AtMost>(meta, { Arg::Type::String, Arg::Type::String });
        const auto mimic = meta.TryGet(0, &Arg::GetStr).Or({});
        const auto args  = meta.TryGet(1, &Arg::GetStr).Or({});
        Provider = Generate(mimic, args);
    }
}

constexpr auto SignednessParser = SWITCH_PACK(
    (U"uu", Dp4aProvider::Signedness::UU),
    (U"ss", Dp4aProvider::Signedness::SS),
    (U"us", Dp4aProvider::Signedness::US),
    (U"su", Dp4aProvider::Signedness::SU));
ReplaceResult NLCLDp4aExtension::ReplaceFunc(xcomp::XCNLRawExecutor& executor, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    constexpr auto HandleResult = [](ReplaceResult result, common::str::StrVariant<char32_t> msg)
    {
        if (!result && result.GetStr().empty())
            result.SetStr(std::move(msg));
        return result;
    };
    if (func == U"oclu.Dp4a"sv || func == U"xcomp.Dp4a"sv)
    {
        executor.ThrowByReplacerArgCount(func, args, 4, ArgLimits::AtLeast);
        if (const auto signedness = SignednessParser(args[0]); !signedness.has_value())
            executor.NLRT_THROW_EX(FMTSTR2(u"Repalcer-Func [Dp4a]'s arg[0] expects to a string of {{uu,us,su,ss}}, get [{}]", args[0]));
        else
        {
            bool isSat = false, isPacked = false;
            for (size_t i = 1; i < args.size() - 3; ++i)
            {
                if (args[i] == U"sat"sv) isSat = true;
                else if (args[i] == U"packed"sv) isPacked = true;
            }
            return HandleResult(Provider->DP4A({ signedness.value(), isSat, isPacked }, args.subspan(args.size() - 3)),
                U"[Dp4a] not supported"sv);
        }
    }
    return {};
}

std::optional<Arg> NLCLDp4aExtension::ConfigFunc(xcomp::XCNLExecutor& executor, FuncEvalPack& func)
{
    if (func.GetName() == U"oclu.IntelDp4a"sv)
    {
        executor.ThrowIfNotFuncTarget(func, xziar::nailang::FuncName::FuncInfo::Empty);
        executor.ThrowByParamTypes<1>(func, { Arg::Type::BoolBit });
        HasIntelDp4a = func.Params[0].GetBool().value();
        return Arg{};
    }
    return {};
}

std::shared_ptr<Dp4aProvider> NLCLDp4aExtension::GetDefaultProvider() const
{
    if (!DefaultProvider)
    {
        DefaultProvider = Generate(U"auto"sv, {});
    }
    return DefaultProvider;
}

enum class MimicType { Auto, Plain, Khr, Intel, Arm, Ptx };
constexpr auto MimicParser = SWITCH_PACK(
    (U"auto",   MimicType::Auto),
    (U"plain",  MimicType::Plain),
    (U"khr",    MimicType::Khr),
    (U"intel",  MimicType::Intel),
    (U"arm",    MimicType::Arm),
    (U"ptx",    MimicType::Ptx));
std::shared_ptr<Dp4aProvider> NLCLDp4aExtension::Generate(std::u32string_view mimic, std::u32string_view args) const
{
    bool hasIntelDp4a = HasIntelDp4a;
    const auto args_ = common::str::to_string(args, Encoding::ASCII);
    for (auto arg : common::str::SplitStream(args_, ',', false))
    {
        if (arg.empty()) continue;
        const bool isDisable = arg[0] == '-';
        if (isDisable) arg.remove_prefix(1);
        switch (common::DJBHash::HashC(arg))
        {
#define Mod(dst, name) HashCase(arg, name) dst = !isDisable; break
            Mod(hasIntelDp4a, "gen12");
#undef Mod
        default: break;
        }
    }

    auto mType = MimicParser(mimic).value_or(MimicType::Auto);
    // https://www.khronos.org/registry/OpenCL/extensions/arm/cl_arm_integer_dot_product.txt
    const auto
        hasKhrDP4A  = Context.Device->Extensions.Has("cl_khr_integer_dot_product"sv),
        hasArmDPA8  = Context.Device->Extensions.Has("cl_arm_integer_dot_product_accumulate_int8"sv),
        hasArmDPA8S = Context.Device->Extensions.Has("cl_arm_integer_dot_product_accumulate_saturate_int8"sv),
        hasArmDP8   = Context.Device->Extensions.Has("cl_arm_integer_dot_product_int8"sv);

    if (mType == MimicType::Auto)
    {
        if (hasKhrDP4A) mType = MimicType::Khr;
        else
        {
            switch (Context.Device->PlatVendor)
            {
            case Vendors::NVIDIA:   mType = MimicType::Ptx;     break;
            case Vendors::ARM:      mType = MimicType::Arm;     break;
            case Vendors::Intel:    mType = MimicType::Intel;   break;
            default:
                if (hasArmDPA8 || hasArmDP8)
                    mType = MimicType::Arm;
                else
                    mType = MimicType::Plain;
                break;
            }
        }
    }
    Ensures(mType != MimicType::Auto);

    if (mType == MimicType::Khr)
    {
        bool supportUnpacked = false;
        if (hasKhrDP4A)
        {
            cl_device_integer_dot_product_capabilities_khr supports;
            Context.PlatFunc().clGetDeviceInfo(*Context.Device->DeviceID,
                CL_DEVICE_INTEGER_DOT_PRODUCT_CAPABILITIES_KHR, sizeof(uint32_t), &supports, nullptr);
            supportUnpacked = supports & CL_DEVICE_INTEGER_DOT_PRODUCT_INPUT_4x8BIT_KHR;
        }
        return std::make_shared<NLCLDp4aKhr>(Context, supportUnpacked);
    }
    else if (mType == MimicType::Intel)
        return std::make_shared<NLCLDp4aIntel>(Context, hasIntelDp4a);
    else if (mType == MimicType::Arm)
        return std::make_shared<NLCLDp4aArm>(Context, hasArmDP8, hasArmDPA8, hasArmDPA8S);
    else if (mType == MimicType::Ptx)
        return std::make_shared<NLCLDp4aPtx>(Context, Context.Device->GetNvidiaSMVersion().value_or(0));
    else
        return std::make_shared<NLCLDp4aPlain>(Context);
}


Dp4aProvider::Dp4aProvider(NLCLContext& context) : Context(context)
{ }

xcomp::ReplaceResult Dp4aProvider::DP4ASat(Dp4aType type, const common::span<const std::u32string_view> args)
{
    Expects(type.IsSat());
    std::u32string_view newArgs[] = { U"0"sv, args[1], args[2] };
    const auto ret = DP4A({ type.GetSignedness(), false, type.IsPacked() }, newArgs);
    return { FMTSTR4(U"add_sat({}, {})"sv, ret.GetStr(), args[0]), ret.GetDepends()};
}


constexpr static std::array<char32_t, 3> GetSignPrefix(Dp4aProvider::Signedness signedness) noexcept
{
    switch (signedness)
    {
    case Dp4aProvider::Signedness::UU: return { U'u', U'u', U'u' };
    case Dp4aProvider::Signedness::US: return { U' ', U'u', U' ' };
    case Dp4aProvider::Signedness::SU: return { U' ', U' ', U'u' };
    case Dp4aProvider::Signedness::SS: return { U' ', U' ', U' ' };
    default: assert(false);            return { U'x', U'x', U'x' };
    }
}
constexpr static std::u32string_view GetSignSuffix(Dp4aProvider::Signedness signedness) noexcept
{
    switch (signedness)
    {
    case Dp4aProvider::Signedness::UU: return U"uu"sv;
    case Dp4aProvider::Signedness::US: return U"us"sv;
    case Dp4aProvider::Signedness::SU: return U"su"sv;
    case Dp4aProvider::Signedness::SS: return U"ss"sv;
    default: assert(false);            return U""sv;
    }
}

#define SignednessFixedData(base, sign, sfx) case Signedness::sign: \
return type.IsSat() ? U"oclu_" STRINGIZE(base) sfx "_sat"sv : U"oclu_" STRINGIZE(base) sfx ""sv;

#define GetSignednessFixedData(base)        \
[](Dp4aType type) -> std::u32string_view    \
{                                           \
    switch (type.GetSignedness())           \
    {                                       \
    SignednessFixedData(base, UU, "_uu")    \
    SignednessFixedData(base, US, "_us")    \
    SignednessFixedData(base, SU, "_su")    \
    SignednessFixedData(base, SS, "_ss")    \
    default: assert(false); return {};      \
    }                                       \
}

inline constexpr std::pair<ConvB4Arg, ConvB4Arg> GetDP4Arg(const Dp4aProvider::Dp4aType type, 
    const common::span<const std::u32string_view> args, const bool wantPacked) noexcept
{
    const auto [pfxC, pfxA, pfxB] = GetSignPrefix(type.GetSignedness());
    return { {args[1], pfxA, type.IsPacked(), wantPacked}, {args[2], pfxB, type.IsPacked(), wantPacked} };
}


ReplaceResult NLCLDp4aPlain::DP4A(const Dp4aType type, const common::span<const std::u32string_view> args)
{
    if (type.IsSat()) 
        return DP4ASat(type, args);
    
    const auto funcName = GetSignednessFixedData(dp4a)(type);
    auto dep = Context.AddPatchedBlock(funcName, [&]()
        {
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(type.GetSignedness());
            std::u32string func = FMTSTR4(U"inline {1}int {0}({1}int acc, const {2}char4 a, const {3}char4 b)",
                funcName, pfxC, pfxA, pfxB);
            func.append(UR"(
{
    return acc + ((a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w));
})"sv);
            return func;
        });
    const auto [argA, argB] = GetDP4Arg(type, args, false);
    return { FMTSTR4(U"{}({}, {}, {})"sv, funcName, args[0], argA, argB), dep };
}


NLCLDp4aKhr::NLCLDp4aKhr(NLCLContext& context, const bool supportUnpacked) : NLCLDp4aPlain(context), SupportUnpacked(supportUnpacked)
{ }

// https://github.com/KhronosGroup/OpenCL-Docs/blob/master/ext/cl_khr_integer_dot_product.asciidoc
ReplaceResult NLCLDp4aKhr::DP4A(const Dp4aType type, const common::span<const std::u32string_view> args)
{
    const auto signedness = type.GetSignedness();
    const auto [pfxC, pfxA, pfxB] = GetSignPrefix(signedness);
    const auto sfx = GetSignSuffix(signedness);
    const auto& satSyntax = FmtString(U"dot_acc_sat{}({},{},{})"sv);
    const auto& notsatSyntax = FmtString(U"(dot{}({2}, {3}) + {1})"sv);
    const auto& syntax = common::str::FormatterCombiner::Combine(satSyntax, notsatSyntax);

    std::u32string ret = type.IsSat() ? U"dot_acc_sat"s : U"(dot"s;
    const auto usePacked = type.IsPacked() || !SupportUnpacked;
    const auto packedSfx = usePacked ? Fmter.FormatStatic(FmtString(U"_4x8packed_{}_{}int"sv), sfx, pfxC) : std::u32string{};
    const auto [argA, argB] = GetDP4Arg(type, args, usePacked);
    
    return syntax(type.IsSat() ? 0 : 1)(Fmter, packedSfx, args[0], argA, argB);
    /*fmt::format_to(std::back_inserter(ret), type.IsSat() ? U"({}, {}, {})"sv : U"({1}, {2}) + {0})"sv,
        args[0], argA, argB, usePacked);
    return ret;*/
}


NLCLDp4aIntel::NLCLDp4aIntel(NLCLContext& context, const bool supportDp4a) :
    NLCLDp4aPlain(context), SupportDp4a(supportDp4a)
{ }

// https://github.com/intel/intel-graphics-compiler/blob/master/IGC/BiFModule/Implementation/IGCBiF_Intrinsics.cl
ReplaceResult NLCLDp4aIntel::DP4A(const Dp4aType type, const common::span<const std::u32string_view> args)
{
    if (!SupportDp4a)
        return NLCLDp4aPlain::DP4A(type, args);
    if (type.IsSat()) 
        return DP4ASat(type, args);
    
    const auto funcName = GetSignednessFixedData(dp4a_intel)(type);
    auto dep = Context.AddPatchedBlock(funcName, [&]()
        {
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(type.GetSignedness());
            const auto sfx = GetSignSuffix(type.GetSignedness());
            const auto& syntax = FmtString(UR"(int __builtin_IB_dp4a_{0}(int c, int a, int b) __attribute__((const));
inline {2}int {1}({2}int acc, const {3}char4 a, const {4}char4 b)
{{
    return __builtin_IB_dp4a_{0}(as_int(acc), as_int(a), as_int(b));
}})"sv);
            return Fmter.FormatStatic(syntax, sfx, funcName, pfxC, pfxA, pfxB);
        });
    const auto [argA, argB] = GetDP4Arg(type, args, false);
    return { FMTSTR4(U"{}({}, {}, {})"sv, funcName, args[0], argA, argB), dep };
}


NLCLDp4aArm::NLCLDp4aArm(NLCLContext& context, const bool supportDP8, const bool supportDPA8, const bool supportDPA8S) :
    NLCLDp4aPlain(context), SupportDP8(supportDP8), SupportDPA8(supportDPA8), SupportDPA8S(supportDPA8S)
{ }

// https://www.khronos.org/registry/OpenCL/extensions/arm/cl_arm_integer_dot_product.txt
ReplaceResult NLCLDp4aArm::DP4A(const Dp4aType type, const common::span<const std::u32string_view> args)
{
    if (!SupportDPA8 && !SupportDP8)
        return NLCLDp4aPlain::DP4A(type, args);
    const auto signedness = type.GetSignedness();
    if (signedness == Signedness::US || signedness == Signedness::SU)
        return NLCLDp4aPlain::DP4A(type, args);
    if (type.IsSat() && !SupportDPA8S)
        return DP4ASat(type, args);

    const auto [argA, argB] = GetDP4Arg(type, args, false);
    if (type.IsSat()) // SupportDPA8S
    {
        EnableDPA8S = true;
        return FMTSTR4(U"arm_dot_acc_sat({}, {}, {})"sv, argA, argB, args[0]);
    }
    if (SupportDPA8)
    {
        EnableDPA8 = true;
        return FMTSTR4(U"arm_dot_acc({}, {}, {})"sv, argA, argB, args[0]);
    }
    else // SupportDP8
    {
        EnableDP8 = true;
        return FMTSTR4(U"(arm_dot({}, {}) + {})"sv, argA, argB, args[0]);
    }
}

void NLCLDp4aArm::OnFinish(NLCLRuntime& runtime)
{
    if (EnableDP8)
        runtime.EnableExtension("cl_arm_integer_dot_product_int8"sv);
    if (EnableDPA8)
        runtime.EnableExtension("cl_arm_integer_dot_product_accumulate_int8"sv);
    if (EnableDPA8S)
        runtime.EnableExtension("cl_arm_integer_dot_product_accumulate_saturate_int8"sv);
    NLCLDp4aPlain::OnFinish(runtime);
}


NLCLDp4aPtx::NLCLDp4aPtx(NLCLContext& context, const uint32_t smVersion) :
    NLCLDp4aPlain(context), SMVersion(smVersion)
{ }

// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#integer-arithmetic-instructions-dp4a
ReplaceResult NLCLDp4aPtx::DP4A(const Dp4aType type, const common::span<const std::u32string_view> args)
{
    if (SMVersion < 61)
        return NLCLDp4aPlain::DP4A(type, args);
    if (type.IsSat())
        return DP4ASat(type, args);

    const auto funcName = GetSignednessFixedData(dp4a_ptx)(type);
    auto dep = Context.AddPatchedBlock(funcName, [&]()
        {
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(type.GetSignedness());
            const char32_t typePfxA = pfxA == U' ' ? U's' : U'u';
            const char32_t typePfxB = pfxB == U' ' ? U's' : U'u';
            const auto& syntax = FmtString(UR"(inline {1}int {0}({1}int acc, const {2}int a, const {3}int b)
{{
    {1}int ret;
    asm volatile("dp4a.{4}32.{5}32 %0, %1, %2, %3;" : "=r"(ret) : "r"(a), "r"(b), "r"(acc));
    return ret;
}})"sv);
            return Fmter.FormatStatic(syntax, funcName, pfxC, pfxA, pfxB, typePfxA, typePfxB);
        });
    const auto [argA, argB] = GetDP4Arg(type, args, true);
    return { FMTSTR4(U"{}({}, {}, {})"sv, funcName, args[0], argA, argB), dep };
}


}