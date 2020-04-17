#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"

namespace oclu
{
using namespace std::string_view_literals;


NLCLProgram::NLCLProgram(std::u32string&& source) :
    Source(std::move(source)) { }
NLCLProgram::~NLCLProgram()
{ }

NLCLProgStub::NLCLProgStub(const std::shared_ptr<NLCLProgram>& program, 
    const oclContext& context, std::unique_ptr<xziar::nailang::NailangRuntimeBase>&& runtime) :
    Program(program), CLContext(context), Runtime(std::move(runtime))
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
    using xziar::nailang::BlockContent;
    for (const auto [meta, tmp] : stub.Program->Program)
    {
        if (tmp.GetType() != BlockContent::Type::Block)
            continue;
        const auto& block = *tmp.Get<xziar::nailang::Block>();
        if (block.Type == U"oclu.prepare"sv)
            continue;
    }
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

NLCLProgStub NLCLProcessor::ConfigureCL(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx)
{
    NLCLProgStub stub(prog, ctx, {});
    ConfigureCL(stub);
    return stub;
}

oclProgram NLCLProcessor::CompileProgram(const std::shared_ptr<NLCLProgram>& prog, const oclContext& ctx)
{
    const auto stub = ConfigureCL(prog, ctx);
    //TODO: 
    return oclProgram();
}


}