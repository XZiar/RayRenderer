#pragma once
#include "NailangStruct.h"
#include "common/Exceptions.hpp"
#include "common/StringPool.hpp"
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

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return Body != nullptr; }
};


class NAILANGAPI EvaluateContext
{
    friend class NailangRuntimeBase;
public:
    virtual ~EvaluateContext();
    [[nodiscard]] virtual ArgLocator LocateArg(const LateBindVar& var, const bool create) noexcept = 0;
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const = 0;
    virtual bool SetFunc(const Block* block, common::span<const Expr> args) = 0;
    virtual bool SetFunc(const Block* block, common::span<const std::u32string_view> args) = 0;
    [[nodiscard]] virtual size_t GetArgCount() const noexcept = 0;
    [[nodiscard]] virtual size_t GetFuncCount() const noexcept = 0;
};

class NAILANGAPI BasicEvaluateContext : public EvaluateContext
{
private:
    std::vector<std::u32string_view> LocalFuncArgNames;
protected:
    using LocalFuncHolder = std::tuple<const Block*, uint32_t, uint32_t>;
    [[nodiscard]] virtual LocalFuncHolder LookUpFuncInside(std::u32string_view name) const = 0;
    virtual bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) = 0;
public:
    ~BasicEvaluateContext() override;

    [[nodiscard]] LocalFunc LookUpFunc(std::u32string_view name) const override;
    bool SetFunc(const Block* block, common::span<const Expr> args) override;
    bool SetFunc(const Block* block, common::span<const std::u32string_view> args) override;
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
    using VType = std::variant<std::monostate, Statement, Arg, Expr, const FuncCall*, FuncCall, const RawBlock*, const Block*>;
    VType Target;

    constexpr ExceptionTarget() noexcept {}
    ExceptionTarget(const CustomVar& arg) noexcept : Target{ Arg(arg) } {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg, std::enable_if_t<common::VariantHelper<VType>::Contains<std::decay_t<T>>()>* = nullptr) noexcept
        : Target{ std::forward<T>(arg) } {}
    template<typename T>
    constexpr ExceptionTarget(T&& arg, std::enable_if_t<common::is_specialization<std::decay_t<T>, std::variant>::value>* = nullptr) noexcept
        : Target{ common::VariantHelper<VType>::Convert(std::forward<T>(arg)) } {}

    constexpr explicit operator bool() const noexcept { return Target.index() != 0; }
    [[nodiscard]] constexpr Type GetType() const noexcept
    {
        switch (Target.index())
        {
        case 0:  return Type::Empty;
        case 2:  return Type::Arg;
        case 3:  return Type::Expr;
        case 4:  return Type::FuncCall;
        case 5:  return Type::FuncCall;
        case 6:  return Type::RawBlock;
        case 7:  return Type::Block;
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
};
}


class NAILANGAPI NailangRuntimeException : public common::BaseException
{
    friend class NailangRuntimeBase;
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
    friend class NailangRuntimeBase;
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
};


class NAILANGAPI NailangRuntimeBase
{
    friend Arg;
    friend CustomVar::Handler;
public:
    enum class ProgramStatus  : uint8_t { Next, Break, Return, End };
    enum class MetaFuncResult : uint8_t { Unhandled, Next, Skip, Return };
    enum class FrameFlags : uint8_t
    {
        Empty     = 0b00000000,
        InLoop    = 0b00000001,
        FlowScope = 0b00000010,
        CallScope = 0b00000100,
        FuncCall  = FlowScope | CallScope,
    };
private:
    class ExprHolder;
protected:
    class FrameHolder;
    struct BlockFrame
    {
        friend class FrameHolder;
        friend class NailangRuntimeBase;
    private:
        BlockFrame* PrevFrame;
    public:
        std::shared_ptr<EvaluateContext> Context;
        common::span<const FuncCall> MetaScope;
        const Block* BlockScope = nullptr;
        const Statement* CurContent = nullptr;
        Expr CurExpr;
        Arg ReturnArg;
        FrameFlags Flags;
        ProgramStatus Status = ProgramStatus::Next;

        BlockFrame(BlockFrame* prev, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag) noexcept :
            PrevFrame(prev), Context(ctx), Flags(flag) { }
        constexpr bool Has(FrameFlags flag) const noexcept;
        constexpr bool IsInLoop() const noexcept;
        constexpr BlockFrame* GetCallScope() noexcept;
    };
    class NAILANGAPI COMMON_EMPTY_BASES FrameHolder : public common::NonCopyable, public common::NonMovable
    {
        friend class NailangRuntimeBase;
        NailangRuntimeBase& Host;
        BlockFrame Frame;
        FrameHolder(NailangRuntimeBase* host, const std::shared_ptr<EvaluateContext>& ctx, const FrameFlags flags);
    public:
        ~FrameHolder();
        constexpr BlockFrame* operator->() const noexcept
        {
            return Frame.PrevFrame;
        }
    };

    std::shared_ptr<EvaluateContext> RootContext;
    BlockFrame* CurFrame = nullptr;
    MemoryPool MemPool;
    void ThrowByArgCount(const FuncCall& call, const size_t count, const ArgLimits limit = ArgLimits::Exact) const;
    void ThrowByArgType(const FuncCall& call, const Expr::Type type, size_t idx) const;
    void ThrowByArgType(const Arg& arg, const Arg::Type type) const;
    void ThrowByArgType(const FuncCall& call, const Arg& arg, const Arg::Type type, size_t idx) const;
    void ThrowIfNotFuncTarget(const FuncCall& call, const FuncName::FuncInfo type) const;
    void ThrowIfStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const;
    void ThrowIfNotStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    void CheckFuncArgs(common::span<const Expr::Type> types, const ArgLimits limit, const FuncCall& call);
    void EvaluateFuncArgs(Arg* args, const Arg::Type* types, const size_t size, const FuncCall& call);
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline void CheckFuncArgs(const FuncCall& call, const std::array<Expr::Type, N>& types)
    {
        CheckFuncArgs(types, Limit, call);
    }
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args = {};
        EvaluateFuncArgs(args.data(), nullptr, std::min(N, call.Args.size()), call);
        return args;
    }
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call, const std::array<Arg::Type, N>& types)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args = {};
        EvaluateFuncArgs(args.data(), types.data(), std::min(N, call.Args.size()), call);
        return args;
    }
    [[nodiscard]] forceinline Arg EvaluateFirstFuncArg(const FuncCall& call, ArgLimits limit = ArgLimits::Exact)
    {
        if (limit == ArgLimits::AtMost && call.Args.size() == 0)
            return {};
        ThrowByArgCount(call, 1, limit);
        return EvaluateExpr(call.Args[0]);
    }
    [[nodiscard]] forceinline Arg EvaluateFirstFuncArg(const FuncCall& call, Arg::Type type, ArgLimits limit = ArgLimits::Exact)
    {
        if (limit == ArgLimits::AtMost && call.Args.size() == 0)
            return {};
        ThrowByArgCount(call, 1, limit);
        auto arg = EvaluateExpr(call.Args[0]);
        ThrowByArgType(call, arg, type, 0);
        return arg;
    }

    [[nodiscard]] FrameHolder PushFrame(std::shared_ptr<EvaluateContext> ctx, const FrameFlags flags = FrameFlags::Empty);
    [[nodiscard]] forceinline FrameHolder PushFrame(const FrameFlags flags = FrameFlags::Empty)
    {
        return PushFrame(ConstructEvalContext(), flags);
    }
    [[nodiscard]] forceinline FrameHolder PushFrame(const bool innerScope, const FrameFlags flags = FrameFlags::Empty)
    {
        return PushFrame(innerScope ? ConstructEvalContext() : (CurFrame ? CurFrame->Context : RootContext), flags);
    }

                  void ExecuteFrame();
    [[nodiscard]] std::variant<std::monostate, bool, xziar::nailang::TempFuncName> HandleMetaIf(const FuncCall& meta);
    [[nodiscard]] bool HandleMetaFuncs  (common::span<const FuncCall> metas, const Statement& target);
                  void HandleContent    (const Statement& content, common::span<const FuncCall> metas);
                  void OnLoop           (const Expr& condition, const Statement& target, common::span<const FuncCall> metas);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const Expr> args);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const Arg> args);
    [[nodiscard]] FuncName* CreateFuncName(std::u32string_view name, FuncName::FuncInfo info = FuncName::FuncInfo::Empty);
    [[nodiscard]] TempFuncName CreateTempFuncName(std::u32string_view name, FuncName::FuncInfo info = FuncName::FuncInfo::Empty) const;
    [[nodiscard]] LateBindVar DecideDynamicVar(const Expr& arg, const std::u16string_view reciever) const;
    [[nodiscard]] ArgLocator LocateArg(const LateBindVar& var, const bool create) const;

    [[noreturn]]  virtual void HandleException(const NailangRuntimeException& ex) const;
    [[nodiscard]] virtual std::shared_ptr<EvaluateContext> ConstructEvalContext() const;
    [[nodiscard]] virtual Arg  LookUpArg(const LateBindVar& var) const;
                  virtual bool SetArg(const LateBindVar& var, SubQuery subq, std::variant<Arg, Expr> arg, NilCheck nilCheck = {});
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const;
                  virtual bool SetFunc(const Block* block, common::span<const Expr> args);
                  virtual bool SetFunc(const Block* block, common::span<const std::u32string_view> args);
    [[nodiscard]] virtual MetaFuncResult HandleMetaFunc(const FuncCall& meta, const Statement& target, common::span<const FuncCall> metas);
                  virtual Arg  EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas);
    [[nodiscard]] virtual Arg  EvaluateLocalFunc(const LocalFunc& func, const FuncCall& call, common::span<const FuncCall> metas);
    [[nodiscard]] virtual Arg  EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall> metas);
    [[nodiscard]] virtual Arg  EvaluateExtendMathFunc(const FuncCall& call, common::span<const FuncCall> metas);
                  virtual Arg  EvaluateExpr(const Expr& arg);
    [[nodiscard]] virtual Arg  EvaluateUnaryExpr(const UnaryExpr& expr);
    [[nodiscard]] virtual Arg  EvaluateBinaryExpr(const BinaryExpr& expr);
    [[nodiscard]] virtual Arg  EvaluateTernaryExpr(const TernaryExpr& expr);
    [[nodiscard]] virtual Arg  EvaluateQueryExpr(const QueryExpr& expr);
                  virtual void OnRawBlock(const RawBlock& block, common::span<const FuncCall> metas);
public:
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

MAKE_ENUM_BITFIELD(NailangRuntimeBase::FrameFlags)


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
