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


class OCLUAPI NLCLEvalContext : public xziar::nailang::CompactEvaluateContext
{
protected:
    const oclDevice Device;
    xziar::nailang::Arg LookUpCLArg(xziar::nailang::detail::VarLookup var) const;
public:
    NLCLEvalContext(const oclDevice dev) : Device(dev) { }
    xziar::nailang::Arg LookUpArgInside(xziar::nailang::detail::VarLookup var) const override;

};

class OCLUAPI NLCLRuntime : public xziar::nailang::NailangRuntimeBase
{
public:
    using xziar::nailang::NailangRuntimeBase::NailangRuntimeBase;
    NLCLRuntime(const oclDevice dev);
    ~NLCLRuntime() override;

    void HandleRawBlock(const xziar::nailang::RawBlock& block, common::span<const xziar::nailang::FuncCall> metas) override;
    xziar::nailang::Arg EvaluateFunc(const std::u32string_view func, common::span<const xziar::nailang::Arg> args, 
        common::span<const xziar::nailang::FuncCall> metas, const xziar::nailang::NailangRuntimeBase::FuncTarget target) override;
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
    const oclDevice Device;
    std::unique_ptr<NLCLRuntime> Runtime;
public:
    NLCLProgStub(const std::shared_ptr<NLCLProgram>& program, const oclDevice dev, std::unique_ptr<NLCLRuntime>&& runtime);
    NLCLProgStub(NLCLProgStub&&) = default;
    virtual ~NLCLProgStub();
};


}