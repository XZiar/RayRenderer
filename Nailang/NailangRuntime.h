#pragma once
#include "NailangStruct.h"
#include "BlockParser.h"
#include <map>
#include <optional>


namespace xziar::nailang
{

class EvaluateContext
{
protected:
    std::shared_ptr<EvaluateContext> ParentContext;
    std::map<std::u32string, Arg, std::less<>> ArgMap;
public:
    virtual ~EvaluateContext() {}

    virtual Arg LookUpArg(std::u32string_view name) const;
    virtual bool SetArg(std::u32string_view name, Arg arg);
};

struct ArgHelper
{
private:
    using Type = Arg::InternalType;
public:
    forceinline static constexpr bool IsBool(const Arg& arg) noexcept
    {
        return arg.TypeData == Type::Bool;
    }
    forceinline static constexpr bool IsInteger(const Arg& arg) noexcept
    {
        return arg.TypeData == Type::Uint || arg.TypeData == Type::Int;
    }
    forceinline static constexpr bool IsFloatPoint(const Arg& arg) noexcept
    {
        return arg.TypeData == Type::FP;
    }
    forceinline static constexpr bool IsNumber(const Arg& arg) noexcept
    {
        return IsFloatPoint(arg) || IsInteger(arg);
    }
    forceinline static constexpr bool IsStr(const Arg& arg) noexcept
    {
        return arg.TypeData == Type::U32Str || arg.TypeData == Type::U32Sv;
    }
    forceinline static constexpr std::optional<bool> GetBool(const Arg& arg) noexcept
    {
        switch (arg.TypeData)
        {
        case Type::Uint:    return arg.GetVar<Type::Uint>() != 0;
        case Type::Int:     return arg.GetVar<Type::Int>() != 0;
        case Type::FP:      return arg.GetVar<Type::FP>() != 0;
        case Type::Bool:    return arg.GetVar<Type::Bool>();
        case Type::U32Str:  return !arg.GetVar<Type::U32Str>().empty();
        case Type::U32Sv:   return !arg.GetVar<Type::U32Sv>().empty();
        default:            return {};
        }
    }
    forceinline static constexpr std::optional<uint64_t> GetUint(const Arg& arg) noexcept
    {
        switch (arg.TypeData)
        {
        case Type::Uint:    return arg.GetVar<Type::Uint>();
        case Type::Int:     return static_cast<uint64_t>(arg.GetVar<Type::Int>());
        case Type::FP:      return static_cast<uint64_t>(arg.GetVar<Type::FP>());
        case Type::Bool:    return arg.GetVar<Type::Bool>() ? 1 : 0;
        default:            return {};
        }
    }
    forceinline static constexpr std::optional<int64_t> GetInt(const Arg& arg) noexcept
    {
        switch (arg.TypeData)
        {
        case Type::Uint:    return static_cast<int64_t>(arg.GetVar<Type::Uint>());
        case Type::Int:     return arg.GetVar<Type::Int>();
        case Type::FP:      return static_cast<int64_t>(arg.GetVar<Type::FP>());
        case Type::Bool:    return arg.GetVar<Type::Bool>() ? 1 : 0;
        default:            return {};
        }
    }
    forceinline static constexpr std::optional<double> GetFP(const Arg& arg) noexcept
    {
        switch (arg.TypeData)
        {
        case Type::Uint:    return static_cast<double>(arg.GetVar<Type::Uint>());
        case Type::Int:     return static_cast<double>(arg.GetVar<Type::Int>());
        case Type::FP:      return arg.GetVar<Type::FP>();
        case Type::Bool:    return arg.GetVar<Type::Bool>() ? 1. : 0.;
        default:            return {};
        }
    }
    forceinline static constexpr std::optional<std::u32string_view> GetStr(const Arg& arg) noexcept
    {
        switch (arg.TypeData)
        {
        case Type::U32Str:  return arg.GetVar<Type::U32Str>();
        case Type::U32Sv:   return arg.GetVar<Type::U32Sv>();
        default:            return {};
        }
    }
};
struct EmbedOpEval
{
    static Arg Equal(const Arg& left, const Arg& right);
    static Arg NotEqual(const Arg& left, const Arg& right);
    static Arg Less(const Arg& left, const Arg& right);
    static Arg LessEqual(const Arg& left, const Arg& right);
    static Arg Greater(const Arg& left, const Arg& right) { return Less(right, left); }
    static Arg GreaterEqual(const Arg& left, const Arg& right) { return LessEqual(right, left); }
    static Arg And(const Arg& left, const Arg& right);
    static Arg Or(const Arg& left, const Arg& right);
    static Arg Add(const Arg& left, const Arg& right);
    static Arg Sub(const Arg& left, const Arg& right);
    static Arg Mul(const Arg& left, const Arg& right);
    static Arg Div(const Arg& left, const Arg& right);
    static Arg Rem(const Arg& left, const Arg& right);
    static Arg Not(const Arg& arg);

    static std::optional<Arg> Eval(const std::u32string_view opname, common::span<const Arg> args);
};

class NailangRuntimeBase
{
protected:
    std::shared_ptr<EvaluateContext> EvalContext;
    
            Arg EvaluateFunc(const FuncCall& call, const BlockContent* target = nullptr);
    virtual Arg EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, const BlockContent* target = nullptr);
    virtual Arg EvaluateArg(const RawArg& arg);
};

}
