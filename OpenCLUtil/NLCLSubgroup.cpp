#include "oclPch.h"
#include "NLCLSubgroup.h"

namespace oclu
{
using namespace std::string_view_literals;
using namespace std::string_literals;
using xziar::nailang::ArgLimits;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;



#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)



NLCLSubgroup::NLCLSubgroup(NLCLRuntime* runtime, NLCLSubgroup::SubgroupCapbility cap) : Runtime(*runtime), Cap(cap) 
{ }

NLCLSubgroup::SubgroupCapbility NLCLSubgroup::GenerateCapabiity(NLCLRuntime* runtime, const SubgroupAttributes& attr)
{
    SubgroupCapbility cap;
    cap.SubgroupSize            = attr.SubgroupSize;
    cap.SupportSubgroupKHR      = runtime->SupportSubgroupKHR;
    cap.SupportSubgroupIntel    = runtime->SupportSubgroupIntel;
    cap.SupportSubgroup8Intel   = runtime->SupportSubgroup8Intel;
    cap.SupportSubgroup16Intel  = runtime->SupportSubgroup16Intel;
    cap.SupportFP16             = runtime->SupportFP16;
    cap.SupportFP64             = runtime->SupportFP64;
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

void NLCLSubgroup::ThrowByReplacerArgCount(const std::u32string_view func, const common::span<const std::u32string_view> args, const size_t count, const xziar::nailang::ArgLimits limit) const
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
    Runtime.NLRT_THROW_EX(fmt::format(FMT_STRING(u"Repalcer-Func [{}] requires {} [{}] args, which gives [{}]."), func, prefix, count, args.size()));
}

void NLCLSubgroup::HandleException(const xziar::nailang::NailangRuntimeException& ex) const
{
    Runtime.HandleException(ex);
}

void NLCLSubgroup::AddArg([[maybe_unused]] std::u32string id, [[maybe_unused]] std::u32string content)
{
}

void NLCLSubgroup::AddInject(std::u32string id, std::u32string content)
{
    Injects.insert_or_assign(std::move(id), std::move(content));
}

std::u32string NLCLSubgroup::ScalarPatch(const std::u32string_view funcName, const std::u32string_view base, VecDataInfo vtype) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    constexpr char32_t idxNames[] = U"0123456789abcdef";
    const auto vecName = Runtime.GetVecTypeName(vtype);
    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{\r\n    {0} ret;"sv),
        vecName, funcName);
    for (uint8_t i = 0; i < vtype.Dim0; ++i)
        APPEND_FMT(func, U"\r\n    ret.s{0} = {1}(val.s{0}, sgId);"sv, idxNames[i], base);
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

forceinline constexpr static bool CheckVNum(const uint32_t num) noexcept
{
    switch (num)
    {
    case 1:case 2: case 4: case 8: case 16: return true;
    default: return false;
    }
}
std::optional<VecDataInfo> NLCLSubgroup::DecideVtype(VecDataInfo vtype, common::span<const VecDataInfo> scalars) noexcept
{
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    for (const auto scalar : scalars)
    {
        if (totalBits % scalar.Bit != 0) continue;
        if (const auto num = totalBits / scalar.Bit; CheckVNum(num))
            return VecDataInfo{ scalar.Type, scalar.Bit, static_cast<uint8_t>(num), 0 };
    }
    return {};
}

std::u32string NLCLSubgroup::ReplaceFunc(const std::u32string_view func, const common::span<const std::u32string_view> args, [[maybe_unused]] void* cookie)
{
    switch (hash_(func))
    {
    HashCase(func, U"SubgroupBroadcast")
    {
        ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
        EnableSubgroup = true;
        if (const auto info = Runtime.ParseVecType(args[0]); info.has_value())
        {
            auto ret = SubgroupBroadcast(info.value(), args[1], args[2]);
            if (ret.empty())
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"SubgroupBroadcast with Type [{}] not supported"sv), args[0]));
            return ret;
        }
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not recognized as vtype"sv), args[0]));
        return {};
    }
    HashCase(func, U"SubgroupShuffle")
    {
        ThrowByReplacerArgCount(func, args, 3, ArgLimits::Exact);
        EnableSubgroup = true;
        if (const auto info = Runtime.ParseVecType(args[0]); info.has_value())
        {
            auto ret = SubgroupShuffle(info.value(), args[1], args[2]);
            if (ret.empty())
                NLRT_THROW_EX(fmt::format(FMT_STRING(u"SubgroupShuffle with Type [{}] not supported"sv), args[0]));
            return ret;
        }
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Type [{}] not recognized as vtype"sv), args[0]));
        return {};
    }
    HashCase(func, U"GetSubgroupLocalId")
    {
        ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        EnableSubgroup = true;
        auto ret = GetSubgroupLocalId();
        if (ret.empty())
            NLRT_THROW_EX(u"[GetSubgroupLocalId] not supported"sv);
        return ret;
    }
    HashCase(func, U"GetSubgroupSize")
    {
        ThrowByReplacerArgCount(func, args, 0, ArgLimits::Exact);
        EnableSubgroup = true;
        auto ret = GetSubgroupSize();
        if (ret.empty())
            NLRT_THROW_EX(u"[GetSubgroupSize] not supported"sv);
        return ret;
    }
    default: break;
    }
    return {};
}

void NLCLSubgroup::Finish(void* cookie)
{
    OnFinish();
    const auto kerCk = BlockCookie::Get<KernelCookie>(cookie);
    if (kerCk)
        kerCk->Injects.merge(Injects);
}


struct BcastShuf
{
    std::u32string_view Func;
    std::u32string_view Extra;
    std::u32string_view Element;
    std::u32string_view Index;
    std::optional<std::pair<VecDataInfo, VecDataInfo>> TypeCast;

    constexpr BcastShuf(const std::u32string_view func, const std::u32string_view ele, const std::u32string_view idx) noexcept :
        Func(func), Element(ele), Index(idx) { }
    constexpr BcastShuf(const std::u32string_view func, const std::u32string_view ele, const std::u32string_view idx,
        const VecDataInfo dst, const VecDataInfo mid) noexcept :
        Func(func), Element(ele), Index(idx), TypeCast({ dst, mid }) { }
    constexpr BcastShuf(const std::u32string_view func, const std::u32string_view extra, 
        const std::u32string_view ele, const std::u32string_view idx) noexcept :
        Func(func), Extra(extra), Element(ele), Index(idx) { }
    constexpr BcastShuf(const std::u32string_view func, const std::u32string_view extra, 
        const std::u32string_view ele, const std::u32string_view idx, const VecDataInfo dst, const VecDataInfo mid) noexcept :
        Func(func), Extra(extra), Element(ele), Index(idx), TypeCast({ dst, mid }) { }

    operator std::u32string() const
    {
        if (TypeCast)
        {
            const auto [dst, mid] = TypeCast.value();
            if (dst != mid)
                return fmt::format(FMT_STRING(U"as_{}({}({}as_{}({}), {}))"sv),
                    NLCLRuntime::GetVecTypeName(dst), Func, Extra, NLCLRuntime::GetVecTypeName(mid), Element, Index);
        }
        return fmt::format(FMT_STRING(U"{}({}{}, {})"sv), Func, Extra, Element, Index);
    }
};
#define RetDirect(func)               return BcastShuf(func, ele, idx)
#define RetCastT(func, mid)           return BcastShuf(func, ele, idx, vtype, mid);
#define RetCast(func, type, bit, dim) return BcastShuf(func, ele, idx, vtype, { type, bit, static_cast<uint8_t>(dim), 0 });
#define RetXDirect(func, extra)       return BcastShuf(func, extra, ele, idx)
#define RetXCastT(func, extra, mid)   return BcastShuf(func, extra, ele, idx, vtype, mid);

void NLCLSubgroupKHR::OnFinish()
{
    if (EnableSubgroup)
        Runtime.EnableExtension("cl_khr_subgroups"sv);
    if (EnableFP16)
        Runtime.EnableExtension("cl_khr_fp16"sv);
    if (EnableFP64)
        Runtime.EnableExtension("cl_khr_fp64"sv) || Runtime.EnableExtension("cl_amd_fp64"sv);
}

std::u32string NLCLSubgroupKHR::GetSubgroupLocalId()
{
    return U"get_sub_group_local_id()";
}
std::u32string NLCLSubgroupKHR::GetSubgroupSize()
{
    return U"get_sub_group_size()";
}
std::u32string NLCLSubgroupKHR::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
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
            RetDirect(func);
        }
        RetCast(func, VecDataInfo::DataTypes::Unsigned, 64, 1);
    case 32:
        RetCast(func, vtype.Type, 32, 1);
    case 16:
        if (Cap.SupportFP16) // only when FP16 is supported
        {
            EnableFP16 = true;
            RetCast(func, VecDataInfo::DataTypes::Float, 16, 1);
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
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_broadcast_{}"sv), totalBits);
        AddPatchedBlock(*this, funcName, &NLCLSubgroupKHR::ScalarPatch, funcName, U"sub_group_broadcast"sv, mid.value());
        RetCastT(funcName, *mid);
    }
    return {};
}
std::u32string NLCLSubgroupKHR::SubgroupShuffle(VecDataInfo, const std::u32string_view, const std::u32string_view)
{
    return {};
}


// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_short.html
// https://www.khronos.org/registry/OpenCL/extensions/intel/cl_intel_subgroups_char.html

std::u32string NLCLSubgroupIntel::VectorPatch(const std::u32string_view funcName, const std::u32string_view base, common::simd::VecDataInfo vtype, common::simd::VecDataInfo mid) noexcept
{
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    Expects(vtype.Bit == mid.Bit * mid.Dim0);
    constexpr char32_t idxNames[] = U"0123456789abcdef";
    const auto vecName = NLCLRuntime::GetVecTypeName(vtype);
    const auto scalarName = NLCLRuntime::GetVecTypeName({ vtype.Type,vtype.Bit,1,0 });
    const auto midName = NLCLRuntime::GetVecTypeName(mid);
    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{\r\n    {0} ret;"sv),
        vecName, funcName);
    for (uint8_t i = 0; i < vtype.Dim0; ++i)
        APPEND_FMT(func, U"\r\n    ret.s{0} = as_{1}({2}(as_{3}(val.s{0}), sgId));"sv,
            idxNames[i], scalarName, base, midName);
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

std::u32string NLCLSubgroupIntel::DirectShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
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
                RetDirect(func);
            }
            else
                RetCast(func, vtype.Type == VecDataInfo::DataTypes::Float ? VecDataInfo::DataTypes::Unsigned : vtype.Type, 64, 1);
        }
        break;
    case 32: // all vector supported
        RetDirect(func);
    case 16:
        if ((vtype.Dim0 != 1 || vtype.Type != VecDataInfo::DataTypes::Float) && Cap.SupportSubgroup16Intel)
        {
            // vector need intel16, scalar int prefer intel16
            EnableSubgroup16Intel = true;
            if (vtype.Type != VecDataInfo::DataTypes::Float)
                RetDirect(func);
            else
                RetCast(func, VecDataInfo::DataTypes::Unsigned, 16, vtype.Dim0);
        }
        if (vtype.Dim0 == 1 && Cap.SupportFP16)
        {
            EnableFP16 = true;
            RetCast(func, VecDataInfo::DataTypes::Float, 16, 1);
        }
        break;
    case 8:
        if (Cap.SupportSubgroup8Intel)
        {
            EnableSubgroup8Intel = true;
            const auto newType = vtype.Type == VecDataInfo::DataTypes::Float ? VecDataInfo::DataTypes::Unsigned : vtype.Type;
            RetCast(func, newType, 8, vtype.Dim0);
        }
        break;
    default:
        break;
    }

    // try match smaller datatype
    std::vector<VecDataInfo> supportSmaller =
    {
        { VecDataInfo::DataTypes::Unsigned, 64u, 16u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 16u, 0u },
    };
    if (Cap.SupportFP16)
        supportSmaller.push_back({ VecDataInfo::DataTypes::Float, 16u, 1u, 0u });
    if (Cap.SupportSubgroup16Intel)
        supportSmaller.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 16u, 0u });
    if (Cap.SupportSubgroup8Intel)
        supportSmaller.push_back({ VecDataInfo::DataTypes::Unsigned, 8u, 16u, 0u });
    
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    for (const auto& mid : supportSmaller)
    {
        if (vtype.Bit <= mid.Bit /*|| vtype.Bit % mid.Bit != 0*/)
            continue;
        const auto vnum = totalBits / mid.Bit;
        if (vnum > mid.Dim0 || !CheckVNum(vnum))
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
        RetCast(func, mid.Type, mid.Bit, vnum);
    }
    return {};
}

void NLCLSubgroupIntel::OnFinish()
{
    if (EnableSubgroupIntel)
        Runtime.EnableExtension("cl_intel_subgroups"sv);
    if (EnableSubgroup16Intel)
        Runtime.EnableExtension("cl_intel_subgroups_short"sv);
    if (EnableSubgroup8Intel)
        Runtime.EnableExtension("cl_intel_subgroups_char"sv); 
    if (EnableSubgroup)
        EnableSubgroup = !Runtime.EnableExtension("cl_intel_subgroups"sv);
    NLCLSubgroupKHR::OnFinish();
}

std::u32string NLCLSubgroupIntel::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
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
                EnableFP64 = true;
                RetDirect(bcKHR);
            }
            RetCast(bcKHR, VecDataInfo::DataTypes::Unsigned, 64, 1);
        case 32: // all supported
            RetDirect(bcKHR);
        case 16:
            if (vtype.Type == VecDataInfo::DataTypes::Float && Cap.SupportFP16)
            {
                EnableFP16 = true;
                RetDirect(bcKHR);
            }
            if (vtype.Type != VecDataInfo::DataTypes::Float && Cap.SupportSubgroup16Intel)
            {
                EnableSubgroup16Intel = true;
                RetDirect(bcIntel);
            }
            if (Cap.SupportFP16)
            {
                EnableFP16 = true;
                RetCast(bcKHR, VecDataInfo::DataTypes::Float, 16, 1);
            }
            if (Cap.SupportSubgroup16Intel)
            {
                EnableSubgroup16Intel = true;
                RetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 16, 1);
            }
            break;
        case 8:
            if (Cap.SupportSubgroup8Intel)
            {
                EnableSubgroup8Intel = true;
                RetDirect(bcIntel);
            }
            break;
        default:
            break;
        }
    }
    
    // fallback to subgroup_shuffle due to reg layout
    if (auto ret = DirectShuffle(vtype, ele, idx); !ret.empty())
        return ret;
    
    // fallback to reinterpret to avoid patch
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    switch (totalBits)
    {
    case 64:
        // no need to consider FP64
        RetCast(bcKHR, VecDataInfo::DataTypes::Unsigned, 64, 1);
    case 32:
        RetCast(bcKHR, vtype.Type, 32, 1);
    case 16:
        if (Cap.SupportFP16)
        {
            EnableFP16 = true;
            RetCast(bcKHR, VecDataInfo::DataTypes::Float, 16, 1);
        }
        if (Cap.SupportSubgroup16Intel)
        {
            EnableSubgroup16Intel = true;
            RetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 16, 1);
        }
        break;
    case 8:
        if (Cap.SupportSubgroup8Intel)
        {
            EnableSubgroup8Intel = true;
            RetCast(bcIntel, VecDataInfo::DataTypes::Unsigned, 8, 1);
        }
        break;
    default:
        break;
    }

    // need patch
    std::vector<VecDataInfo> supportScalars = 
    {
        { VecDataInfo::DataTypes::Unsigned, 64u, 1u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 1u, 0u },
    };
    if (Cap.SupportFP16)
        supportScalars.push_back({ VecDataInfo::DataTypes::Float, 16u, 1u, 0u });
    else if (Cap.SupportSubgroup16Intel)
        supportScalars.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 1u, 0u });
    if (Cap.SupportSubgroup8Intel)
        supportScalars.push_back({ VecDataInfo::DataTypes::Unsigned, 8u, 1u, 0u });

    if (const auto mid = DecideVtype(vtype, supportScalars); mid.has_value())
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
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_broadcast{}_{}"sv), useIntel? U"_intel"sv : U""sv, totalBits);
        AddPatchedBlock(*this, funcName, &NLCLSubgroupIntel::ScalarPatch, funcName, useIntel ? bcIntel : bcKHR, mid.value());
        RetCastT(funcName, *mid);
    }
    return {};
}

std::u32string NLCLSubgroupIntel::SubgroupShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    constexpr auto func = U"intel_sub_group_shuffle"sv;
    // fallback to subgroup_shuffle to due to reg layout
    if (auto ret = DirectShuffle(vtype, ele, idx); !ret.empty())
        return ret;

    // need patch, prefer smaller type due to reg layout
    std::vector<VecDataInfo> supportSmaller =
    {
        { VecDataInfo::DataTypes::Unsigned, 32u, 1u, 0u },
    };
    if (Cap.SupportSubgroup16Intel)
        supportSmaller.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 1u, 0u });
    if (Cap.SupportSubgroup8Intel)
        supportSmaller.push_back({ VecDataInfo::DataTypes::Unsigned, 8u, 1u, 0u });

    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const VecDataInfo newType{ VecDataInfo::DataTypes::Unsigned, vtype.Bit, vtype.Dim0, 0u };
    for (const auto& mid : supportSmaller)
    {
        if (vtype.Bit <= mid.Bit /*|| vtype.Bit % mid.Bit != 0*/)
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
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_shuffle_intel_{}"sv), totalBits);
        AddPatchedBlock(*this, funcName, &NLCLSubgroupIntel::VectorPatch, funcName, func, newType, mid);
        RetCastT(funcName, newType);
    }

    // need patch, ignore reg size
    std::vector<VecDataInfo> supportScalars =
    {
        { VecDataInfo::DataTypes::Unsigned, 64u, 1u, 0u },
        { VecDataInfo::DataTypes::Unsigned, 32u, 1u, 0u },
    };
    if (Cap.SupportFP16)
        supportScalars.push_back({ VecDataInfo::DataTypes::Float, 16u, 1u, 0u });
    else if (Cap.SupportSubgroup16Intel)
        supportScalars.push_back({ VecDataInfo::DataTypes::Unsigned, 16u, 1u, 0u });
    if (Cap.SupportSubgroup8Intel)
        supportScalars.push_back({ VecDataInfo::DataTypes::Unsigned, 8u, 1u, 0u });
    
    if (const auto mid = DecideVtype(vtype, supportScalars); mid.has_value())
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
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_shuffle_intel_{}"sv), totalBits);
        AddPatchedBlock(*this, funcName, &NLCLSubgroupIntel::ScalarPatch, funcName, func, mid.value());
        RetCastT(funcName, *mid);
    }

    return {};
}

void NLCLSubgroupLocal::OnFinish()
{
    if (NeedLocalTemp)
    {
        AddInject(U"oclu_subgroup_mimic_local",
            fmt::format(FMT_STRING(U"    local ulong _oclu_subgroup_local[{}];\r\n"sv), std::max<uint32_t>(Cap.SubgroupSize, 16)));
        Logger().debug(u"Subgroup mimic [local] is enabled with size [{}].\n", Cap.SubgroupSize);
    }
}

std::u32string NLCLSubgroupLocal::BroadcastPatch(const std::u32string_view funcName, const VecDataInfo vtype) noexcept
{
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16);
    const auto vecName = NLCLRuntime::GetVecTypeName(vtype);
    const auto scalarName = NLCLRuntime::GetVecTypeName({ vtype.Type, vtype.Bit, 1, 0 });
    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)"sv),
        vecName, funcName);
    func.append(UR"(
{
    barrier(CLK_LOCAL_MEM_FENCE);
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
)"sv);
    APPEND_FMT(func, U"    local {0}* restrict ptr = (local {0}*)(tmp);\r\n"sv, scalarName);

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
    const auto vecName = NLCLRuntime::GetVecTypeName(vtype);
    const auto scalarName = NLCLRuntime::GetVecTypeName({ vtype.Type, vtype.Bit, 1, 0 });
    constexpr char32_t idxNames[] = U"0123456789abcdef";
    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(local ulong* tmp, const {0} val, const uint sgId)"sv),
        vecName, funcName);
    func.append(UR"(
{
    barrier(CLK_LOCAL_MEM_FENCE);
    const uint lid = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
)"sv);
    APPEND_FMT(func, U"    local {0}* restrict ptr = (local {0}*)(tmp);\r\n"sv, scalarName);

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
        APPEND_FMT(func, U"\r\n    {0} ret;"sv, vecName); 
        static constexpr auto temp = UR"(
    ptr[lid] = val.s{0};
    barrier(CLK_LOCAL_MEM_FENCE);
    ret.s{0} = ptr[sgId];)"sv;
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
            APPEND_FMT(func, temp, idxNames[i]);
        func.append(U"\r\n    return ret;\r\n"sv);
    }

    func.append(U"}"sv);
    return func;
}

std::u32string NLCLSubgroupLocal::GetSubgroupLocalId()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupLocalId();
    return U"(get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0))";
}

std::u32string NLCLSubgroupLocal::GetSubgroupSize()
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::GetSubgroupSize();
    return fmt::format(FMT_STRING(U"(/*local*/{})"sv), Cap.SubgroupSize);
}

std::u32string NLCLSubgroupLocal::SubgroupBroadcast(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupBroadcast(vtype, ele, idx);
    
    NeedLocalTemp = true;
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_local_broadcast_{}"sv), totalBits);
    for (const auto targetBits : std::array<uint8_t, 4>{ 64u, 32u, 16u, 8u })
    {
        const auto vnum = totalBits / targetBits;
        if (!CheckVNum(vnum)) continue;
        if (vnum == 0) continue;
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, targetBits, gsl::narrow_cast<uint8_t>(vnum), 0 };
        AddPatchedBlock(*this, funcName, &NLCLSubgroupLocal::BroadcastPatch, funcName, mid);
        RetXCastT(funcName, U"_oclu_subgroup_local, "sv, mid);
    }
    return {};
}

std::u32string NLCLSubgroupLocal::SubgroupShuffle(VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    if (Cap.SupportBasicSubgroup)
        return NLCLSubgroupKHR::SubgroupShuffle(vtype, ele, idx);
    
    NeedLocalTemp = true;
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_local_shuffle_{}"sv), totalBits);
    for (const auto targetBits : std::array<uint8_t, 4>{ 64u, 32u, 16u, 8u })
    {
        const auto vnum = totalBits / targetBits;
        if (!CheckVNum(vnum)) continue;
        if (vnum == 0) continue;
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, targetBits, gsl::narrow_cast<uint8_t>(vnum), 0 };
        AddPatchedBlock(*this, funcName, &NLCLSubgroupLocal::ShufflePatch, funcName, mid);
        RetXCastT(funcName, U"_oclu_subgroup_local, "sv, mid);
    }
    return {};
}


// https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#data-movement-and-conversion-instructions-shfl

NLCLSubgroupPtx::NLCLSubgroupPtx(NLCLRuntime* runtime, SubgroupCapbility cap, const uint32_t smVersion) :
    NLCLSubgroup(runtime, cap)
{
    if (smVersion == 30)
        Logger().warning(u"Trying to use PtxSubgroup on non-NV platform.\n");
    else if (smVersion < 30)
        Logger().warning(u"Trying to use PtxSubgroup on sm{}, shuf is introduced since sm30.\n", smVersion);
    if (smVersion < 60)
        ShufFunc = U"shfl"sv;
    else
        ShufFunc = U"shfl.sync"sv, ExtraMask = U", 0x1f"sv;
}

std::u32string NLCLSubgroupPtx::Shuffle64Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept
{
    Expects(vtype.Bit == 64 && vtype.Type == VecDataInfo::DataTypes::Unsigned);
    Expects(vtype.Dim0 > 1 && vtype.Dim0 <= 16);
    const auto vecName = NLCLRuntime::GetVecTypeName(vtype);
    constexpr char32_t idxNames[] = U"0123456789abcdef";

    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{\r\n    {0} ret;"sv),
        vecName, funcName);
    for (uint8_t i = 0; i < vtype.Dim0; ++i)
    {
        static constexpr auto temp = UR"(
    const uint2 val_{3} = as_uint2(val.s{2});
    uint2 ret_{3};
    asm volatile("{0}.idx.b32 %0, %2, %4, 0x1f{1}; {0}.idx.b32 %1, %3, %4, 0x1f{1};" 
        : "=r"(ret_{3}.s0), "=r"(ret_{3}.s1) : "r"(val_{3}.s0), "r"(val_{3}.s1), "r"(sgId));
    ret.s{2} = as_ulong(ret_{3});
    )"sv;
        APPEND_FMT(func, temp, ShufFunc, ExtraMask, idxNames[i], i);
    }
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

std::u32string NLCLSubgroupPtx::Shuffle32Patch(const std::u32string_view funcName, const common::simd::VecDataInfo vtype) noexcept
{
    Expects(vtype.Dim0 > 0 && vtype.Dim0 <= 16 && vtype.Bit == 32);
    const auto vecName = NLCLRuntime::GetVecTypeName(vtype);
    constexpr char32_t idxNames[] = U"0123456789abcdef";

    std::u32string func = fmt::format(FMT_STRING(U"inline {0} {1}(const {0} val, const uint sgId)\r\n{{\r\n    {0} ret;"sv),
        vecName, funcName);
    if (vtype.Dim0 == 1)
    {
        static constexpr auto temp = UR"(
    asm volatile("{0}.idx.b32 %0, %1, %2, 0x1f{1};" : "=r"(ret) : "r"(val), "r"(sgId));)"sv;
        APPEND_FMT(func, temp, ShufFunc, ExtraMask);
    }
    else
    {
        for (uint8_t i = 0; i < vtype.Dim0; ++i)
        {
            static constexpr auto temp = UR"(
    asm volatile("{0}.idx.b32 %0, %1, %2, 0x1f{1};" : "=r"(ret.s{2}) : "r"(val.s{2}), "r"(sgId));)"sv;
            APPEND_FMT(func, temp, ShufFunc, ExtraMask, idxNames[i]);
        }
    }
    func.append(U"\r\n    return ret;\r\n}"sv);
    return func;
}

std::u32string NLCLSubgroupPtx::GetSubgroupLocalId()
{
    AddPatchedBlock(U"oclu_subgroup_ptx_get_local_id()"sv, []() 
        { 
            return UR"(inline uint oclu_subgroup_ptx_get_local_id()
{
    uint ret;
    asm volatile("mov.u32 %0, %%laneid;" : "=r"(ret));
    return ret;
})"s; 
        });
    return U"oclu_subgroup_ptx_get_local_id()"s;
}

std::u32string NLCLSubgroupPtx::GetSubgroupSize()
{
    return fmt::format(FMT_STRING(U"(/*nv_warp*/{})"sv), std::min<uint32_t>(32, Cap.SubgroupSize));
}

std::u32string NLCLSubgroupPtx::SubgroupBroadcast(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    return SubgroupShuffle(vtype, ele, idx);
}

std::u32string NLCLSubgroupPtx::SubgroupShuffle(common::simd::VecDataInfo vtype, const std::u32string_view ele, const std::u32string_view idx)
{
    const uint32_t totalBits = vtype.Bit * vtype.Dim0;
    if (const auto vnum = totalBits / 32; CheckVNum(vnum))
    {
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_ptx_shuffle_{}"sv), totalBits);
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, 32u, static_cast<uint8_t>(vnum), 0u };
        AddPatchedBlock(*this, funcName, &NLCLSubgroupPtx::Shuffle32Patch, funcName, mid);
        RetCastT(funcName, mid);
    }
    if (const auto vnum = totalBits / 64; CheckVNum(vnum))
    {
        const auto funcName = fmt::format(FMT_STRING(U"oclu_subgroup_ptx_shuffle_{}"sv), totalBits);
        const VecDataInfo mid{ VecDataInfo::DataTypes::Unsigned, 64u, static_cast<uint8_t>(vnum), 0u };
        AddPatchedBlock(*this, funcName, &NLCLSubgroupPtx::Shuffle64Patch, funcName, mid);
        RetCastT(funcName, mid);
    }
    return {};
}


std::unique_ptr<NLCLSubgroup> NLCLSubgroup::Generate(NLCLRuntime* runtime, const SubgroupAttributes& attr)
{
    auto cap = GenerateCapabiity(runtime, attr);

    SubgroupAttributes::MimicType mType = attr.Mimic;
    if (mType == SubgroupAttributes::MimicType::Auto)
    {
        if (runtime->Device->PlatVendor == Vendors::NVIDIA)
            mType = SubgroupAttributes::MimicType::Ptx;
        else
            mType = SubgroupAttributes::MimicType::Local;
    }
    Ensures(mType != SubgroupAttributes::MimicType::Auto);

    if (cap.SupportSubgroupIntel)
        return std::make_unique<NLCLSubgroupIntel>(runtime, cap);
    else if (mType == SubgroupAttributes::MimicType::Ptx)
    {
        uint32_t smVer = 0;
        if (runtime->Device->Extensions.Has("cl_nv_device_attribute_query"sv))
        {
            uint32_t major = 0, minor = 0;
            clGetDeviceInfo(*runtime->Device, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(uint32_t), &major, nullptr);
            clGetDeviceInfo(*runtime->Device, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(uint32_t), &minor, nullptr);
            smVer = major * 10 + minor;
        }
        return std::make_unique<NLCLSubgroupPtx>(runtime, cap, smVer);
    }
    else if (mType == SubgroupAttributes::MimicType::Local)
        return std::make_unique<NLCLSubgroupLocal>(runtime, cap);
    else if (cap.SupportBasicSubgroup)
        return std::make_unique<NLCLSubgroupKHR>(runtime, cap);
    else
        return std::make_unique<NLCLSubgroup>(runtime, cap);
}

}