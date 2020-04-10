#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "Nailang/NailangRuntime.h"

namespace oclu
{


class OCLUAPI NLCLParser : xziar::nailang::BlockParser
{
    using BlockParser::BlockParser;
public:
    static void GetBlock(xziar::nailang::MemoryPool& pool, const std::u32string_view src, xziar::nailang::Block& dst)
    {
        common::parser::ParserContext context(src);
        NLCLParser parser(pool, context);
        parser.ParseContentIntoBlock<true>(dst);
    }
};

class OCLUAPI NLCLProcessor::NailangHolder
{
    friend class NLCLProcessor;
protected:
    common::mlog::MiniLogger<false>& Logger;
    xziar::nailang::MemoryPool MemPool;
    std::shared_ptr<xziar::nailang::EvaluateContext> Context;
    std::unique_ptr<xziar::nailang::NailangRuntimeBase> Runtime;
    std::u32string Source;
    xziar::nailang::Block Program;
public:
    NailangHolder(common::mlog::MiniLogger<false>& logger);
    virtual ~NailangHolder();
    virtual void Parse(std::u32string source);
};


}