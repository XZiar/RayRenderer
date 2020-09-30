#include "oclPch.h"
#include "oclKernelDebug.h"
#include "oclNLCLRely.h"
#include "oclException.h"

namespace oclu
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


std::pair<VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept
{
#define CASE(str, dtype, bit, n, least) \
    HashCase(type, str) return { {common::simd::VecDataInfo::DataTypes::dtype, bit, n, 0}, least };
#define CASEV(pfx, type, bit, least) \
    CASE(PPCAT(U, STRINGIZE(pfx)""),    type, bit, 1,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v2"),  type, bit, 2,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v3"),  type, bit, 3,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v4"),  type, bit, 4,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v8"),  type, bit, 8,  least) \
    CASE(PPCAT(U, STRINGIZE(pfx)"v16"), type, bit, 16, least) \

#define CASE2(tstr, type, bit)                  \
    CASEV(PPCAT(tstr, bit),  type, bit, false)  \
    CASEV(PPCAT(tstr, bit+), type, bit, true)   \

    switch (hash_(type))
    {
    CASE2(u, Unsigned, 8)
    CASE2(u, Unsigned, 16)
    CASE2(u, Unsigned, 32)
    CASE2(u, Unsigned, 64)
    CASE2(i, Signed,   8)
    CASE2(i, Signed,   16)
    CASE2(i, Signed,   32)
    CASE2(i, Signed,   64)
    CASE2(f, Float,    16)
    CASE2(f, Float,    32)
    CASE2(f, Float,    64)
    default: break;
    }
    return { {VecDataInfo::DataTypes::Unsigned, 0, 0, 0}, false };

#undef CASE2
#undef CASEV
#undef CASE
}

std::u32string_view StringifyVDataType(const VecDataInfo vtype) noexcept
{
#define CASE(s, type, bit, n) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): return PPCAT(PPCAT(U,s),sv);
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),             type, bit, 1)  \
    CASE(STRINGIZE(PPCAT(pfx, v2)),  type, bit, 2)  \
    CASE(STRINGIZE(PPCAT(pfx, v3)),  type, bit, 3)  \
    CASE(STRINGIZE(PPCAT(pfx, v4)),  type, bit, 4)  \
    CASE(STRINGIZE(PPCAT(pfx, v8)),  type, bit, 8)  \
    CASE(STRINGIZE(PPCAT(pfx, v16)), type, bit, 16) \

    switch (static_cast<uint32_t>(vtype))
    {
    CASEV(u8,  Unsigned, 8)
    CASEV(u16, Unsigned, 16)
    CASEV(u32, Unsigned, 32)
    CASEV(u64, Unsigned, 64)
    CASEV(i8,  Signed,   8)
    CASEV(i16, Signed,   16)
    CASEV(i32, Signed,   32)
    CASEV(i64, Signed,   64)
    CASEV(f16, Float,    16)
    CASEV(f32, Float,    32)
    CASEV(f64, Float,    64)
    default: return U"Error"sv;
    }

#undef CASEV
#undef CASE
}


struct NLCLRuntime_ : public NLCLRuntime
{
    friend struct NLCLDebugExtension;
    friend struct KernelDebugExtension;
};

#define NLRT_THROW_EX(...) Runtime.HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
struct NLCLDebugExtension : public NLCLExtension
{
    oclDebugManager DebugManager;
    using DbgContent = std::pair<std::u32string, std::vector<common::simd::VecDataInfo>>;
    std::map<std::u32string, DbgContent, std::less<>> DebugInfos;
    bool AllowDebug = false, HasDebug = false;
    NLCLDebugExtension(NLCLContext& context) : NLCLExtension(context) { }
    ~NLCLDebugExtension() override { }

    [[nodiscard]] std::optional<xziar::nailang::Arg> NLCLFunc(NLCLRuntime& runtime, const xziar::nailang::FuncCall& call,
        common::span<const xziar::nailang::FuncCall>, const xziar::nailang::NailangRuntimeBase::FuncTargetType target) override
    {
        auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
        using namespace xziar::nailang;
        if (call.Name == U"oclu.EnableDebug")
        {
            Runtime.ThrowIfNotFuncTarget(call.Name, target, NLCLRuntime_::FuncTargetType::Plain);
            Runtime.Logger.verbose(u"Manually enable debug.\n");
            AllowDebug = true;
            return Arg{};
        }
        else if (call.Name == U"oclu.DefineDebugString"sv)
        {
            Runtime.ThrowIfNotFuncTarget(call.Name, target, NLCLRuntime_::FuncTargetType::Plain);
            Runtime.ThrowByArgCount(call, 3, ArgLimits::AtLeast);
            const auto arg2 = Runtime.EvaluateFuncArgs<2, ArgLimits::AtLeast>(call, { Arg::Type::String, Arg::Type::String });
            const auto id = arg2[0].GetStr().value();
            const auto formatter = arg2[1].GetStr().value();
            std::vector<common::simd::VecDataInfo> argInfos;
            argInfos.reserve(call.Args.size() - 2);
            size_t i = 2;
            for (const auto& rawarg : call.Args.subspan(2))
            {
                const auto arg = Runtime.EvaluateArg(rawarg);
                if (!arg.IsStr())
                    NLRT_THROW_EX(FMTSTR(u"Arg[{}] of [DefineDebugString] should be string, which gives [{}]", 
                        i, NLCLRuntime_::ArgTypeName(arg.TypeData)), call);
                const auto vtype = Runtime.ParseVecType(arg.GetStr().value(), [&]()
                    {
                        return fmt::format(FMT_STRING(u"Arg[{}] of [DefineDebugString]"sv), i);
                    });
                argInfos.push_back(vtype);
            }
            if (!DebugInfos.try_emplace(std::u32string(id), formatter, std::move(argInfos)).second)
                NLRT_THROW_EX(fmt::format(u"DebugString [{}] repeately defined"sv, id), call);
            return Arg{};
        }
        return {};
    }
    bool KernelMeta(NLCLRuntime& runtime, const xziar::nailang::FuncCall& meta, KernelContext& kernel) override
    {
        auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
        using namespace xziar::nailang;
        if (meta.Name == U"oclu.DebugOutput"sv)
        {
            if (AllowDebug)
            {
                const auto size = Runtime.EvaluateFuncArgs<1, ArgLimits::AtMost>(meta, { Arg::Type::Integer })[0].GetUint();
                kernel.SetDebugBuffer(gsl::narrow_cast<uint32_t>(size.value_or(512)));
            }
            else
                Runtime.Logger.info(u"DebugOutput is disabled and ignored.\n");
            return true;
        }
        return false;
    }
    void FinishNLCL(NLCLRuntime& runtime) override
    {
        auto& Runtime = static_cast<NLCLRuntime_&>(runtime);
        if (AllowDebug && HasDebug)
        {
            Runtime.EnableExtension("cl_khr_global_int32_base_atomics"sv, u"Use oclu-debugoutput"sv);
            Runtime.EnableExtension("cl_khr_byte_addressable_store"sv, u"Use oclu-debugoutput"sv);
        }
        if (Context.SupportBasicSubgroup)
            DebugManager.InfoMan = std::make_unique<SubgroupInfoMan>();
        else
            DebugManager.InfoMan = std::make_unique<NonSubgroupInfoMan>();
    }
    void OnCompile(oclProgStub& stub) override
    { 
        stub.DebugManager = std::move(DebugManager);
    }

    inline static uint32_t ID = NLCLExtension::RegisterNLCLExtenstion(
        [](auto&, NLCLContext& context, const xziar::nailang::Block&) -> std::unique_ptr<NLCLExtension>
        {
            return std::make_unique<NLCLDebugExtension>(context);
        });
};
void SetAllowDebug(const NLCLContext& context) noexcept
{
    if (const auto ext = context.GetNLCLExt<NLCLDebugExtension>(); ext)
        ext->AllowDebug = true;
}

struct KernelDebugExtension : public KernelExtension
{
public:
    KernelContext& Kernel;
    NLCLDebugExtension& Host;
    bool HasDebug = false;
    KernelDebugExtension(NLCLContext& context, KernelContext& kernel) :
        KernelExtension(context), Kernel(kernel), Host(*Context.GetNLCLExt<NLCLDebugExtension>())
    {
        Expects(&Host);
    }
    ~KernelDebugExtension() override { }

    std::u32string DebugStringBase() noexcept
    {
        return UR"(
inline global uint* oglu_debug(const uint dbgId, const uint dbgSize,
    const uint tid, const uint total,
    global uint* restrict counter, global uint* restrict data)
{
    const uint size = dbgSize + 1;
    const uint dId  = (dbgId + 1) << 24;
    const uint uid  = tid & 0x00ffffffu;
    if (total == 0) 
        return 0;
    if (counter[0] + size > total) 
        return 0;
    const uint old = atom_add(counter, size);
    if (old >= total)
        return 0;
    if (old + size > total) 
    { 
        data[old] = uid; 
        return 0;
    }
    data[old] = uid | dId;
    return data + old + 1;
}
)"s;
    }

    std::u32string DebugStringPatch(NLCLRuntime_& runtime, const std::u32string_view dbgId, 
        const std::u32string_view formatter, common::span<const common::simd::VecDataInfo> args) noexcept
    {
        // prepare arg layout
        const auto& dbgBlock = Host.DebugManager.AppendBlock(dbgId, formatter, args);
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

        std::u32string func = fmt::format(FMT_STRING(U"inline void oglu_debug_{}("sv), dbgId);
        func.append(U"\r\n    const  uint           tid,"sv)
            .append(U"\r\n    const  uint           total,"sv)
            .append(U"\r\n    global uint* restrict counter,"sv)
            .append(U"\r\n    global uint* restrict data,"sv);
        for (size_t i = 0; i < args.size(); ++i)
        {
            APPEND_FMT(func, U"\r\n    const  {:7} arg{},"sv, NLCLContext::GetCLTypeName(args[i]), i);
        }
        func.pop_back();
        func.append(U")\r\n{"sv);
        static constexpr auto syntax = UR"(
    global uint* const restrict ptr = oglu_debug({}, {}, tid, total, counter, data);
    if (ptr == 0) return;)"sv;
        APPEND_FMT(func, syntax, dbgBlock.DebugId, dbgData.TotalSize / 4);
        const auto WriteOne = [&](uint32_t offset, uint16_t argIdx, uint8_t vsize, VecDataInfo dtype, std::u32string_view argAccess, bool needConv)
        {
            const VecDataInfo vtype{ dtype.Type, dtype.Bit, vsize, 0 };
            const auto dstTypeStr = NLCLContext::GetCLTypeName(dtype);
            std::u32string getData = needConv ?
                fmt::format(FMT_STRING(U"as_{}(arg{}{})"sv), NLCLContext::GetCLTypeName(vtype), argIdx, argAccess) :
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
            for (const auto eleBit : std::array<uint8_t, 3>{ 32u, 16u, 8u })
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
                        WriteOne(offset + eleByte * 0, idx, 16, dstType, U".s01234567"sv, needConv);
                        WriteOne(offset + eleByte * 16, idx, 16, dstType, U".s89abcdef"sv, needConv);
                    }
                    else if (cnt == 6u)
                    {
                        Expects(info.Bit == 64 && info.Dim0 == 3);
                        WriteOne(offset + eleByte * 0, idx, 4, dstType, U".s01"sv, needConv);
                        WriteOne(offset + eleByte * 4, idx, 2, dstType, U".s2"sv, needConv);
                    }
                    else if (cnt == 3u)
                    {
                        Expects(info.Bit == dstType.Bit && info.Dim0 == 3);
                        WriteOne(offset + eleByte * 0, idx, 2, dstType, U".s01"sv, needConv);
                        WriteOne(offset + eleByte * 2, idx, 1, dstType, U".s2"sv, needConv);
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

    std::u32string GenerateDebugFunc(NLCLRuntime_& runtime, const std::u32string_view id, 
        const NLCLDebugExtension::DbgContent& item, common::span<const std::u32string_view> vals)
    {
        Context.AddPatchedBlock(*this, U" oglu_debug"sv, &KernelDebugExtension::DebugStringBase);
        Context.AddPatchedBlock(*this, id, &KernelDebugExtension::DebugStringPatch, runtime, id, item.first, item.second);

        std::u32string str = FMTSTR(U"oglu_debug_{}(_oclu_thread_id, _oclu_debug_buffer_size, _oclu_debug_buffer_info, _oclu_debug_buffer_data, ", id);
        for (size_t i = 0; i < vals.size(); ++i)
        {
            APPEND_FMT(str, U"{}, "sv, vals[i]);
        }
        str.pop_back();
        str.back() = U')';
        HasDebug = true;
        return str;
    }

    ReplaceResult ReplaceFunc(NLCLRuntime& runtime, std::u32string_view func, const common::span<const std::u32string_view> args) override
    {
        NLCLRuntime_& Runtime = static_cast<NLCLRuntime_&>(runtime);
        using namespace xziar::nailang;
        if (func == U"oclu.DebugString"sv)
        {
            Runtime.ThrowByReplacerArgCount(func, args, 1, ArgLimits::AtLeast);
            if (!Host.AllowDebug)
                return U"do{} while(false)"sv;

            const auto id = args[0];
            const auto info = common::container::FindInMap(Host.DebugInfos, id);
            if (info)
            {
                Runtime.ThrowByReplacerArgCount(func, args, info->second.size() + 1, ArgLimits::Exact);
                return GenerateDebugFunc(Runtime, id, *info, args.subspan(1));
            }
            NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugString] reference to unregisted info [{}].", id));
        }
        else if (func == U"oclu.DebugStr"sv)
        {
            Runtime.ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtLeast);
            if (!Host.AllowDebug)
                return U"do{} while(false)"sv;
            if (args.size() % 2)
                NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr] requires even number of args, which gives [{}]."sv, args.size()));
            if (args[1].size() < 2 || args[1].front() != U'"' || args[1].back() != U'"')
                NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr]'s arg[1] expects to a string with \", get [{}]", args[1]));
            const auto id = args[0], formatter = args[1].substr(1, args[1].size() - 2);

            std::vector<common::simd::VecDataInfo> types;
            std::vector<std::u32string_view> vals;
            const auto argCnt = args.size() / 2 - 1;
            types.reserve(argCnt); vals.reserve(argCnt);
            for (size_t i = 2; i < args.size(); i += 2)
            {
                const auto vtype = Runtime.ParseVecType(args[i], [&]()
                    {
                        return FMTSTR(u"Arg[{}] of [DebugStr]", i);
                    });
                types.push_back(vtype);
                vals. push_back(args[i + 1]);
            }
            const auto [info, ret] = Host.DebugInfos.try_emplace(std::u32string(id), formatter, std::move(types));
            if (!ret)
                NLRT_THROW_EX(FMTSTR(u"DebugString [{}] repeately defined", id));
            
            return GenerateDebugFunc(Runtime, id, info->second, vals);
        }
        return {};
    }
    void FinishKernel(NLCLRuntime&, KernelContext& kernel) override
    {
        Expects(&kernel == &Kernel);
        if (HasDebug)
        {
            kernel.Args.HasDebug = true;
            Host.HasDebug = true;
            kernel.AddTailArg(KerArgType::Simple, KerArgSpace::Private, ImgAccess::None, KerArgFlag::Const,
                "_oclu_debug_buffer_size", "uint");
            kernel.AddTailArg(KerArgType::Buffer, KerArgSpace::Global, ImgAccess::None, KerArgFlag::Restrict,
                "_oclu_debug_buffer_info", "uint*");
            kernel.AddTailArg(KerArgType::Buffer, KerArgSpace::Global, ImgAccess::None, KerArgFlag::Restrict,
                "_oclu_debug_buffer_data", "uint*"); 
            kernel.AddBodyPrefix(U"oclu_debug", UR"(
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
)");
        }
    }

    inline static uint32_t ID = NLCLExtension::RegisterKernelExtenstion(
        [](auto&, NLCLContext& context, KernelContext& kernel) -> std::unique_ptr<KernelExtension>
        {
            return std::make_unique<KernelDebugExtension>(context, kernel);
        });
};
#undef NLRT_THROW_EX



DebugDataLayout::DebugDataLayout(common::span<const VecDataInfo> infos, const uint16_t align) :
    Blocks(std::make_unique<DataBlock[]>(infos.size())), ArgLayout(std::make_unique<uint16_t[]>(infos.size())),
    TotalSize(0), ArgCount(gsl::narrow_cast<uint32_t>(infos.size()))
{
    Expects(ArgCount <= UINT8_MAX);
    std::vector<DataBlock> tmp;
    uint16_t offset = 0, layoutidx = 0;
    { 
        uint16_t idx = 0;
        for (const auto info : infos)
        {
            Blocks[idx].Info   = info;
            Blocks[idx].ArgIdx = idx;
            const auto size = info.Bit * info.Dim0 / 8;
            if (size % align == 0)
            {
                Blocks[idx].Offset = offset;
                offset = static_cast<uint16_t>(offset + size);
                ArgLayout[layoutidx++] = idx;
            }
            else
            {
                Blocks[idx].Offset = UINT16_MAX;
                tmp.emplace_back(Blocks[idx]);
            }
            idx++;
        }
    }
    std::sort(tmp.begin(), tmp.end(), [](const auto& left, const auto& right)
        { 
            return left.Info.Bit == right.Info.Bit ? 
                left.Info.Bit * left.Info.Dim0 > right.Info.Bit * right.Info.Dim0 : 
                left.Info.Bit > right.Info.Bit; 
        });

    for (const auto [info, dummy, idx] : tmp)
    {
        Expects(dummy == UINT16_MAX);
        Blocks[idx].Offset = offset;
        const auto size = info.Bit * info.Dim0 / 8;
        offset = static_cast<uint16_t>(offset + size);
        ArgLayout[layoutidx++] = idx;
    }
    TotalSize = (offset + (align - 1)) / align * align;
}



oclDebugInfoMan::~oclDebugInfoMan()
{
}

uint32_t oclDebugInfoMan::GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept
{
    size_t size = 1;
    for (uint32_t i = 0; i < dims; ++i)
        size *= worksize[i];
    Expects(size < 0x00ffffffu);
    return static_cast<uint32_t>(sizeof(uint32_t) * (size + 7));
}

oclThreadInfo NonSubgroupInfoMan::GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept
{
    const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
    const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
    auto info = BasicInfo(gsize, lsize, tid);
    info.SubgroupId = info.SubgroupLocalId = 0;
    const auto infodata = space[tid + 7];
    const auto lid = info.LocalId[0] + info.LocalId[1] * lsize[0] + info.LocalId[2] * lsize[1] * lsize[0];
    Expects(infodata == lid);
    return info;
}

oclThreadInfo SubgroupInfoMan::GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept
{
    const auto& gsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 1);
    const auto& lsize = *reinterpret_cast<const uint32_t(*)[3]>(space.data() + 4);
    auto info = BasicInfo(gsize, lsize, tid);
    const auto infodata = space[tid + 7];
    info.SubgroupId = static_cast<uint16_t>(infodata / 65536u);
    info.SubgroupLocalId = static_cast<uint16_t>(infodata % 65536u);
    return info;
}


template<typename T>
static void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, common::span<const T> data)
{
    if (data.size() == 1)
    {
        if constexpr (std::is_same_v<T, half_float::half>)
            store.push_back(static_cast<float>(data[0]));
        else
            store.push_back(data[0]);
    }
    else
    {
        if constexpr (std::is_same_v<T, half_float::half>)
        {
            Expects(data.size() <= 16);
            std::array<float, 16> tmp;
            size_t idx = 0;
            for (const auto val : data)
                tmp[idx++] = val;
            const auto datspan = common::to_span(tmp).subspan(0, data.size());
            store.push_back(fmt::format(FMT_STRING("{}"sv), datspan));
        }
        else
        {
            store.push_back(fmt::format(FMT_STRING("{}"sv), data));
        }
    }
}
static void Insert(fmt::dynamic_format_arg_store<fmt::u32format_context>& store, std::nullopt_t)
{
    store.push_back(U"Unknown"sv);
}
common::str::u8string oclDebugBlock::GetString(common::span<const std::byte> data) const
{
    fmt::dynamic_format_arg_store<fmt::u32format_context> store;
    for (const auto& arg : Layout.ByIndex())
    {
        arg.VisitData(data, [&](auto ele) { Insert(store, ele); });
    }
    auto str = fmt::vformat(Formatter, store);
    return common::str::to_u8string(str, common::str::Charset::UTF32LE);
}


void oclDebugManager::CheckNewBlock(const std::u32string_view name) const
{
    const auto idx = Blocks.size();
    if (idx >= 250u)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Too many DebugString defined, maximum 250");
    for (const auto& block : Blocks)
        if (block.Name == name)
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"DebugString re-defiend");
}

std::pair<const oclDebugBlock*, uint32_t> oclDebugManager::RetriveMessage(common::span<const uint32_t> data) const
{
    const auto uid = data[0];
    const auto dbgIdx = uid >> 24;
    const auto tid = uid & 0x00ffffffu;
    if (dbgIdx == 0) return { nullptr, 0 };
    if (dbgIdx > Blocks.size())
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Wrong message with idx overflow");
    const auto& block = Blocks[dbgIdx - 1];
    if (data.size_bytes() < block.Layout.TotalSize)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Wrong message with insufficiant buffer space");
    return { &block, tid };
}


oclDebugPackage::~oclDebugPackage()
{ }

CachedDebugData oclDebugPackage::GetCachedData() const
{
    return this;
}

common::str::u8string_view CachedDebugData::DebugItemWrapper::Str()
{
    if (!Item.Str.GetLength())
    {
        const auto& block = Host.Package.DebugManager->GetBlocks()[Item.BlockId];
        const auto data = Host.Package.DataBuffer.AsSpan<uint32_t>().subspan(Item.DataOffset, Item.DataLength);
        const auto str = block.GetString(data);
        Item.Str = Host.AllocateString(str);
    }
    return Host.GetStringView(Item.Str);
}

CachedDebugData::CachedDebugData(const oclDebugPackage* package) : Package(*package)
{
    const auto blocks  = Package.DebugManager->GetBlocks();
    const auto alldata = Package.DataBuffer.AsSpan<uint32_t>();
    Package.VisitData([&](const uint32_t tid, const oclDebugInfoMan&, const oclDebugBlock& block,
        const common::span<const uint32_t>, const common::span<const uint32_t> dat) 
        {
            const auto offset = gsl::narrow_cast<uint32_t>(dat.data() - alldata.data());
            const auto datLen = gsl::narrow_cast<uint16_t>(dat.size());
            const auto blkId  = gsl::narrow_cast<uint16_t>(&block - blocks.data());
            Items.push_back({ {0, 0}, tid, offset, datLen, blkId });
        });
}
CachedDebugData::~CachedDebugData()
{ }


}
