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
using xziar::nailang::FuncCall;
using xziar::nailang::NailangRuntimeBase;
using common::str::Charset;
using common::str::IsBeginWith;


Arg NLCLEvalContext::LookUpCLArg(xziar::nailang::detail::VarLookup var) const
{
    if (IsBeginWith(var.Name, U"oglu."sv))
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


NLCLRuntime::NLCLRuntime(oclDevice dev) : 
    NailangRuntimeBase(std::make_shared<NLCLEvalContext>(dev)),
    Device(dev), EnabledExtensions(Device->Extensions.Size())
{ }
NLCLRuntime::~NLCLRuntime()
{ }

void NLCLRuntime::HandleRawBlock(const RawBlock& block, common::span<const FuncCall> metas)
{
    NailangRuntimeBase::HandleRawBlock(block, metas);
}

Arg NLCLRuntime::EvaluateFunc(const std::u32string_view func, common::span<const Arg> args,
    common::span<const FuncCall> metas, const NailangRuntimeBase::FuncTarget target)
{
    if (IsBeginWith(func, U"oglu."sv))
    {
        const auto subName = func.substr(5);
        switch (hash_(subName))
        {
        case "EnableExtension"_hash:
        {
            ThrowByArgCount(args, 1);
            if (const auto sv = args[0].GetStr(); sv.has_value())
            {
                EnableExtension(sv.value());
            }
            else
                throw U"Arg of [EnableExtension] should be string"sv;
        } return {};
        default: break;
        }
    }
    return NailangRuntimeBase::EvaluateFunc(func, args, metas, target);
}

void NLCLRuntime::EnableExtension(std::string_view ext)
{
    if (ext == "all"sv)
        EnabledExtensions.assign(EnabledExtensions.size(), true);
    else if (const auto idx = Device->Extensions.GetIndex(ext); idx != SIZE_MAX)
        EnabledExtensions[idx] = true;
}

void NLCLRuntime::EnableExtension(std::u32string_view ext)
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
}


NLCLProgram::NLCLProgram(std::u32string&& source) :
    Source(std::move(source)) { }
NLCLProgram::~NLCLProgram()
{ }


NLCLProgStub::NLCLProgStub(const std::shared_ptr<NLCLProgram>& program, 
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


void NLCLProcessor::ConfigureCL(NLCLProgStub& stub)
{
    stub.Program->ForEachBlockType(U"oclu.Prepare"sv,
        [&](const Block& block, common::span<const FuncCall> metas) 
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
}

std::shared_ptr<NLCLProgram> NLCLProcessor::Parse(common::span<const std::byte> source)
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

NLCLProgStub NLCLProcessor::ConfigureCL(const std::shared_ptr<NLCLProgram>& prog, oclDevice dev)
{
    NLCLProgStub stub(prog, dev, std::make_unique<NLCLRuntime>(dev));
    ConfigureCL(stub);
    return stub;
}

oclProgram NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx, oclDevice dev)
{
    if (!ctx->CheckIncludeDevice(dev))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"device not included in the context"sv);
    const auto stub = ConfigureCL(prog, dev);
    //TODO: 
    return oclProgram();
}


}