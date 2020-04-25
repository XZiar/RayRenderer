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
struct VarLookup
{
    std::u32string_view Name;
    constexpr VarLookup(std::u32string_view name) : Name(name), Offset(0), NextOffset(0)
    {
        FindNext();
    }
    constexpr VarLookup(const VarLookup& lookup) : Name(lookup.Name), Offset(0), NextOffset(0)
    {
        FindNext();
    }
    constexpr std::u32string_view Part() const noexcept
    {
        return Name.substr(Offset, NextOffset - Offset);
    }
    constexpr bool Next() noexcept
    {
        Offset = std::min(NextOffset + 1, Name.size());
        return FindNext();
    }
private:
    constexpr bool FindNext() noexcept
    {
        if (Offset >= Name.size())
            return false;
        const auto pos = Name.find(U'.', Offset);
        NextOffset = pos == Name.npos ? Name.size() : pos;
        return true;
    }
    size_t Offset, NextOffset;
};

struct LocalFunc
{
    const Block* Body;
    common::span<std::u32string_view> ArgNames;

    constexpr operator bool() const noexcept { return Body != nullptr; }
};
}


class NAILANGAPI EvaluateContext
{
    friend class NailangRuntimeBase;
protected:
    static bool CheckIsLocal(std::u32string_view& name) noexcept;

    std::shared_ptr<EvaluateContext> ParentContext;

    virtual Arg LookUpArgInside(detail::VarLookup var) const = 0;
    virtual bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) = 0;
public:
    virtual ~EvaluateContext();

    virtual Arg LookUpArg(std::u32string_view var) const;
    virtual bool SetArg(std::u32string_view var, Arg arg);
    virtual detail::LocalFunc LookUpFunc(std::u32string_view name) = 0;
    virtual bool SetFunc(const Block* block, common::span<const RawArg> args) = 0;
    virtual bool SetFunc(const detail::LocalFunc& func) = 0;
    virtual void SetReturnArg(Arg arg) = 0;
    virtual Arg  GetReturnArg() const = 0;
    virtual size_t GetArgCount() const noexcept = 0;
    virtual size_t GetFuncCount() const noexcept = 0;
};

class NAILANGAPI BasicEvaluateContext : public EvaluateContext
{
protected:
    using LocalFuncHolder = std::tuple<const Block*, uint32_t, uint32_t>;
    std::vector<std::u32string_view> LocalFuncArgNames;
    std::optional<Arg> ReturnArg;
    virtual bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) = 0;
public:
    ~BasicEvaluateContext() override;

    bool SetFunc(const Block* block, common::span<const RawArg> args) override;
    bool SetFunc(const detail::LocalFunc& func) override;

    void SetReturnArg(Arg arg) override;
    Arg GetReturnArg() const override;
};

class NAILANGAPI LargeEvaluateContext : public BasicEvaluateContext
{
protected:
    std::map<std::u32string, Arg, std::less<>> ArgMap;
    std::map<std::u32string_view, LocalFuncHolder, std::less<>> LocalFuncMap;
    std::optional<Arg> ReturnArg;

    Arg LookUpArgInside(detail::VarLookup var) const override;
    bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~LargeEvaluateContext() override;

    detail::LocalFunc LookUpFunc(std::u32string_view name) override;

    size_t GetArgCount() const noexcept override;
    size_t GetFuncCount() const noexcept override;
};

class NAILANGAPI CompactEvaluateContext : public BasicEvaluateContext
{
protected:
    std::vector<std::tuple<uint32_t, uint32_t, Arg>> Args;
    std::vector<std::pair<std::u32string_view, LocalFuncHolder>> LocalFuncs;
    std::vector<char32_t> StrPool;
    std::optional<Arg> ReturnArg;

    std::u32string_view GetStr(const uint32_t offset, const uint32_t len) const noexcept;

    Arg LookUpArgInside(detail::VarLookup var) const override;
    bool SetArgInside(detail::VarLookup var, Arg arg, const bool force) override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~CompactEvaluateContext() override;

    detail::LocalFunc LookUpFunc(std::u32string_view name) override;

    size_t GetArgCount() const noexcept override;
    size_t GetFuncCount() const noexcept override;
};


struct NAILANGAPI EmbedOpEval
{
    static Arg Equal        (const Arg& left, const Arg& right) noexcept;
    static Arg NotEqual     (const Arg& left, const Arg& right) noexcept;
    static Arg Less         (const Arg& left, const Arg& right) noexcept;
    static Arg LessEqual    (const Arg& left, const Arg& right) noexcept;
    static Arg Greater      (const Arg& left, const Arg& right) noexcept { return Less(right, left); }
    static Arg GreaterEqual (const Arg& left, const Arg& right) noexcept { return LessEqual(right, left); }
    static Arg And          (const Arg& left, const Arg& right) noexcept;
    static Arg Or           (const Arg& left, const Arg& right) noexcept;
    static Arg Add          (const Arg& left, const Arg& right) noexcept;
    static Arg Sub          (const Arg& left, const Arg& right) noexcept;
    static Arg Mul          (const Arg& left, const Arg& right) noexcept;
    static Arg Div          (const Arg& left, const Arg& right) noexcept;
    static Arg Rem          (const Arg& left, const Arg& right) noexcept;
    static Arg Not          (const Arg& arg) noexcept;

    static std::optional<Arg> Eval(const std::u32string_view opname, common::span<const Arg> args) noexcept;
};


namespace detail
{
struct ExceptionTarget
{
    enum class Type { Empty, Arg, RawArg, Assignment, FuncCall, RawBlock, Block };
    std::variant<std::monostate, Arg, RawArg, const FuncCall*, FuncCall, BlockContent> Target;

    constexpr ExceptionTarget() noexcept {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg) noexcept : Target(std::forward<T>(arg)) {}

    constexpr Type GetType() const noexcept
    {
        switch (Target.index())
        {
        case 0:  return Type::Empty;
        case 1:  return Type::Arg;
        case 2:  return Type::RawArg;
        case 3:  return Type::FuncCall;
        case 4:  return Type::FuncCall;
        case 5: 
            switch (std::get<5>(Target).GetType())
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
    constexpr auto GetVar() const
    {
        Expects(GetType() == T);
        if constexpr (T == Type::Empty)
            return {};
        else if constexpr (T == Type::Arg)
            return std::get<1>(Target);
        else if constexpr (T == Type::RawArg)
            return std::get<2>(Target);
        else if constexpr (T == Type::Assignment)
            return std::get<5>(Target).Get<Assignment>();
        else if constexpr (T == Type::RawBlock)
            return std::get<5>(Target).Get<RawBlock>();
        else if constexpr (T == Type::Block)
            return std::get<5>(Target).Get<Block>();
        else if constexpr (T == Type::FuncCall)
        {
            switch (Target.index())
            {
            case 3:  return std::get<3>(Target);
            case 4:  return &std::get<4>(Target);
            case 5:  return std::get<5>(Target).Get<FuncCall>();
            default: return nullptr;
            }
        }
        else
            static_assert(!common::AlwaysTrue<decltype(T)>, "");
    }

    static ExceptionTarget NewFuncCall(const std::u32string_view func) noexcept
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
public:
    NailangRuntimeException(const std::u16string_view& msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {}, const std::any& data = {}) :
        BaseException(TYPENAME, msg, data), Target(std::move(target)), Scope(std::move(scope))
    { }
private:
    mutable std::shared_ptr<EvaluateContext> EvalContext;
}; 


class NAILANGAPI NailangRuntimeBase
{
protected:
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
    struct NAILANGAPI InnerContextScope : public common::NonCopyable, public common::NonMovable
    {
        NailangRuntimeBase& Host;
        std::shared_ptr<EvaluateContext> Context;
        InnerContextScope(NailangRuntimeBase& host, std::shared_ptr<EvaluateContext>&& context);
        ~InnerContextScope();
    };

    std::shared_ptr<EvaluateContext> EvalContext;
    
    void ThrowByArgCount(const FuncCall& call, const size_t count) const;
    void ThrowByArgCount(common::span<const Arg> args, const size_t count) const;
    void ThrowByArgType(const Arg& arg, const Arg::InternalType type) const;
    void ThrowIfNotFuncTarget(const std::u32string_view func, const FuncTarget target, const FuncTarget::Type type) const;
    void ThrowIfBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    void ThrowIfNotBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    virtual MetaFuncResult HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& target, common::span<const FuncCall> metas);
            bool HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target, BlockContext& ctx);
    virtual void HandleException(const NailangRuntimeException& ex) const;

    inline Arg EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target)
    {
        if (call.Args.size() == 0)
            return EvaluateFunc(call.Name, {}, metas, target);
        std::vector<Arg> args;
        args.reserve(call.Args.size());
        for (const auto& rawarg : call.Args)
            args.emplace_back(EvaluateArg(rawarg));
        return EvaluateFunc(call.Name, args, metas, target);
    }
    virtual Arg  EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall> metas, const FuncTarget target);
    virtual Arg  EvaluateLocalFunc(const detail::LocalFunc& func, common::span<const Arg> args, common::span<const FuncCall> metas, const FuncTarget target);
    virtual Arg  EvaluateUnknwonFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall> metas, const FuncTarget target);
    virtual Arg  EvaluateArg(const RawArg& arg);
    virtual void OnAssignment(const Assignment& assign, common::span<const FuncCall> metas);
    virtual void OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas);
    virtual void OnFuncCall(const FuncCall& call, common::span<const FuncCall> metas, BlockContext& ctx);
    virtual ProgramStatus OnInnerBlock(const Block& block, common::span<const FuncCall> metas);
    virtual void ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas, BlockContext& ctx);
    virtual ProgramStatus ExecuteBlock(BlockContext ctx);
public:
    NailangRuntimeBase(std::shared_ptr<EvaluateContext> context);
    virtual ~NailangRuntimeBase();
    void ExecuteBlock(const Block& block, common::span<const FuncCall> metas, const bool checkMetas = true);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
