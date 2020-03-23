#include "NailangRuntime.h"
#include "common/StringEx.hpp"
#include "common/linq2.hpp"
#include <cmath>


namespace xziar::nailang
{
using namespace std::string_view_literals;


EvaluateContext::~EvaluateContext()
{ }

bool EvaluateContext::ShouldContinue() const
{
    return true;
}

BasicEvaluateContext::~BasicEvaluateContext()
{ }

Arg BasicEvaluateContext::LookUpArg(std::u32string_view name) const
{
    if (const auto it = ArgMap.find(name); it != ArgMap.end())
        return it->second;
    if (ParentContext)
        return ParentContext->LookUpArg(name);
    return Arg{};
}

bool BasicEvaluateContext::SetArg(std::u32string_view name, Arg arg)
{
    if (common::str::IsBeginWith(name, U"_."sv))
    {
        if (ParentContext)
            return ParentContext->SetArg(name, arg);
        else
            name.remove_prefix(2);
    }
    auto it = ArgMap.find(name);
    const bool hasIt = it != ArgMap.end();
    if (arg.IsEmpty())
    {
        if (!hasIt)
            return false;
        ArgMap.erase(it);
        return true;
    }
    else
    {
        if (hasIt)
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
}

bool BasicEvaluateContext::ShouldContinue() const
{
    return !ReturnArg.has_value();
}


#define LR_BOTH(left, right, func) left.func() && right.func()
Arg EmbedOpEval::Equal(const Arg& left, const Arg& right)
{
    if (LR_BOTH(left, right, IsInteger))
        return left.GetUint() == right.GetUint();
    if (LR_BOTH(left, right, IsBool))
        return left.GetVar<Arg::InternalType::Bool>() == right.GetVar<Arg::InternalType::Bool>();
    if (LR_BOTH(left, right, IsStr))
        return left.GetStr() == right.GetStr();
    if (LR_BOTH(left, right, IsFloatPoint))
        return left.GetVar<Arg::InternalType::FP>() == right.GetVar<Arg::InternalType::FP>();
    if (LR_BOTH(left, right, IsNumber))
        return left.GetFP() == right.GetFP();
    return {};
}
Arg EmbedOpEval::NotEqual(const Arg& left, const Arg& right)
{
    const auto ret = Equal(left, right);
    if (ret.TypeData != Arg::InternalType::Empty)
        return !ret.GetVar<Arg::InternalType::Bool>();
    return ret;
}
//Arg EmbedOpEval::Less(const Arg& left, const Arg& right)
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

Arg EmbedOpEval::Less(const Arg& left, const Arg& right)
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
Arg EmbedOpEval::LessEqual(const Arg& left, const Arg& right)
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

Arg EmbedOpEval::And(const Arg& left, const Arg& right)
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ && *right_;
    return {};
}
Arg EmbedOpEval::Or(const Arg& left, const Arg& right)
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ || *right_;
    return {};
}

Arg EmbedOpEval::Add(const Arg& left, const Arg& right)
{
    if (LR_BOTH(left, right, IsStr))
    {
        const auto left_  = * left.GetStr();
        const auto right_ = *right.GetStr();
        std::u32string ret;
        ret.reserve(left_.size() + right_.size());
        ret.append(left_).append(right_);
        return ret;
    }
    return NumOp(left, right, [](const auto l, const auto r) { return l + r; });
}
Arg EmbedOpEval::Sub(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l - r; });
}
Arg EmbedOpEval::Mul(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l * r; });
}
Arg EmbedOpEval::Div(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) { return l / r; });
}
Arg EmbedOpEval::Rem(const Arg& left, const Arg& right)
{
    return NumOp(left, right, [](const auto l, const auto r) 
        {
            if constexpr (std::is_floating_point_v<decltype(l)> || std::is_floating_point_v<decltype(r)>)
                return std::fmod(static_cast<double>(l), static_cast<double>(r));
            else
                return l % r; 
        });
}

Arg EmbedOpEval::Not(const Arg& arg)
{
    const auto arg_ = arg.GetBool();
    if (arg_.has_value())
        return !*arg_;
    return {};
}
#undef LR_BOTH

std::optional<Arg> EmbedOpEval::Eval(const std::u32string_view opname, common::span<const Arg> args)
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
#undef CALL_BIN_OP
#undef CALL_UN_OP
}



NailangRuntimeBase::NailangRuntimeBase(std::shared_ptr<EvaluateContext> context) : 
    EvalContext(std::move(context))
{ }

NailangRuntimeBase::~NailangRuntimeBase()
{ }

void NailangRuntimeBase::ThrowByArgCount(const FuncCall& call, const size_t count) const
{
    if (static_cast<size_t>(call.Args.size()) != count)
        throw U"Expect Arg Count Not Match"sv;
}

void NailangRuntimeBase::ThrowByArgType(const Arg& arg, const Arg::InternalType type) const
{
    if (arg.TypeData != type)
        throw U"Arg type not xxx"sv;
}

bool NailangRuntimeBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        throw std::u32string(varName).append(U" should be bool-able"sv);
    return ret.value();
}

bool NailangRuntimeBase::HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& content, common::span<const FuncCall> metas)
{
    if (meta.Name == U"Skip"sv)
    {
        switch (meta.Args.size())
        {
        case 0:  return false;
        case 1:  return !ThrowIfNotBool(EvaluateArg(meta.Args[0]), U"arg of MetaFunc[Skip]"sv);
        default: throw U"MetaFunc[Skip] accept 0 or 1 arg only."sv;
        }
    }
    if (meta.Name == U"DefFunc"sv)
        return false;
    if (meta.Name == U"If"sv)
    {
        ThrowByArgCount(meta, 1);
        return ThrowIfNotBool(EvaluateArg(meta.Args[0]), U"arg of MetaFunc[If]"sv);
    }
    if (meta.Name == U"While"sv)
    {
        ThrowByArgCount(meta, 1);
        while (ThrowIfNotBool(EvaluateArg(meta.Args[0]), U"arg of MetaFunc[While]"sv))
        {
            ExecuteContent(content, metas);
        }
        return false;
    }
    return true;
}

bool NailangRuntimeBase::HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target)
{
    for (const auto& meta : metas)
    {
        if (!HandleMetaFuncBefore(meta, target, metas))
            return false;
    }
    return true;
}

void NailangRuntimeBase::HandleRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}

Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& func, common::span<const FuncCall> metas, const BlockContent* target)
{
    if (func.Args.size() == 0)
        return EvaluateFunc(func.Name, {}, metas, target);
    std::vector<Arg> args;
    args.reserve(func.Args.size());
    for (const auto& rawarg : func.Args)
        args.emplace_back(EvaluateArg(rawarg));
    return EvaluateFunc(func.Name, args, metas, target);
}

Arg NailangRuntimeBase::EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall>, const BlockContent*)
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
        return EvaluateFunc(*arg.GetVar<Type::Func>(), {});
    case Type::Unary:
    {
        const auto& stmt = *arg.GetVar<Type::Unary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.Oprend, 1 };
        return EvaluateFunc(func, {});
    }
    case Type::Binary:
    {
        const auto& stmt = *arg.GetVar<Type::Binary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.LeftOprend, 2 };
        return EvaluateFunc(func, {});
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

void NailangRuntimeBase::ExecuteAssignment(const Assignment& assign, common::span<const FuncCall>)
{
    EvalContext->SetArg(assign.Variable.Name, EvaluateArg(assign.Statement));
}

void NailangRuntimeBase::ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas)
{
    switch (content.GetType())
    {
    case BlockContent::Type::Assignment:
        ExecuteAssignment(*content.Get<Assignment>(), metas);
        break;
    case BlockContent::Type::Block:
        ExecuteBlock(*content.Get<Block>(), metas);
        break;
    case BlockContent::Type::FuncCall:
        EvaluateFunc(*content.Get<FuncCall>(), metas);
        break;
    default:
        Expects(false);
        break;
    }
}

void NailangRuntimeBase::ExecuteBlock(const Block& block, common::span<const FuncCall>)
{
    for (size_t idx = 0; idx < block.Size(); ++idx)
    {
        const auto& [metas, content] = block[idx];
        const auto type = content.GetType();
        if (type == BlockContent::Type::RawBlock)
        {
            HandleRawBlock(*content.Get<RawBlock>(), metas);
        }
        else if (HandleMetaFuncsBefore(metas, content))
        {
            ExecuteContent(content, metas);
        }
        if (!EvalContext->ShouldContinue())
            break;
    }
}



}
