#pragma once
#include "oclRely.h"
#include "oclNLCL.h"
#include "oclKernelDebug.h"
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
    [[nodiscard]] xziar::nailang::Arg LookUpArgInside(xziar::nailang::detail::VarLookup var) const override;
public:
    NLCLEvalContext(oclDevice dev);
    ~NLCLEvalContext() override;

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

class ReplaceExtension
{
public:
    virtual ~ReplaceExtension() { }
    virtual std::u32string ReplaceFunc(const std::u32string_view func, const common::span<const std::u32string_view> args, void* cookie) = 0;
    virtual void Finish(void*) { }
};
struct BlockCookie
{
    uint32_t Type;
    constexpr BlockCookie() noexcept : Type(0) { }
    constexpr BlockCookie(const uint32_t type) noexcept : Type(type) { }
    template<typename T>
    static T* Get(void* ptr) noexcept
    {
        static_assert(std::is_base_of_v<BlockCookie, T>);
        if (ptr != nullptr)
        {
            const auto ret = reinterpret_cast<T*>(ptr);
            if (ret->Type == T::TYPE)
                return ret;
        }
        return nullptr;
    }
};
struct KernelCookie : public BlockCookie
{
    static constexpr uint32_t TYPE = 1;
    std::map<std::u32string, std::u32string, std::less<>> Injects;
    std::unique_ptr<ReplaceExtension> SubgroupSolver;
    uint32_t WorkgroupSize = 0;
    uint8_t  SubgroupSize = 0;
    KernelCookie() noexcept : BlockCookie(TYPE) { }
};


class OCLUAPI NLCLRuntime : public xziar::nailang::NailangRuntimeBase, public common::NonCopyable, protected xziar::nailang::ReplaceEngine
{
    friend class NLCLReplacer;
    friend class NLCLProcessor;
    friend class NLCLSubgroup;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using MetaFuncs = ::common::span<const xziar::nailang::FuncCall>;

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
    oclDebugManager DebugManager;
    bool EnableUnroll, AllowDebug;

    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, std::shared_ptr<NLCLEvalContext>&& evalCtx, const common::CLikeDefines& info);
    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;
    void ThrowByReplacerArgCount(const std::u32string_view call, const common::span<const std::u32string_view> args, 
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const;
    
    template<typename T, typename F, typename... Args>
    bool AddPatchedBlock(T& obj, std::u32string_view id, F generator, Args&&... args)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F, T&, Args...>, "need accept args and return u32string");
        if (const auto it = PatchedBlocks.find(id); it == PatchedBlocks.end())
        {
            PatchedBlocks.insert_or_assign(std::u32string(id), 
                (obj.*generator)(std::forward<Args>(args)...));
            return true;
        }
        return false;
    }
    template<typename F>
    bool AddPatchedBlock(std::u32string_view id, F&& generator)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F>, "need return u32string");
        if (const auto it = PatchedBlocks.find(id); it == PatchedBlocks.end())
        {
            PatchedBlocks.insert_or_assign(std::u32string(id), generator());
            return true;
        }
        return false;
    }

    [[nodiscard]] std::u32string SkipDebugPatch() const noexcept;
    [[nodiscard]] std::u32string DebugStringPatch(const std::u32string_view dbgId, const std::u32string_view formatter,
        common::span<const common::simd::VecDataInfo> args) noexcept;
    [[nodiscard]] std::u32string GenerateDebugString(const common::span<const std::u32string_view> args) const;
    void OnReplaceOptBlock(std::u32string& output, void* cookie, const std::u32string_view cond, const std::u32string_view content) override;
    void OnReplaceVariable(std::u32string& output, void* cookie, const std::u32string_view var) override;
    void OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args) override;

    xziar::nailang::Arg EvaluateFunc(const xziar::nailang::FuncCall& call, MetaFuncs metas, const FuncTarget target) override;
    void DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst, BlockCookie* cookie = nullptr);
    virtual void OutputConditions(MetaFuncs metas, std::u32string& dst) const;
    virtual void OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputTemplateKernel(const RawBlock& block, MetaFuncs metas, uint32_t extraInfo, std::u32string& dst);
public:
    [[nodiscard]] static std::u32string_view GetVecTypeName(common::simd::VecDataInfo info) noexcept;

    const bool SupportFP16, SupportFP64, SupportNVUnroll,
        SupportSubgroupKHR, SupportSubgroupIntel, SupportSubgroup8Intel, SupportSubgroup16Intel, SupportBasicSubgroup;
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, oclDevice dev, const common::CLikeDefines& info);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
    void ProcessRawBlock(const RawBlock& block, MetaFuncs metas);
    std::optional<common::simd::VecDataInfo> ParseVecType(const std::u32string_view type) const noexcept;

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