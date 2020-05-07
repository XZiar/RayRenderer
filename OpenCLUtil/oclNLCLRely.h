#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "Nailang/NailangParser.h"
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
    [[nodiscard]] xziar::nailang::Arg LookUpArg(xziar::nailang::detail::VarHolder var) const override;

};

struct OutputBlock
{
    const xziar::nailang::RawBlock* Block;
    const xziar::nailang::FuncCall* MetaPtr;
    uint32_t MetaCount;
    uint32_t ExtraInfo;
    constexpr OutputBlock(const xziar::nailang::RawBlock* block, common::span<const xziar::nailang::FuncCall> meta, uint32_t extra = 0) noexcept :
        Block(block), MetaPtr(meta.data()), MetaCount(gsl::narrow_cast<uint32_t>(meta.size())), ExtraInfo(extra) {}
};

class OCLUAPI NLCLRuntime : public xziar::nailang::NailangRuntimeBase
{
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using MetaFuncs = common::span<const xziar::nailang::FuncCall>;

    class OCLUAPI NLCLReplacer : public xziar::nailang::ReplaceEngine
    {
    public:
        const NLCLRuntime& Runtime;
        NLCLReplacer(std::u32string_view source, const NLCLRuntime& runtime) : ReplaceEngine(source), Runtime(runtime) { }
        ~NLCLReplacer() override;
        void OnReplaceVariable(const std::u32string_view var) override;
        void OnReplaceFunction(const std::u32string_view func, const common::span<std::u32string_view> args) override;
        using ReplaceEngine::ProcessVariable;
        using ReplaceEngine::ProcessFunction;
    };

    common::mlog::MiniLogger<false>& Logger;
    oclDevice Device;
    std::vector<bool> EnabledExtensions;
    std::vector<OutputBlock> OutputBlocks;
    std::vector<OutputBlock> StructBlocks;
    std::vector<OutputBlock> KernelBlocks;
    std::vector<OutputBlock> TemplateBlocks;
    std::vector<OutputBlock> KernelStubBlocks;
    
    //void OnRawBlock(const xziar::nailang::RawBlock& block, common::span<const xziar::nailang::FuncCall> metas) override;
    xziar::nailang::Arg EvaluateFunc(const xziar::nailang::FuncCall& call, MetaFuncs metas, const FuncTarget target) override;
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;

    bool CheckExtension(std::string_view ext, std::u16string_view desc) const;
    void DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const;
    virtual void OutputConditions(MetaFuncs metas, std::u32string& dst) const;
    virtual std::u32string DoReplaceVariable(const std::u32string_view src) const;
    virtual std::u32string DoReplaceFunction(const std::u32string_view src) const;
    virtual void OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputTemplateKernel(const RawBlock& block, MetaFuncs metas, uint32_t extraInfo, std::u32string& dst);
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext);
    bool EnableExtension(std::u32string_view ext);
    [[nodiscard]] bool CheckExtensionEnabled(std::string_view ext) const;
    void ProcessRawBlock(const RawBlock& block, MetaFuncs metas);

    std::string GenerateOutput();
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
    template<bool IsBlock, bool FullMatch, typename F>
    forceinline void ForEachBlockType(const std::u32string_view type, F&& func) const
    {
        using Target = std::conditional_t<IsBlock, xziar::nailang::Block, xziar::nailang::RawBlock>;
        static_assert(std::is_invocable_v<F, const Target&, common::span<const xziar::nailang::FuncCall>>,
            "nedd to accept block/rawblock and meta.");
        using xziar::nailang::BlockContent;
        constexpr auto contentType = IsBlock ? BlockContent::Type::Block : BlockContent::Type::RawBlock;
        for (const auto& [meta, tmp] : Program)
        {
            if (tmp.GetType() != contentType)
                continue;
            const auto& block = *tmp.template Get<Target>();
            if ((FullMatch && block.Type == type) || 
                (!FullMatch && common::str::IsBeginWith(block.Type, type)))
                func(block, meta);
        }
    }
};


class OCLUAPI NLCLProgStub
{
    friend class NLCLProcessor;
protected:
    xziar::nailang::MemoryPool MemPool;
    std::shared_ptr<const NLCLProgram> Program;
    const oclDevice Device;
    std::unique_ptr<NLCLRuntime> Runtime;

public:
    NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program, oclDevice dev, std::unique_ptr<NLCLRuntime>&& runtime);
    NLCLProgStub(NLCLProgStub&&) = default;
    virtual ~NLCLProgStub();
};


}