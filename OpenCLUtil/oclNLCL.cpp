#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "oclException.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"
#include "common/SIMD.hpp"

namespace oclu
{
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::BlockContent;
using xziar::nailang::FuncCall;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::detail::ExceptionTarget;
using common::str::Charset;
using common::str::IsBeginWith;


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException, __VA_ARGS__))

Arg NLCLEvalContext::LookUpCLArg(xziar::nailang::detail::VarLookup var) const
{
    const auto plat = Device->GetPlatform();
    if (var.Part() == U"Extension"sv)
    {
        const auto extName = common::strchset::to_string(var.Rest(), Charset::UTF8, Charset::UTF32LE);
        return plat->GetExtensions().Has(extName);
    }
    if (var.Part() == U"Dev"sv)
    {
        const auto propName = var.Rest();
        switch (hash_(propName))
        {
#define U_PROP(name) case PPCAT(STRINGIZE(name),_hash): return static_cast<uint64_t>(Device->name)
        U_PROP(LocalMemSize);
        U_PROP(GlobalMemSize);
        U_PROP(GlobalCacheSize);
        U_PROP(GlobalCacheLine);
        U_PROP(ConstantBufSize);
        U_PROP(MaxMemSize);
        U_PROP(ComputeUnits);
        U_PROP(Version);
        U_PROP(CVersion);
#undef U_PROP
        case "type"_hash:
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
        case "vendor"_hash:
        {
            switch (plat->PlatVendor)
            {
#define U_VENDOR(name) case Vendors::name: return PPCAT(U, STRINGIZE(name))
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


NLCLRuntime::NLCLReplacer::NLCLReplacer(const NLCLRuntime& runtime) : 
    Runtime(runtime), 
    SupportFP16(Runtime.Device->Extensions.Has("cl_khr_fp16")),
    SupportFP64(Runtime.Device->Extensions.Has("cl_khr_fp64"))
{ }
NLCLRuntime::NLCLReplacer::~NLCLReplacer() { }

void NLCLRuntime::NLCLReplacer::ThrowByArgCount(const std::u32string_view func, const common::span<std::u32string_view> args, const size_t count) const
{
    if (args.size() != count)
        Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
            fmt::format(FMT_STRING(u"Repace-func [{}] requires [{}] args, which gives [{}]."), func, args.size(), count)));
}

template<typename T>
forceinline bool FastCompare(const std::basic_string_view<T> src, const std::basic_string_view<T> target) noexcept
{
    if (src.size() != target.size()) 
        return false;
    [[maybe_unused]] const auto targetSBytes = target.size() * sizeof(T);
#if COMMON_SIMD_LV >= 20
    if (targetSBytes <= 128u / 8u)
    {
        const auto mask = (1u << targetSBytes) - 1u;
        const auto vecT = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target.data()));
        const auto vecS = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()));
        const auto cmpbits = (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vecS, vecT));
        return (cmpbits & mask) == mask;
    }
#endif
#if COMMON_SIMD_LV >= 200
    if (targetSBytes <= 256u / 8u)
    {
        const auto mask = (1u << targetSBytes) - 1u;
        const auto vecT = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(target.data()));
        const auto vecS = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src.data()));
        const auto cmpbits = (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(vecS, vecT));
        return (cmpbits & mask) == mask;
    }
#elif COMMON_SIMD_LV >= 20
    if (targetSBytes <= 256u / 8u)
    {
        const auto mask = (1u << (targetSBytes - 128u / 8u)) - 1u;
        const auto vecT0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target.data()) + 0);
        const auto vecT1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target.data()) + 1);
        const auto vecS0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 0);
        const auto vecS1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 1);
        const auto cmpbits0 = (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vecS0, vecT0));
        const auto cmpbits1 = (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vecS1, vecT1));
        return cmpbits0 == 0xffffu && (cmpbits1 & mask) == mask;
    }
#endif
    return src == target;
}

static constexpr std::pair<common::simd::VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept
{
#define CASE(str, dtype, bit, n, least) \
    case hash_(str): if (FastCompare(type, PPCAT(U, str))) return {{common::simd::VecDataInfo::DataTypes::dtype, bit, n, 0}, least}; else break;

#define CASEV(pfx, type, bit, least) \
    CASE(STRINGIZE(pfx)""sv,    type, bit, 1,  least) \
    CASE(STRINGIZE(pfx)"v2"sv,  type, bit, 2,  least) \
    CASE(STRINGIZE(pfx)"v3"sv,  type, bit, 3,  least) \
    CASE(STRINGIZE(pfx)"v4"sv,  type, bit, 4,  least) \
    CASE(STRINGIZE(pfx)"v8"sv,  type, bit, 8,  least) \
    CASE(STRINGIZE(pfx)"v16"sv, type, bit, 16, least) \

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
    return { {common::simd::VecDataInfo::DataTypes::Unsigned, 0, 0, 0}, false };

#undef CASE2
#undef CASEV
#undef CASE
}

void NLCLRuntime::NLCLReplacer::OnReplaceVariable(std::u32string& output, const std::u32string_view var)
{
    if (var.size() > 0 && var[0] == U'@')
    {
        using common::simd::VecDataInfo;
        auto [info, least] = ParseVDataType(var.substr(1));
        if (info.Bit == 0)
        {
            Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
                fmt::format(FMT_STRING(u"Type [{}] not found when repace-variable"sv), var)));
            return;
        }
        if (info.Type == VecDataInfo::DataTypes::Float) // FP ext handling
        {
            if (info.Bit == 16 && !SupportFP16) // FP16 check
            {
                if (least) // promotion
                    info.Bit = 32;
                else
                    Runtime.Logger.warning(u"Potential use of unsupportted FP16 with [{}].\n"sv, var);
            }
            else if (info.Bit == 64 && !SupportFP64)
            {
                Runtime.Logger.warning(u"Potential use of unsupportted FP64 with [{}].\n"sv, var);
            }
        }

        std::u32string_view str;

#define CASE(s, type, bit, n) case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): str = PPCAT(PPCAT(U,s),sv); break;
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),            type, bit, 1); \
    CASE(STRINGIZE(PPCAT(pfx, 2)),  type, bit, 2); \
    CASE(STRINGIZE(PPCAT(pfx, 3)),  type, bit, 3); \
    CASE(STRINGIZE(PPCAT(pfx, 4)),  type, bit, 4); \
    CASE(STRINGIZE(PPCAT(pfx, 8)),  type, bit, 8); \
    CASE(STRINGIZE(PPCAT(pfx, 16)), type, bit, 16);\

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
        default:
            Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
                fmt::format(FMT_STRING(u"Type [{}] not found when repace-variable"sv), var)));
            return;
        }

#undef CASEV
#undef CASE

        output.append(str);
    }
    else // '@' not allowed in var anyway
    {
        const auto ret = Runtime.EvalContext->LookUpArg(var);
        if (ret.IsEmpty() || ret.TypeData == Arg::InternalType::Var)
        {
            Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
                fmt::format(FMT_STRING(u"Arg [{}] not found when repace-variable"sv), var)));
            return;
        }
        output.append(ret.ToString().StrView());
    }
}

void NLCLRuntime::NLCLReplacer::OnReplaceFunction(std::u32string& output, const std::u32string_view func, const common::span<std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        switch (args.size())
        {
        case 0:  output.append(U"__attribute__((opencl_unroll_hint))"sv); break;
        case 1:  fmt::format_to(std::back_inserter(output), FMT_STRING(U"__attribute__((opencl_unroll_hint({})))"sv), args[0]); break;
        default: Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
                fmt::format(FMT_STRING(u"Repace-func [unroll] requires [0,1] args, which gives [{}]."), args.size())));
        }
        return;
    }
    if (IsBeginWith(func, U"oglu."sv))
    {
        const auto subName = func.substr(5);
        switch (hash_(subName))
        {
        default: break;
        }
    }
    Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException,
        u"replace-function not ready"sv));
}


NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev) :
    NailangRuntimeBase(std::make_shared<NLCLEvalContext>(dev)),
    Logger(logger), Device(dev), EnabledExtensions(Device->Extensions.Size())
{ }
NLCLRuntime::~NLCLRuntime()
{ }

Arg NLCLRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas, const FuncTarget target)
{
    if (IsBeginWith(call.Name, U"oclu."sv))
    {
        const auto subName = call.Name.substr(5);
        switch (hash_(subName))
        {
        case "EnableExtension"_hash:
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
        default: break;
        }
    }
    return NailangRuntimeBase::EvaluateFunc(call, metas, target);
}

void NLCLRuntime::HandleException(const xziar::nailang::NailangRuntimeException& ex) const
{
    Logger.error(u"{}", ex.message);
    NailangRuntimeBase::HandleException(ex);
}

bool NLCLRuntime::CheckExtension(std::string_view ext, std::u16string_view desc) const
{
    if (!Device->Extensions.Has(ext))
    {
        Logger.warning(FMT_STRING(u"{} on unsupportted device.\n"sv), desc);
        return false;
    }
    else if (!CheckExtensionEnabled(ext))
    {
        Logger.warning(FMT_STRING(u"{} without enabling extenssion.\n"sv), desc);
        return false;
    }
    return true;
}

void NLCLRuntime::DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const
{
    OutputConditions(metas, dst);
    common::str::StrVariant<char32_t> source(block.Source);
    if (common::linq::FromIterable(metas).ContainsIf([](const FuncCall& fcall) { return fcall.Name == U"oglu.ReplaceVariable"sv; }))
        source = Replacer->ProcessVariable(source.StrView(), U"$$!{"sv, U"}"sv);
    if (common::linq::FromIterable(metas).ContainsIf([](const FuncCall& fcall) { return fcall.Name == U"oglu.ReplaceFunction"sv; }))
        source = Replacer->ProcessFunction(source.StrView(), U"$$!"sv, U""sv);
    dst.append(source.StrView());
}

std::unique_ptr<NLCLRuntime::NLCLReplacer> NLCLRuntime::PrepareRepalcer() const
{
    return std::make_unique<NLCLReplacer>(*this);
}


void NLCLRuntime::OutputConditions(MetaFuncs metas, std::u32string& dst) const
{
    // std::set<std::u32string_view> lateVars;
    for (const auto& meta : metas)
    {
        std::u32string_view prefix;
        if (meta.Name == U"If"sv)
            prefix = U"If"sv;
        else if (meta.Name == U"Skip"sv)
            prefix = U"Skip if"sv;
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

void NLCLRuntime::OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst)
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From KernelBlock [{}] */\r\n"sv), block.Name);
    for (const auto meta : metas)
    {
        switch (hash_(meta.Name))
        {
        case "oglu.RequestSubgroupSize"_hash:
        {
            ThrowByArgCount(meta, 1);
            const auto sgSize = EvaluateArg(meta.Args[0]).GetUint();
            if (sgSize)
            {
                CheckExtension("cl_intel_required_subgroup_size"sv, u"Request Subggroup Size"sv);
                fmt::format_to(std::back_inserter(dst), FMT_STRING(U"__attribute__((intel_reqd_sub_group_size({})))\r\n"sv), sgSize.value());
            }
            else
                Logger.verbose(u"Skip subgroup size due to empty arg.\n"sv);
        } break;
        case "oglu.RequestWorkgroupSize"_hash:
        {
            const auto args = EvaluateFuncArgs<3>(meta);
            const auto x = args[0].GetUint().value_or(1),
                       y = args[1].GetUint().value_or(1),
                       z = args[2].GetUint().value_or(1);
            fmt::format_to(std::back_inserter(dst), FMT_STRING(U"__attribute__((reqd_work_group_size({}, {}, {})))\r\n"sv),
                x, y, z);
        } break;
        default:
            break;
        }
    }
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputTemplateKernel(const RawBlock& block, [[maybe_unused]] MetaFuncs metas, [[maybe_unused]] uint32_t extraInfo, std::u32string& dst)
{
    // TODO:
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"\r\n/* From TempalteKernel [{}] */\r\n"sv), block.Name);
    NLRT_THROW_EX(u"TemplateKernel not supportted yet"sv,
        &block);
}

bool NLCLRuntime::EnableExtension(std::string_view ext)
{
    if (ext == "all"sv)
        EnabledExtensions.assign(EnabledExtensions.size(), true);
    else if (const auto idx = Device->Extensions.GetIndex(ext); idx != SIZE_MAX)
        EnabledExtensions[idx] = true;
    else
        return false;
    return true;
}

bool NLCLRuntime::EnableExtension(std::u32string_view ext)
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
        return false;
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
        const auto subName = block.Type.substr(5);
        bool output = false;
        switch (hash_(subName))
        {
        case "Global"_hash:     output = true;  break;
        case "Struct"_hash:     output = true;  StructBlocks.emplace_back(&block, metas); break;
        case "Kernel"_hash:     output = true;  KernelBlocks.emplace_back(&block, metas); break;
        case "Template"_hash:   output = false; TemplateBlocks.emplace_back(&block, metas); break;
        case "KernelStub"_hash: output = false; KernelStubBlocks.emplace_back(&block, metas); break;
        default: break;
        }
        if (output)
            OutputBlocks.emplace_back(&block, metas);
    }
}

std::string NLCLRuntime::GenerateOutput()
{
    std::u32string output;

    Replacer = PrepareRepalcer();

    { // Output extentions
        output.append(U"/* Extensions */\r\n"sv);
        if (common::linq::FromIterable(EnabledExtensions).All(true))
        {
            output.append(U"#pragma OPENCL EXTENSION all : enable\r\n"sv);
        }
        else
        {
            common::linq::FromIterable(EnabledExtensions).Pair(common::linq::FromIterable(Device->Extensions))
                .ForEach([&](const auto& pair)
                    {
                        if (pair.first)
                            fmt::format_to(std::back_inserter(output), FMT_STRING(U"#pragma OPENCL EXTENSION {} : enable\r\n"sv), pair.second);
                    });
        }
        output.append(U"\r\n"sv);
    }

    for (const auto& item : OutputBlocks)
    {
        const MetaFuncs metas(item.MetaPtr, item.MetaCount);
        switch (hash_(item.Block->Type))
        {
        case "oclu.Global"_hash:     OutputGlobal(*item.Block, metas, output); break;
        case "oclu.Struct"_hash:     OutputStruct(*item.Block, metas, output); break;
        case "oclu.Kernel"_hash:     OutputKernel(*item.Block, metas, output); break;
        case "oclu.KernelStub"_hash: OutputTemplateKernel(*item.Block, metas, item.ExtraInfo, output); break;
        default: Logger.warning(u"Unexpected RawBlock (Type[{}], Name[{}]) when generating output.\n", item.Block->Type, item.Block->Name); break;
        }
    }

    return common::strchset::to_string(output, Charset::UTF8, Charset::UTF32LE);
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

std::string NLCLProcessor::ProcessCL(const std::shared_ptr<NLCLProgram>& prog, const oclDevice dev) const
{
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(Logger(), dev));
    ConfigureCL(stub);
    return stub.Runtime->GenerateOutput();
}

oclProgram NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, oclDevice dev) const
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(Logger(), dev));
    ConfigureCL(stub);
    //TODO: 
    return oclProgram();
}


}