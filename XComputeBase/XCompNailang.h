#pragma once
#include "XCompRely.h"
#include "XCompNailang.h"
#include "XCompDebug.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "common/CLikeConfig.hpp"


namespace xcomp
{
#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class XCNLContext;
class XCNLRuntime;
class XCNLProgStub;


struct OutputBlock
{
    using MetaFuncs = ::common::span<const xziar::nailang::FuncCall>;
    enum class BlockType : uint8_t { None, Global, Struct, Instance, Template };
    struct BlockInfo
    {
        virtual ~BlockInfo() { }
    };

    const xziar::nailang::RawBlock* Block;
    MetaFuncs MetaFunc;
    std::vector<std::pair<const xziar::nailang::LateBindVar*, xziar::nailang::RawArg>> PreAssignArgs;
    std::unique_ptr<BlockInfo> ExtraInfo;
    BlockType Type;
    bool ReplaceVar = false, ReplaceFunc = false;

    OutputBlock(const xziar::nailang::RawBlock* block, MetaFuncs meta, BlockType type) noexcept :
        Block(block), MetaFunc(meta), Type(type) { }
    forceinline std::u32string_view Name() const noexcept { return Block->Name; }
    forceinline std::u32string_view GetBlockTypeName() const noexcept { return BlockTypeName(Type); }
    
    XCOMPBASAPI static std::u32string_view BlockTypeName(const BlockType type) noexcept;
};


class XCOMPBASAPI NamedTextHolder
{
private:
    common::StringPool<char32_t> Names;
    std::vector<uint32_t> Dependencies;
public:
    struct NamedText
    {
        friend NamedTextHolder;
        std::u32string Content;
        constexpr uint32_t DependCount() const noexcept { return Dependency.second; }
    protected:
        NamedText(std::u32string_view content, common::StringPiece<char32_t> idstr, size_t offset, size_t cnt);
    private:
        common::StringPiece<char32_t> IDStr;
        std::pair<uint32_t, uint32_t> Dependency = { 0,0 };
    };
    virtual ~NamedTextHolder();
protected:
    forceinline std::u32string_view GetID(const NamedText& item) const noexcept
    {
        return Names.GetStringView(item.IDStr);
    }
    forceinline std::u32string_view GetDepend(const NamedText& item, const std::vector<NamedText>& container, const uint32_t idx) const noexcept
    {
        Expects(idx < item.DependCount());
        return GetID(container[Dependencies[(size_t)item.Dependency.first + idx]]);
    }
    forceinline bool CheckExists(std::vector<NamedText>& container, std::u32string_view id) const noexcept
    {
        for (const auto& item : container)
        {
            if (GetID(item) == id)
                return true;
        }
        return false;
    }
    forceinline bool Add(std::vector<NamedText>& container, std::u32string_view id, common::str::StrVariant<char32_t> content, common::span<const std::u32string_view> depends)
    {
        // check exists
        if (CheckExists(container, id))
            return false;
        const auto ret = CheckDependencies(container, depends);
        if (ret.index() == 0)
            ThrowDepEx(id, std::get<0>(ret));
        AddNoCheck(container, id, std::move(content), std::get<1>(ret), depends.size());
        return true;
    }
    std::variant<std::u32string_view, size_t> CheckDependencies(std::vector<NamedText>& container, common::span<const std::u32string_view> depends) noexcept;
    void AddNoCheck(std::vector<NamedText>& container, std::u32string_view id, common::str::StrVariant<char32_t> content, size_t offset, size_t size);
    void ThrowDepEx(std::u32string_view id, std::u32string_view dep) const;
    void Write(std::u32string& output, const std::vector<NamedText>& container) const;

    virtual void Write(std::u32string& output, const NamedText& item) const;
};


class InstanceContext : public NamedTextHolder
{
public:
    virtual ~InstanceContext() {}

    forceinline bool AddBodyPrefix(const std::u32string_view id, std::u32string_view content, common::span<const std::u32string_view> depends = {})
    {
        return Add(BodyPrefixes, id, content, depends);
    }
    forceinline bool AddBodySuffix(const std::u32string_view id, std::u32string_view content, common::span<const std::u32string_view> depends = {})
    {
        return Add(BodySuffixes, id, content, depends);
    }

    forceinline void WritePrefixes(std::u32string& output) const
    {
        Write(output, BodyPrefixes);
    }
    forceinline void WriteSuffixes(std::u32string& output) const
    {
        Write(output, BodySuffixes);
    }

    std::u32string Content;
protected:
    std::vector<NamedText> BodyPrefixes;
    std::vector<NamedText> BodySuffixes;
};


struct BlockCookie
{
    const OutputBlock& Block;
    BlockCookie(const OutputBlock& block) noexcept : Block(block) {}
    virtual ~BlockCookie() {}
    virtual InstanceContext* GetInstanceCtx() noexcept { return nullptr; }
};


struct NestedCookie : public BlockCookie
{
    InstanceContext* Context;
    NestedCookie(BlockCookie& parent, const OutputBlock& block) noexcept : 
        BlockCookie(block), Context(parent.GetInstanceCtx()) {}
    ~NestedCookie() override {}
    InstanceContext* GetInstanceCtx() noexcept override { return Context; }
};


class ReplaceResult
{
    common::str::StrVariant<char32_t> Str;
    common::str::StrVariant<char32_t> Depends; // depends of body prefix
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
    void SetDepends(common::str::StrVariant<char32_t> str) noexcept { Depends = std::move(str); }
};


class XCOMPBASAPI XCNLExtension
{
    friend XCNLRuntime;
    friend XCNLContext;
    friend XCNLProgStub;
protected:
    XCNLContext& Context;
public:
    XCNLExtension(XCNLContext& context);
    virtual ~XCNLExtension();
    virtual void  BeginXCNL(XCNLRuntime&) { }
    virtual void FinishXCNL(XCNLRuntime&) { }
    virtual void  BeginInstance(XCNLRuntime&, InstanceContext&) { } 
    virtual void FinishInstance(XCNLRuntime&, InstanceContext&) { }
    virtual void InstanceMeta(XCNLRuntime&, const xziar::nailang::FuncCall&, InstanceContext&) { }
    [[nodiscard]] virtual ReplaceResult ReplaceFunc(XCNLRuntime&, std::u32string_view, const common::span<const std::u32string_view>)
    {
        return {};
    }
    [[nodiscard]] virtual std::optional<xziar::nailang::Arg> XCNLFunc(XCNLRuntime&, const xziar::nailang::FuncCall&,
        common::span<const xziar::nailang::FuncCall>)
    {
        return {};
    }

    using XCNLExtGen = std::unique_ptr<XCNLExtension>(*)(common::mlog::MiniLogger<false>&, XCNLContext&);
    template<typename T>
    static uintptr_t RegisterXCNLExtension() noexcept
    {
        return RegExt(T::Create);
    }
private:
    static uintptr_t RegExt(XCNLExtGen creator) noexcept;
    uintptr_t ID;
};
#define XCNL_EXT_REG(type, ...)                                                         \
    static std::unique_ptr<XCNLExtension> Create(                                       \
        [[maybe_unused]] common::mlog::MiniLogger<false>& logger,                       \
        [[maybe_unused]] xcomp::XCNLContext& context)                                   \
    __VA_ARGS__                                                                         \
    inline static uintptr_t Dummy = xcomp::XCNLExtension::RegisterXCNLExtension<type>() \


class XCOMPBASAPI COMMON_EMPTY_BASES XCNLContext : public common::NonCopyable, public xziar::nailang::CompactEvaluateContext, public NamedTextHolder
{
    friend class XCNLProcessor;
    friend class XCNLRuntime;
    friend class XCNLProgStub;
public:
    XCNLContext(const common::CLikeDefines& info);
    ~XCNLContext() override;
    template<typename T, typename F, typename... Args>
    bool AddPatchedBlock(T& obj, std::u32string_view id, F generator, Args&&... args)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F, T&, Args...>, "need accept args and return u32string");
        if (CheckExists(PatchedBlocks, id))
            return false;
        AddNoCheck(PatchedBlocks, id, (obj.*generator)(std::forward<Args>(args)...), 0, 0);
        return true;
    }
    template<typename F>
    bool AddPatchedBlock(std::u32string_view id, F&& generator)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F>, "need return u32string");
        if (CheckExists(PatchedBlocks, id))
            return false;
        AddNoCheck(PatchedBlocks, id, generator(), 0, 0);
        return true;
    }
    template<typename T, typename F, typename... Args>
    forceinline bool AddPatchedBlockD(T& obj, std::u32string_view id, common::span<const std::u32string_view> depends, F generator, Args&&... args)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F, T&, Args...>, "need accept args and return u32string");
        const auto ret = PreCheckPatchedBlocks(id, depends);
        if (ret.has_value())
        {
            AddNoCheck(PatchedBlocks, id, (obj.*generator)(std::forward<Args>(args)...), ret.value(), depends.size());
            return true;
        }
        return false;
    }
    template<typename F>
    forceinline bool AddPatchedBlockD(std::u32string_view id, common::span<const std::u32string_view> depends, F&& generator)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F>, "need return u32string");
        const auto ret = PreCheckPatchedBlocks(id, depends);
        if (ret.has_value())
        {
            AddNoCheck(PatchedBlocks, id, generator(), ret.value(), depends.size());
            return true;
        }
        return false;
    }
    forceinline void WritePatchedBlock(std::u32string& output) const
    {
        NamedTextHolder::Write(output, PatchedBlocks);
    }
    template<typename T>
    [[nodiscard]] forceinline T* GetXCNLExt() const
    {
        return static_cast<T*>(FindExt(T::Create));
    }
    struct VecTypeResult
    {
        common::simd::VecDataInfo Info;
        bool TypeSupport;
        constexpr explicit operator bool() const noexcept { return Info.Bit != 0; }
    };
    [[nodiscard]] virtual VecTypeResult ParseVecType(const std::u32string_view type) const noexcept = 0;
    [[nodiscard]] virtual std::u32string_view GetVecTypeName(common::simd::VecDataInfo vec) const noexcept = 0;
protected:
    std::vector<std::unique_ptr<XCNLExtension>> Extensions;
    std::vector<OutputBlock> OutputBlocks;
    std::vector<OutputBlock> TemplateBlocks;
    std::vector<NamedText> PatchedBlocks;
private:
    [[nodiscard]] XCNLExtension* FindExt(XCNLExtension::XCNLExtGen func) const;
    std::optional<size_t> PreCheckPatchedBlocks(std::u32string_view id, common::span<const std::u32string_view> depends);
    void Write(std::u32string& output, const NamedText& item) const override;
};


class XCOMPBASAPI COMMON_EMPTY_BASES XCNLRuntime : public xziar::nailang::NailangRuntimeBase, public common::NonCopyable, protected xziar::nailang::ReplaceEngine
{
    friend class XCNLExtension;
    friend class XCNLProgStub;
protected:
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;
    using MetaFuncs = ::common::span<const FuncCall>;

    XCNLContext& XCContext;
    common::mlog::MiniLogger<false>& Logger;

    void HandleException(const xziar::nailang::NailangParseException& ex) const override;
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;
    void ThrowByReplacerArgCount(const std::u32string_view call, const common::span<const std::u32string_view> args,
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const;

    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    [[nodiscard]] common::simd::VecDataInfo ParseVecType(const std::u32string_view type,
        std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo = {}) const;
    [[nodiscard]] std::u32string_view GetVecTypeName(const std::u32string_view vname, 
        std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo = u"call [GetVecTypeName]") const;

    std::optional<xziar::nailang::Arg> CommonFunc(const std::u32string_view name, const FuncCall& call, MetaFuncs metas);
    std::optional<common::str::StrVariant<char32_t>> CommonReplaceFunc(const std::u32string_view name, const std::u32string_view call, 
        const common::span<const std::u32string_view> args, BlockCookie& cookie);
    void OutputConditions(MetaFuncs metas, std::u32string& dst) const;
    void DirectOutput(BlockCookie& cookie, std::u32string& dst);
    void ProcessInstance(BlockCookie& cookie);

    void OnReplaceOptBlock(std::u32string& output, void* cookie, std::u32string_view cond, std::u32string_view content) override;
    void OnReplaceVariable(std::u32string& output, void* cookie, std::u32string_view var) override;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) override;

    void OnRawBlock(const RawBlock& block, MetaFuncs metas) override;
    xziar::nailang::Arg EvaluateFunc(const FuncCall& call, MetaFuncs metas) override;

    [[nodiscard]] virtual OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept;
    [[nodiscard]] virtual std::unique_ptr<OutputBlock::BlockInfo> PrepareBlockInfo(OutputBlock& blk);
    [[nodiscard]] virtual std::unique_ptr<BlockCookie> PrepareInstance(const OutputBlock& block) = 0;
    virtual void HandleInstanceMeta(const FuncCall& meta, InstanceContext& ctx);
    virtual void BeforeOutputBlock(const OutputBlock& block, std::u32string& dst) const;
    virtual void OutputStruct   (BlockCookie& cookie, std::u32string& dst) = 0;
    virtual void OutputInstance (BlockCookie& cookie, std::u32string& dst) = 0;
    virtual void BeforeFinishOutput(std::u32string& prefix, std::u32string& content);
    //virtual void HandleOutputBlockMeta(const FuncCall& meta, InstanceCookie& cookie);
private:

public:
    XCNLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<XCNLContext> evalCtx);
    ~XCNLRuntime() override;
    void ProcessRawBlock(const RawBlock& block, MetaFuncs metas);

    std::string GenerateOutput();
};


class XCNLProgram
{
    friend XCNLProgStub;
private:
    MAKE_ENABLER();
    [[nodiscard]] XCOMPBASAPI static std::shared_ptr<XCNLProgram> Create_(std::u32string source);
protected:
    xziar::nailang::MemoryPool MemPool;
    std::u32string Source;
    xziar::nailang::Block Program;
    XCNLProgram(std::u32string&& source) : Source(std::move(source)) { }
public:
    ~XCNLProgram() {}

    template<bool IsBlock, typename F>
    forceinline void ForEachBlockType(F&& func) const
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
            func(block, meta);
        }
    }

    template<bool IsBlock, typename F>
    forceinline void ForEachBlockTypeName(const std::u32string_view type, F&& func) const
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
            if (block.Type == type)
                func(block, meta);
        }
    }

    [[nodiscard]] constexpr const xziar::nailang::Block& GetProgram() const noexcept { return Program; }

    [[nodiscard]] XCOMPBASAPI static std::shared_ptr<XCNLProgram> Create(std::u32string source);
    template<typename T>
    [[nodiscard]] static std::shared_ptr<XCNLProgram> CreateBy(std::u32string source)
    {
        auto prog = Create_(std::move(source));
        T::GetBlock(prog->MemPool, prog->Source, prog->Program);
        return prog;
    }
};


class XCOMPBASAPI XCNLProgStub : public common::NonCopyable
{
private:
    void Prepare(common::mlog::MiniLogger<false>& logger);
protected:
    xziar::nailang::MemoryPool MemPool;
    std::shared_ptr<const XCNLProgram> Program;
    std::shared_ptr<XCNLContext> Context;
    std::unique_ptr<XCNLRuntime> Runtime;
public:
    XCNLProgStub(const std::shared_ptr<const XCNLProgram>& program, std::shared_ptr<XCNLContext>&& context, std::unique_ptr<XCNLRuntime>&& runtime);
    XCNLProgStub(XCNLProgStub&&) = default;
    virtual ~XCNLProgStub();

    [[nodiscard]] constexpr const std::shared_ptr<const XCNLProgram>& GetProgram() const noexcept
    {
        return Program;
    }
    [[nodiscard]] constexpr const std::shared_ptr<XCNLContext>& GetContext() const noexcept
    {
        return Context;
    }
    [[nodiscard]] XCNLRuntime& GetRuntime() const noexcept
    {
        return *Runtime;
    }
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}