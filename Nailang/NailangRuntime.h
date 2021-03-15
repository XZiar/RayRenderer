#pragma once
#include "NailangStruct.h"
#include "common/Exceptions.hpp"
#include "common/StringPool.hpp"
#include "common/STLEx.hpp"
#include <map>
#include <vector>
#include <memory>

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::nailang
{


struct LocalFunc
{
    const Block* Body;
    common::span<const std::u32string_view> ArgNames;
    common::span<const Arg> CapturedArgs;

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return Body != nullptr; }
};


class NAILANGAPI EvaluateContext
{
    friend class NailangRuntimeBase;
public:
    virtual ~EvaluateContext();
    [[nodiscard]] virtual ArgLocator LocateArg(const LateBindVar& var, const bool create) noexcept = 0;
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const = 0;
    virtual bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args) = 0;
    virtual bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args) = 0;
    [[nodiscard]] virtual size_t GetArgCount() const noexcept = 0;
    [[nodiscard]] virtual size_t GetFuncCount() const noexcept = 0;
};

class NAILANGAPI BasicEvaluateContext : public EvaluateContext
{
private:
    std::vector<std::u32string_view> LocalFuncArgNames;
    std::vector<Arg> LocalFuncCapturedArgs;
    std::pair<uint32_t, uint32_t> InsertCaptures(
        common::span<std::pair<std::u32string_view, Arg>> capture);
    template<typename T>
    std::pair<uint32_t, uint32_t> InsertNames(
        common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const T> argNames);
protected:
    using LocalFuncHolder = std::tuple<const Block*, std::pair<uint32_t, uint32_t>, std::pair<uint32_t, uint32_t>>;
    [[nodiscard]] virtual LocalFuncHolder LookUpFuncInside(std::u32string_view name) const = 0;
    virtual bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) = 0;
public:
    ~BasicEvaluateContext() override;

    [[nodiscard]] LocalFunc LookUpFunc(std::u32string_view name) const override;
    bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args) override;
    bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args) override;
};

class NAILANGAPI LargeEvaluateContext : public BasicEvaluateContext
{
protected:
    std::map<std::u32string, Arg, std::less<>> ArgMap;
    std::map<std::u32string_view, LocalFuncHolder, std::less<>> LocalFuncMap;

    [[nodiscard]] LocalFuncHolder LookUpFuncInside(std::u32string_view name) const override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~LargeEvaluateContext() override;

    [[nodiscard]] ArgLocator LocateArg(const LateBindVar& var, const bool create) noexcept override;
    [[nodiscard]] size_t GetArgCount() const noexcept override;
    [[nodiscard]] size_t GetFuncCount() const noexcept override;
};

class NAILANGAPI CompactEvaluateContext : public BasicEvaluateContext
{
private:
    common::HashedStringPool<char32_t> ArgNames;
protected:
    std::vector<std::pair<common::StringPiece<char32_t>, Arg>> Args;
    std::vector<std::pair<std::u32string_view, LocalFuncHolder>> LocalFuncs;

    std::u32string_view GetStringView(common::StringPiece<char32_t> piece) const noexcept
    {
        return ArgNames.GetStringView(piece);
    }
    [[nodiscard]] LocalFuncHolder LookUpFuncInside(std::u32string_view name) const override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~CompactEvaluateContext() override;

    [[nodiscard]] ArgLocator LocateArg(const LateBindVar& var, const bool create) noexcept override;
    [[nodiscard]] size_t    GetArgCount() const noexcept override;
    [[nodiscard]] size_t    GetFuncCount() const noexcept override;
};


struct NAILANGAPI EmbedOpEval
{
    [[nodiscard]] static Arg Equal        (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg NotEqual     (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Less         (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg LessEqual    (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Greater      (const Arg& left, const Arg& right) noexcept { return Less(right, left); }
    [[nodiscard]] static Arg GreaterEqual (const Arg& left, const Arg& right) noexcept { return LessEqual(right, left); }
    [[nodiscard]] static Arg And          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Or           (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Add          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Sub          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Mul          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Div          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Rem          (const Arg& left, const Arg& right) noexcept;
    [[nodiscard]] static Arg Not          (const Arg& arg) noexcept;
};


namespace detail
{
struct ExceptionTarget
{
    enum class Type { Empty, Arg, Expr, AssignExpr, FuncCall, RawBlock, Block };
    using VType = std::variant<std::monostate, Statement, Arg, Expr, FuncCall, const RawBlock*, const Block*>;
    VType Target;

    constexpr ExceptionTarget() noexcept {}
    ExceptionTarget(const CustomVar& arg) noexcept : Target{ Arg(arg) } {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg, std::enable_if_t<common::VariantHelper<VType>::Contains<std::decay_t<T>>()>* = nullptr) noexcept
        : Target{ std::forward<T>(arg) } {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg, std::enable_if_t<common::is_specialization<std::decay_t<T>, std::variant>::value>* = nullptr) noexcept
        : Target{ common::VariantHelper<VType>::Convert(std::forward<T>(arg)) } {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg, std::enable_if_t<std::is_base_of_v<FuncCall, std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, FuncCall>>* = nullptr) noexcept
        : Target{ static_cast<const FuncCall&>(arg) } {}

    constexpr explicit operator bool() const noexcept { return Target.index() != 0; }
    [[nodiscard]] constexpr Type GetType() const noexcept
    {
        switch (Target.index())
        {
        case 0:  return Type::Empty;
        case 2:  return Type::Arg;
        case 3:  return Type::Expr;
        case 4:  return Type::FuncCall;
        case 5:  return Type::RawBlock;
        case 6:  return Type::Block;
        case 1:
            switch (std::get<1>(Target).TypeData)
            {
            case Statement::Type::Assign:   return Type::AssignExpr;
            case Statement::Type::FuncCall: return Type::FuncCall;
            case Statement::Type::RawBlock: return Type::RawBlock;
            case Statement::Type::Block:    return Type::Block;
            default:                        return Type::Empty;
            }
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
        else if constexpr (T == Type::Expr)
            return std::get<3>(Target);
        else if constexpr (T == Type::AssignExpr)
            return std::get<1>(Target).Get<AssignExpr>();
        else if constexpr (T == Type::RawBlock)
        {
            switch (Target.index())
            {
            case 5:  return std::get<5>(Target);
            case 1:  return std::get<1>(Target).Get<RawBlock>();
            default: return nullptr;
            }
        }
        else if constexpr (T == Type::Block)
        {
            switch (Target.index())
            {
            case 6:  return std::get<6>(Target);
            case 1:  return std::get<1>(Target).Get<Block>();
            default: return nullptr;
            }
        }
        else if constexpr (T == Type::FuncCall)
        {
            switch (Target.index())
            {
            case 4:  return &std::get<4>(Target);
            case 1:  return  std::get<1>(Target).Get<FuncCall>();
            default: return nullptr;
            }
        }
        else
            static_assert(!common::AlwaysTrue2<T>, "");
    }
};
}


class NAILANGAPI NailangRuntimeException : public common::BaseException
{
    friend NailangRuntimeBase;
    PREPARE_EXCEPTION(NailangRuntimeException, BaseException,
        detail::ExceptionTarget Target;
        detail::ExceptionTarget Scope;
        ExceptionInfo(const std::u16string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope)
            : ExceptionInfo(TYPENAME, msg, target, scope)
        { }
    protected:
        template<typename T>
        ExceptionInfo(const char* type, T&& msg, detail::ExceptionTarget target, detail::ExceptionTarget scope)
            : TPInfo(type, std::forward<T>(msg)), Target(std::move(target)), Scope(std::move(scope))
        { }
    );
public:
    NailangRuntimeException(const std::u16string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {}) :
        NailangRuntimeException(T_<ExceptionInfo>{}, msg, std::move(target), std::move(scope))
    { }
};

class NAILANGAPI NailangFormatException : public NailangRuntimeException
{
    friend NailangRuntimeBase;
    PREPARE_EXCEPTION(NailangFormatException, NailangRuntimeException,
        std::u32string Formatter;
        ExceptionInfo(const std::u16string_view msg, const std::u32string_view formatter, detail::ExceptionTarget target = {})
            : ExceptionInfo(TYPENAME, msg, formatter, target)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u16string_view msg, const std::u32string_view formatter, detail::ExceptionTarget target)
            : TPInfo(type, msg, target, {}), Formatter(formatter)
        { }
    );
public:
    NailangFormatException(const std::u32string_view formatter, const std::runtime_error& err);
    NailangFormatException(const std::u32string_view formatter, const Arg& arg, const std::u16string_view reason = u"");
};

class NAILANGAPI NailangCodeException : public NailangRuntimeException
{
    PREPARE_EXCEPTION(NailangCodeException, NailangRuntimeException,
        ExceptionInfo(const std::u32string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope)
            : ExceptionInfo(TYPENAME, msg, target, scope)
        { }
    protected:
        ExceptionInfo(const char* type, const std::u32string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope);
    );
public:
    NailangCodeException(const std::u32string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {});
};


enum class ArgLimits { Exact, AtMost, AtLeast };


struct NAILANGAPI NailangHelper
{
    [[nodiscard]] static size_t BiDirIndexCheck(const size_t size, const Arg& idx, const Expr* src = nullptr);
    template<typename F>
    static bool LocateWrite(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func);
    template<typename F>
    static Arg LocateRead(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func);
    template<typename F>
    static Arg LocateRead(Arg& target, SubQuery query, NailangExecutor& executor, const F& func);
    template<bool IsWrite, typename F>
    static auto LocateAndExecute(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func)
        -> decltype(func(std::declval<ArgLocator&>()));
};


struct FuncPack : public FuncCall
{
    common::span<Arg> Params;
    constexpr FuncPack(const FuncCall& call, common::span<Arg> params) noexcept :
        FuncCall(call), Params(params) { }
    auto NamePart(size_t idx) const { return Name->GetPart(idx); }
    constexpr size_t NamePartCount() const noexcept { return Name->PartCount; }
    common::Optional<Arg&> TryGet(size_t idx) noexcept
    {
        if (idx < Params.size())
            return { std::in_place_t{}, &Params[idx] };
        else
            return {};
    }
    common::Optional<const Arg&> TryGet(size_t idx) const noexcept
    {
        if (idx < Params.size())
            return { std::in_place_t{}, &Params[idx] };
        else
            return {};
    }
    template<typename... Args, typename R>
    common::OptionalT<R> TryGet(size_t idx, R(Arg::* func)(Args...), Args&&... args)
    {
        if (idx < Params.size())
            return { common::detail::OptionalT<R>::Create((Params[idx].*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R>
    common::OptionalT<R> TryGet(size_t idx, R(Arg::* func)(Args...) noexcept, Args&&... args) noexcept
    {
        if (idx < Params.size())
            return { common::detail::OptionalT<R>::Create((Params[idx].*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R>
    common::OptionalT<R> TryGet(size_t idx, R(Arg::* func)(Args...) const, Args&&... args) const
    {
        if (idx < Params.size())
            return { common::detail::OptionalT<R>::Create((Params[idx].*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R>
    common::OptionalT<R> TryGet(size_t idx, R(Arg::* func)(Args...) const noexcept, Args&&... args) const noexcept
    {
        if (idx < Params.size())
            return { common::detail::OptionalT<R>::Create((Params[idx].*func)(std::forward<Args>(args)...)) };
        else
            return {};
    }
    template<typename... Args, typename R>
    R TryGetOr(size_t idx, R(Arg::* func)(Args...), R def, Args&&... args)
    {
        if (idx < Params.size())
            return (Params[idx].*func)(std::forward<Args>(args)...);
        else
            return std::move(def);
    }
    template<typename... Args, typename R>
    R TryGetOr(size_t idx, R(Arg::* func)(Args...) noexcept, R def, Args&&... args) noexcept
    {
        if (idx < Params.size())
            return (Params[idx].*func)(std::forward<Args>(args)...);
        else
            return std::move(def);
    }
    template<typename... Args, typename R>
    R TryGetOr(size_t idx, R(Arg::* func)(Args...) const, R def, Args&&... args) const
    {
        if (idx < Params.size())
            return (Params[idx].*func)(std::forward<Args>(args)...);
        else
            return std::move(def);
    }
    template<typename... Args, typename R>
    R TryGetOr(size_t idx, R(Arg::* func)(Args...) const noexcept, R def, Args&&... args) const noexcept
    {
        if (idx < Params.size())
            return (Params[idx].*func)(std::forward<Args>(args)...);
        else
            return std::move(def);
    }
};
struct FuncEvalPack : public FuncPack
{
    common::span<const FuncCall> Metas;
    constexpr FuncEvalPack(const FuncCall& call, common::span<Arg> params, common::span<const FuncCall> metas = {}) noexcept :
        FuncPack(call, params), Metas(metas) { }
};
class MetaSet
{
private:
    struct MetaFuncWrapper
    {
        MetaSet& Host;
        size_t Index;
        constexpr const FuncCall* operator->() const noexcept { return &Host.Metas[Index]; }
        constexpr const FuncCall& operator*()  const noexcept { return  Host.Metas[Index]; }
        bool IsUsed() const noexcept { return Host.AccessBitmap.Get(Index); }
        void SetUsed() noexcept { Host.AccessBitmap.Set(Index, true); }
    };
    [[nodiscard]] constexpr MetaFuncWrapper Get(size_t index) noexcept
    {
        return { *this, index };
    }
    using ItType = common::container::IndirectIterator<MetaSet, MetaFuncWrapper, &MetaSet::Get>;
    friend ItType;
    common::SmallBitset AccessBitmap;
public:
    common::span<const FuncCall> Metas;
    MetaSet(common::span<const FuncCall> metas) noexcept : AccessBitmap(metas.size(), false), Metas(metas)
    { }
    [[nodiscard]] constexpr ItType begin() noexcept
    {
        return { this, 0 };
    }
    [[nodiscard]] constexpr ItType end() noexcept
    {
        return { this, Metas.size() };
    }
};
struct MetaEvalPack : public FuncPack
{
    MetaSet& Metas;
    const Statement& Target;
    constexpr MetaEvalPack(const FuncCall& call, common::span<Arg> params, MetaSet& metas, const Statement& target) noexcept :
        FuncPack(call, params), Metas(metas), Target(target) { }
};


class NailangFrame
{
    friend NailangExecutor;
    friend NailangRuntimeBase;
private:
    struct FuncStackInfo
    {
        const FuncStackInfo* PrevInfo;
        const FuncCall* Func;
    };
    NailangFrame* const PrevFrame;
    const FuncStackInfo* FuncInfo;
    class FuncInfoHolder;
public:
    enum class ProgramStatus : uint8_t 
    { 
        Next    = 0b00000000, 
        End     = 0b00000001,
        LoopEnd = 0b00000010,
    };
    enum class FrameFlags : uint8_t
    {
        Empty       = 0b00000000,
        FlowScope   = 0b00000001,
        CallScope   = 0b00000010,
        FuncCall    = FlowScope | CallScope,
        LoopScope   = 0b00000100,
    };

    NailangExecutor* Executor;
    std::shared_ptr<EvaluateContext> Context;
    common::span<const FuncCall> MetaScope;
    const Block* BlockScope = nullptr;
    const Statement* CurContent = nullptr;
    Arg ReturnArg;
    FrameFlags Flags;
    ProgramStatus Status = ProgramStatus::Next;
    uint8_t IfRecord = 0;

    NailangFrame(NailangFrame* prev, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag) noexcept :
        PrevFrame(prev), FuncInfo(nullptr), Executor(nullptr), Context(ctx), Flags(flag) { }
    [[nodiscard]] constexpr bool Has(FrameFlags flag) const noexcept;
    [[nodiscard]] constexpr bool Has(ProgramStatus status) const noexcept;
    [[nodiscard]] constexpr NailangFrame* GetCallScope() noexcept;
    NailangFrame* SetReturn(Arg returnVal) noexcept;
    constexpr NailangFrame* SetBreak() noexcept;
    constexpr NailangFrame* SetContinue() noexcept;
};
MAKE_ENUM_BITFIELD(NailangFrame::ProgramStatus)
MAKE_ENUM_BITFIELD(NailangFrame::FrameFlags)
inline constexpr bool NailangFrame::Has(FrameFlags flag) const noexcept
{
    return HAS_FIELD(Flags, flag);
}
inline constexpr bool NailangFrame::Has(ProgramStatus status) const noexcept
{
    return HAS_FIELD(Status, status);
}
inline constexpr NailangFrame* NailangFrame::GetCallScope() noexcept
{
    for (auto frame = this; frame; frame = frame->PrevFrame)
    {
        if (HAS_FIELD(frame->Flags, FrameFlags::CallScope))
            return frame;
    }
    return nullptr;
}
inline NailangFrame* NailangFrame::SetReturn(Arg returnVal) noexcept
{
    for (auto frame = this; frame; frame = frame->PrevFrame)
    {
        frame->Status = ProgramStatus::End | ProgramStatus::LoopEnd;
        if (HAS_FIELD(frame->Flags, FrameFlags::FlowScope))
        {
            frame->ReturnArg = std::move(returnVal);
            return frame;
        }
    }
    return nullptr;
}
inline constexpr NailangFrame* NailangFrame::SetBreak() noexcept
{
    for (auto frame = this; frame; frame = frame->PrevFrame)
    {
        frame->Status = ProgramStatus::End | ProgramStatus::LoopEnd;
        if (HAS_FIELD(frame->Flags, FrameFlags::LoopScope))
        {
            return frame;
        }
    }
    return nullptr;
}
inline constexpr NailangFrame* NailangFrame::SetContinue() noexcept
{
    for (auto frame = this; frame; frame = frame->PrevFrame)
    {
        frame->Status = ProgramStatus::End;
        if (HAS_FIELD(frame->Flags, FrameFlags::LoopScope))
        {
            return frame;
        }
    }
    return nullptr;
}


class NAILANGAPI NailangBase
{
protected:
    [[noreturn]] virtual void HandleException(const NailangRuntimeException& ex) const = 0;

    void ThrowByArgCount(const FuncCall& func, const size_t count, const ArgLimits limit = ArgLimits::Exact) const;
    void ThrowByArgType(const FuncCall& func, const Expr::Type type, size_t idx) const;
    void ThrowByArgTypes(const FuncCall& func, common::span<const Expr::Type> types, const ArgLimits limit, size_t offset = 0) const;
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline void ThrowByArgTypes(const FuncCall& func, const std::array<Expr::Type, N>& types, size_t offset = 0) const
    {
        ThrowByArgTypes(func, types, Limit, offset);
    }
    template<size_t Min, size_t Max>
    forceinline void ThrowByArgTypes(const FuncCall& func, const std::array<Expr::Type, Max>& types, size_t offset = 0) const
    {
        ThrowByArgCount(func, Min + offset, ArgLimits::AtLeast);
        ThrowByArgTypes(func, types, ArgLimits::AtMost, offset);
    }
    void ThrowByParamType(const FuncCall& func, const Arg& arg, const Arg::Type type, size_t idx) const;
    void ThrowByParamTypes(const FuncPack& func, common::span<const Arg::Type> types, const ArgLimits limit = ArgLimits::Exact, size_t offset = 0) const;
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline void ThrowByParamTypes(const FuncPack& func, const std::array<Arg::Type, N>& types, size_t offset = 0) const
    {
        ThrowByParamTypes(func, types, Limit, offset);
    }
    template<size_t Min, size_t Max>
    forceinline void ThrowByParamTypes(const FuncPack& func, const std::array<Arg::Type, Max>& types, size_t offset = 0) const
    {
        ThrowByArgCount(func, Min + offset, ArgLimits::AtLeast);
        ThrowByParamTypes(func, types, ArgLimits::AtMost, offset);
    }
    void ThrowIfNotFuncTarget(const FuncCall& call, const FuncName::FuncInfo type) const;
    void ThrowIfStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const;
    void ThrowIfNotStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    [[nodiscard]] LateBindVar DecideDynamicVar(const Expr& arg, const std::u16string_view reciever) const;
};

class NAILANGAPI NailangExecutor : protected NailangBase
{
    friend NailangRuntimeBase;
protected:
    NailangRuntimeBase* Runtime = nullptr;
    forceinline constexpr NailangFrame& GetFrame() const noexcept;
    void EvalFuncArgs(const FuncCall& func, Arg* args, const Arg::Type* types, const size_t size);
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvalFuncArgs(const FuncCall& func)
    {
        ThrowByArgCount(func, N, Limit);
        std::array<Arg, N> args = {};
        EvalFuncArgs(func, args.data(), nullptr, std::min(N, func.Args.size()));
        return args;
    }
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvalFuncArgs(const FuncCall& func, const std::array<Arg::Type, N>& types)
    {
        ThrowByArgCount(func, N, Limit);
        std::array<Arg, N> args = {};
        EvalFuncArgs(func, args.data(), types.data(), std::min(N, func.Args.size()));
        return args;
    }
    [[nodiscard]] forceinline Arg EvalFuncSingleArg(const FuncCall& func, Arg::Type type, ArgLimits limit = ArgLimits::Exact, size_t offset = 0)
    {
        if (limit == ArgLimits::AtMost && func.Args.size() == offset)
            return {};
        ThrowByArgCount(func, 1 + offset, limit);
        auto arg = EvaluateExpr(func.Args[offset]);
        ThrowByParamType(func, arg, type, offset);
        return arg;
    }
    std::vector<Arg> EvalFuncAllArgs(const FuncCall& func);
public:
    enum class MetaFuncResult : uint8_t { Unhandled, Next, Skip, Return };
    NailangExecutor(NailangRuntimeBase* runtime) noexcept;
    virtual ~NailangExecutor();
    [[nodiscard]] virtual MetaFuncResult HandleMetaFunc(const FuncCall& meta, const Statement& target, MetaSet& allMetas);
    [[nodiscard]] virtual MetaFuncResult HandleMetaFunc(MetaEvalPack& meta);
    [[nodiscard]] virtual Arg EvaluateFunc(FuncEvalPack& func);
    [[nodiscard]] virtual Arg EvaluateExtendMathFunc(FuncEvalPack& func);
    [[nodiscard]] virtual Arg EvaluateUnknwonFunc(FuncEvalPack& func);
    [[nodiscard]] virtual Arg EvaluateUnaryExpr(const UnaryExpr& expr);
    [[nodiscard]] virtual Arg EvaluateBinaryExpr(const BinaryExpr& expr);
    [[nodiscard]] virtual Arg EvaluateTernaryExpr(const TernaryExpr& expr);
    [[nodiscard]] virtual Arg EvaluateQueryExpr(const QueryExpr& expr);
    [[noreturn]] void HandleException(const NailangRuntimeException& ex) const final;
    void ExecuteFrame();
    void HandleContent(const Statement& content, common::span<const FuncCall> metas);
    [[nodiscard]] virtual bool HandleMetaFuncs(MetaSet& allMetas, const Statement& target); // return if need eval target
    [[nodiscard]] virtual Arg EvaluateExpr(const Expr& arg);
    [[nodiscard]] virtual Arg EvaluateFunc(const FuncCall& func, common::span<const FuncCall> metas);
};


class NAILANGAPI NailangRuntimeBase : protected NailangBase
{
    friend NailangExecutor;
protected:
    template<typename T>
    class FrameHolder
    {
        static_assert(std::is_base_of_v<NailangFrame, T>);
        friend NailangRuntimeBase;
        NailangRuntimeBase& Host;
        T Frame;
        template<typename... Args>
        forceinline FrameHolder(NailangRuntimeBase* host, const std::shared_ptr<EvaluateContext>& ctx, NailangFrame::FrameFlags flags,
            Args&&... args) : Host(*host), Frame(host->CurFrame, ctx, flags, std::forward<Args>(args)...)
        {
            Expects(ctx);
            Host.CurFrame = &Frame;
        }
    public:
        COMMON_NO_COPY(FrameHolder)
        COMMON_NO_MOVE(FrameHolder)
        ~FrameHolder()
        {
            Expects(Host.CurFrame == &Frame);
            Host.CurFrame = Frame.PrevFrame;
        }
        forceinline constexpr NailangFrame* operator->() const noexcept
        {
            return Frame.PrevFrame;
        }
    };

    std::shared_ptr<EvaluateContext> RootContext;
    std::unique_ptr<NailangExecutor> BasicExecutor;
    NailangFrame* CurFrame = nullptr;
    MemoryPool MemPool;

    template<typename T = NailangFrame, typename... Args>
    [[nodiscard]] forceinline FrameHolder<T> PushFrame(std::shared_ptr<EvaluateContext> ctx, NailangFrame::FrameFlags flags, Args&&... args)
    {
        return { this, std::move(ctx), flags, std::forward<Args>(args)... };
    }
    template<typename T = NailangFrame, typename... Args>
    [[nodiscard]] forceinline FrameHolder<T> PushFrame(NailangFrame::FrameFlags flags, Args&&... args)
    {
        return { this, ConstructEvalContext(), flags, std::forward<Args>(args)... };
    }
    template<typename T = NailangFrame, typename... Args>
    [[nodiscard]] forceinline FrameHolder<T> PushFrame(const bool innerScope, NailangFrame::FrameFlags flags, Args&&... args)
    {
        return { this, innerScope ? ConstructEvalContext() : (CurFrame ? CurFrame->Context : RootContext), flags, std::forward<Args>(args)... };
    }

    [[noreturn]] void HandleException(const NailangRuntimeException& ex) const override;
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const Arg> args);
    //[[nodiscard]] FuncName* CreateFuncName(std::u32string_view name, FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] TempFuncName CreateTempFuncName(std::u32string_view name, FuncName::FuncInfo info = FuncName::FuncInfo::Empty) const;
    [[nodiscard]] ArgLocator LocateArg(const LateBindVar& var, const bool create) const;
    [[nodiscard]] ArgLocator LocateArgForWrite(const LateBindVar& var, NilCheck nilCheck, std::variant<bool, EmbedOps> extra) const;
    bool SetArg(const LateBindVar& var, SubQuery subq, Arg arg, NilCheck nilCheck = {});
    bool SetArg(const LateBindVar& var, SubQuery subq, const Expr& expr, NilCheck nilCheck = {});
    bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args);
    bool SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args);

    [[nodiscard]] virtual std::shared_ptr<EvaluateContext> ConstructEvalContext() const;
    [[nodiscard]] virtual Arg  LookUpArg(const LateBindVar& var, const bool checkNull = true) const;
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const;
    virtual void OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas);
    virtual void OnBlock(const Block& block, common::span<const FuncCall> metas);
    [[nodiscard]] virtual Arg OnLocalFunc(const LocalFunc& func, FuncEvalPack& pack);

public:
    template<typename T> using FrameT = FrameHolder<T>;
    NailangRuntimeBase(std::shared_ptr<EvaluateContext> context);
    virtual ~NailangRuntimeBase();
    void ExecuteBlock(const Block& block, common::span<const FuncCall> metas, std::shared_ptr<EvaluateContext> ctx, const bool checkMetas = true);
    void ExecuteBlock(const Block& block, common::span<const FuncCall> metas, const bool checkMetas = true, const bool innerScope = true)
    {
        ExecuteBlock(block, metas, innerScope ? ConstructEvalContext() : (CurFrame ? CurFrame->Context : RootContext), checkMetas);
    }
    Arg EvaluateRawStatement(std::u32string_view content, const bool innerScope = true);
    Arg EvaluateRawStatements(std::u32string_view content, const bool innerScope = true);
};

inline constexpr NailangFrame& NailangExecutor::GetFrame() const noexcept
{ 
    return *Runtime->CurFrame;
}


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
