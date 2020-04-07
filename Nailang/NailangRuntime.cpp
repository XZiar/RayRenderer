#include "NailangRuntime.h"
#include "common/StringEx.hpp"
#include "common/Linq2.hpp"
#include <cmath>


namespace xziar::nailang
{
using namespace std::string_view_literals;


bool EvaluateContext::CheckIsLocal(std::u32string_view& name) noexcept
{
    if (name.size() > 0 && name[0] == U'.')
    {
        name.remove_prefix(1);
        return true;
    }
    return false;
}

EvaluateContext::~EvaluateContext()
{ }

Arg EvaluateContext::LookUpArg(std::u32string_view var) const
{
    const bool isLocal = CheckIsLocal(var);
    const auto arg = LookUpArgInside({ var });
    if (!isLocal && arg.TypeData == Arg::InternalType::Empty && ParentContext)
        return ParentContext->LookUpArg(var);
    else
        return arg;
}

bool EvaluateContext::SetArg(std::u32string_view var, Arg arg)
{
    if (!CheckIsLocal(var))
    {
        if (SetArgInside({ var }, arg, false))
            return true;
        if (ParentContext && ParentContext->SetArgInside({ var }, arg, false))
                return true;
    }
    return SetArgInside({ var }, arg, true);
}


BasicEvaluateContext::~BasicEvaluateContext()
{ }

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<const RawArg> args)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(args.size());
    LocalFuncArgNames.reserve(offset + size);
    for (const auto& arg : args)
        LocalFuncArgNames.emplace_back(arg.GetVar<RawArg::Type::Var>().Name);
    return SetFuncInside(block->Name, { block, offset, size });
}

bool BasicEvaluateContext::SetFunc(const LocalFunc & func)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(func.ArgNames.size());
    LocalFuncArgNames.reserve(offset + size);
    for (const auto& name : func.ArgNames)
        LocalFuncArgNames.emplace_back(name);
    return SetFuncInside(func.Body->Name, { func.Body, offset, size });
}

void BasicEvaluateContext::SetReturnArg(Arg arg)
{
    ReturnArg = arg;
}

Arg BasicEvaluateContext::GetReturnArg() const
{
    return ReturnArg.value_or(Arg{});
}


LargeEvaluateContext::~LargeEvaluateContext()
{ }

Arg LargeEvaluateContext::LookUpArgInside(VarLookup var) const
{
    if (const auto it = ArgMap.find(var.Name); it != ArgMap.end())
        return it->second;
    return Arg{};
}

bool LargeEvaluateContext::SetArgInside(VarLookup var, Arg arg, const bool force)
{
    auto it = ArgMap.find(var.Name);
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
            if (force)
                ArgMap.insert_or_assign(std::u32string(var.Name), arg);
            return false;
        }
    }
}

bool LargeEvaluateContext::SetFuncInside(std::u32string_view name, LocalFuncHolder func)
{
    return LocalFuncMap.insert_or_assign(name, func).second;
}

EvaluateContext::LocalFunc LargeEvaluateContext::LookUpFunc(std::u32string_view name)
{
    const auto it = LocalFuncMap.find(name);
    if (it == LocalFuncMap.end())
        return { nullptr, {} };
    const auto [ptr, offset, size] = it->second;
    return { ptr, common::to_span(LocalFuncArgNames).subspan(offset, size) };
}

size_t LargeEvaluateContext::GetArgCount() const noexcept
{
    return ArgMap.size();
}

size_t LargeEvaluateContext::GetFuncCount() const noexcept
{
    return LocalFuncMap.size();
}


CompactEvaluateContext::~CompactEvaluateContext()
{ }


std::u32string_view CompactEvaluateContext::GetStr(const uint32_t offset, const uint32_t len) const noexcept
{
    return { &StrPool[offset], len };
}

Arg CompactEvaluateContext::LookUpArgInside(VarLookup var) const
{
    for (const auto& [off, len, val] : Args)
        if (GetStr(off, len) == var.Name)
            return val;
    return Arg{};
}

bool CompactEvaluateContext::SetArgInside(VarLookup var, Arg arg, const bool force)
{
    Arg* target = nullptr;
    for (auto& [off, len, val] : Args)
        if (GetStr(off, len) == var.Name)
            target = &val;
    const bool hasIt = target != nullptr;
    if (hasIt)
    {
        *target = arg;
        return true;
    }
    else
    {
        if (arg.IsEmpty())
            return false;
        if (force)
        {

            const uint32_t offset = gsl::narrow_cast<uint32_t>(StrPool.size()),
                size = gsl::narrow_cast<uint32_t>(var.Name.size());
            StrPool.insert(StrPool.end(), var.Name.cbegin(), var.Name.cend());
            Args.emplace_back(offset, size, arg);
        }
        return false;
    }
}

bool CompactEvaluateContext::SetFuncInside(std::u32string_view name, LocalFuncHolder func)
{
    for (auto& [key, val] : LocalFuncs)
    {
        if (key == name)
        {
            val = func;
            return false;
        }
    }
    LocalFuncs.emplace_back(name, func);
    return true;
}

EvaluateContext::LocalFunc CompactEvaluateContext::LookUpFunc(std::u32string_view name)
{
    for (auto& [key, val] : LocalFuncs)
        if (key == name)
        {
            const auto [ptr, offset, size] = val;
            return { ptr, common::to_span(LocalFuncArgNames).subspan(offset, size) };
        }
    return { nullptr, {} };
}

size_t CompactEvaluateContext::GetArgCount() const noexcept
{
    return common::linq::FromIterable(Args)
        .Where([](const auto& tuple) { return !std::get<2>(tuple).IsEmpty(); })
        .Count();
}

size_t CompactEvaluateContext::GetFuncCount() const noexcept
{
    return LocalFuncs.size();
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

void NailangRuntimeBase::ThrowByArgCount(common::span<const Arg> args, const size_t count) const
{
    if (static_cast<size_t>(args.size()) != count)
        throw U"Expect Arg Count Not Match"sv;
}

void NailangRuntimeBase::ThrowByArgType(const Arg& arg, const Arg::InternalType type) const
{
    if (arg.TypeData != type)
        throw U"Arg type not xxx"sv;
}

void NailangRuntimeBase::ThrowByFuncContext(const std::u32string_view func, const FuncTarget target, const FuncTarget::Type type) const
{
    if (target.GetType() != type)
    {
        const auto name = type == FuncTarget::Type::Empty ? U"part of statement"sv : 
            (type == FuncTarget::Type::Block ? U"funccall"sv : U"metafunc"sv);
        throw std::u32string(func).append(U" only support as "sv).append(name);
    }
}

bool NailangRuntimeBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        throw std::u32string(varName).append(U" should be bool-able"sv);
    return ret.value();
}

bool NailangRuntimeBase::HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& content, common::span<const FuncCall>, BlockContext& ctx)
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
    {
        if (content.GetType() != BlockContent::Type::Block)
            throw U"MetaFunc[DefFunc] can only be applied to [Block]"sv;
        for (const auto& arg : meta.Args)
            if (arg.TypeData != RawArg::Type::Var)
                throw U"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv;
        EvalContext->SetFunc(content.Get<Block>(), meta.Args);
        return false;
    }
    if (meta.Name == U"If"sv)
    {
        ThrowByArgCount(meta, 1);
        return ThrowIfNotBool(EvaluateArg(meta.Args[0]), U"arg of MetaFunc[If]"sv);
    }
    if (meta.Name == U"While"sv)
    {
        ThrowByArgCount(meta, 1);
        if (ThrowIfNotBool(EvaluateArg(meta.Args[0]), U"arg of MetaFunc[While]"sv))
        {
            ctx.Status = ProgramStatus::Repeat;
            return true;
        }
        else
        {
            ctx.Status = ProgramStatus::Next;
            return false;
        }
    }
    return true;
}

bool NailangRuntimeBase::HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target, BlockContext& ctx)
{
    for (const auto& meta : metas)
    {
        if (!HandleMetaFuncBefore(meta, target, metas, ctx))
            return false;
    }
    return true;
}

void NailangRuntimeBase::HandleRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}

Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& func, common::span<const FuncCall> metas, const FuncTarget target)
{
    if (func.Args.size() == 0)
        return EvaluateFunc(func.Name, {}, metas, target);
    std::vector<Arg> args;
    args.reserve(func.Args.size());
    for (const auto& rawarg : func.Args)
        args.emplace_back(EvaluateArg(rawarg));
    return EvaluateFunc(func.Name, args, metas, target);
}

Arg NailangRuntimeBase::EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall>, const FuncTarget target)
{
    using Type = Arg::InternalType;
    // only for block-scope
    if (target.GetType() == FuncTarget::Type::Block)
    {
        auto& blkCtx = target.GetBlockContext();
        if (func == U"Break"sv)
        {
            if (!common::linq::FromIterable(blkCtx.MetaScope)
                .ContainsIf([](const FuncCall& meta) { return meta.Name == U"While"sv; }))
                throw U"Break can only be used inside While"sv;
            ThrowByArgCount(args, 0);
            blkCtx.Status = ProgramStatus::Break;
            return {};
        }
        else if (func == U"Return")
        {
            if (args.size() > 0)
            {
                ThrowByArgCount(args, 1);
                EvalContext->SetReturnArg(args[0]);
            }
            blkCtx.Status = ProgramStatus::Return;
            return {};
        }
    }
    // suitable for all
    if (common::str::IsBeginWith(func, U"EmbedOp."sv))
    {
        ThrowByFuncContext(func, target, FuncTarget::Type::Empty);
        const auto opname = func.substr(8);
        const auto ret = EmbedOpEval::Eval(opname, args);
        if (ret.has_value())
        {
            if (ret->TypeData == Type::Empty)
                throw U"Emebd op's arg type does not match requirement"sv;
            return *ret;
        }
    }
    else if (const auto lcFunc = EvalContext->LookUpFunc(func); lcFunc)
    {
        ThrowByArgCount(args, lcFunc.ArgNames.size());
        return ExecuteLocalFunc(lcFunc, args);
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
        return EvaluateFunc(*arg.GetVar<Type::Func>(), {}, {});
    case Type::Unary:
    {
        const auto& stmt = *arg.GetVar<Type::Unary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.Oprend, 1 };
        return EvaluateFunc(func, {}, {});
    }
    case Type::Binary:
    {
        const auto& stmt = *arg.GetVar<Type::Binary>();
        FuncCall func;
        func.Name = OpToFuncName(stmt.Operator);
        func.Args = { &stmt.LeftOprend, 2 };
        return EvaluateFunc(func, {}, {});
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

Arg NailangRuntimeBase::ExecuteLocalFunc(const EvaluateContext::LocalFunc& func, common::span<const Arg> args)
{
    const auto ctx = std::make_shared<CompactEvaluateContext>();
    NailangRuntimeBase runtime(ctx);
    for (const auto& [var, val] : common::linq::FromIterable(func.ArgNames).Pair(common::linq::FromIterable(args)))
        ctx->SetArg(var, val);
    ctx->SetFunc(func);
    runtime.ExecuteBlock({ *func.Body, {} });
    return ctx->GetReturnArg();
}

void NailangRuntimeBase::ExecuteAssignment(const Assignment& assign, common::span<const FuncCall>)
{
    EvalContext->SetArg(assign.Variable.Name, EvaluateArg(assign.Statement));
}

void NailangRuntimeBase::ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas, BlockContext& ctx)
{
    switch (content.GetType())
    {
    case BlockContent::Type::Assignment:
    {
        ExecuteAssignment(*content.Get<Assignment>(), metas);
    } break;
    case BlockContent::Type::Block:
    {
        const auto blkStatus = ExecuteBlock({ *content.Get<Block>(), metas });
        if (blkStatus == ProgramStatus::Return)
            ctx.Status = ProgramStatus::Return;
        else if (ctx.Status == ProgramStatus::Repeat && blkStatus == ProgramStatus::Break)
            ctx.Status = ProgramStatus::Next;
    } break;
    case BlockContent::Type::FuncCall:
    {    
        EvaluateFunc(*content.Get<FuncCall>(), metas, ctx);
    } break;
    default:
        Expects(false);
        break;
    }
}

NailangRuntimeBase::ProgramStatus NailangRuntimeBase::ExecuteBlock(BlockContext ctx)
{
    for (size_t idx = 0; idx < ctx.BlockScope.Size();)
    {
        const auto& [metas, content] = ctx.BlockScope[idx];
        const auto type = content.GetType();
        if (type == BlockContent::Type::RawBlock)
        {
            HandleRawBlock(*content.Get<RawBlock>(), metas);
        }
        else if (HandleMetaFuncsBefore(metas, content, ctx))
        {
            ExecuteContent(content, metas, ctx);
        }
        //else
        //{
        //    idx++;
        //}
        switch (ctx.Status)
        {
        case ProgramStatus::Return:
        case ProgramStatus::End:
        case ProgramStatus::Break:
            return ctx.Status;
        case ProgramStatus::Next:
            idx++; break;
        case ProgramStatus::Repeat:
            break;
        }
    }
    return ProgramStatus::End;
}

void NailangRuntimeBase::ExecuteBlock(const Block& block, common::span<const FuncCall> metas)
{
    ExecuteBlock({ block, metas });
}



}
