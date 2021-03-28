#pragma once
#include "XCompRely.h"
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


class XCNLProcessor;
class XCNLExecutor;
class XCNLConfigurator;
class XCNLRawExecutor;
class XCNLStructHandler;
class XCNLContext;
class XCNLRuntime;
class XCNLProgStub;
class ReplaceDepend;


using U32StrSpan    = ::common::span<const std::u32string_view>;
using MetaFuncs     = ::common::span<const xziar::nailang::FuncCall>;


struct OutputBlock
{
    enum class BlockType : uint8_t { None, Global, Struct, Instance, Template };
    struct BlockInfo
    {
        virtual ~BlockInfo() { }
    };

    const xziar::nailang::RawBlock* Block;
    MetaFuncs MetaFunc;
    std::vector<std::pair<xziar::nailang::LateBindVar, xziar::nailang::Expr>> PreAssignArgs;
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
    std::vector<std::pair<common::StringPiece<char32_t>, uint32_t>> Dependencies;
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
    forceinline std::u32string_view GetDepend(const NamedText& item, const uint32_t idx) const noexcept
    {
        Expects(idx < item.DependCount());
        return Names.GetStringView(Dependencies[(size_t)item.Dependency.first + idx].first);
    }
    forceinline std::optional<uint32_t> SearchDepend(const std::vector<NamedText>& container, const NamedText& item, const uint32_t idx) const noexcept
    {
        Expects(idx < item.DependCount());
        const auto [depStr, depIdx] = Dependencies[(size_t)item.Dependency.first + idx];
        if (depIdx != UINT32_MAX)
            return depIdx;
        // late bind dependency
        const auto depName = Names.GetStringView(depStr);
        for (uint32_t i = 0; i < container.size(); ++i)
        {
            if (GetID(container[i]) == depName)
            {
                return i;
            }
        }
        return {};
    }
    forceinline std::optional<uint32_t> CheckExists(std::vector<NamedText>& container, std::u32string_view id) const noexcept
    {
        uint32_t idx = 0;
        for (const auto& item : container)
        {
            if (GetID(item) == id)
                return idx;
            idx++;
        }
        return {};
    }
    forceinline bool Add(std::vector<NamedText>& container, std::u32string_view id, common::str::StrVariant<char32_t> content, U32StrSpan depends)
    {
        // check exists
        if (CheckExists(container, id))
            return false;
        ForceAdd(container, id, std::move(content), depends);
        return true;
    }
    void ForceAdd(std::vector<NamedText>& container, std::u32string_view id, common::str::StrVariant<char32_t> content, U32StrSpan depends);
    void Write(std::u32string& output, const std::vector<NamedText>& container) const;
    std::shared_ptr<const ReplaceDepend> GenerateReplaceDepend(const NamedText& item, bool isPthBlk, U32StrSpan pthBlk = {}, U32StrSpan bdyPfx = {}) const;

    virtual void Write(std::u32string& output, const NamedText& item) const;
};


class InstanceContext : public NamedTextHolder
{
public:
    virtual ~InstanceContext() {}

    forceinline bool AddBodyPrefix(const std::u32string_view id, std::u32string_view content, U32StrSpan depends = {})
    {
        return Add(BodyPrefixes, id, content, depends);
    }
    forceinline bool AddBodySuffix(const std::u32string_view id, std::u32string_view content, U32StrSpan depends = {})
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

    std::u32string_view InsatnceName;
    std::u32string Content;
protected:
    std::vector<NamedText> BodyPrefixes;
    std::vector<NamedText> BodySuffixes;
};


struct InstanceArgInfo
{
    enum class Types : uint8_t { RawBuf, TypedBuf, Texture, Simple };
    enum class TexTypes : uint8_t { Empty = 0, Tex1D, Tex2D, Tex3D, Tex1DArray, Tex2DArray };
    enum class Flags : uint16_t { Empty = 0, Read = 0x1, Write = 0x2, ReadWrite = Read | Write, Restrict = 0x4 };
    std::u32string_view Name;
    std::u32string_view DataType;
    common::span<const xziar::nailang::Arg> ExtraArg;
    std::vector<std::u32string_view> Extra;
    TexTypes TexType;
    Types Type;
    Flags Flag;
    InstanceArgInfo(Types type, TexTypes texType, std::u32string_view name, std::u32string_view dtype, 
        const std::vector<xziar::nailang::Arg>& args);
    XCOMPBASAPI static std::string_view GetTexTypeName(const TexTypes type) noexcept;
};
MAKE_ENUM_BITFIELD(InstanceArgInfo::Flags)
struct InstanceArgData;


struct CombinedType
{
private:
    common::simd::VecDataInfo TypeData;
    forceinline constexpr uint32_t ToIndex() const noexcept
    {
        return (static_cast<uint32_t>(TypeData.Bit) << 0)
            | (static_cast<uint32_t>(TypeData.Dim0) << 8) | (static_cast<uint32_t>(TypeData.Dim1) << 16);
    }
public:
    constexpr CombinedType(common::simd::VecDataInfo type) noexcept : TypeData(type)
    {
        Expects(!IsCustomType());
    }
    constexpr CombinedType(uint32_t idx) noexcept :
        TypeData{ common::simd::VecDataInfo::DataTypes::Custom, static_cast<uint8_t>(idx),
        static_cast<uint8_t>(idx >> 8), static_cast<uint8_t>(idx >> 16) }
    {
        Expects(idx <= 0xffffff);
    }
    forceinline constexpr bool IsCustomType() const noexcept
    {
        return TypeData.Type == common::simd::VecDataInfo::DataTypes::Custom;
    }
    forceinline constexpr std::optional<common::simd::VecDataInfo> GetVecType() const noexcept
    {
        if (!IsCustomType())
            return TypeData;
        return {};
    }
    forceinline constexpr std::optional<uint32_t> GetCustomTypeIdx() const noexcept
    {
        if (IsCustomType())
            return ToIndex();
        return {};
    }
    forceinline constexpr std::variant<common::simd::VecDataInfo, uint32_t> GetType() const noexcept
    {
        if (IsCustomType())
            return ToIndex();
        else
            return TypeData;
    }
};
struct XCNLStruct
{
    struct Field
    {
        common::StringPiece<char32_t> Name;
        CombinedType Type;
        uint16_t Length;
        uint16_t Offset;
        uint32_t MetaInfo;
        constexpr Field(common::StringPiece<char32_t> name, CombinedType type, uint16_t len) noexcept :
            Name(name), Type(type), Length(len), Offset(0), MetaInfo(0)
        { }
    };
    common::StringPool<char32_t> StrPool;
    common::StringPiece<char32_t> Name;
    std::vector<Field> Fields;
    uint32_t Alignment = 0;
    uint32_t Size = 0;
    XCNLStruct(std::u32string_view name) : Name(StrPool.AllocateString(name))
    { }
    std::u32string_view GetName() const noexcept { return StrPool.GetStringView(Name); }
    [[nodiscard]] Field& AddField(std::u32string_view name, CombinedType type, uint16_t len)
    {
        const auto name_ = StrPool.AllocateString(name);
        return Fields.emplace_back(name_, type, len);
    }
};


class COMMON_EMPTY_BASES ReplaceDepend : public common::NonCopyable, public common::NonMovable
{
private:
    MAKE_ENABLER();
    common::StringPool<char32_t> Names;
public:
    U32StrSpan PatchedBlock;
    U32StrSpan BodyPrefixes;

private:
    template<size_t I, size_t N, typename T, typename... Args>
    forceinline static std::pair<size_t, size_t> CreateFrom_(std::array<std::u32string_view, N>& arr, const bool isBody, [[maybe_unused]] T&& arg, Args&&... args)
    {
        static_assert(I < N);
        if constexpr (std::is_same_v<std::decay_t<T>, std::nullopt_t>)
        {
            Expects(!isBody);
            return CreateFrom_<I>(arr, true, std::forward<Args>(args)...);
        }
        else if constexpr (sizeof...(Args) > 0)
        {
            arr[I] = arg;
            const auto [x, y] = CreateFrom_<I + 1>(arr, isBody, std::forward<Args>(args)...);
            return isBody ? std::pair{ x, y + 1 } : std::pair{ x + 1, y };
        }
        else
        {
            arr[I] = arg;
            return isBody ? std::pair{ 0, 1 } : std::pair{ 1, 0 };
        }
    }
public:
    XCOMPBASAPI static std::shared_ptr<const ReplaceDepend> Create(U32StrSpan patchedBlk, U32StrSpan bodyPfx);
    XCOMPBASAPI static std::shared_ptr<const ReplaceDepend> Empty();
    template<typename... Args>
    static std::shared_ptr<const ReplaceDepend> CreateFrom(Args&&... args)
    {
        std::array<std::u32string_view, sizeof...(Args)> arr;
        const auto [x, y] = CreateFrom_<0>(arr, false, std::forward<Args>(args)...);
        Expects(x + y <= arr.size());
        U32StrSpan sp(arr);
        return Create(sp.subspan(0, x), sp.subspan(x, y));
    }
    static std::shared_ptr<const ReplaceDepend> CreateFrom()
    {
        return Empty();
    }
};


class ReplaceResult
{
    common::str::StrVariant<char32_t> Str;
    std::function<std::shared_ptr<const ReplaceDepend>()> Depends;
    bool IsSuccess, AllowFallback;
public:
    ReplaceResult() noexcept : IsSuccess(false), AllowFallback(true) 
    { }
    template<typename T, typename = std::enable_if_t<std::is_constructible_v<common::str::StrVariant<char32_t>, T>>>
    ReplaceResult(T&& str) noexcept : 
        Str(std::forward<T>(str)), IsSuccess(true), AllowFallback(false) 
    { }
    template<typename T>
    ReplaceResult(T&& str, std::function<std::shared_ptr<const ReplaceDepend>()> depend) noexcept :
        Str(std::forward<T>(str)), Depends(std::move(depend)), IsSuccess(true), AllowFallback(false) 
    { }
    template<typename T>
    ReplaceResult(T&& str, std::shared_ptr<const ReplaceDepend> depend) noexcept :
        Str(std::forward<T>(str)), Depends([dep = std::move(depend)](){ return dep; }),
        IsSuccess(true), AllowFallback(false) 
    { }
    template<typename T>
    ReplaceResult(T&& str, const bool allowFallback) noexcept :
        Str(std::forward<T>(str)), IsSuccess(false), AllowFallback(allowFallback) 
    { }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsSuccess; }
    [[nodiscard]] constexpr bool CheckAllowFallback() const noexcept { return AllowFallback; }
    [[nodiscard]] constexpr std::u32string_view GetStr() const noexcept { return Str.StrView(); }
    [[nodiscard]] std::shared_ptr<const ReplaceDepend> GetDepends() const
    {
        if (Depends)
            return Depends();
        return {};
    }
    void SetStr(common::str::StrVariant<char32_t> str) noexcept { Str = std::move(str); }
};


class XCOMPBASAPI XCNLExtension
{
    friend XCNLExecutor;
    friend XCNLRuntime;
    friend XCNLContext;
    friend XCNLProgStub;
protected:
    XCNLContext& Context;
    forceinline static constexpr common::mlog::MiniLogger<false>& GetLogger(XCNLRuntime& runtime);
public:
    XCNLExtension(XCNLContext& context);
    virtual ~XCNLExtension();
    virtual void  BeginXCNL(XCNLRuntime&) { }
    virtual void FinishXCNL(XCNLRuntime&) { }
    virtual void  BeginInstance(XCNLRuntime&, InstanceContext&) { } 
    virtual void FinishInstance(XCNLRuntime&, InstanceContext&) { }
    virtual void InstanceMeta(XCNLExecutor&, const xziar::nailang::FuncPack&, InstanceContext&) { }
    [[nodiscard]] virtual ReplaceResult ReplaceFunc(XCNLRawExecutor&, std::u32string_view, U32StrSpan)
    {
        return {};
    }
    [[nodiscard]] virtual std::optional<xziar::nailang::Arg> ConfigFunc(XCNLExecutor&, xziar::nailang::FuncEvalPack&)
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
};
#define XCNL_EXT_FUNC(type, ...)                                    \
    static std::unique_ptr<XCNLExtension> Create(                   \
        [[maybe_unused]] common::mlog::MiniLogger<false>& logger,   \
        [[maybe_unused]] xcomp::XCNLContext& context)               \
    __VA_ARGS__                                                     \

#define XCNL_EXT_REG(type, ...) XCNL_EXT_FUNC(type, __VA_ARGS__)                        \
    inline static uintptr_t Dummy = xcomp::XCNLExtension::RegisterXCNLExtension<type>() \


class XCOMPBASAPI XCNLContext : public xziar::nailang::CompactEvaluateContext, public NamedTextHolder
{
    friend XCNLProcessor;
    friend XCNLExecutor;
    friend XCNLRawExecutor;
    friend XCNLStructHandler;
    friend XCNLRuntime;
    friend XCNLProgStub;
public:
    XCNLContext(const common::CLikeDefines& info);
    ~XCNLContext() override;
    template<typename T, typename F, typename... Args>
    forceinline std::pair<std::shared_ptr<const ReplaceDepend>, bool> AddPatchedBlockD(
        T& obj, std::u32string_view id, U32StrSpan depends, F generator, Args&&... args)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F, T&, Args...>, "need accept args and return u32string");
        if (const auto idx = CheckExists(PatchedBlocks, id); idx)
            return { PatchedSelfDepends[*idx], false };
        ForceAdd(PatchedBlocks, id, (obj.*generator)(std::forward<Args>(args)...), depends);
        PatchedSelfDepends.push_back(ReplaceDepend::CreateFrom(id));
        return { PatchedSelfDepends.back(), true };
    }
    template<typename F>
    forceinline std::pair<std::shared_ptr<const ReplaceDepend>, bool> AddPatchedBlockD(
        std::u32string_view id, U32StrSpan depends, F&& generator)
    {
        static_assert(std::is_invocable_r_v<std::u32string, F>, "need return u32string");
        if (const auto idx = CheckExists(PatchedBlocks, id); idx)
            return { PatchedSelfDepends[*idx], false };
        ForceAdd(PatchedBlocks, id, generator(), depends);
        PatchedSelfDepends.push_back(ReplaceDepend::CreateFrom(id));
        return { PatchedSelfDepends.back(), true };
    }
    template<typename T, typename F, typename... Args>
    forceinline std::pair<std::shared_ptr<const ReplaceDepend>, bool> AddPatchedBlockD(
        T& obj, std::u32string_view id, std::u32string_view depend, F generator, Args&&... args)
    {
        return AddPatchedBlockD(obj, id, U32StrSpan{ &depend, 1 }, std::forward<F>(generator), std::forward<Args>(args)...);
    }
    template<typename F>
    forceinline std::pair<std::shared_ptr<const ReplaceDepend>, bool> AddPatchedBlockD(
        std::u32string_view id, std::u32string_view depend, F&& generator)
    {
        return AddPatchedBlockD(id, U32StrSpan{ &depend, 1 }, std::forward<F>(generator));
    }
    template<typename T, typename F, typename... Args>
    forceinline auto AddPatchedBlock(T& obj, std::u32string_view id, F&& generator, Args&&... args)
    {
        return AddPatchedBlockD(obj, id, U32StrSpan{}, std::forward<F>(generator), std::forward<Args>(args)...);
    }
    template<typename F>
    forceinline auto AddPatchedBlock(std::u32string_view id, F&& generator)
    {
        return AddPatchedBlockD(id, U32StrSpan{}, std::forward<F>(generator));
    }
    forceinline void WritePatchedBlock(std::u32string& output) const
    {
        NamedTextHolder::Write(output, PatchedBlocks);
    }
    template<typename T>
    [[nodiscard]] forceinline T* GetXCNLExt() const
    {
        return static_cast<T*>(FindExt([](const XCNLExtension* ext) -> bool { return dynamic_cast<const T*>(ext); }));
    }
protected:
    std::vector<std::unique_ptr<XCNLExtension>> Extensions;
    std::vector<OutputBlock> OutputBlocks;
    std::vector<OutputBlock> TemplateBlocks;
    std::vector<NamedText> PatchedBlocks;
    std::vector<std::unique_ptr<XCNLStruct>> CustomStructs;
private:
    COMMON_NO_COPY(XCNLContext)
    std::vector<std::shared_ptr<const ReplaceDepend>> PatchedSelfDepends;
    [[nodiscard]] XCNLExtension* FindExt(std::function<bool(const XCNLExtension*)> func) const;
    [[nodiscard]] size_t FindStruct(std::u32string_view name) const;
    void Write(std::u32string& output, const NamedText& item) const override;
};


class XCOMPBASAPI XCNLRuntime : public xziar::nailang::NailangRuntime
{
    friend XCNLExecutor;
    friend XCNLRawExecutor;
    friend XCNLExtension;
    friend XCNLProgStub;
protected:
    using Block = xziar::nailang::Block;
    using RawBlock = xziar::nailang::RawBlock;
    using FuncCall = xziar::nailang::FuncCall;

    XCNLContext& XCContext;
    common::mlog::MiniLogger<false>& Logger;

    XCNLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<XCNLContext> evalCtx);
    [[nodiscard]] std::optional<xziar::nailang::Arg> CommonFunc(const std::u32string_view name, xziar::nailang::FuncEvalPack& func);
    [[nodiscard]] xziar::nailang::Arg CreateGVec(const std::u32string_view type, xziar::nailang::FuncEvalPack& func);

    [[nodiscard]] virtual OutputBlock::BlockType GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept;
    virtual void HandleInstanceArg(const InstanceArgInfo& arg, InstanceContext& ctx, const xziar::nailang::FuncPack& meta, const xziar::nailang::Arg* source);
    virtual void BeforeFinishOutput(std::u32string& prefix, std::u32string& content);
private:
    virtual XCNLConfigurator& GetConfigurator() noexcept = 0;
    virtual XCNLRawExecutor& GetRawExecutor() noexcept = 0;
    virtual XCNLStructHandler& GetStructHandler() noexcept = 0;
    InstanceArgData ParseInstanceArg(std::u32string_view argTypeName, xziar::nailang::FuncPack& func);
public:
    struct VecTypeResult
    {
        common::simd::VecDataInfo Info;
        bool TypeSupport;
        constexpr explicit operator bool() const noexcept { return Info.Bit != 0; }
    };
    ~XCNLRuntime() override;
    COMMON_NO_COPY(XCNLRuntime)
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;
    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    void PrintStruct(const XCNLStruct& target) const;
    [[nodiscard]] virtual VecTypeResult TryParseVecType(const std::u32string_view type) const noexcept = 0;
    [[nodiscard]] virtual std::u32string_view GetVecTypeName(common::simd::VecDataInfo vec) const noexcept = 0;
    [[nodiscard]] common::simd::VecDataInfo ParseVecType(const std::u32string_view type, std::u16string_view = {}) const;
    [[nodiscard]] common::simd::VecDataInfo ParseVecType(const std::u32string_view type, const std::function<std::u16string(void)>& extraInfo) const;
    [[nodiscard]] std::u32string_view GetVecTypeName(const std::u32string_view vname, std::u16string_view extraInfo = u"call [GetVecTypeName]") const;
    [[nodiscard]] std::u32string_view GetVecTypeName(const std::u32string_view vname, const std::function<std::u16string(void)>& extraInfo) const;
    [[noreturn]] void ThrowByVecType(std::variant<std::u32string_view, common::simd::VecDataInfo> src, std::u16string_view reason, std::u16string_view exInfo) const;
    void ProcessConfigBlock(const Block& block, MetaFuncs metas);
    void ProcessStructBlock(const Block& block, MetaFuncs metas);
    void CollectRawBlock(const RawBlock& block, MetaFuncs metas);

    std::string GenerateOutput();
};

inline constexpr common::mlog::MiniLogger<false>& XCNLExtension::GetLogger(XCNLRuntime& runtime)
{
    return runtime.Logger;
}


class XCOMPBASAPI XCNLExecutor : public xziar::nailang::NailangExecutor
{
    friend debug::XCNLDebugExt;
    friend XCNLConfigurator;
    friend XCNLRawExecutor;
    friend XCNLStructHandler;
    friend XCNLRuntime;
protected:
    [[nodiscard]] constexpr XCNLRuntime& GetRuntime() const noexcept
    {
        return *static_cast<XCNLRuntime*>(Runtime);
    }
    [[nodiscard]] constexpr XCNLContext& GetContext() const noexcept
    {
        return GetRuntime().XCContext;
    }
    [[nodiscard]] constexpr const std::vector<std::unique_ptr<XCNLExtension>>& GetExtensions() const noexcept
    {
        return GetRuntime().XCContext.Extensions;
    }
    [[nodiscard]] std::shared_ptr<xziar::nailang::EvaluateContext> CreateContext() const
    {
        return GetRuntime().ConstructEvalContext();
    }
    [[nodiscard]] xziar::nailang::Arg EvaluateFunc(xziar::nailang::FuncEvalPack& func) override;
    using NailangExecutor::NailangExecutor;
};


class XCOMPBASAPI XCNLConfigurator
{
    friend XCNLRuntime;
private:
    [[nodiscard]] virtual XCNLExecutor& GetExecutor() noexcept = 0;
protected:
    struct OutputBlockFrame : public xziar::nailang::NailangRawBlockFrame
    {
        OutputBlock& Block;
        OutputBlockFrame(NailangFrame* prev, xziar::nailang::NailangExecutor* executor, const std::shared_ptr<xziar::nailang::EvaluateContext>& ctx, OutputBlock& block) :
            NailangRawBlockFrame(prev, executor, ctx, NailangFrame::FrameFlags::Empty, block.Block, block.MetaFunc), Block(block)
        { }
        ~OutputBlockFrame() override { }
        constexpr const xziar::nailang::RawBlock& GetBlock() const noexcept
        {
            return *Block.Block;
        }
        size_t GetSize() const noexcept override { return sizeof(OutputBlockFrame); }
    };
    XCNLConfigurator();
    forceinline OutputBlockFrame* TryGetOutputFrame() noexcept
    {
        return dynamic_cast<OutputBlockFrame*>(&GetExecutor().GetFrame());
    }
    bool HandleRawBlockMeta(const xziar::nailang::FuncCall& meta, OutputBlock& block, xziar::nailang::MetaSet&);
public:
    virtual ~XCNLConfigurator();
    [[nodiscard]] bool PrepareOutputBlock(OutputBlock& block);
};


class XCOMPBASAPI XCNLRawExecutor : protected xziar::nailang::ReplaceEngine
{
    friend debug::XCNLDebugExt;
    friend XCNLRuntime;
private:
    [[nodiscard]] virtual const XCNLExecutor& GetExecutor() const noexcept = 0;
    [[nodiscard]] virtual XCNLExecutor& GetExecutor() noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<InstanceContext> PrepareInstance(const OutputBlock& block) = 0;
    virtual void OutputInstance(const OutputBlock& block, std::u32string& dst) = 0;
protected:
    struct InstanceFrame : public xziar::nailang::NailangRawBlockFrame
    {
        const OutputBlock& Block;
        InstanceContext* Instance;
        InstanceFrame(NailangFrame* prev, XCNLExecutor* executor, const std::shared_ptr<xziar::nailang::EvaluateContext>& ctx, 
            const OutputBlock& block, InstanceContext* instance) :
            NailangRawBlockFrame(prev, executor, ctx, NailangFrame::FrameFlags::Empty, block.Block, block.MetaFunc), 
            Block(block), Instance(instance)
        { }
        ~InstanceFrame() override { }
        constexpr const xziar::nailang::RawBlock& GetBlock() const noexcept
        {
            return *Block.Block;
        }
        size_t GetSize() const noexcept override { return sizeof(InstanceFrame); }
    };

    [[nodiscard]] forceinline auto& GetExtensions() const noexcept 
    { 
        return GetExecutor().GetExtensions();
    }
    [[nodiscard]] InstanceContext& GetInstanceInfo() const;
    static void OutputConditions(common::span<const xziar::nailang::FuncCall> metas, std::u32string& dst);
    [[nodiscard]] xziar::nailang::NailangFrameStack::FrameHolder<xziar::nailang::NailangRawBlockFrame> PushFrame(
        const OutputBlock& block, std::shared_ptr<xziar::nailang::EvaluateContext> ctx, InstanceContext* instance = nullptr);
    [[nodiscard]] bool HandleInstanceMeta(xziar::nailang::FuncPack& meta);
    [[nodiscard]] std::optional<common::str::StrVariant<char32_t>> CommonReplaceFunc(const std::u32string_view name,
        const std::u32string_view call, U32StrSpan args);
    [[nodiscard]] ReplaceResult ExtensionReplaceFunc(std::u32string_view func, U32StrSpan args);
    void BeforeOutputBlock(const OutputBlock& block, std::u32string& dst) const;
    void DirectOutput(const OutputBlock& block, std::u32string& dst);

    void OnReplaceOptBlock(std::u32string& output, void* cookie, std::u32string_view cond, std::u32string_view content) final;
    void OnReplaceVariable(std::u32string& output, void* cookie, std::u32string_view var) final;
    void OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args) override;
public:
    ~XCNLRawExecutor() override;
    void ThrowByReplacerArgCount(const std::u32string_view call, const U32StrSpan args,
        const size_t count, const xziar::nailang::ArgLimits limit = xziar::nailang::ArgLimits::Exact) const;
    virtual void ProcessGlobal(const OutputBlock& block, std::u32string& dst);
    virtual void ProcessStruct(const OutputBlock& block, std::u32string& dst);
    void ProcessInstance(const OutputBlock& block, std::u32string& output);
    [[noreturn]] void HandleException(const xziar::nailang::NailangParseException& ex) const final;
    [[noreturn]] void HandleException(const xziar::nailang::NailangRuntimeException& ex) const;
    [[nodiscard]] XCNLRuntime& GetRuntime() const noexcept 
    { 
        return GetExecutor().GetRuntime();
    }
}; 


class XCOMPBASAPI XCNLStructHandler
{
    friend XCNLRuntime;
private:
    [[nodiscard]] virtual const XCNLExecutor& GetExecutor() const noexcept = 0;
    [[nodiscard]] virtual XCNLExecutor& GetExecutor() noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<XCNLStruct> GenerateStruct(std::u32string_view name);
    virtual void OnNewField(const XCNLStruct&, XCNLStruct::Field&, xziar::nailang::MetaSet&) {}
    virtual void FillFieldOffsets(XCNLStruct& target);
protected:
    struct StructFrame : public xziar::nailang::NailangBlockFrame
    {
        XCNLStruct& Struct;
        StructFrame(NailangFrame* prev, XCNLExecutor* executor, const std::shared_ptr<xziar::nailang::EvaluateContext>& ctx,
            const xziar::nailang::Block* block, MetaFuncs metas, XCNLStruct& target) :
            NailangBlockFrame(prev, executor, ctx, NailangFrame::FrameFlags::Empty, block, metas), Struct(target)
        { }
        ~StructFrame() override { }
        size_t GetSize() const noexcept override { return sizeof(StructFrame); }
    };
    forceinline StructFrame* TryGetStructFrame() noexcept
    {
        return dynamic_cast<StructFrame*>(&GetExecutor().GetFrame());
    }
    [[nodiscard]] XCNLStruct& GetStruct() const;
    [[nodiscard]] std::pair<uint32_t, uint32_t> GetBasicFieldAlignment(const XCNLStruct::Field& field) const noexcept;
    [[nodiscard]] xziar::nailang::NailangFrameStack::FrameHolder<xziar::nailang::NailangBlockFrame> PushFrame(
        XCNLStruct* target, const xziar::nailang::Block* block, MetaFuncs metas);
    [[nodiscard]] bool EvaluateStructFunc(xziar::nailang::FuncEvalPack& func);
public:
    virtual ~XCNLStructHandler();
    [[nodiscard]] XCNLRuntime& GetRuntime() const noexcept
    {
        return GetExecutor().GetRuntime();
    }
};


class XCNLProgram
{
    friend XCNLProgStub;
private:
    MAKE_ENABLER();
    [[nodiscard]] XCOMPBASAPI static std::shared_ptr<XCNLProgram> Create_(std::u32string source, std::u16string fname);
protected:
    xziar::nailang::MemoryPool MemPool;
    std::u32string Source;
    std::u16string FileName;
    xziar::nailang::Block Program;
    using ExtGen = std::function<std::unique_ptr<XCNLExtension>(common::mlog::MiniLogger<false>&, XCNLContext&)>;
    std::vector<ExtGen> ExtraExtension;
    XCNLProgram(std::u32string source, std::u16string fname) : 
        Source(std::move(source)), FileName(std::move(fname)) { }
public:
    ~XCNLProgram() {}

    template<typename T>
    void AttachExtension() noexcept
    {
        ExtraExtension.push_back(T::Create);
    }
    void AttachExtension(ExtGen creator) noexcept
    {
        ExtraExtension.push_back(std::move(creator));
    }

    [[nodiscard]] constexpr const xziar::nailang::Block& GetProgram() const noexcept { return Program; }

    [[nodiscard]] XCOMPBASAPI static std::shared_ptr<XCNLProgram> Create(std::u32string source, std::u16string fname);
    template<typename T>
    [[nodiscard]] static std::shared_ptr<XCNLProgram> CreateBy(std::u32string source, std::u16string fname)
    {
        auto prog = Create_(std::move(source), std::move(fname));
        T::GetBlock(prog->MemPool, prog->Source, prog->FileName, prog->Program);
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
    void ExecuteBlocks(const std::u32string_view type) const;
    void Prepare(common::span<const std::u32string_view> types) const;
    void Collect(common::span<const std::u32string_view> prefixes) const;
    void PostAct(common::span<const std::u32string_view> types) const;
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


class XCOMPBASAPI GeneralVecRef : public xziar::nailang::CustomVar::Handler
{
protected:
    static xziar::nailang::CustomVar Create(xziar::nailang::FixedArray arr);
    static size_t ToIndex(const xziar::nailang::CustomVar& var, const xziar::nailang::FixedArray& arr, std::u32string_view field);
    xziar::nailang::ArgLocator HandleQuery(xziar::nailang::CustomVar& var, xziar::nailang::SubQuery subq, xziar::nailang::NailangExecutor& executor) override;
    bool HandleAssign(xziar::nailang::CustomVar& var, xziar::nailang::Arg arg) override;
    xziar::nailang::CompareResult CompareSameClass(const xziar::nailang::CustomVar&, const xziar::nailang::CustomVar&) final;
    common::str::StrVariant<char32_t> ToString(const xziar::nailang::CustomVar& var) noexcept override;
    std::u32string_view GetTypeName() noexcept override;
    std::u32string GetExactType(const xziar::nailang::FixedArray& arr) noexcept;
public:
    static xziar::nailang::FixedArray ToArray(const xziar::nailang::CustomVar& var) noexcept;
    template<typename T>
    static constexpr bool CheckType() noexcept
    {
        using R = std::remove_cv_t<T>;
        return std::is_arithmetic_v<R> || std::is_same_v<R, half_float::half>;
    }
    template<typename T>
    static xziar::nailang::CustomVar Create(common::span<T> target)
    {
        static_assert(CheckType<T>(), "only allow arithmetic type");
        return Create(xziar::nailang::FixedArray::Create(target));
    }
};

class XCOMPBASAPI GeneralVec : public GeneralVecRef
{
    void IncreaseRef(xziar::nailang::CustomVar& var) noexcept override;
    void DecreaseRef(xziar::nailang::CustomVar& var) noexcept override;
    common::str::StrVariant<char32_t> ToString(const xziar::nailang::CustomVar& var) noexcept override;
    std::u32string_view GetTypeName() noexcept override;
public:
    static xziar::nailang::CustomVar Create(xziar::nailang::FixedArray::Type type, size_t len);
    template<typename T>
    static xziar::nailang::CustomVar Create(size_t len)
    {
        static_assert(GeneralVecRef::CheckType<T>(), "only allow arithmetic type");
        common::span<T> dummy(reinterpret_cast<T*>(nullptr), 1);
        const auto type = xziar::nailang::FixedArray::Create(dummy).ElementType;
        return Create(type, len);
    }
};



#if COMPILER_MSVC
#   pragma warning(pop)
#endif
}