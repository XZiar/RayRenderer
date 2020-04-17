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


class OCLUAPI NLCLProgram
{
    friend class NLCLProcessor;
    friend class NLCLProgStub;
protected:
    xziar::nailang::MemoryPool MemPool;
    std::u32string Source;
    xziar::nailang::Block Program;
public:
    NLCLProgram(std::u32string&& source);
    ~NLCLProgram();
};


class OCLUAPI NLCLProgStub
{
    friend class NLCLProcessor;
protected:
    xziar::nailang::MemoryPool MemPool;
    std::shared_ptr<NLCLProgram> Program;
    oclContext CLContext;
    std::unique_ptr<xziar::nailang::NailangRuntimeBase> Runtime;
public:
    NLCLProgStub(const std::shared_ptr<NLCLProgram>& program, const oclContext& context, std::unique_ptr<xziar::nailang::NailangRuntimeBase>&& runtime);
    NLCLProgStub(NLCLProgStub&&) = default;
    virtual ~NLCLProgStub();
};


}