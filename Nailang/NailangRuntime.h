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


struct VarLookup
{
    enum class VarType { Normal, Local, Root };
    constexpr VarLookup(const std::u32string_view name) noexcept : Ptr(name.data()), Size(gsl::narrow_cast<uint32_t>(name.size())),
        Offset(0), NextOffset(0), Type(InitType(name))
    {
        if (Type != VarType::Normal)
            Offset = 1;
        FindNext();
    }
    template<typename T>
    constexpr VarLookup(T&& val, std::enable_if_t<std::is_constructible_v<std::u32string_view, T>>* = nullptr) noexcept
        : VarLookup(common::str::ToStringView(val)) { }
    constexpr VarLookup(const VarLookup& other) noexcept : Ptr(other.Ptr), Size(other.Size), 
        Offset(other.Type == VarType::Normal ? 0 : 1), NextOffset(0), Type(other.Type)
    {
        if (Offset == other.Offset)
            NextOffset = other.NextOffset;
        else
            FindNext();
    }

    [[nodiscard]] constexpr std::u32string_view GetRawName() const noexcept { return { Ptr, Size }; }
    [[nodiscard]] constexpr std::u32string_view GetFull() const noexcept
    {
        const uint32_t prefix = Type == VarType::Normal ? 0 : 1;
        Expects(Size > prefix);
        return { Ptr + prefix, static_cast<size_t>(Size) - prefix };
    }
    [[nodiscard]] constexpr VarType GetType() const noexcept { return Type; }
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
    uint32_t Offset, NextOffset;
    VarType Type;
    constexpr bool FindNext() noexcept
    {
        if (Offset >= Size)
            return false;
        const auto ptr = std::char_traits<char32_t>::find(Ptr + Offset, Size - Offset, U'.');
        Ensures(ptr == nullptr || ptr >= Ptr);
        NextOffset = ptr == nullptr ? Size : static_cast<uint32_t>(ptr - Ptr);
        return true;
    }
};

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
    [[nodiscard]] virtual Arg LookUpArg(VarLookup var) const = 0;
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const = 0;
    virtual bool SetArg(VarLookup var, Arg arg, const bool force) = 0;
    virtual bool SetFunc(const Block* block, common::span<const RawArg> args) = 0;
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
    bool SetFunc(const Block* block, common::span<const RawArg> args) override;
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

    [[nodiscard]] Arg    LookUpArg(VarLookup var) const override;
                  bool   SetArg(VarLookup var, Arg arg, const bool force) override;
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

    [[nodiscard]] LocalFuncHolder LookUpFuncInside(std::u32string_view name) const override;
    bool SetFuncInside(std::u32string_view name, LocalFuncHolder func) override;
public:
    ~CompactEvaluateContext() override;

    [[nodiscard]] Arg    LookUpArg(VarLookup var) const override;
                  bool   SetArg(VarLookup var, Arg arg, const bool force) override;
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
protected:
    NailangRuntimeException(const char* const type, const std::u16string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope) :
        BaseException(type, msg), Target(std::move(target)), Scope(std::move(scope))
    { }
public:
    EXCEPTION_CLONE_EX(NailangRuntimeException);
    detail::ExceptionTarget Target;
    detail::ExceptionTarget Scope;
    NailangRuntimeException(const std::u16string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {}) :
        NailangRuntimeException(TYPENAME, msg, std::move(target), std::move(scope))
    { }
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
    NailangCodeException(const std::u32string_view msg, detail::ExceptionTarget target = {}, detail::ExceptionTarget scope = {});
};


enum class ArgLimits { Exact, AtMost, AtLeast };
enum class FrameFlags : uint8_t
{
    Empty     = 0b00000000,
    InLoop    = 0b00000001,
    FlowScope = 0b00000010,
    CallScope = 0b00000100,
    FuncCall  = FlowScope | CallScope,
    Virtual   = 0b00001000,
};
MAKE_ENUM_BITFIELD(FrameFlags)

class NAILANGAPI NailangRuntimeBase
{
public:
    enum class FuncTargetType { Plain, Expr, Meta };
    static std::u32string_view ArgTypeName(const Arg::Type type) noexcept;
    static std::u32string_view ArgTypeName(const RawArg::Type type) noexcept;
    static std::u32string_view FuncTargetName(const FuncTargetType type) noexcept;
    enum class ProgramStatus  : uint8_t { Next, Break, Return, End };
    enum class MetaFuncResult : uint8_t { Unhandled, Next, Skip, Return };
protected:
    class FrameHolder;
    struct FuncFrame
    {
        const FuncFrame* PrevFrame = nullptr;
        const FuncCall*  Func      = nullptr;
    };
    struct StackFrame
    {
        friend class FrameHolder;
        friend class NailangRuntimeBase;
    private:
        StackFrame* PrevFrame;
    public:
        std::shared_ptr<EvaluateContext> Context;
        common::span<const FuncCall> MetaScope;
        const Block* BlockScope = nullptr;
        const BlockContent* CurContent  = nullptr;
        const FuncFrame*    CurFuncCall = nullptr;
        Arg ReturnArg;
        FrameFlags Flags;
        ProgramStatus Status = ProgramStatus::Next;

        StackFrame(StackFrame* prev, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag) noexcept :
            PrevFrame(prev), Context(ctx), Flags(flag) { }

        constexpr bool Has(FrameFlags flag) const noexcept { return HAS_FIELD(Flags, flag); }
        constexpr bool IsInLoop() const noexcept
        {
            for (auto frame = this; frame; frame = frame->PrevFrame)
            {
                if (HAS_FIELD(frame->Flags, FrameFlags::FlowScope)) // cannot pass flow scope
                    return false;
                if (HAS_FIELD(frame->Flags, FrameFlags::InLoop))
                    return true;
            }
            return false;
        }
        constexpr StackFrame* GetCallScope() noexcept
        {
            for (auto frame = this; frame; frame = frame->PrevFrame)
            {
                if (HAS_FIELD(frame->Flags, FrameFlags::CallScope))
                    return frame;
            }
            return nullptr;
        }
    };
    class FCallHost
    {
        friend class NailangRuntimeBase;
        NailangRuntimeBase& Host;
        FuncFrame Frame;
        constexpr FCallHost(NailangRuntimeBase* host, const FuncCall* call = nullptr) noexcept :
            Host(*host), Frame({ Host.CurFrame->CurFuncCall, call })
        {
            Host.CurFrame->CurFuncCall = &Frame;
        }
    public:
        void operator=(const FuncCall* call) noexcept { Frame.Func = call; }
        ~FCallHost();
    };
    class NAILANGAPI COMMON_EMPTY_BASES FrameHolder : public common::NonCopyable, public common::NonMovable
    {
        friend class NailangRuntimeBase;
        NailangRuntimeBase& Host;
        StackFrame Frame;
        FrameHolder(NailangRuntimeBase* host, const std::shared_ptr<EvaluateContext>& ctx, const FrameFlags flags);
    public:
        ~FrameHolder();
        constexpr StackFrame* operator->() const noexcept
        {
            return Frame.PrevFrame;
        }
    };

    std::shared_ptr<EvaluateContext> RootContext;
    StackFrame* CurFrame = nullptr;
    MemoryPool MemPool;
    void ThrowByArgCount(const FuncCall& call, const size_t count, const ArgLimits limit = ArgLimits::Exact) const;
    void ThrowByArgType(const FuncCall& call, const RawArg::Type type, size_t idx) const;
    void ThrowByArgType(const Arg& arg, const Arg::Type type) const;
    void ThrowByArgType(const FuncCall& call, const Arg& arg, const Arg::Type type, size_t idx) const;
    void ThrowIfNotFuncTarget(const std::u32string_view func, const FuncTargetType target, const FuncTargetType type) const;
    void ThrowIfBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    void ThrowIfNotBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    void CheckFuncArgs(common::span<const RawArg::Type> types, const ArgLimits limit, const FuncCall& call);
    void EvaluateFuncArgs(Arg* args, const Arg::Type* types, const size_t size, const FuncCall& call);
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    forceinline void CheckFuncArgs(const FuncCall& call, const std::array<RawArg::Type, N>& types)
    {
        CheckFuncArgs(types, Limit, call);
    }
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args;
        EvaluateFuncArgs(args.data(), nullptr, std::min(N, call.Args.size()), call);
        return args;
    }
    template<size_t N, ArgLimits Limit = ArgLimits::Exact>
    [[nodiscard]] forceinline std::array<Arg, N> EvaluateFuncArgs(const FuncCall& call, const std::array<Arg::Type, N>& types)
    {
        ThrowByArgCount(call, N, Limit);
        std::array<Arg, N> args;
        EvaluateFuncArgs(args.data(), types.data(), std::min(N, call.Args.size()), call);
        return args;
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
    [[nodiscard]] bool HandleMetaFuncs  (common::span<const FuncCall> metas, const BlockContent& target);
                  void HandleContent    (const BlockContent& content, common::span<const FuncCall> metas);
                  void OnLoop           (const RawArg& condition, const BlockContent& target, common::span<const FuncCall> metas);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const RawArg> args);
    [[nodiscard]] std::u32string FormatString(const std::u32string_view formatter, common::span<const Arg> args);

                  virtual void HandleException(const NailangRuntimeException& ex) const;
    [[nodiscard]] virtual std::shared_ptr<EvaluateContext> ConstructEvalContext() const;
    [[nodiscard]] virtual Arg  LookUpArg(VarLookup var) const;
                  virtual bool SetArg(VarLookup var, Arg arg);
    [[nodiscard]] virtual LocalFunc LookUpFunc(std::u32string_view name) const;
                  virtual bool SetFunc(const Block* block, common::span<const RawArg> args);
                  virtual bool SetFunc(const Block* block, common::span<const std::u32string_view> args);
    [[nodiscard]] virtual MetaFuncResult HandleMetaFunc(const FuncCall& meta, const BlockContent& target, common::span<const FuncCall> metas);
                  virtual Arg  EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTargetType target);
    [[nodiscard]] virtual Arg  EvaluateLocalFunc(const LocalFunc& func, const FuncCall& call, common::span<const FuncCall> metas, const FuncTargetType target);
    [[nodiscard]] virtual Arg  EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTargetType target);
    [[nodiscard]] virtual std::optional<Arg> EvaluateExtendMathFunc(const FuncCall& call, std::u32string_view mathName, common::span<const FuncCall> metas);
                  virtual Arg  EvaluateArg(const RawArg& arg);
    [[nodiscard]] virtual std::optional<Arg> EvaluateUnaryExpr(const UnaryExpr& expr);
    [[nodiscard]] virtual std::optional<Arg> EvaluateBinaryExpr(const BinaryExpr& expr);
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

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
