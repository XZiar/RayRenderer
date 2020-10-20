#include "oclPch.h"
#include "oclKernelDebug.h"
#include "oclNLCLRely.h"
#include "oclException.h"
#include "XComputeBase/XCompDebugExt.h"


template<> OCLUAPI common::span<const xcomp::debug::WorkItemInfo::InfoField> xcomp::debug::WGInfoHelper::Fields<oclu::debug::SubgroupWgInfo>() noexcept
{
    using oclu::debug::SubgroupWgInfo;
    static const auto FIELDS = XCOMP_WGINFO_REG_INHERIT(SubgroupWgInfo, xcomp::debug::WorkItemInfo,
        SubgroupId, SubgroupLocalId);
    return FIELDS;
}

namespace oclu::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using xcomp::debug::ArgsLayout;
using xziar::nailang::NailangRuntimeException;
using common::simd::VecDataInfo;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


struct NonSubgroupInfoProvider final : public xcomp::debug::InfoProviderT<xcomp::debug::WorkItemInfo>
{
public:
    ~NonSubgroupInfoProvider() override {}
    void GetThreadInfo(xcomp::debug::WorkItemInfo& dst, common::span<const uint32_t> space, const uint32_t tid) const noexcept override
    {
        const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
        const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
        SetBasicInfo(gsize, lsize, tid, dst);
        const auto infodata = space[tid + 7];
        const auto lid = dst.LocalId[0] + dst.LocalId[1] * lsize[0] + dst.LocalId[2] * lsize[1] * lsize[0];
        Expects(infodata == lid);
    }
    std::unique_ptr<xcomp::debug::InfoPack> GetInfoPack(common::span<const uint32_t> space) const override
    {
        const auto count = GetBasicExeInfo(space).ThreadCount;
        return std::make_unique<xcomp::debug::InfoPackT<xcomp::debug::WorkItemInfo>>(*this, count);
    }
    ExecuteInfo GetExecuteInfo(common::span<const uint32_t> space) const noexcept override
    {
        return GetBasicExeInfo(space);
    }
    std::unique_ptr<xcomp::debug::WorkItemInfo> GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept override
    {
        auto info = std::make_unique<xcomp::debug::WorkItemInfo>();
        GetThreadInfo(*info, space, tid);
        return info;
    }
};
struct SubgroupInfoProvider final : public xcomp::debug::InfoProviderT<SubgroupWgInfo>
{
public:
    ~SubgroupInfoProvider() override {}
    void GetThreadInfo(xcomp::debug::WorkItemInfo& dst_, common::span<const uint32_t> space, const uint32_t tid) const noexcept override
    {
        auto& dst = static_cast<SubgroupWgInfo&>(dst_);
        const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
        const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
        SetBasicInfo(gsize, lsize, tid, dst);
        const auto infodata = space[tid + 7];
        dst.SubgroupId = static_cast<uint16_t>(infodata / 65536u);
        dst.SubgroupLocalId = static_cast<uint16_t>(infodata % 65536u);
    }
    std::unique_ptr<xcomp::debug::InfoPack> GetInfoPack(common::span<const uint32_t> space) const override
    {
        const auto count = GetBasicExeInfo(space).ThreadCount;
        return std::make_unique<xcomp::debug::InfoPackT<SubgroupWgInfo>>(*this, count);
    }
    ExecuteInfo GetExecuteInfo(common::span<const uint32_t> space) const noexcept override
    {
        return GetBasicExeInfo(space);
    }
    std::unique_ptr<xcomp::debug::WorkItemInfo> GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept override
    {
        auto info = std::make_unique<SubgroupWgInfo>();
        GetThreadInfo(*info, space, tid);
        return info;
    }
};



bool HasSubgroupInfo(const xcomp::debug::InfoProvider& infoProv) noexcept
{
    // fast path since `SubgroupInfoProvider` is final
    return typeid(infoProv) == typeid(SubgroupInfoProvider);
    // return dynamic_cast<const SubgroupInfoProvider*>(&infoProv) != nullptr;
}


struct NLCLDebugExtension;
struct Runtime_ : public xcomp::XCNLRuntime
{
    friend NLCLDebugExtension;
};
struct NLCLRuntime_ : public NLCLRuntime
{
    friend NLCLDebugExtension;
};

#define NLRT_THROW_EX(...) Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
struct NLCLDebugExtension : public NLCLExtension, public xcomp::debug::XCNLDebugExt
{
    // For whole program
    bool EnableDebug = false, AnyDebug = false;
    // For kernel
    bool AllowDebug = false, HasDebug = false;
    NLCLDebugExtension(NLCLContext& context) : NLCLExtension(context) { }
    ~NLCLDebugExtension() override { }

    void FinishXCNL(xcomp::XCNLRuntime& runtime) override
    {
        Expects(dynamic_cast<NLCLRuntime*>(&runtime) != nullptr);
        auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
        if (EnableDebug && AnyDebug)
        {
            Runtime.EnableExtension("cl_khr_global_int32_base_atomics"sv, u"Use oclu-debugoutput"sv);
            Runtime.EnableExtension("cl_khr_byte_addressable_store"sv, u"Use oclu-debugoutput"sv);
        }
        if (Context.SupportBasicSubgroup)
            SetInfoProvider(std::make_unique<SubgroupInfoProvider>());
        else
            SetInfoProvider(std::make_unique<NonSubgroupInfoProvider>());
    }

    void BeginInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext&) override
    {
        AllowDebug = EnableDebug;
    }
    
    void FinishInstance(xcomp::XCNLRuntime&, xcomp::InstanceContext& ctx) override
    {
        Expects(dynamic_cast<KernelContext*>(&ctx) != nullptr);
        auto& kerCtx = static_cast<KernelContext&>(ctx);
        if (HasDebug)
        {
            AnyDebug = true;
            kerCtx.Args.HasDebug = true;
            kerCtx.AddTailArg(KerArgType::Simple, KerArgSpace::Private, ImgAccess::None, KerArgFlag::Const,
                "_oclu_debug_buffer_size", "uint");
            kerCtx.AddTailArg(KerArgType::Buffer, KerArgSpace::Global, ImgAccess::None, KerArgFlag::Restrict,
                "_oclu_debug_buffer_info", "uint*");
            kerCtx.AddTailArg(KerArgType::Buffer, KerArgSpace::Global, ImgAccess::None, KerArgFlag::Restrict,
                "_oclu_debug_buffer_data", "uint*"); 
            AppendDebugTid();
            Context.AddPatchedBlockD(U"oclu_debuginfo"sv, Depend_DebugTid, &NLCLDebugExtension::DebugInfoSetter);
            kerCtx.AddBodyPrefix(U"oclu_debug", UR"(
    oclu_debuginfo(_oclu_debug_buffer_size, _oclu_debug_buffer_info);
)");
        }
    }

    xcomp::ReplaceResult ReplaceFunc(xcomp::XCNLRuntime& runtime, std::u32string_view func, const common::span<const std::u32string_view> args) override
    {
        auto& Runtime = static_cast<Runtime_&>(runtime);
        using namespace xziar::nailang;
        if (func == U"oclu.DebugString"sv || func == U"xcomp.DebugString"sv)
        {
            Runtime.ThrowByReplacerArgCount(func, args, 1, ArgLimits::AtLeast);
            if (!AllowDebug)
                return U"do{} while(false)"sv;

            const auto id = args[0];
            const auto info = common::container::FindInMap(DebugInfos, id);
            if (info)
            {
                Runtime.ThrowByReplacerArgCount(func, args, info->second.size() + 1, ArgLimits::Exact);
                return GenerateDebugFunc(Runtime, id, *info, args.subspan(1));
            }
            NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugString] reference to unregisted info [{}].", id));
        }
        else if (func == U"oclu.DebugStr"sv || func == U"xcomp.DebugStr"sv)
        {
            if (!AllowDebug)
                return U"do{} while(false)"sv;
            const auto& content = DefineMessage(runtime, func, args);
            std::vector<std::u32string_view> vals;
            const auto argCnt = args.size() / 2 - 1;
            vals.reserve(argCnt);
            for (size_t i = 3; i < args.size(); i += 2)
            {
                vals.push_back(args[i]);
            }
            return GenerateDebugFunc(Runtime, args[0], content, vals);
        }
        return {};
    }

    [[nodiscard]] std::optional<xziar::nailang::Arg> XCNLFunc(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall>) override
    {
        auto& Runtime = static_cast<Runtime_&>(runtime);
        const std::u32string_view name = *call.Name;
        using namespace xziar::nailang;
        if (name == U"oclu.EnableDebug" || name == U"xcomp.EnableDebug")
        {
            Runtime.ThrowIfNotFuncTarget(call, xziar::nailang::FuncName::FuncInfo::Empty);
            Runtime.Logger.verbose(u"Manually enable debug.\n");
            EnableDebug = true;
            return Arg{};
        }
        else if (name == U"oclu.DefineDebugString" || name == U"xcomp.DefineDebugString")
        {
            DefineMessage(runtime, call);
            return Arg{};
        }
        return {};
    }

    void InstanceMeta(xcomp::XCNLRuntime& runtime, const xziar::nailang::FuncCall& meta, xcomp::InstanceContext& ctx)
    {
        Expects(dynamic_cast<KernelContext*>(&ctx) != nullptr);
        auto& Runtime = static_cast<Runtime_&>(runtime);
        using namespace xziar::nailang;
        if (*meta.Name == U"oclu.DebugOutput"sv || *meta.Name == U"xcomp.DebugOutput"sv)
        {
            if (AllowDebug)
            {
                auto& kerCtx = static_cast<KernelContext&>(ctx);
                const auto size = Runtime.EvaluateFuncArgs<1, ArgLimits::AtMost>(meta, { Arg::Type::Integer })[0].GetUint();
                kerCtx.SetDebugBuffer(gsl::narrow_cast<uint32_t>(size.value_or(512)));
            }
            else
                Runtime.Logger.info(u"DebugOutput is disabled and ignored.\n");
        }
    }
    
    static std::u32string DebugTid() noexcept
    {
        return UR"(
inline uint oclu_debugtid()
{
    return get_global_id(0) + get_global_id(1) * get_global_size(0) + get_global_id(2) * get_global_size(1) * get_global_size(0);
}
)"s;
    }
    static constexpr auto Depend_DebugTid = std::array{ U"oclu_debugtid"sv };
    void AppendDebugTid()
    {
        Context.AddPatchedBlock(U"oclu_debugtid"sv, &NLCLDebugExtension::DebugTid);
    }

    static std::u32string DebugStringBase() noexcept
    {
        return UR"(
inline global uint* oclu_debug(const uint dbgId, const uint dbgSize, const uint total,
    global uint* restrict counter, global uint* restrict data)
{
    const uint size = dbgSize + 1;
    if (total == 0) 
        return 0;
    if (counter[0] + size > total) 
        return 0;
    const uint old = atom_add(counter, size);
    if (old >= total)
        return 0;
    const uint tid = oclu_debugtid();
    const uint uid  = tid & 0x00ffffffu;
    if (old + size > total) 
    { 
        data[old] = uid; 
        return 0;
    }
    const uint dId  = (dbgId + 1) << 24;
    data[old] = uid | dId;
    return data + old + 1;
}
)"s;
    }
    static constexpr auto Depend_DebugBase = std::array{ U"oclu_debug"sv };
    void AppendDebugBase()
    {
        Context.AddPatchedBlock(U"oclu_debug"sv, &NLCLDebugExtension::DebugStringBase);
    }

    static std::u32string DebugInfoSetter() noexcept
    {
        return UR"(
inline void oclu_debuginfo(const uint total, global uint* restrict info)
{
    if (total > 0)
    {
        const uint tid = oclu_debugtid();
        if (tid == 0)
        {
            info[1] = get_global_size(0);
            info[2] = get_global_size(1);
            info[3] = get_global_size(2);
            info[4] = get_local_size(0);
            info[5] = get_local_size(1);
            info[6] = get_local_size(2);
        }

#if defined(cl_khr_subgroups) || defined(cl_intel_subgroups)
        const ushort sgid  = get_sub_group_id();
        const ushort sglid = get_sub_group_local_id();
        const uint sginfo  = sgid * 65536u + sglid;
#else
        const uint sginfo  = get_local_id(0) + get_local_id(1) * get_local_size(0) + get_local_id(2) * get_local_size(1) * get_local_size(0);
#endif
        info[7 + tid] = sginfo;
    }
}
)"s;
    }

    std::u32string DebugStringPatch(Runtime_& runtime, const std::u32string_view dbgId,
        const std::u32string_view formatter, common::span<const xcomp::debug::NamedVecPair> args) noexcept
    {
        // prepare arg layout
        const auto& dbgBlock = AppendBlock(dbgId, formatter, args);
        const auto& dbgData = dbgBlock.Layout;
        // test format
        try
        {
            std::vector<std::byte> test(dbgData.TotalSize);
            runtime.Logger.debug(FMT_STRING(u"DebugString:[{}]\n{}\ntest output:\n{}\n"sv), dbgId, formatter, dbgBlock.GetString(test));
        }
        catch (const fmt::format_error& fe)
        {
            runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangFormatException, formatter, fe));
            return U"// Formatter not match the datatype provided\r\n";
        }

        std::u32string func = fmt::format(FMT_STRING(U"inline void oclu_debug_{}("sv), dbgId);
        func.append(U"\r\n    const  uint           total,"sv)
            .append(U"\r\n    global uint* restrict counter,"sv)
            .append(U"\r\n    global uint* restrict data,"sv);
        for (size_t i = 0; i < args.size(); ++i)
        {
            APPEND_FMT(func, U"\r\n    const  {:7} arg{},"sv, Context.GetVecTypeName(args[i].second), i);
        }
        func.pop_back();
        func.append(U")\r\n{"sv);
        static constexpr auto syntax = UR"(
    global uint* const restrict ptr = oclu_debug({}, {}, total, counter, data);
    if (ptr == 0) return;)"sv;
        APPEND_FMT(func, syntax, dbgBlock.DebugId, dbgData.TotalSize / 4);
        const auto WriteOne = [&](uint32_t offset, uint16_t argIdx, uint8_t vsize, uint8_t eleBit, std::u32string_view argAccess, bool needConv)
        {
            const auto eleByte = eleBit / 8;
            const VecDataInfo dtype{ VecDataInfo::DataTypes::Unsigned, eleBit,     1, 0 };
            const auto dstTypeStr = Context.GetVecTypeName(dtype);
            std::u32string getData;
            if (needConv)
            {
                const VecDataInfo vtype{ VecDataInfo::DataTypes::Unsigned, eleBit, vsize, 0 };
                getData = FMTSTR(U"as_{}(arg{}{})"sv, Context.GetVecTypeName(vtype), argIdx, argAccess);
            }
            else
                getData = FMTSTR(U"arg{}{}"sv, argIdx, argAccess);
            if (vsize == 1)
            {
                APPEND_FMT(func, U"\r\n    ((global {}*)(ptr))[{}] = {};"sv, dstTypeStr, offset / eleByte, getData);
            }
            else
            {
                APPEND_FMT(func, U"\r\n    vstore{}({}, 0, (global {}*)(ptr) + {});"sv, vsize, getData, dstTypeStr, offset / eleByte);
            }
        };
        for (const auto& blk : dbgData.ByLayout())
        {
            const auto& [info, name, offset, idx] = blk;
            const auto size = info.Bit * info.Dim0;
            APPEND_FMT(func, U"\r\n    // arg[{:3}], offset[{:3}], size[{:3}], type[{:6}], name[{}]"sv,
                idx, offset, size / 8, xcomp::StringifyVDataType(info), dbgData.GetName(blk));
            for (const auto eleBit : std::array<uint8_t, 3>{ 32u, 16u, 8u })
            {
                const auto eleByte = eleBit / 8;
                if (size % eleBit == 0)
                {
                    const bool needConv = info.Bit != eleBit || info.Type != VecDataInfo::DataTypes::Unsigned;
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
                        WriteOne(offset + eleByte *  0, idx, 16, eleBit, U".s01234567"sv, needConv);
                        WriteOne(offset + eleByte * 16, idx, 16, eleBit, U".s89abcdef"sv, needConv);
                    }
                    else if (cnt == 6u)
                    {
                        Expects(info.Bit == 64 && info.Dim0 == 3);
                        WriteOne(offset + eleByte * 0, idx, 4, eleBit, U".s01"sv, needConv);
                        WriteOne(offset + eleByte * 4, idx, 2, eleBit, U".s2"sv,  needConv);
                    }
                    else if (cnt == 3u)
                    {
                        Expects(info.Bit == eleBit && info.Dim0 == 3);
                        WriteOne(offset + eleByte * 0, idx, 2, eleBit, U".s01"sv, needConv);
                        WriteOne(offset + eleByte * 2, idx, 1, eleBit, U".s2"sv,  needConv);
                    }
                    else
                    {
                        Expects(cnt == 1 || cnt == 2 || cnt == 4 || cnt == 8 || cnt == 16);
                        WriteOne(offset, idx, cnt, eleBit, {}, needConv);
                    }
                    break;
                }
            }
        }
        func.append(U"\r\n}\r\n"sv);
        return func;
    }

    std::u32string GenerateDebugFunc(Runtime_& runtime, const std::u32string_view id,
        const NLCLDebugExtension::DbgContent& item, common::span<const std::u32string_view> vals)
    {
        AppendDebugTid();
        Context.AddPatchedBlockD(U"oclu_debug"sv, Depend_DebugTid, &NLCLDebugExtension::DebugStringBase);
        Context.AddPatchedBlockD(*this, id, Depend_DebugBase, &NLCLDebugExtension::DebugStringPatch, runtime, id, item.first, item.second);

        std::u32string str = FMTSTR(U"oclu_debug_{}(_oclu_debug_buffer_size, _oclu_debug_buffer_info, _oclu_debug_buffer_data, ", id);
        for (size_t i = 0; i < vals.size(); ++i)
        {
            APPEND_FMT(str, U"{}, "sv, vals[i]);
        }
        str.pop_back(); // pop space
        str.back() = U')'; // replace ',' with ')'
        HasDebug = true;
        return str;
    }

    XCNL_EXT_REG(NLCLDebugExtension,
    {
        if(const auto ctx = dynamic_cast<NLCLContext*>(&context); ctx)
            return std::make_unique<NLCLDebugExtension>(*ctx);
        return {};
    });
};

void SetAllowDebug(const NLCLContext& context) noexcept
{
    if (const auto ext = context.GetXCNLExt<NLCLDebugExtension>(); ext)
        ext->EnableDebug = true;
}

xcomp::debug::DebugManager* ExtractDebugManager(const NLCLContext& context) noexcept
{
    if (const auto ext = context.GetXCNLExt<NLCLDebugExtension>(); ext)
        return &ext->DebugMan;
    return nullptr;
}



}
