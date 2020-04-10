#include "oclPch.h"
#include "oclNLCL.h"
#include "oclNLCLRely.h"
#include "StringCharset/Convert.h"
#include "StringCharset/Detect.h"

namespace oclu
{

NLCLProcessor::NailangHolder::NailangHolder(common::mlog::MiniLogger<false>& logger) :
    Logger(logger),
    Context(std::make_shared<xziar::nailang::CompactEvaluateContext>()),
    Runtime(std::make_unique<xziar::nailang::NailangRuntimeBase>(Context))
{ }
NLCLProcessor::NailangHolder::~NailangHolder()
{ }

void NLCLProcessor::NailangHolder::Parse(std::u32string source)
{
    Source = std::move(source);
    NLCLParser::GetBlock(MemPool, Source, Program);
    Logger.verbose(u"Parse done, get [{}] Blocks.\n", Program.Size());
}


NLCLProcessor::NLCLProcessor(common::mlog::MiniLogger<false>&& logger, common::span<const std::byte> source):
    Logger(std::move(logger))
{
    const auto encoding = common::strchset::DetectEncoding(source);
    Logger.info(u"Detected encoding[{}].\n", common::str::getCharsetName(encoding));
    auto u32src = common::strchset::to_u32string(source, encoding);
    Logger.info(u"Translate into [utf32] for [{}] chars.\n", u32src.size());
    
    InitHolder();
    Holder->Parse(std::move(u32src));
}
void NLCLProcessor::InitHolder()
{
    Holder = std::make_unique<NailangHolder>(Logger);
}
NLCLProcessor::NLCLProcessor(common::span<const std::byte> source) :
    NLCLProcessor({ u"NLCLPorc", { common::mlog::GetConsoleBackend() } }, source)
{
}
NLCLProcessor::~NLCLProcessor()
{
}

void NLCLProcessor::ConfigureCL()
{
}

std::u32string NLCLProcessor::GenerateCL()
{
    return std::u32string();
}

oclProgram NLCLProcessor::CompileProgram()
{
    return oclProgram();
}


}