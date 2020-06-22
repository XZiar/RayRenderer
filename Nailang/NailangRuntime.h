#pragma once
#include "NailangStruct.h"
#include "common/Exceptions.hpp"
#include <map>
#include <vector>
#include <memory>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::nailang
{


namespace detail
{
struct VarHolder
{
    enum class VarType { Normal, Local, Root };
    constexpr VarHolder(const std::u32string_view name) : 
        Ptr(name.data()), Size(gsl::narrow_cast<uint32_t>(name.size())), Type(InitType(name))
    { }
    constexpr VarHolder(const VarHolder& other) = default;
    template<typename T>
    constexpr VarHolder(T&& val, std::enable_if_t<std::is_constructible_v<std::u32string_view, T>>* = nullptr) 
        : VarHolder(common::str::ToStringView(val)) { }

    constexpr std::u32string_view GetRawName() const noexcept { return { Ptr, Size }; }
    constexpr std::u32string_view GetFull() const noexcept 
    {
        const uint32_t prefix = Type == VarType::Normal ? 0 : 1;
        Expects(Size > prefix);
        return { Ptr + prefix, static_cast<size_t>(Size) - prefix };
    }
    constexpr VarType GetType() const noexcept { return Type; }
protected:
    static constexpr VarType InitType(const std::u32string_view name) noexcept
    {
        if (name.size() > 0)
        {
            switch (name[0])
            {
            case U':':  return VarType::Local;
            case U'`':  return VarType::Root;
            default:    break;
            }
        }
        return VarType::Normal;
    }

    const char32_t* Ptr;
    uint32_t Size;
    VarType Type;
};
struct VarLookup : public VarHolder
{
    constexpr VarLookup(const std::u32string_view name) : VarHolder(name), Offset(Type == VarType::Normal ? 0 : 1), NextOffset(0)
    {
        FindNext();
    }
    constexpr VarLookup(const VarHolder& other) : VarHolder(other), Offset(Type == VarType::Normal ? 0 : 1), NextOffset(0)
    {
        FindNext();
    }
    constexpr VarLookup(const VarLookup& other) : VarHolder(other), Offset(Type == VarType::Normal ? 0 : 1), NextOffset(0)
    {
        if (Offset == other.Offset)
            NextOffset = other.NextOffset;
        else
            FindNext();
    }
    
    [[nodiscard]] constexpr std::u32string_view Part() const noexcept
    {
        return { Ptr + Offset, NextOffset - Offset };
    }
    [[nodiscard]] constexpr std::u32string_view Rest() const noexcept
    {
        const auto offset = std::min(NextOffset + 1, Size);
        return { Ptr + offset, Size - offset };
    }
    [[nodiscard]] constexpr bool Next() noexcept
    {
        Offset = std::min(NextOffset + 1, Size);
        return FindNext();
    }
private:
    constexpr bool FindNext() noexcept
    {
        if (Offset >= Size)
            return false;
        const auto ptr = std::char_traits<char32_t>::find(Ptr + Offset, Size - Offset, U'.');
        Ensures(ptr == nullptr || ptr >= Ptr);
        NextOffset = ptr == nullptr ? Size : static_cast<uint32_t>(ptr - Ptr);
        return true;
    }
    uint32_t Offset, NextOffset;
};

struct LocalFunc
{
    const Block* Body;
    common::span<std::u32string_view> ArgNames;

    [[nodiscard]] constexpr operator bool() const noexcept { return Body != nullptr; }
};
}


class NAILANGAPI EvaluateContext
{
    friend class NailangRuntimeBase;
protected:
    std::shared_ptr<EvaluateContext> ParentContext;

    EvaluateContext* FindRoot() noexcept
    {
        auto ptr = this;
        while (ptr->ParentContext)
            ptr = ptr->ParentContext.get();
        return ptr;
    }
    const EvaluateContext* FindRoot() const noexcept
    {
        auto ptr = this;
        while (ptr->ParentContext)
            ptr = ptr->ParentContext.get();
        return ptr;
    }

    [[nodiscard]] virtual Arg LookUpArgInside(detail::VarLookup var) const = 0;
    virtual bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) = 0;
public:
    virtual ~EvaluateContext();

    [[nodiscard]] virtual Arg LookUpArg(detail::VarHolder var) const;
                  virtual bool SetArg(detail::VarHolder var, Arg arg);
    [[nodiscard]] virtual detail::LocalFunc LookUpFunc(std::u32string_view name) = 0;
                  virtual bool SetFunc(const Block* block, common::span<const RawArg> args) = 0;
                  virtual bool SetFunc(const detail::LocalFunc& func) = 0;
                  virtual void SetReturnArg(Arg arg) = 0;
    [[nodiscard]] virtual Arg  GetReturnArg() const = 0;
    [[nodiscard]] virtual size_t GetArgCount() const noexcept = 0;
    [[nodiscard]] virtual size_t GetFuncCount() const noexcept = 0;
};

class NAILANGAPI BasicEvaluateContext : public EvaluateContext
{
protected:
    using LocalFuncHolder = std::tuple<const Block*, uint32_t, uint32_t>;
    std::vector<std::u32string_view> LocalFuncArgNames;
    Arg ReturnArg;
    virtual bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) = 0;
    [[nodiscard]] virtual detail::LocalFunc LookUpFuncInside(std::u32string_view name) = 0;
public:
    ~BasicEvaluateContext() override;

    [[nodiscard]] detail::LocalFunc LookUpFunc(std::u32string_view name) override;
    bool SetFunc(const Block* block, common::span<const RawArg> args) override;
    bool SetFunc(const detail::LocalFunc& func) override;

    void SetReturnArg(Arg arg) override;
    [[nodiscard]] Arg GetReturnArg() const override;
};

class NAILANGAPI LargeEvaluateContext : public BasicEvaluateContext
{
protected:
    std::map<std::u32string, Arg, std::less<>> ArgMap;
    std::map<std::u32string_view, LocalFuncHolder, std::less<>> LocalFuncMap;

    [[nodiscard]] Arg LookUpArgInside(detail::VarLookup var) const override;
    bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) override;
    [[nodiscard]] detail::LocalFunc LookUpFuncInside(std::u32string_view name) override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~LargeEvaluateContext() override;

    [[nodiscard]] size_t GetArgCount() const noexcept override;
    [[nodiscard]] size_t GetFuncCount() const noexcept override;
};

class NAILANGAPI CompactEvaluateContext : public BasicEvaluateContext
{
protected:
    std::vector<std::pair<std::pair<uint32_t, uint32_t>, Arg>> Args;
    std::vector<std::pair<std::u32string_view, LocalFuncHolder>> LocalFuncs;
    std::vector<char32_t> StrPool;

    std::u32string_view GetStr(const uint32_t offset, const uint32_t len) const noexcept;

    [[nodiscard]] Arg LookUpArgInside(detail::VarLookup var) const override;
    bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) override;
    [[nodiscard]] detail::LocalFunc LookUpFuncInside(std::u32string_view name) override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~CompactEvaluateContext() override;

    [[nodiscard]] size_t GetArgCount() const noexcept override;
    [[nodiscard]] size_t GetFuncCount() const noexcept override;
};


struct NAILANGAPI EmbedOpEval
{
    [[nodiscard]] static std::optional<Arg> Equal        (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> NotEqual     (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Less         (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> LessEqual    (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Greater      (const Arg& left, const Arg& right) noexcept { return Less(right, left); }
    [[nodiscard]] static std::optional<Arg> GreaterEqual (const Arg& left, const Arg& right) noexcept { return LessEqual(right, left); }
    [[nodiscard]] static std::optional<Arg> And          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Or           (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Add          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Sub          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Mul          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Div          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Rem          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static std::optional<Arg> Not          (const Arg& arg) noexcept;
};


namespace detail
{
struct ExceptionTarget
{
    enum class Type { Empty, Arg, RawArg, Assignment, FuncCall, RawBlock, Block };
    std::variant<std::monostate, BlockContent, Arg, RawArg, const FuncCall*, FuncCall, const RawBlock*, const Block*> Target;

    constexpr ExceptionTarget() noexcept {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg) noexcept : Target(std::forward<T>(arg)) {}

    [[nodiscard]] constexpr Type GetType() const noexcept
    {
        switch (Target.index())
        {
        case 0:  return Type::Empty;
        case 2:  return Type::Arg;
        case 3:  return Type::RawArg;
        case 4:  return Type::FuncCall;
        case 5:  return Type::FuncCall;
        case 6:  return Type::RawBlock;
        case 7:  return Type::Block;
        case 1:
            switch (std::get<1>(Target).GetType())
            {
            case BlockContent::Type::Assignment: return Type::Assignment;
            case BlockContent::Type::FuncCall:   return Type::FuncCall;
            case BlockContent::Type::RawBlock:   return Type::RawBlock;
            case BlockContent::Type::Block:      return Type::Block;
            }
            return Type::Empty;
        default: return Type::Empty;
        }
    }
    template<Type T>
    [[nodiscard]] constexpr auto GetVar() const
    {
        Expects(GetType() == T);
        if constexpr (T == Type::Empty)
            return;
        else if constexpr (T == Type::Arg)
            return std::get<2>(Target);
        else if constexpr (T == Type::RawArg)
            return std::get<3>(Target);
        else if constexpr (T == Type::Assignment)
            return std::get<1>(Target).Get<Assignment>();
        else if constexpr (T == Type::RawBlock)
        {
            switch (Target.index())
            {
            case 6:  return std::get<6>(Target);
            case 1:  return std::get<1>(Target).Get<RawBlock>();
            default: return nullptr;
            }
        }
        else if constexpr (T == Type::Block)
        {
            switch (Target.index())
            {
            case 7:  return std::get<7>(Target);
            case 1:  return std::get<1>(Target).Get<Block>();
            default: return nullptr;
            }
        }
        else if constexpr (T == Type::FuncCall)
        {
            switch (Target.index())
            {
            case 4:  return  std::get<4>(Target);
            case 5:  return &std::get<5>(Target);
            case 1:  return  std::get<1>(Target).Get<FuncCall>();
            default: return nullptr;
            }
        }
        else
            static_assert(!common::AlwaysTrue2<T>, "");
    }

    [[nodiscard]] static ExceptionTarget NewFuncCall(const std::u32string_view func) noexcept
    {
        return FuncCall{ func, {} };
    }
};
}

class NAILANGAPI NailangRuntimeException : public common::BaseException
{
    friend class NailangRuntimeBase;
public:
    EXCEPTION_CLONE_EX(NailangRuntimeException);
    detail::ExceptionTarget Target;
    detail::ExceptionTarget Scope;
    NailangRuntimeException(const std::u16string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {}, const std::any& data = {}) :
        BaseException(TYPENAME, msg, data), Target(std::move(target)), Scope(std::move(scope))
    { }
protected:
    NailangRuntimeException(const char* const type, const std::u16string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope, const std::any& data) :
        BaseException(type, msg, data), Target(std::move(target)), Scope(std::move(scope))
    { }
private:
    mutable std::shared_ptr<EvaluateContext> EvalContext;
};

class NAILANGAPI NailangFormatException : public NailangRuntimeException
{
    common::SharedString<char32_t> Formatter;
public:
    EXCEPTION_CLONE_EX(NailangFormatException);
    NailangFormatException(const std::u32string_view formatter, const std::runtime_error& err);
    NailangFormatException(const std::u32string_view formatter, const Arg& arg, const std::u16string_view reason = u"");
    ~NailangFormatException() override;
};

class NAILANGAPI NailangCodeException : public NailangRuntimeException
{
public:
    EXCEPTION_CLONE_EX(NailangCodeException);
    NailangCodeException(const std::u32string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {}, const std::any& data = {});
};


enum class ArgLimits { Exact, AtMost, AtLeast };
class NAILANGAPI NailangRuntimeBase
{
private:
    struct NAILANGAPI InnerContextScope : public common::NonCopyable, public common::NonMovable
    {
        NailangRuntimeBase& Host;
        std::shared_ptr<EvaluateContext> Context;
        InnerContextScope(NailangRuntimeBase* host, std::shared_ptr<EvaluateContext>&& context);
        ~InnerContextScope();
    };
protected:
    static std::u32string_view ArgTypeName(const Arg::Type type) noexcept;
    static std::u32string_view ArgTypeName(const RawArg::Type type) noexcept;
    enum class ProgramStatus { Next = 0, Repeat, Break, Return, End };
    enum class MetaFuncResult { Unhandled = 0, Next, Repeat, Skip };
    struct BlockContext
    {
        common::span<const FuncCall> MetaScope;
        const Block* BlockScope;
        ProgramStatus Status = ProgramStatus::Next;
        constexpr BlockContext() noexcept : 
            MetaScope(), BlockScope(nullptr) {}
        constexpr BlockContext(const Block& block, common::span<const FuncCall> metas) noexcept :
            MetaScope(metas), BlockScope(&block) { }

        constexpr bool SearchMeta(const std::u32string_view name) const noexcept
        {
            for (const auto& meta : MetaScope)
                if (meta.Name == name)
                    return true;
            return false;
        }
    };
    struct ContentContext
    {
        common::span<const FuncCall> MetaScope;
        const BlockContent& ContentScope;
        constexpr ContentContext(const BlockContent& content, common::span<const FuncCall> metas) noexcept :
            MetaScope(metas), ContentScope(content) { }
    };
    struct FuncTarget
    {
        enum class Type { Empty, Block, Meta };
        constexpr static uintptr_t Flag     = 0x1;
        constexpr static uintptr_t InvFlag  = ~Flag;
        static constexpr std::u32string_view FuncTargetName(const Type type) noexcept
        {
            using namespace std::string_view_literals;
            switch (type)
            {
            case Type::Empty: return U"part of statement"sv;
            case Type::Block: return U"funccall"sv;
            case Type::Meta:  return U"metafunc"sv;
            default:          return U"error"sv;
            }
        }

        uintptr_t Pointer;
        constexpr FuncTarget() noexcept : Pointer(0) {}
        FuncTarget(BlockContext& ctx) noexcept : Pointer(reinterpret_cast<uintptr_t>(&ctx)) { }
        FuncTarget(const ContentContext& ctx) noexcept : Pointer(reinterpret_cast<uintptr_t>(&ctx) | Flag) { }
        
        constexpr Type GetType() const noexcept 
        {
            if (Pointer == 0)   return Type::Empty;
            if (Pointer & Flag) return Type::Meta;
            else                return Type::Block;
        }
        BlockContext& GetBlockContext() const noexcept 
        { 
            return *reinterpret_cast<BlockContext*>(Pointer);
        }
        const ContentContext& GetContentContext() const noexcept 
        { 
            return *reinterpret_cast<const ContentContext*>(Pointer & InvFlag);
        }
    };

    std::shared_ptr<EvaluateContext> RootContext;
    std::shared_ptr<EvaluateContext> EvalContext;
    MemoryPool MemPool;
    
    void ThrowByArgCount(const FuncCall& call, const size_t count, const ArgLimits limit = ArgLimits::Exact) const;
    void ThrowByArgType(const Arg& arg, const Arg::Type type) const;
    void ThrowByArgType(const FuncCall& call, const Arg& arg, const Arg::Type type, size_t idx) const;
    void ThrowIfNotFuncTarget(const std::u32string_view func, const FuncTarget target, const FuncTarget::Type type) const;
    void ThrowIfBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    void ThrowIfNotBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    void EvaluateFuncArgs(Arg* args, const Arg::Type* types, const size_t size, const ArgLimits limit, const FuncCall& call);
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args;
        EvaluateFuncArgs(args.data(), nullptr, N, Limit, call);
        return args;
    }

    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call, const std::array<Arg::Type, N>& types)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args;
        EvaluateFuncArgs(args.data(), types.data(), N, Limit, call);
        return args;
    }

    [[nodiscard]] std::optional<InnerContextScope> InnerScope(const bool newContext = true);
    [[nodiscard]] bool HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target, BlockContext& ctx);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const RawArg> args);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const Arg> args);

                  virtual void HandleException(const NailangRuntimeException& ex) const;
                  virtual std::shared_ptr<EvaluateContext> ConstructEvalContext() const;
    [[nodiscard]] virtual MetaFuncResult HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& target, common::span<const FuncCall> metas);
                  virtual Arg  EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target);
    [[nodiscard]] virtual Arg  EvaluateLocalFunc(const detail::LocalFunc& func, const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target);
    [[nodiscard]] virtual Arg  EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target);
                  virtual Arg  EvaluateArg(const RawArg& arg);
    [[nodiscard]] virtual std::optional<Arg> EvaluateExtendMathFunc(const FuncCall& call, std::u32string_view mathName, common::span<const FuncCall> metas);
    [[nodiscard]] virtual std::optional<Arg> EvaluateUnaryExpr(const UnaryExpr& expr);
    [[nodiscard]] virtual std::optional<Arg> EvaluateBinaryExpr(const BinaryExpr& expr);
                  virtual void OnAssignment(const Assignment& assign, common::span<const FuncCall> metas);
                  virtual void OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas);
                  virtual void OnFuncCall(const FuncCall& call, common::span<const FuncCall> metas, BlockContext& ctx);
    [[nodiscard]] virtual std::pair<ProgramStatus, Arg> OnInnerBlock(const Block& block, common::span<const FuncCall> metas);
                  virtual void ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas, BlockContext& ctx);
                  virtual ProgramStatus ExecuteBlock(BlockContext ctx);
public:
    NailangRuntimeBase(std::shared_ptr<EvaluateContext> context);
    virtual ~NailangRuntimeBase();
    void ExecuteBlock(const Block& block, common::span<const FuncCall> metas, const bool checkMetas = true, const bool innerScope = true);
    Arg EvaluateRawStatement(std::u32string_view content, const bool innerScope = true);
    Arg EvaluateRawStatements(std::u32string_view content, const bool innerScope = true);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
