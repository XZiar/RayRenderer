#pragma once
#include "XCompRely.h"
#include "XCompDebug.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "common/CLikeConfig.hpp"

#include <boost/container/small_vector.hpp>


namespace xcomp
{
#if COMMON_COMPILER_MSVC
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
    enum class BlockType : uint8_t { None, Prefix, Global, Struct, Instance, Template };
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


struct XCNLStruct
{
    struct ArrayDim
    {
        uint16_t Size;
        uint16_t StrideAlign;
    };
    template<bool IsConst>
    struct DimInfo
    {
        common::span<std::conditional_t<IsConst, const ArrayDim, ArrayDim>> Dims;
        constexpr uint32_t GetTotalElements() const noexcept
        {
            uint32_t cnt = 1;
            for (const auto& dim : Dims)
                cnt *= dim.Size;
            return cnt;
        }
        constexpr uint32_t GetAlignCounts() const noexcept
        {
            if (Dims.empty())
                return 1;
            const auto& dim = Dims.back();
            return dim.Size * dim.StrideAlign;
        }
    };
    struct Field
    {
        common::StringPiece<char32_t> Name;
        VTypeInfo Type;
        uint16_t ArrayInfo;
        uint16_t Offset;
        uint32_t MetaInfo;
        constexpr Field(common::StringPiece<char32_t> name, VTypeInfo type, uint16_t arrayInfo) noexcept :
            Name(name), Type(type), ArrayInfo(arrayInfo), Offset(0), MetaInfo(0)
        { }
        template<typename T>
        [[nodiscard]] std::pair<uint32_t, uint32_t> GetElementInfo(const T& structs) const noexcept
        {
            if (Type.IsCustomType())
            {
                const auto& target = structs[Type.ToIndex()];
                if constexpr (std::is_base_of_v<XCNLStruct, std::decay_t<decltype(target)>>)
                    return { target.Alignment, target.Size };
                else if constexpr (std::is_base_of_v<XCNLStruct, std::decay_t<decltype(*target)>>)
                    return { (*target).Alignment, (*target).Size };
                else
                    static_assert(!common::AlwaysTrue<T>, "structs should be a container of Type that inherit XCNLStruct or contains XCNLStruct");
            }
            else
                return { Type.Bits / 8u, Type.Dim0() * std::max<uint8_t>(Type.Dim1(), 1) * Type.Bits / 8u };
        }
    };
    common::StringPool<char32_t> StrPool;
    common::StringPiece<char32_t> Name;
    boost::container::small_vector<Field, 8> Fields;
    boost::container::small_vector<uint16_t, 16> ArrayInfos;
    uint32_t Alignment = 0;
    uint32_t Size = 0;
    XCNLStruct(std::u32string_view name) : Name(StrPool.AllocateString(name))
    { }
    std::u32string_view GetName() const noexcept { return StrPool.GetStringView(Name); }
    std::u32string_view GetFieldName(const Field& field) const noexcept { return StrPool.GetStringView(field.Name); }
    DimInfo<true> GetFieldDims(const Field& field) const noexcept
    {
        if (field.ArrayInfo == UINT16_MAX) return { {} };
        const auto cnt = ArrayInfos[field.ArrayInfo];
        return { { reinterpret_cast<const ArrayDim*>(&ArrayInfos[field.ArrayInfo + 1u]), cnt } };
    }
    DimInfo<false> GetFieldDims(const Field& field) noexcept
    {
        if (field.ArrayInfo == UINT16_MAX) return { {} };
        const auto cnt = ArrayInfos[field.ArrayInfo];
        return { { reinterpret_cast<ArrayDim*>(&ArrayInfos[field.ArrayInfo + 1u]), cnt } };
    }
    [[nodiscard]] Field& AddField(std::u32string_view name, VTypeInfo type, uint16_t arrayInfo)
    {
        const auto name_ = StrPool.AllocateString(name);
        return Fields.emplace_back(name_, type, arrayInfo);
    }
    [[nodiscard]] size_t AddArrayInfos(common::span<const ArrayDim> dims)
    {
        if (dims.empty()) return SIZE_MAX;
        const auto idxVal = ArrayInfos.size();
        ArrayInfos.push_back(gsl::narrow_cast<uint16_t>(dims.size()));
        for (const auto& dim : dims)
        {
            ArrayInfos.push_back(dim.Size);
            ArrayInfos.push_back(dim.StrideAlign);
        }
        return idxVal;
    }
};


class DependBase
{
private:
    virtual std::u32string_view Get([[maybe_unused]] size_t idx) const noexcept { return {}; };
    using DependIterator = ::common::container::IndirectIterator<const DependBase, std::u32string_view, &DependBase::Get>;
    friend DependIterator;
public:
    virtual ~DependBase() {}
    virtual size_t Count() const noexcept = 0;
    boost::container::small_vector<std::u32string_view, 4> GetDependSpan() const noexcept
    {
        boost::container::small_vector<std::u32string_view, 4> ret;
        const auto count = Count();
        ret.reserve(count);
        for (size_t i = 0; i < count; ++i)
            ret.emplace_back(Get(i));
        return ret;
    }
    constexpr DependIterator begin() const noexcept { return { this, 0 }; }
              DependIterator end()   const noexcept { return { this, Count() }; }
};


class ReplaceDepend
{
private:
    common::StringPool<char32_t> StrPool;
    boost::container::small_vector<common::StringPiece<char32_t>, 4> PatchedBlock;
    boost::container::small_vector<common::StringPiece<char32_t>, 4> BodyPrefixes;
    template<bool IsPatchBlock>
    class ReplaceDependItem : public DependBase
    {
        const ReplaceDepend* Host;
        constexpr const auto& Target() const noexcept 
        {
            return IsPatchBlock ? Host->PatchedBlock : Host->BodyPrefixes;
        }
        std::u32string_view Get(size_t idx) const noexcept override
        {
            return Host->StrPool.GetStringView(Target()[idx]);
        }
    public:
        constexpr ReplaceDependItem(const ReplaceDepend* host) noexcept : Host(host) {}
        ~ReplaceDependItem() override {}
        size_t Count() const noexcept override
        {
            return Target().size();
        }
    };
    void Merge(const ReplaceDepend& other, const size_t offset, bool isPatchedBlock)
    {
        auto& target = isPatchedBlock ? PatchedBlock : BodyPrefixes;
        const auto& source = isPatchedBlock ? other.PatchedBlock : other.BodyPrefixes;
        const auto prevSize = target.size();
        target.reserve(prevSize + source.size());
        for (const auto dep : source)
        {
            bool hasDepend = false;
            for (size_t i = 0; i < prevSize && !hasDepend; ++i)
                hasDepend = StrPool.GetStringView(target[i]) == other.StrPool.GetStringView(dep);
            if (hasDepend)
                target.emplace_back(gsl::narrow_cast<uint32_t>(offset + dep.GetOffset()), static_cast<uint32_t>(dep.GetLength()));
        }
    }
public:
    void DependPatchedBlock(std::u32string_view depend)
    {
        PatchedBlock.emplace_back(StrPool.AllocateString(depend));
    }
    void DependBodyPrefix(std::u32string_view depend)
    {
        BodyPrefixes.emplace_back(StrPool.AllocateString(depend));
    }
    void MergeDepends(const ReplaceDepend& other)
    {
        const auto offset = StrPool.AllocateString(other.StrPool.GetAllStr());
        Merge(other, offset.GetOffset(), true);
        Merge(other, offset.GetOffset(), false);
    }
    boost::container::small_vector<std::u32string_view, 4> GetDependSpan(bool isPatchedBlock) const noexcept
    {
        boost::container::small_vector<std::u32string_view, 4> ret;
        auto& target = isPatchedBlock ? PatchedBlock : BodyPrefixes;
        ret.reserve(target.size());
        for (const auto dep : target)
            ret.emplace_back(StrPool.GetStringView(dep));
        return ret;
    }
    ReplaceDependItem<true>  GetPatchedBlock() const noexcept { return this; }
    ReplaceDependItem<false> GetBodyPrefixes() const noexcept { return this; }
};


class ReplaceResult
{
    common::str::StrVariant<char32_t> Str;
    ReplaceDepend Depends;
    bool IsSuccess, AllowFallback;
public:
    ReplaceResult() noexcept : IsSuccess(false), AllowFallback(true) 
    { }
    template<typename T, typename = std::enable_if_t<std::is_constructible_v<common::str::StrVariant<char32_t>, T>>>
    ReplaceResult(T&& str) noexcept : 
        Str(std::forward<T>(str)), IsSuccess(true), AllowFallback(false) 
    { }
    template<typename T>
    ReplaceResult(T&& str, const DependBase& depend) noexcept :
        Str(std::forward<T>(str)), IsSuccess(true), AllowFallback(false)
    {
        for (const auto dep : depend)
            Depends.DependPatchedBlock(dep);
    }
    template<typename T>
    ReplaceResult(T&& str, const std::u32string_view depend) noexcept :
        Str(std::forward<T>(str)), IsSuccess(true), AllowFallback(false)
    {
        if (!depend.empty())
            Depends.DependPatchedBlock(depend);
    }
    template<typename T, typename D, typename = std::enable_if_t<std::is_same_v<std::decay_t<D>, ReplaceDepend>>>
    ReplaceResult(T&& str, D&& depend) noexcept :
        Str(std::forward<T>(str)), Depends(std::forward<D>(depend)), IsSuccess(true), AllowFallback(false)
    { }
    template<typename T>
    ReplaceResult(T&& str, const bool allowFallback) noexcept :
        Str(std::forward<T>(str)), IsSuccess(false), AllowFallback(allowFallback) 
    { }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsSuccess; }
    [[nodiscard]] constexpr bool CheckAllowFallback() const noexcept { return AllowFallback; }
    [[nodiscard]] constexpr std::u32string_view GetStr() const noexcept { return Str.StrView(); }
    [[nodiscard]] std::u32string ExtractStr() noexcept { return Str.ExtractStr(); }
    [[nodiscard]] const ReplaceDepend& GetDepends() const { return Depends; }
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
private:
    class PatchResult : public DependBase
    {
    private:
        const XCNLContext* Host;
        uint32_t PatchIdx;
        bool IsNewAdd;
        std::u32string_view Get(size_t) const noexcept override 
        {
            return Host->GetID(Host->PatchedBlocks[PatchIdx]);
        }
    public:
        constexpr PatchResult(const XCNLContext* host, size_t idx, bool isNew) noexcept :
            Host(host), PatchIdx(gsl::narrow_cast<uint32_t>(idx)), IsNewAdd(isNew) {}
        ~PatchResult() override {}
        constexpr size_t Count() const noexcept override { return 1; }
        constexpr bool IsNew() const noexcept { return IsNewAdd; }
    };
    class PatchSession
    {
        friend XCNLContext;
    private:
        XCNLContext& Context;
        std::u32string_view ID;
        size_t Idx;
        constexpr PatchSession(XCNLContext& ctx, std::u32string_view id) noexcept : Context(ctx), ID(id), Idx(SIZE_MAX)
        { }
    public:
        template<typename S, typename D>
        void Commit(S&& content, const D& depends)
        {
            Expects(Idx == SIZE_MAX);
            Idx = Context.PatchedBlocks.size();
            if constexpr (std::is_base_of_v<DependBase, D>)
                Context.ForceAdd(Context.PatchedBlocks, ID, std::forward<S>(content), depends.GetDependSpan());
            else if constexpr (std::is_same_v<ReplaceDepend, D>)
                Context.ForceAdd(Context.PatchedBlocks, ID, std::forward<S>(content), depends.GetPatchedBlock().GetDependSpan());
            else if constexpr (std::is_convertible_v<D, std::u32string_view>)
            {
                const std::u32string_view depend = depends;
                if (depend.empty())
                    Context.ForceAdd(Context.PatchedBlocks, ID, std::forward<S>(content), {});
                else
                    Context.ForceAdd(Context.PatchedBlocks, ID, std::forward<S>(content), { &depend, 1 });
            }
            else
                Context.ForceAdd(Context.PatchedBlocks, ID, std::forward<S>(content), depends);
        }
    };
public:
    XCNLContext(const common::CLikeDefines& info);
    ~XCNLContext() override;
    template<typename F, typename... Args>
    forceinline PatchResult AddPatchedBlock(std::u32string_view id, F&& generator, Args&&... args)
    {
        if (const auto idx = CheckExists(PatchedBlocks, id); idx)
            return PatchResult{ this, *idx, false };
        using R = std::decay_t<std::invoke_result_t<F, Args...>>;
        auto ret = generator(std::forward<Args>(args)...);
        if constexpr (std::is_convertible_v<R, std::u32string_view>)
            ForceAdd(PatchedBlocks, id, std::move(ret), {});
        else if constexpr (common::is_specialization<R, std::pair>::value)
        {
            static_assert(std::is_convertible_v<typename R::first_type, std::u32string_view>, "need return pair<str,str[]>");
            if constexpr (std::is_base_of_v<DependBase, std::decay_t<typename R::second_type>>)
            {
                auto depends = ret.second.GetDependSpan();
                ForceAdd(PatchedBlocks, id, std::move(ret.first), depends);
            }
            else if constexpr (std::is_same_v<ReplaceDepend, std::decay_t<typename R::second_type>>)
            {
                auto depends = ret.second.GetPatchedBlock().GetDependSpan();
                ForceAdd(PatchedBlocks, id, std::move(ret.first), depends);
            }
            else if constexpr (std::is_convertible_v<typename R::second_type, std::u32string_view>)
                ForceAdd(PatchedBlocks, id, std::move(ret.first), std::array<std::u32string_view, 1>{ret.second});
            else
                ForceAdd(PatchedBlocks, id, std::move(ret.first), ret.second);
        }
        else
            static_assert(common::AlwaysTrue<R>, "need return str or pair<str,str[]>");
        return PatchResult{ this, PatchedBlocks.size() - 1, true };
    }
    template<typename F, typename... Args>
    forceinline PatchResult AddPatchedBlockEx(std::u32string_view id, F&& generator, Args&&... args)
    {
        if (const auto idx = CheckExists(PatchedBlocks, id); idx)
            return PatchResult{ this, *idx, false };
        PatchSession session(*this, id);
        generator(session, std::forward<Args>(args)...);
        return PatchResult{ this, session.Idx, true };
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
    [[nodiscard]] size_t FindStruct(std::u32string_view name) const;
private:
    COMMON_NO_COPY(XCNLContext)
    [[nodiscard]] XCNLExtension* FindExt(std::function<bool(const XCNLExtension*)> func) const;
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
    virtual void BeforeFinishOutput(std::u32string& prefixes, std::u32string& structs, std::u32string& globals, std::u32string& kernels);
private:
    virtual XCNLConfigurator& GetConfigurator() noexcept = 0;
    virtual XCNLRawExecutor& GetRawExecutor() noexcept = 0;
    virtual XCNLStructHandler& GetStructHandler() noexcept = 0;
    InstanceArgData ParseInstanceArg(std::u32string_view argTypeName, xziar::nailang::FuncPack& func);
public:
    ~XCNLRuntime() override;
    COMMON_NO_COPY(XCNLRuntime)
    void HandleException(const xziar::nailang::NailangRuntimeException& ex) const override;
    void InnerLog(common::mlog::LogLevel level, std::u32string_view str);
    virtual void PrintStruct(const XCNLStruct& target) const;
    [[nodiscard]] virtual VTypeInfo TryParseVecType(const std::u32string_view type, bool allowMinBits = true) const noexcept = 0;
    [[nodiscard]] virtual std::u32string_view GetVecTypeName(VTypeInfo vec) const noexcept = 0;
    [[nodiscard]] VTypeInfo ParseVecType(const std::u32string_view type, std::u16string_view = {}, bool allowMinBits = true) const;
    [[nodiscard]] VTypeInfo ParseVecType(const std::u32string_view type, const std::function<std::u16string(void)>& extraInfo, bool allowMinBits = true) const;
    [[nodiscard]] common::simd::VecDataInfo ParseVecDataInfo(const std::u32string_view type, std::u16string_view extraInfo = {}) const
    {
        return ParseVecType(type, extraInfo, false).ToVecDataInfo();
    }
    [[nodiscard]] common::simd::VecDataInfo ParseVecDataInfo(const std::u32string_view type, const std::function<std::u16string(void)>& extraInfo) const
    {
        return ParseVecType(type, extraInfo, false).ToVecDataInfo();
    }
    [[nodiscard]] std::u32string_view GetVecTypeName(const std::u32string_view vname, std::u16string_view extraInfo = u"call [GetVecTypeName]") const;
    [[nodiscard]] std::u32string_view GetVecTypeName(const std::u32string_view vname, const std::function<std::u16string(void)>& extraInfo) const;
    [[noreturn]] void ThrowByVecType(std::variant<std::u32string_view, VTypeInfo> src, std::u16string_view reason, std::u16string_view exInfo) const;
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
    [[nodiscard]] constexpr common::mlog::MiniLogger<false>& GetLogger() const noexcept
    {
        return GetRuntime().Logger;
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
    virtual void ProcessGlobal(const OutputBlock& block, std::u32string& output);
    virtual void ProcessStruct(const OutputBlock& block, std::u32string& output);
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
    [[nodiscard]] virtual std::unique_ptr<XCNLStruct> GenerateStruct(std::u32string_view name, xziar::nailang::MetaSet& metas);
    virtual void OnNewField(XCNLStruct&, XCNLStruct::Field&, xziar::nailang::MetaSet&) {}
    virtual void OutputStruct(const XCNLStruct& target, std::u32string& output) = 0;
protected:
    virtual void FillFieldOffsets(XCNLStruct& target);
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
    [[nodiscard]] common::span<std::unique_ptr<XCNLStruct>> GetCustomStructs() const noexcept
    {
        return GetExecutor().GetContext().CustomStructs;
    }
    void StringifyBasicField(const XCNLStruct::Field& field, const XCNLStruct& target, std::u32string& output) const noexcept;
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
    static xziar::nailang::CustomVar Create(xziar::nailang::ArrarRef arr);
    static size_t ToIndex(const xziar::nailang::CustomVar& var, const xziar::nailang::ArrarRef& arr, std::u32string_view field);
    xziar::nailang::Arg HandleSubFields(const xziar::nailang::CustomVar&, xziar::nailang::SubQuery<const xziar::nailang::Expr>&) override;
    xziar::nailang::Arg HandleIndexes(const xziar::nailang::CustomVar&, xziar::nailang::SubQuery<xziar::nailang::Arg>&) override;
    bool HandleAssign(xziar::nailang::CustomVar& var, xziar::nailang::Arg arg) override;
    xziar::nailang::CompareResult CompareSameClass(const xziar::nailang::CustomVar&, const xziar::nailang::CustomVar&) final;
    common::str::StrVariant<char32_t> ToString(const xziar::nailang::CustomVar& var) noexcept override;
    std::u32string_view GetTypeName(const xziar::nailang::CustomVar&) noexcept override;
    std::u32string GetExactType(const xziar::nailang::ArrarRef& arr) noexcept;
public:
    static xziar::nailang::ArrarRef ToArray(const xziar::nailang::CustomVar& var) noexcept;
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
        return Create(xziar::nailang::ArrarRef::Create(target));
    }
};

class XCOMPBASAPI GeneralVec : public GeneralVecRef
{
    void IncreaseRef(const xziar::nailang::CustomVar& var) noexcept override;
    void DecreaseRef(xziar::nailang::CustomVar& var) noexcept override;
    common::str::StrVariant<char32_t> ToString(const xziar::nailang::CustomVar& var) noexcept override;
    std::u32string_view GetTypeName(const xziar::nailang::CustomVar&) noexcept override;
public:
    static xziar::nailang::CustomVar Create(xziar::nailang::ArrarRef::Type type, size_t len);
    template<typename T>
    static xziar::nailang::CustomVar Create(size_t len)
    {
        static_assert(GeneralVecRef::CheckType<T>(), "only allow arithmetic type");
        common::span<T> dummy(reinterpret_cast<T*>(nullptr), 1);
        const auto type = xziar::nailang::ArrarRef::Create(dummy).ElementType;
        return Create(type, len);
    }
};



#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
}