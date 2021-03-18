#include "oclPch.h"
#include "NLCLDp4a.h"

namespace oclu
{
using namespace std::string_view_literals;
using namespace std::string_literals;
using xcomp::ReplaceResult;
using xziar::nailang::ArgLimits;
using xziar::nailang::Arg;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::MetaEvalPack;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;
using common::str::Charset;



#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
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

void NLCLDp4aExtension::InstanceMeta(xcomp::XCNLExecutor& executor, const MetaEvalPack& meta, xcomp::InstanceContext&)
{
    if (meta.GetName() == U"oclu.Dp4aExt"sv)
    {
        executor.ThrowByParamTypes<2, ArgLimits::AtMost>(meta, { Arg::Type::String, Arg::Type::String });
        const auto mimic = meta.TryGet(0, &Arg::GetStr).Or({});
        const auto args  = meta.TryGet(1, &Arg::GetStr).Or({});
        Provider = Generate(mimic, args);
    }
}

constexpr auto SignednessParser = SWITCH_PACK(Hash,
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
        executor.ThrowByReplacerArgCount(func, args, 4, ArgLimits::Exact);
        if (const auto signedness = SignednessParser(args[0]); !signedness.has_value())
            executor.NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [Dp4a]'s arg[0] expects to a string of {{uu,us,su,ss}}, get [{}]", args[0]));
        else
            return HandleResult(Provider->DP4A(signedness.value(), args.subspan(1)),
                U"[Dp4a] not supported"sv);
    }
    return {};
}

std::optional<Arg> NLCLDp4aExtension::ConfigFunc(xcomp::XCNLExecutor& executor, FuncEvalPack& func)
{
    if (func.GetName() == U"oclu.IntelDp4a"sv)
    {
        executor.ThrowIfNotFuncTarget(func, xziar::nailang::FuncName::FuncInfo::Empty);
        executor.ThrowByParamTypes<1>(func, { Arg::Type::Boolable });
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

enum class MimicType { Auto, Plain, Intel, Arm, Ptx };
constexpr auto MimicParser = SWITCH_PACK(Hash,
    (U"auto",   MimicType::Auto),
    (U"plain",  MimicType::Plain),
    (U"intel",  MimicType::Intel),
    (U"arm",    MimicType::Arm),
    (U"ptx",    MimicType::Ptx));
std::shared_ptr<Dp4aProvider> NLCLDp4aExtension::Generate(std::u32string_view mimic, std::u32string_view args) const
{
    bool hasIntelDp4a = HasIntelDp4a;
    for (auto arg : common::str::SplitStream(common::str::to_string(args, Charset::ASCII), ',', false))
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
    const auto hasArmDPA8 = Context.Device->Extensions.Has("cl_arm_integer_dot_product_accumulate_int8"sv),
        hasArmDP8 = Context.Device->Extensions.Has("cl_arm_integer_dot_product_int8"sv);
    if (mType == MimicType::Auto)
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
    Ensures(mType != MimicType::Auto);

    if (mType == MimicType::Intel)
        return std::make_shared<NLCLDp4aIntel>(Context, hasIntelDp4a);
    else if (mType == MimicType::Arm)
        return std::make_shared<NLCLDp4aArm>(Context, hasArmDPA8, hasArmDP8);
    else if (mType == MimicType::Ptx)
    {
        uint32_t smVer = 0;
        if (Context.Device->Extensions.Has("cl_nv_device_attribute_query"sv))
        {
            uint32_t major = 0, minor = 0;
            clGetDeviceInfo(*Context.Device, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(uint32_t), &major, nullptr);
            clGetDeviceInfo(*Context.Device, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(uint32_t), &minor, nullptr);
            smVer = major * 10 + minor;
        }
        return std::make_shared<NLCLDp4aPtx>(Context, smVer);
    }
    else
        return std::make_shared<NLCLDp4aPlain>(Context);
}


Dp4aProvider::Dp4aProvider(NLCLContext& context) : Context(context)
{ }


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

#define SignednessFixedData(base, sign, sfx) case Signedness::sign:                 \
{                                                                                   \
    static constexpr auto name = U"oclu_" STRINGIZE(base) sfx;                      \
    static const std::function<std::shared_ptr<const xcomp::ReplaceDepend>()> dep = \
    []() { return xcomp::ReplaceDepend::CreateFrom(name); };                        \
    return {name, dep};                                                             \
}

#define GetSignednessFixedData(base)                                \
[](Signedness signedness) ->                                        \
std::pair<std::u32string_view,                                      \
    std::function<std::shared_ptr<const xcomp::ReplaceDepend>()>>   \
{                                                                   \
    switch (signedness)                                             \
    {                                                               \
    SignednessFixedData(base, UU, "_uu"sv)                          \
    SignednessFixedData(base, US, "_us"sv)                          \
    SignednessFixedData(base, SU, "_su"sv)                          \
    SignednessFixedData(base, SS, "_ss"sv)                          \
    default: assert(false); return {};                              \
    }                                                               \
}

ReplaceResult NLCLDp4aPlain::DP4A(Signedness signedness, const common::span<const std::u32string_view> args)
{
    const auto& [funcName, depend] = GetSignednessFixedData(dp4a)(signedness);
    Context.AddPatchedBlock(funcName, [&, fname = funcName]()
        {
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(signedness);
            std::u32string func = FMTSTR(U"inline {1}int {0}({1}int acc, const {2}char4 a, const {3}char4 b)",
                fname, pfxC, pfxA, pfxB);
            func.append(UR"(
{
    return acc + ((a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w));
})"sv);
            return func;
        });
    return { FMTSTR(U"{}({}, {}, {})"sv, funcName, args[0], args[1], args[2]), depend };
}


NLCLDp4aIntel::NLCLDp4aIntel(NLCLContext& context, const bool supportDp4a) :
    NLCLDp4aPlain(context), SupportDp4a(supportDp4a)
{ }

// https://github.com/intel/intel-graphics-compiler/blob/master/IGC/BiFModule/Implementation/IGCBiF_Intrinsics.cl
ReplaceResult NLCLDp4aIntel::DP4A(Signedness signedness, const common::span<const std::u32string_view> args)
{
    if (!SupportDp4a)
        return NLCLDp4aPlain::DP4A(signedness, args);

    const auto& [funcName, depend] = GetSignednessFixedData(dp4a_intel)(signedness);
    Context.AddPatchedBlock(funcName, [&, fname = funcName]()
        {
            const auto sfx = [&]()
            {
                switch (signedness)
                {
                case Dp4aProvider::Signedness::UU: return U"uu"sv;
                case Dp4aProvider::Signedness::US: return U"us"sv;
                case Dp4aProvider::Signedness::SU: return U"su"sv;
                case Dp4aProvider::Signedness::SS: return U"ss"sv;
                default: assert(false);            return U""sv;
                }
            }();
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(signedness);
            static constexpr auto syntax = UR"(int __builtin_IB_dp4a_{0}(int c, int a, int b) __attribute__((const));
inline {2}int {1}({2}int acc, const {3}char4 a, const {4}char4 b)
{{
    return __builtin_IB_dp4a_{0}(as_int(acc), as_int(a), as_int(b));
}})"sv;
            return FMTSTR(syntax, sfx, fname, pfxC, pfxA, pfxB);
        });
    return { FMTSTR(U"{}({}, {}, {})"sv, funcName, args[0], args[1], args[2]), depend };
}


NLCLDp4aArm::NLCLDp4aArm(NLCLContext& context, const bool supportDPA8, const bool supportDP8) :
    NLCLDp4aPlain(context), SupportDPA8(supportDPA8), SupportDP8(supportDP8)
{ }

// https://www.khronos.org/registry/OpenCL/extensions/arm/cl_arm_integer_dot_product.txt
ReplaceResult NLCLDp4aArm::DP4A(Signedness signedness, const common::span<const std::u32string_view> args)
{
    if (!SupportDPA8 && !SupportDP8)
        return NLCLDp4aPlain::DP4A(signedness, args);
    if (signedness == Signedness::US || signedness == Signedness::SU)
        return NLCLDp4aPlain::DP4A(signedness, args);
    
    if (SupportDPA8)
        return FMTSTR(U"arm_dot_acc({}, {}, {})"sv, args[1], args[2], args[3]);
    else // if (SupportDP8)
        return FMTSTR(U"({} + arm_dot({}, {}))"sv, args[0], args[1], args[2]);
}

void NLCLDp4aArm::OnFinish(NLCLRuntime& runtime)
{
    if (EnableDPA8)
        runtime.EnableExtension("cl_arm_integer_dot_product_accumulate_int8"sv);
    if (EnableDP8)
        runtime.EnableExtension("cl_arm_integer_dot_product_int8"sv);
    NLCLDp4aPlain::OnFinish(runtime);
}


NLCLDp4aPtx::NLCLDp4aPtx(NLCLContext& context, const uint32_t smVersion) :
    NLCLDp4aPlain(context), SMVersion(smVersion)
{ }

// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#integer-arithmetic-instructions-dp4a
ReplaceResult NLCLDp4aPtx::DP4A(Signedness signedness, const common::span<const std::u32string_view> args)
{
    if (SMVersion < 61)
        return NLCLDp4aPlain::DP4A(signedness, args);
    if (signedness == Signedness::US || signedness == Signedness::SU)
        return NLCLDp4aPlain::DP4A(signedness, args);

    const auto& [funcName, depend] = GetSignednessFixedData(dp4a_ptx)(signedness);
    Context.AddPatchedBlock(funcName, [&, fname = funcName]()
        {
            const auto [pfxC, pfxA, pfxB] = GetSignPrefix(signedness);
            static constexpr auto syntax = UR"(inline {1}int {0}({1}int acc, const {2}char4 a, const {3}char4 b)
{{
    {1}int ret; {2}int a_ = as_{2}int(a); {3}int b_ = as_{3}int(b);
    asm volatile("dp4a.{2}32.{3}32 %0, %1, %2, %3;" : "=r"(ret) : "r"(a_), "r"(b_), "r"(acc));
    return ret;
}})"sv;
            return FMTSTR(syntax, fname, pfxC, pfxA, pfxB);
        });
    return { FMTSTR(U"{}({}, {}, {})"sv, funcName, args[0], args[1], args[2]), depend };
}


}