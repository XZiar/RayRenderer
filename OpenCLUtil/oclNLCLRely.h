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
    friend class NLCLRuntime;
protected:
    oclDevice Device;
    xziar::nailang::Arg LookUpCLArg(xziar::nailang::detail::VarLookup var) const;
public:
    NLCLEvalContext(oclDevice dev) : Device(dev) { }
    xziar::nailang::Arg LookUpArgInside(xziar::nailang::detail::VarLookup var) const override;

};

class OCLUAPI NLCLRuntime : public xziar::nailang::NailangRuntimeBase
{
protected:
    common::mlog::MiniLogger<false>& Logger;
    oclDevice Device;
    std::vector<bool> EnabledExtensions;
    std::vector<const xziar::nailang::RawBlock*> GlobalBlocks;
    std::vector<const xziar::nailang::RawBlock*> StructBlocks;
    std::vector<const xziar::nailang::RawBlock*> KernelBlocks;
    std::vector<const xziar::nailang::RawBlock*> KernelTemplateBlocks;
    
    void HandleRawBlock(const xziar::nailang::RawBlock& block, common::span<const xziar::nailang::FuncCall> metas) override;
    xziar::nailang::Arg EvaluateFunc(const std::u32string_view func, common::span<const xziar::nailang::Arg> args, 
        common::span<const xziar::nailang::FuncCall> metas, const FuncTarget target) override;
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext);
    bool EnableExtension(std::u32string_view ext);
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
    template<typename F>
    void ForEachBlockType(const std::u32string_view type, F&& func)
    {
        using xziar::nailang::Block;
        static_assert(std::is_invocable_v<F, const Block&, common::span<const xziar::nailang::FuncCall>>,
            "nedd to accept block and meta.");
        using xziar::nailang::BlockContent;
        for (const auto& [meta, tmp] : Program)
        {
            if (tmp.GetType() != BlockContent::Type::Block)
                continue;
            const auto& block = *tmp.template Get<Block>();
            if (block.Type != type)
                continue;
            func(block, meta);
        }
    }
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
    NLCLProgStub(const std::shared_ptr<NLCLProgram>& program, oclDevice dev, std::unique_ptr<NLCLRuntime>&& runtime);
    NLCLProgStub(NLCLProgStub&&) = default;
    virtual ~NLCLProgStub();
};


}