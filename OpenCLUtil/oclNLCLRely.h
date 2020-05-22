#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"

namespace oclu
{
#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

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
    NLCLEvalContext(oclDevice dev);
    ~NLCLEvalContext() override;
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

class NLCLRuntime;
class OCLUAPI NLCLReplacer : public xziar::nailang::ReplaceEngine
{
    friend class NLCLRuntime;
protected:
    NLCLRuntime& Runtime;
    const bool SupportFP16, SupportFP64, SupportNVUnroll, SupportSubgroupKHR, SupportSubgroupIntel, SupportSubgroup8Intel, SupportSubgroup16Intel;
    bool EnableUnroll;
    void ThrowByArgCount(const std::u32string_view func, const common::span<std::u32string_view> args, const size_t count) const;
    void OnReplaceVariable(std::u32string& output, const std::u32string_view var) override;
    void OnReplaceFunction(std::u32string& output, const std::u32string_view func, const common::span<std::u32string_view> args) override;
public:
    NLCLReplacer(NLCLRuntime& runtime);
    ~NLCLReplacer() override;

    std::optional<common::simd::VecDataInfo> ParseVecType(const std::u32string_view type) const noexcept;
    static std::u32string_view GetVecTypeName(common::simd::VecDataInfo info) noexcept;
    std::u32string GenerateSubgroupShuffle(const common::span<std::u32string_view> args, const bool needShuffle) const;
    std::u32string GenerateDebugString(const common::span<std::u32string_view> args) const;

    using ReplaceEngine::ProcessVariable;
    using ReplaceEngine::ProcessFunction;
};


class OCLUAPI NLCLRuntime : public xziar::nailang::NailangRuntimeBase
{
    friend class NLCLReplacer;
    friend class NLCLProcessor;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using MetaFuncs = common::span<const xziar::nailang::FuncCall>;

    common::mlog::MiniLogger<false>& Logger;
    oclDevice Device;
    std::vector<bool> EnabledExtensions;
    std::vector<OutputBlock> OutputBlocks;
    std::vector<OutputBlock> StructBlocks;
    std::vector<OutputBlock> KernelBlocks;
    std::vector<OutputBlock> TemplateBlocks;
    std::vector<OutputBlock> KernelStubBlocks;
    std::map<std::u32string, std::u32string, std::less<>> PatchedBlocks;
    std::vector<std::pair<std::string, KernelArgStore>> CompiledKernels;
    std::unique_ptr<NLCLReplacer> Replacer;
    bool AllowDebug = false;

    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, std::shared_ptr<NLCLEvalContext>&& evalCtx, const common::CLikeDefines& info);
    //void OnRawBlock(const xziar::nailang::RawBlock& block, common::span<const xziar::nailang::FuncCall> metas) override;
    xziar::nailang::Arg EvaluateFunc(const xziar::nailang::FuncCall& call, MetaFuncs metas, const FuncTarget target) override;
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;

    template<typename F, typename... Args>
    void AddPatchedBlock(std::u32string_view id, F&& generator, Args&&... args)
    {
        if (const auto it = PatchedBlocks.find(id); it == PatchedBlocks.end())
            PatchedBlocks.insert_or_assign(std::u32string(id), generator(std::forward<Args>(args)...));
    }

    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    void DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst) const;
    virtual std::unique_ptr<NLCLReplacer> PrepareRepalcer();
    virtual void OutputConditions(MetaFuncs metas, std::u32string& dst) const;
    virtual void OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputTemplateKernel(const RawBlock& block, MetaFuncs metas, uint32_t extraInfo, std::u32string& dst);
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, const common::CLikeDefines& info);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
    [[nodiscard]] bool CheckExtensionEnabled(std::string_view ext) const;
    void ProcessRawBlock(const RawBlock& block, MetaFuncs metas);

    std::string GenerateOutput();
};


class OCLUAPI NLCLBaseResult : public NLCLResult
{
protected:
    std::shared_ptr<xziar::nailang::EvaluateContext> EvalContext;
public:
    NLCLBaseResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx);
    ~NLCLBaseResult() override;
    NLCLResult::ResultType QueryResult(std::u32string_view name) const override;
};

class OCLUAPI NLCLUnBuildResult : public NLCLBaseResult
{
protected:
    std::shared_ptr<xziar::nailang::EvaluateContext> EvalContext;
    std::string Source;
public:
    NLCLUnBuildResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, std::string&& source);
    ~NLCLUnBuildResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuiltResult : public NLCLBaseResult
{
protected:
    std::shared_ptr<xziar::nailang::EvaluateContext> EvalContext;
    oclProgram Prog;
public:
    NLCLBuiltResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, oclProgram prog);
    ~NLCLBuiltResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuildFailResult : public NLCLUnBuildResult
{
protected:
    std::shared_ptr<xziar::nailang::EvaluateContext> EvalContext;
    std::shared_ptr<common::BaseException> Exception;
public:
    NLCLBuildFailResult(std::shared_ptr<xziar::nailang::EvaluateContext> evalCtx, std::string&& source, std::shared_ptr<common::BaseException> ex);
    ~NLCLBuildFailResult() override;
    oclProgram GetProgram() const override;
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
            "need to accept block/rawblock and meta.");
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


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}