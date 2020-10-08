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

class NLCLContext;
class NLCLRuntime;

class OCLUAPI NLCLParser : xziar::nailang::BlockParser
{
    using BlockParser::BlockParser;
public:
    static void GetBlock(xziar::nailang::MemoryPool& pool, const std::u32string_view src, xziar::nailang::Block& dst)
    {
        common::parser::ParserContext context(src);
        NLCLParser parser(pool, context);
        parser.ParseContentIntoBlock(true, dst);
    }
};


struct OutputBlock
{
    using MetaFuncs = ::common::span<const xziar::nailang::FuncCall>;
    struct BlockInfo
    {
        virtual ~BlockInfo() { }
    };
    const xziar::nailang::RawBlock* Block;
    MetaFuncs MetaFunc;
    std::unique_ptr<BlockInfo> ExtraInfo;
    OutputBlock(const xziar::nailang::RawBlock* block, MetaFuncs meta, std::unique_ptr<BlockInfo> extra = {}) noexcept :
        Block(block), MetaFunc(meta), ExtraInfo(std::move(extra)) { }
};
struct TemplateBlockInfo : public OutputBlock::BlockInfo
{
    std::vector<std::u32string_view> ReplaceArgs;
    std::vector<std::pair<std::u32string_view, xziar::nailang::RawArg>> PreAssignArgs;
    TemplateBlockInfo(common::span<const xziar::nailang::RawArg> args);
    ~TemplateBlockInfo() override {}
};


struct OCLUAPI KernelContext
{
    struct NamedText
    {
        std::u32string ID;
        std::u32string Content;
    };
    std::vector<NamedText> Attributes;
    KernelArgStore Args;
    KernelArgStore TailArgs; // args that won't be recorded
    std::vector<NamedText> BodyPrefixes;
    std::vector<NamedText> BodySuffixes;
    uint32_t WorkgroupSize = 0;

    static bool Add(std::vector<NamedText>& container, const std::u32string_view id, std::u32string_view content);
    forceinline bool AddAttribute(const std::u32string_view id, std::u32string_view content)
    {
        return Add(Attributes, id, content);
    }
    forceinline bool AddBodyPrefix(const std::u32string_view id, std::u32string_view content)
    {
        return Add(BodyPrefixes, id, content);
    }
    forceinline bool AddBodySuffix(const std::u32string_view id, std::u32string_view content)
    {
        return Add(BodySuffixes, id, content);
    }
    template<typename... Ts>
    forceinline void AddArg(Ts&&... args)
    {
        Args.AddArg(std::forward<Ts>(args)...);
    }
    template<typename... Ts>
    forceinline void AddTailArg(Ts&&... args)
    {
        TailArgs.AddArg(std::forward<Ts>(args)...);
    }
    forceinline void SetDebugBuffer(uint32_t size)
    {
        Args.HasDebug = true;
        Args.DebugBuffer = std::max(Args.DebugBuffer, size);
    }
};


class ReplaceResult
{
    common::str::StrVariant<char32_t> Str;
    bool IsSuccess, AllowFallback;
public:
    ReplaceResult() noexcept : IsSuccess(false), AllowFallback(true) { }
    template<typename T>
    ReplaceResult(T&& str) noexcept : 
        Str(std::forward<T>(str)), IsSuccess(true), AllowFallback(false) { }
    template<typename T>
    ReplaceResult(T&& str, const bool allowFallback) noexcept :
        Str(std::forward<T>(str)), IsSuccess(false), AllowFallback(allowFallback) { }
    constexpr explicit operator bool() const noexcept { return IsSuccess; }
    constexpr bool CheckAllowFallback() const noexcept { return AllowFallback; }
    constexpr std::u32string_view GetStr() const noexcept { return Str.StrView(); }
    void SetStr(common::str::StrVariant<char32_t> str) noexcept { Str = std::move(str); }
};
class OCLUAPI KernelExtension
{
    friend class NLCLRuntime;
    friend class NLCLProgStub;
    friend class NLCLContext;
protected:
    NLCLContext& Context;
public:
    KernelExtension(NLCLContext& context);
    virtual ~KernelExtension();
    virtual bool KernelMeta(NLCLRuntime&, const xziar::nailang::FuncCall&, KernelContext&)
    {
        return false;
    }
    virtual ReplaceResult ReplaceFunc(NLCLRuntime&, std::u32string_view, const common::span<const std::u32string_view>)
    {
        return {};
    }
    virtual void FinishKernel(NLCLRuntime&, KernelContext&) { }
private:
    uint32_t ID;
};
class OCLUAPI NLCLExtension : public KernelExtension
{
    friend class NLCLRuntime;
public:
    using KernelExtension::KernelExtension;
    virtual ~NLCLExtension();
    [[nodiscard]] virtual std::optional<xziar::nailang::Arg> NLCLFunc(NLCLRuntime&, const xziar::nailang::FuncCall&,
        common::span<const xziar::nailang::FuncCall>, const xziar::nailang::NailangRuntimeBase::FuncTargetType)
    {
        return {};
    }
    virtual void FinishNLCL(NLCLRuntime&) { }
    virtual void OnCompile(oclProgStub&) { }

    using KerExtGen  = std::function<std::unique_ptr<KernelExtension>(common::mlog::MiniLogger<false>&, NLCLContext&, KernelContext&)>;
    using NLCLExtGen = std::function<std::unique_ptr<NLCLExtension>(common::mlog::MiniLogger<false>&, NLCLContext&, const xziar::nailang::Block&)>;
    static uint32_t RegisterKernelExtenstion(KerExtGen creator) noexcept;
    static uint32_t RegisterNLCLExtenstion(NLCLExtGen creator) noexcept;
};


class OCLUAPI COMMON_EMPTY_BASES NLCLContext : public common::NonCopyable, public xziar::nailang::CompactEvaluateContext
{
    friend class NLCLProcessor;
    friend class NLCLRuntime;
    friend class NLCLProgStub;
protected:
    std::vector<std::unique_ptr<NLCLExtension>> NLCLExts;
    xziar::nailang::Arg LookUpCLArg(const xziar::nailang::LateBindVar& var) const;
public:
    const oclDevice Device;
    const bool SupportFP16, SupportFP64, SupportNVUnroll,
        SupportSubgroupKHR, SupportSubgroupIntel, SupportSubgroup8Intel, SupportSubgroup16Intel, SupportBasicSubgroup;
    NLCLContext(oclDevice dev, const common::CLikeDefines& info);
    ~NLCLContext() override;
    [[nodiscard]] xziar::nailang::Arg LookUpArg(const xziar::nailang::LateBindVar& var) const override;
    template<typename T, typename F, typename... Args>
    bool AddPatchedBlock(T& obj, std::u32string_view id, F generator, Args&&... args)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F, T&, Args...>, "need accept args and return u32string");
        if (const auto it = PatchedBlocks.find(id); it == PatchedBlocks.end())
        {
            PatchedBlocks.insert_or_assign(std::u32string(id), (obj.*generator)(std::forward<Args>(args)...));
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
    template<typename T>
    T* GetNLCLExt() const
    {
        for (const auto& ext : NLCLExts)
        {
            if (ext->ID == T::ID)
                return static_cast<T*>(ext.get());
        }
        return nullptr;
    }
    struct VecTypeResult
    {
        common::simd::VecDataInfo Info;
        bool TypeSupport;
        constexpr explicit operator bool() const noexcept { return Info.Bit != 0; }
    };
    VecTypeResult ParseVecType(const std::u32string_view type) const noexcept;
    [[nodiscard]] static std::u32string_view GetCLTypeName(common::simd::VecDataInfo info) noexcept;
protected:
    std::vector<bool> EnabledExtensions;
    std::vector<OutputBlock> OutputBlocks;
    std::vector<OutputBlock> StructBlocks;
    std::vector<OutputBlock> KernelBlocks;
    std::vector<OutputBlock> TemplateBlocks;
    std::vector<OutputBlock> KernelStubBlocks;
    std::map<std::u32string, std::u32string, std::less<>> PatchedBlocks;
    std::vector<std::string> CompilerFlags;
    std::vector<std::pair<std::string, KernelArgStore>> CompiledKernels;
    bool EnableUnroll, AllowDebug;
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
    KernelContext Context;
    std::vector<std::unique_ptr<KernelExtension>> Extensions;
    KernelCookie() noexcept : BlockCookie(TYPE) { }
};


class OCLUAPI COMMON_EMPTY_BASES NLCLRuntime : public xziar::nailang::NailangRuntimeBase, public common::NonCopyable, protected xziar::nailang::ReplaceEngine
{
    friend class KernelExtension;
    friend class NLCLProcessor;
    friend class NLCLSubgroup;
    friend class NLCLBaseResult;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    NLCLContext& Context;
    common::mlog::MiniLogger<false>& Logger;

    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    void HandleException(const xziar::nailang::NailangParseException& ex) const override;
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;
    [[nodiscard]] xziar::nailang::Arg LookUpArg(const xziar::nailang::LateBindVar& var) const override;
    void ThrowByReplacerArgCount(const std::u32string_view call, const common::span<const std::u32string_view> args,
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const;
    common::simd::VecDataInfo ParseVecType(const std::u32string_view type, 
        std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo = {}) const;

    void OnReplaceOptBlock(std::u32string& output, void* cookie, const std::u32string_view cond, const std::u32string_view content) override;
    void OnReplaceVariable(std::u32string& output, void* cookie, const std::u32string_view var) override;
    void OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args) override;

    void OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas) override;
    xziar::nailang::Arg EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTargetType target) override;
    void DirectOutput(const RawBlock& block, MetaFuncs metas, std::u32string& dst, BlockCookie* cookie = nullptr);
    virtual void OutputConditions(MetaFuncs metas, std::u32string& dst) const;
    virtual void OutputGlobal(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputStruct(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void HandleKernelMeta(const FuncCall& meta, KernelContext& kerCtx, const std::vector<std::unique_ptr<KernelExtension>>& kerExts);
    virtual void StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg);
    virtual void OutputKernel(const RawBlock& block, MetaFuncs metas, std::u32string& dst);
    virtual void OutputTemplateKernel(const RawBlock& block, [[maybe_unused]] MetaFuncs metas, [[maybe_unused]] const OutputBlock::BlockInfo& extraInfo, std::u32string& dst);
public:
    NLCLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLCLContext> evalCtx);
    ~NLCLRuntime() override;
    bool EnableExtension(std::string_view ext, std::u16string_view desc = {});
    bool EnableExtension(std::u32string_view ext, std::u16string_view desc = {});
    void ProcessRawBlock(const RawBlock& block, MetaFuncs metas);

    std::string GenerateOutput();
};


class OCLUAPI NLCLBaseResult : public NLCLResult
{
protected:
    std::shared_ptr<NLCLContext> Context;
public:
    NLCLBaseResult(const std::shared_ptr<NLCLContext>& context);
    ~NLCLBaseResult() override;
    NLCLResult::ResultType QueryResult(std::u32string_view name) const override;
};

class OCLUAPI NLCLUnBuildResult : public NLCLBaseResult
{
protected:
    std::string Source;
public:
    NLCLUnBuildResult(const std::shared_ptr<NLCLContext>& context, std::string&& source);
    ~NLCLUnBuildResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuiltResult : public NLCLBaseResult
{
protected:
    oclProgram Prog;
public:
    NLCLBuiltResult(const std::shared_ptr<NLCLContext>& context, oclProgram prog);
    ~NLCLBuiltResult() override;
    std::string_view GetNewSource() const noexcept override;
    oclProgram GetProgram() const override;
};

class OCLUAPI NLCLBuildFailResult : public NLCLUnBuildResult
{
protected:
    std::shared_ptr<common::ExceptionBasicInfo> Exception;
public:
    NLCLBuildFailResult(const std::shared_ptr<NLCLContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex);
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


class OCLUAPI NLCLProgStub : public common::NonCopyable
{
    friend class NLCLProcessor;
private:
    void Prepare();
protected:
    xziar::nailang::MemoryPool MemPool;
    std::shared_ptr<const NLCLProgram> Program;
    std::shared_ptr<NLCLContext> Context;
    std::unique_ptr<NLCLRuntime> Runtime;
public:
    NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program, std::shared_ptr<NLCLContext>&& context, std::unique_ptr<NLCLRuntime>&& runtime);
    NLCLProgStub(const std::shared_ptr<const NLCLProgram>& program, const std::shared_ptr<NLCLContext> context, common::mlog::MiniLogger<false>& logger);
    NLCLProgStub(NLCLProgStub&&) = default;
    virtual ~NLCLProgStub();

    constexpr const std::shared_ptr<const NLCLProgram>& GetProgram() const noexcept
    {
        return Program;
    }
    constexpr const std::shared_ptr<NLCLContext>& GetContext() const noexcept
    {
        return Context;
    }
    NLCLRuntime& GetRuntime() const noexcept
    {
        return *Runtime;
    }
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}