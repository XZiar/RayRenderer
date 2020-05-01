#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "oclException.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"

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
    if (IsBeginWith(var.Name, U"oclu."sv))
    {
        const auto subName = var.Name.substr(5);
        const auto plat = Device->GetPlatform();
        if (IsBeginWith(subName, U"Extension."sv))
        {
            const auto extName = common::strchset::to_string(subName.substr(10), Charset::UTF8, Charset::UTF32LE);
            return plat->GetExtensions().Has(extName);
        }
        if (IsBeginWith(var.Name, U"Dev."sv))
        {
            const auto propName = var.Name.substr(4);
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
    }
    return {};
}

Arg oclu::NLCLEvalContext::LookUpArgInside(xziar::nailang::detail::VarLookup var) const
{
    if (const auto arg = LookUpCLArg(var); arg.TypeData != Arg::InternalType::Empty)
        return arg;
    return CompactEvaluateContext::LookUpArgInside(var);
}


NLCLRuntime::NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev) :
    NailangRuntimeBase(std::make_shared<NLCLEvalContext>(dev)),
    Logger(logger), Device(dev), EnabledExtensions(Device->Extensions.Size())
{ }
NLCLRuntime::~NLCLRuntime()
{ }

Arg NLCLRuntime::EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, MetaFuncs metas, const FuncTarget target)
{
    if (IsBeginWith(func, U"oclu."sv))
    {
        const auto subName = func.substr(5);
        switch (hash_(subName))
        {
        case "EnableExtension"_hash:
        {
            ThrowIfNotFuncTarget(func, target, FuncTarget::Type::Block);
            ThrowByArgCount(args, 1);
            if (const auto sv = args[0].GetStr(); sv.has_value())
            {
                if (!EnableExtension(sv.value()))
                    Logger.warning(u"Extension [{}] not found in support list, skipped.\n", sv.value());
            }
            else
                NLRT_THROW_EX(u"Arg of [EnableExtension] should be string"sv,
                    ExceptionTarget::NewFuncCall(func));
        } return {};
        default: break;
        }
    }
    return NailangRuntimeBase::EvaluateFunc(func, args, metas, target);
}

void NLCLRuntime::HandleException(const xziar::nailang::NailangRuntimeException& ex) const
{
    Logger.error(u"{}", ex.message);
    NailangRuntimeBase::HandleException(ex);
}

void NLCLRuntime::DirectOutput(const RawBlock& block, MetaFuncs, std::u32string& dst) const
{
    dst.append(block.Source);
}

void NLCLRuntime::OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"/* From GlobalBlock [{}] */\r\n"sv), block.Name);
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"/* From StructBlock [{}] */\r\n"sv), block.Name);
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const
{
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"/* From KernelBlock [{}] */\r\n"sv), block.Name);
    DirectOutput(block, metas, dst);
}

void NLCLRuntime::OutputTemplateKernel(const RawBlock& block, [[maybe_unused]] MetaFuncs metas, [[maybe_unused]] uint32_t extraInfo, std::u32string& dst) const
{
    // TODO:
    fmt::format_to(std::back_inserter(dst), FMT_STRING(U"/* From TempalteKernel [{}] */\r\n"sv), block.Name);
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

std::string NLCLRuntime::GenerateOutput() const
{
    std::u32string output;

    { // Output extentions
        common::linq::FromIterable(EnabledExtensions).Pair(common::linq::FromIterable(Device->Extensions))
            .ForEach([&](const auto& pair) 
                {
                    if (pair.first)
                        fmt::format_to(std::back_inserter(output), FMT_STRING(U"#pragma OPENCL EXTENSION {} : enable\r\r"sv), pair.second);
                });
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