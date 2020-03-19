#include "NailangRuntime.h"
#include "common/StringEx.hpp"
#include <cmath>


namespace xziar::nailang
{
using namespace std::string_view_literals;

Arg EvaluateContext::LookUpArg(std::u32string_view name) const
{
    if (const auto it = ArgMap.find(name); it != ArgMap.end())
        return it->second;
    if (ParentContext)
        return ParentContext->LookUpArg(name);
    return Arg{};
}

bool EvaluateContext::SetArg(std::u32string_view name, Arg arg)
{
    if (common::str::IsBeginWith(name, U"_."sv))
    {
        if (ParentContext)
            return ParentContext->SetArg(name, arg);
        else
            name.remove_prefix(2);
    }
    if (auto it = ArgMap.find(name); it != ArgMap.end())
    {
        it->second = arg; 
        return true;
    }
    else
    {
        ArgMap.insert_or_assign(std::u32string(name), arg);
        return false;
    }
}


#define LR_BOTH(left, right, func) ArgHelper::func(left) && ArgHelper::func(right)
Arg NailangRuntimeBase::EmbedOpEval::Equal(const Arg& left, const Arg& right)
{
    if (LR_BOTH(left, right, IsInteger))
        return ArgHelper::GetUint(left) == ArgHelper::GetUint(right);
    if (LR_BOTH(left, right, IsBool))
        return left.GetVar<Arg::InternalType::Bool>() == right.GetVar<Arg::InternalType::Bool>();
    if (LR_BOTH(left, right, IsStr))
        return ArgHelper::GetStr(left) == ArgHelper::GetStr(right);
    if (LR_BOTH(left, right, IsFloatPoint))
        return left.GetVar<Arg::InternalType::FP>() == right.GetVar<Arg::InternalType::FP>();
    if (LR_BOTH(left, right, IsNumber))
        return ArgHelper::GetFP(left) == ArgHelper::GetFP(right);
    return {};
}
Arg NailangRuntimeBase::EmbedOpEval::NotEqual(const Arg& left, const Arg& right)
{
    const auto ret = Equal(left, right);
    if (ret.TypeData != Arg::InternalType::Empty)
        return !ret.GetVar<Arg::InternalType::Bool>();
    return ret;
}
//Arg NailangRuntimeBase::EmbedOpEval::Less(const Arg& left, const Arg& right)
//{
//    using Type = Arg::InternalType;
//    if (left.TypeData == right.TypeData)
//    {
//        switch (left.TypeData)
//        {
//        case Type::FP:      return left.GetVar<Type::FP>()   < right.GetVar<Type::FP>();
//        case Type::Uint:    return left.GetVar<Type::Uint>() < right.GetVar<Type::Uint>();
//        case Type::Int:     return left.GetVar<Type::Int>()  < right.GetVar<Type::Int>();
//        default:            return {};
//        }
//    }
//    if (LR_BOTH(left, right, IsInteger))
//    {
//        if (left.TypeData == Type::Int)
//        {
//            const auto left_ = left.GetVar<Type::Int>();
//            return left_ < 0 ? true : static_cast<uint64_t>(left_) < right.GetVar<Type::Uint>();
//        }
//        else
//        {
//            const auto right_ = right.GetVar<Type::Int>();
//            return right_ < 0 ? false : left.GetVar<Type::Uint>() < static_cast<uint64_t>(right_);
//        }
//    }
//    if (LR_BOTH(left, right, IsNumber))
//    {
//        return ArgHelper::GetFP(left) < ArgHelper::GetFP(right);
//    }
//    return {};
//}
template<typename F>
forceinline static Arg NumOp(const Arg& left, const Arg& right, F func) noexcept
{
    using Type = Arg::InternalType;
    switch (left.TypeData)
    {
    case Type::FP:
    {
        const auto left_ = left.GetVar<Type::FP>();
        switch (right.TypeData)
        {
        case Type::FP:      return func(left_, right.GetVar<Type::FP>());
        case Type::Uint:    return func(left_, right.GetVar<Type::Uint>());
        case Type::Int:     return func(left_, right.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    case Type::Uint:
    {
        const auto left_ = left.GetVar<Type::Uint>();
        switch (right.TypeData)
        {
        case Type::FP:      return func(left_, right.GetVar<Type::FP>());
        case Type::Uint:    return func(left_, right.GetVar<Type::Uint>());
        case Type::Int:     return func(left_, right.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    case Type::Int:
    {
        const auto left_ = left.GetVar<Type::Int>();
        switch (right.TypeData)
        {
        case Type::FP:      return func(left_, right.GetVar<Type::FP>());
        case Type::Uint:    return func(left_, right.GetVar<Type::Uint>());
        case Type::Int:     return func(left_, right.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    default: break;
    }
    return {};
}

Arg NailangRuntimeBase::EmbedOpEval::Less(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) -> bool
        {
            using TL = std::decay_t<decltype(l)>;
            using TR = std::decay_t<decltype(r)>;
            if constexpr (std::is_same_v<TL, int64_t> && std::is_same_v<TR, uint64_t>)
                return l < 0 ? true  : static_cast<uint64_t>(l) < r;
            else if constexpr (std::is_same_v<TL, uint64_t> && std::is_same_v<TR, int64_t>)
                return r < 0 ? false : l < static_cast<uint64_t>(r);
            else
                return l < r; 
        });
}
Arg NailangRuntimeBase::EmbedOpEval::LessEqual(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) -> bool
        {
            using TL = std::decay_t<decltype(l)>;
            using TR = std::decay_t<decltype(r)>;
            if constexpr (std::is_same_v<TL, int64_t>&& std::is_same_v<TR, uint64_t>)
                return l < 0 ? true  : static_cast<uint64_t>(l) <= r;
            else if constexpr (std::is_same_v<TL, uint64_t>&& std::is_same_v<TR, int64_t>)
                return r < 0 ? false : l <= static_cast<uint64_t>(r);
            else
                return l <= r;
        });
}

Arg NailangRuntimeBase::EmbedOpEval::And(const Arg& left, const Arg& right)
{
    const auto left_ = ArgHelper::GetBool(left), right_ = ArgHelper::GetBool(right);
    if (left_.has_value() && right_.has_value())
        return *left_ && *right_;
    return {};
}
Arg NailangRuntimeBase::EmbedOpEval::Or(const Arg& left, const Arg& right)
{
    const auto left_ = ArgHelper::GetBool(left), right_ = ArgHelper::GetBool(right);
    if (left_.has_value() && right_.has_value())
        return *left_ || *right_;
    return {};
}

Arg NailangRuntimeBase::EmbedOpEval::Add(const Arg& left, const Arg& right)
{
    if (LR_BOTH(left, right, IsStr))
    {
        const auto left_  = *ArgHelper::GetStr(left);
        const auto right_ = *ArgHelper::GetStr(right);
        std::u32string ret;
        ret.reserve(left_.size() + right_.size());
        ret.append(left_).append(right_);
        return ret;
    }
    return NumOp(left, right, [](const auto l, const auto r) { return l + r; });
}
Arg NailangRuntimeBase::EmbedOpEval::Sub(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l - r; });
}
Arg NailangRuntimeBase::EmbedOpEval::Mul(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l * r; });
}
Arg NailangRuntimeBase::EmbedOpEval::Div(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l / r; });
}
Arg NailangRuntimeBase::EmbedOpEval::Rem(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) 
        {
            if constexpr (std::is_floating_point_v<decltype(l)> || std::is_floating_point_v<decltype(r)>)
                return std::fmod(static_cast<double>(l), static_cast<double>(r));
            else
                return l % r; 
        });
}

Arg NailangRuntimeBase::EmbedOpEval::Not(const Arg& arg)
{
    const auto arg_ = ArgHelper::GetBool(arg);
    if (arg_.has_value())
        return !*arg_;
    return {};
}

std::optional<Arg> NailangRuntimeBase::EmbedOpEval::Eval(const std::u32string_view opname, common::span<const Arg> args)
{
#define CALL_BIN_OP(type) case #type ## _hash: return EmbedOpEval::type(args[0], args[1])
#define CALL_UN_OP(type)  case #type ## _hash: return EmbedOpEval::type(args[0])
    switch (hash_(opname))
    {
        CALL_BIN_OP(Equal);
        CALL_BIN_OP(NotEqual);
        CALL_BIN_OP(Less);
        CALL_BIN_OP(LessEqual);
        CALL_BIN_OP(Greater);
        CALL_BIN_OP(GreaterEqual);
        CALL_BIN_OP(And);
        CALL_BIN_OP(Or);
        CALL_BIN_OP(Add);
        CALL_BIN_OP(Sub);
        CALL_BIN_OP(Mul);
        CALL_BIN_OP(Div);
        CALL_BIN_OP(Rem);
        CALL_UN_OP (Not);
    default:    return {};
    }
}

#undef LR_BOTH


Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& func, const BlockContent* target)
{
    if (func.Args.size() == 0)
        return EvaluateFunc(func.Name, {});
    std::vector<Arg> args;
    args.reserve(func.Args.size());
    for (const auto& rawarg : func.Args)
        args.emplace_back(EvaluateArg(rawarg));
    return EvaluateFunc(func.Name, args, target);
}

Arg NailangRuntimeBase::EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, const BlockContent*)
{
    using Type = Arg::InternalType;
    if (common::str::IsBeginWith(func, U"EmbedOp."sv))
    {
        const auto opname = func.substr(8);
        const auto ret = EmbedOpEval::Eval(opname, args);
        if (ret.has_value())
        {
            if (ret->TypeData == Type::Empty)
                throw U"Emebd op's arg type does not match requirement"sv;
            return *ret;
        }
    }
    throw U"Does not find correponding func"sv;
}

static std::u32string_view OpToFuncName(EmbedOps op)
{
#define RET_NAME(x) case EmbedOps::x: return U ## "EmbedOp." #x ## sv
    switch (op)
    {
        RET_NAME(Equal);
        RET_NAME(NotEqual);
        RET_NAME(Less);
        RET_NAME(LessEqual);
        RET_NAME(Greater);
        RET_NAME(GreaterEqual);
        RET_NAME(And);
        RET_NAME(Or);
        RET_NAME(Not);
        RET_NAME(Add);
        RET_NAME(Sub);
        RET_NAME(Mul);
        RET_NAME(Div);
        RET_NAME(Rem);
    default: return U"UnknownOp"sv;
    }
#undef RET_NAME
}

Arg NailangRuntimeBase::EvaluateArg(const RawArg& arg)
{
    using Type = RawArg::Type;
    switch (arg.TypeData)
    {
    case Type::Func:
        return EvaluateFunc(*arg.GetVar<Type::Func>());
    case Type::Unary:
    {
        const auto& stmt = *arg.GetVar<Type::Unary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.Oprend, 1 };
        return EvaluateFunc(*arg.GetVar<Type::Func>());
    }
    case Type::Binary:
    {
        const auto& stmt = *arg.GetVar<Type::Binary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.LeftOprend, 2 };
        return EvaluateFunc(*arg.GetVar<Type::Func>());
    }
    case Type::Var:
        return EvalContext->LookUpArg(arg.GetVar<Type::Var>().Name);
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            Expects(false); return {};
    }
}



}
