#include "NailangRuntime.h"
#include "StringCharset/Convert.h"
#include "3rdParty/fmt/utfext.h"
#include "common/StringEx.hpp"
#include "common/Linq2.hpp"
#include <cmath>


namespace xziar::nailang
{
using namespace std::string_view_literals;
using VarType = detail::VarHolder::VarType;


EvaluateContext::~EvaluateContext()
{ }

Arg EvaluateContext::LookUpArg(detail::VarHolder var) const
{
    if (var.GetType() == VarType::Root)
        return FindRoot()->LookUpArgInside(var);
    auto arg = LookUpArgInside(var);
    if (var.GetType() != VarType::Local && arg.TypeData == Arg::InternalType::Empty && ParentContext)
        return ParentContext->LookUpArg(var);
    else
        return arg;
}

bool EvaluateContext::SetArg(detail::VarHolder var, Arg arg)
{
    detail::VarLookup lookup(var);
    switch (var.GetType())
    {
    case VarType::Local:    
        return SetArgInside(lookup, arg, true);
    case VarType::Normal:   
        if (SetArgInside(lookup, arg, false) || (ParentContext && ParentContext->SetArg(var, arg)))
            return true;
        return SetArgInside(lookup, arg, true);
    case VarType::Root: 
        return FindRoot()->SetArgInside(lookup, arg, true);
    }
    Expects(false);
    return false;
}


BasicEvaluateContext::~BasicEvaluateContext()
{ }

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<const RawArg> args)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(args.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
    for (const auto& arg : args)
        LocalFuncArgNames.emplace_back(arg.GetVar<RawArg::Type::Var>().Name);
    return SetFuncInside(block->Name, { block, offset, size });
}

bool BasicEvaluateContext::SetFunc(const detail::LocalFunc & func)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(func.ArgNames.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
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

Arg LargeEvaluateContext::LookUpArgInside(detail::VarLookup var) const
{
    if (const auto it = ArgMap.find(var.GetFull()); it != ArgMap.end())
        return it->second;
    return Arg{};
}

bool LargeEvaluateContext::SetArgInside(detail::VarLookup var, Arg arg, const bool force)
{
    auto it = ArgMap.find(var.GetFull());
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
                ArgMap.insert_or_assign(std::u32string(var.GetFull()), arg);
            return false;
        }
    }
}

bool LargeEvaluateContext::SetFuncInside(std::u32string_view name, LocalFuncHolder func)
{
    return LocalFuncMap.insert_or_assign(name, func).second;
}

detail::LocalFunc LargeEvaluateContext::LookUpFunc(std::u32string_view name)
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

Arg CompactEvaluateContext::LookUpArgInside(detail::VarLookup var) const
{
    for (const auto& [pos, val] : Args)
        if (GetStr(pos.first, pos.second) == var.GetFull())
            return val;
    return Arg{};
}

bool CompactEvaluateContext::SetArgInside(detail::VarLookup var, Arg arg, const bool force)
{
    Arg* target = nullptr;
    for (auto& [pos, val] : Args)
        if (GetStr(pos.first, pos.second) == var.GetFull())
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
            const auto name = var.GetFull();
            const uint32_t offset = gsl::narrow_cast<uint32_t>(StrPool.size()),
                size = gsl::narrow_cast<uint32_t>(name.size());
            StrPool.insert(StrPool.end(), name.cbegin(), name.cend());
            Args.emplace_back(std::pair{ offset, size }, arg);
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

detail::LocalFunc CompactEvaluateContext::LookUpFunc(std::u32string_view name)
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
        .Where([](const auto& arg) { return !arg.second.IsEmpty(); })
        .Count();
}

size_t CompactEvaluateContext::GetFuncCount() const noexcept
{
    return LocalFuncs.size();
}



#define LR_BOTH(left, right, func) left.func() && right.func()
Arg EmbedOpEval::Equal(const Arg& left, const Arg& right) noexcept
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
Arg EmbedOpEval::NotEqual(const Arg& left, const Arg& right) noexcept
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

Arg EmbedOpEval::Less(const Arg& left, const Arg& right) noexcept
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
Arg EmbedOpEval::LessEqual(const Arg& left, const Arg& right) noexcept
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

Arg EmbedOpEval::And(const Arg& left, const Arg& right) noexcept
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ && *right_;
    return {};
}
Arg EmbedOpEval::Or(const Arg& left, const Arg& right) noexcept
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ || *right_;
    return {};
}

Arg EmbedOpEval::Add(const Arg& left, const Arg& right) noexcept
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
Arg EmbedOpEval::Sub(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l - r; });
}
Arg EmbedOpEval::Mul(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l * r; });
}
Arg EmbedOpEval::Div(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l / r; });
}
Arg EmbedOpEval::Rem(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) 
        {
            if constexpr (std::is_floating_point_v<decltype(l)> || std::is_floating_point_v<decltype(r)>)
                return std::fmod(static_cast<double>(l), static_cast<double>(r));
            else
                return l % r; 
        });
}

Arg EmbedOpEval::Not(const Arg& arg) noexcept
{
    const auto arg_ = arg.GetBool();
    if (arg_.has_value())
        return !*arg_;
    return {};
}
#undef LR_BOTH

std::optional<Arg> EmbedOpEval::Eval(const std::u32string_view opname, const std::array<Arg, 2>& args) noexcept
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


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))

NaailangCodeException::NaailangCodeException(const std::u32string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope, const std::any& data) :
    NailangRuntimeException(TYPENAME, common::strchset::to_u16string(msg, common::str::Charset::UTF32LE), std::move(target), std::move(scope), data)
{ }


NailangRuntimeBase::InnerContextScope::InnerContextScope(NailangRuntimeBase& host, std::shared_ptr<EvaluateContext>&& context) :
    Host(host), Context(std::move(Host.EvalContext))
{
    context->ParentContext = Context;
    Host.EvalContext = std::move(context);
}
NailangRuntimeBase::InnerContextScope::~InnerContextScope()
{
    Host.EvalContext = std::move(Context);
}

NailangRuntimeBase::NailangRuntimeBase(std::shared_ptr<EvaluateContext> context) : 
    EvalContext(std::move(context))
{ }
NailangRuntimeBase::~NailangRuntimeBase()
{ }

static constexpr auto ArgTypeName(const Arg::InternalType type) noexcept
{
    switch (type)
    {
    case Arg::InternalType::Empty:  return U"empty"sv;
    case Arg::InternalType::Var:    return U"variable"sv;
    case Arg::InternalType::U32Str:
    case Arg::InternalType::U32Sv:  return U"string"sv;
    case Arg::InternalType::Uint:   return U"uint"sv;
    case Arg::InternalType::Int:    return U"int"sv;
    case Arg::InternalType::FP:     return U"fp"sv;
    case Arg::InternalType::Bool:   return U"bool"sv;
    default:                        return U"error"sv;
    }
}

static constexpr auto ContentTypeName(const BlockContent::Type type) noexcept
{
    switch (type)
    {
    case BlockContent::Type::Assignment: return U"assignment"sv;
    case BlockContent::Type::FuncCall:   return U"funccall"sv;
    case BlockContent::Type::RawBlock:   return U"rawblock"sv;
    case BlockContent::Type::Block:      return U"block"sv;
    default:                             return U"error"sv;
    }
}

void NailangRuntimeBase::ThrowByArgCount(const FuncCall& call, const size_t count) const
{
    if (call.Args.size() != count)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Func [{}] requires [{}] args, which gives [{}]."), call.Name, call.Args.size(), count),
            detail::ExceptionTarget{}, &call);
}

void NailangRuntimeBase::ThrowByArgType(const Arg& arg, const Arg::InternalType type) const
{
    if (arg.TypeData != type)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Expected arg of [{}], which gives [{}]."), ArgTypeName(type), ArgTypeName(arg.TypeData)),
            arg);
}

void NailangRuntimeBase::ThrowByArgType(const FuncCall& call, const Arg& arg, const Arg::InternalType type, size_t idx) const
{
    if (arg.TypeData != type)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Expected [{}] for [{}]'s {} arg, which gives [{}]."), 
                ArgTypeName(type), call.Name, idx, ArgTypeName(arg.TypeData)),
            arg);
}

void NailangRuntimeBase::ThrowIfNotFuncTarget(const std::u32string_view func, const FuncTarget target, const FuncTarget::Type type) const
{
    if (target.GetType() != type)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Func [{}] only support as [{}]."), func, FuncTarget::FuncTargetName(type)),
            detail::ExceptionTarget::NewFuncCall(func));
}

void NailangRuntimeBase::ThrowIfBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const
{
    if (target.GetType() == type)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Metafunc [{}] cannot be appllied to [{}]."), meta.Name, ContentTypeName(type)),
            meta);
}

void NailangRuntimeBase::ThrowIfNotBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const
{
    if (target.GetType() != type)
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"Metafunc [{}] can only be appllied to [{}]."), meta.Name, ContentTypeName(type)),
            meta);
}

bool NailangRuntimeBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        NLRT_THROW_EX(fmt::format(FMT_STRING(u"[{}] should be bool-able, which is [{}]."), varName, ArgTypeName(arg.TypeData)),
            arg);
    return ret.value();
}

NailangRuntimeBase::MetaFuncResult NailangRuntimeBase::HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& content, common::span<const FuncCall>)
{
    if (meta.Name == U"Skip"sv)
    {
        const auto arg = EvaluateFuncArgs<1, false>(meta)[0];
        return arg.IsEmpty() || ThrowIfNotBool(arg, U"Arg of MetaFunc[Skip]"sv) ? MetaFuncResult::Skip : MetaFuncResult::Next;
    }
    if (meta.Name == U"If"sv)
    {
        const auto arg = EvaluateFuncArgs<1>(meta)[0];
        return ThrowIfNotBool(arg, U"Arg of MetaFunc[If]"sv) ? MetaFuncResult::Next : MetaFuncResult::Skip;
    }
    if (meta.Name == U"DefFunc"sv)
    {
        ThrowIfNotBlockContent(meta, content, BlockContent::Type::Block);
        for (const auto& arg : meta.Args)
            if (arg.TypeData != RawArg::Type::Var)
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv, arg, &meta);
        EvalContext->SetFunc(content.Get<Block>(), meta.Args);
        return MetaFuncResult::Skip;
    }
    if (meta.Name == U"While"sv)
    {
        ThrowIfBlockContent(meta, content, BlockContent::Type::RawBlock);
        if (content.GetType() == BlockContent::Type::RawBlock)
            NLRT_THROW_EX(u"MetaFunc[While] cannot be applied to [RawBlock]"sv, &meta, content);
        const auto arg = EvaluateFuncArgs<1>(meta)[0];
        return ThrowIfNotBool(arg, U"Arg of MetaFunc[While]"sv) ? MetaFuncResult::Repeat : MetaFuncResult::Skip;
    }
    return MetaFuncResult::Unhandled;
}

bool NailangRuntimeBase::HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target, BlockContext& ctx)
{
    for (const auto& meta : metas)
    {
        switch (HandleMetaFuncBefore(meta, target, metas))
        {
        case MetaFuncResult::Repeat:
            ctx.Status = ProgramStatus::Repeat; return true;
        case MetaFuncResult::Skip:
            ctx.Status = ProgramStatus::Next;   return false;
        case MetaFuncResult::Next:
        default:
            ctx.Status = ProgramStatus::Next;   break;
        }
    }
    return true;
}

void NailangRuntimeBase::HandleException(const NailangRuntimeException& ex) const
{
    ex.EvalContext = EvalContext;
    ex.ThrowSelf();
}

Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target)
{
    using Type = Arg::InternalType;
    // only for block-scope
    if (target.GetType() == FuncTarget::Type::Block)
    {
        auto& blkCtx = target.GetBlockContext();
        if (call.Name == U"Break"sv)
        {
            ThrowByArgCount(call, 0);
            if (!blkCtx.SearchMeta(U"While"sv))
                NLRT_THROW_EX(u"Break can only be used inside While"sv, call);
            blkCtx.Status = ProgramStatus::Break;
            return {};
        }
        else if (call.Name == U"Return")
        {
            EvalContext->SetReturnArg(EvaluateFuncArgs<1, false>(call)[0]);
            blkCtx.Status = ProgramStatus::Return;
            return {};
        }
        else if (call.Name == U"Throw"sv)
        {
            const auto args = EvaluateFuncArgs<1>(call);
            this->HandleException(CREATE_EXCEPTION(NaailangCodeException, args[0].ToString().StrView(), call));
            return {};
        }
    }
    // suitable for all
    if (common::str::IsBeginWith(call.Name, U"EmbedOp."sv))
    {
        ThrowIfNotFuncTarget(call.Name, target, FuncTarget::Type::Empty);
        const auto opname = call.Name.substr(8);
        const auto ret = EmbedOpEval::Eval(opname, EvaluateFuncArgs<2, false>(call));
        if (ret.has_value())
        {
            if (ret->TypeData == Type::Empty)
                NLRT_THROW_EX(u"Emebd op's arg type does not match requirement"sv, call);
            return *ret;
        }
        // TODO: should explicitly handle unsolved EmbedOp
    }
    else if (const auto lcFunc = EvalContext->LookUpFunc(call.Name); lcFunc)
    {
        return EvaluateLocalFunc(lcFunc, call, metas, target);
    }
    return EvaluateUnknwonFunc(call, metas, target);
}

Arg NailangRuntimeBase::EvaluateLocalFunc(const detail::LocalFunc& func, const FuncCall& call, common::span<const FuncCall>, const FuncTarget)
{
    ThrowByArgCount(call, func.ArgNames.size());
    std::vector<Arg> args;
    args.reserve(call.Args.size());
    for (const auto& rawarg : call.Args)
        args.emplace_back(EvaluateArg(rawarg));

    const auto ctx = std::make_shared<CompactEvaluateContext>();
    NailangRuntimeBase runtime(ctx);
    for (const auto& [var, val] : common::linq::FromIterable(func.ArgNames).Pair(common::linq::FromIterable(args)))
        ctx->SetArg(var, val);
    ctx->SetFunc(func);
    runtime.ExecuteBlock({ *func.Body, {} });
    return ctx->GetReturnArg();
}

Arg NailangRuntimeBase::EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall>, const FuncTarget target)
{
    NLRT_THROW_EX(fmt::format(FMT_STRING(u"Func [{}] with [{}] args cannot be resolved as [{}]."), 
        call.Name, call.Args.size(), FuncTarget::FuncTargetName(target.GetType())), call);
    return {};
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

void NailangRuntimeBase::OnAssignment(const Assignment& assign, common::span<const FuncCall>)
{
    EvalContext->SetArg(assign.Variable.Name, EvaluateArg(assign.Statement));
}

void NailangRuntimeBase::OnRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}

void NailangRuntimeBase::OnFuncCall(const FuncCall& call, common::span<const FuncCall> metas, BlockContext& ctx)
{
    EvaluateFunc(call, metas, ctx);
}

NailangRuntimeBase::ProgramStatus NailangRuntimeBase::OnInnerBlock(const Block& block, common::span<const FuncCall> metas)
{
    auto innerCtx = std::make_shared<CompactEvaluateContext>();
    InnerContextScope scope(*this, innerCtx);
    return ExecuteBlock({ block, metas });
}

void NailangRuntimeBase::ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas, BlockContext& ctx)
{
    switch (content.GetType())
    {
    case BlockContent::Type::Assignment:
    {
        OnAssignment(*content.Get<Assignment>(), metas);
    } break;
    case BlockContent::Type::Block:
    {
        const auto blkStatus = OnInnerBlock(*content.Get<Block>(), metas);
        if (blkStatus == ProgramStatus::Return)
            ctx.Status = ProgramStatus::Return;
        else if (ctx.Status == ProgramStatus::Repeat && blkStatus == ProgramStatus::Break)
            ctx.Status = ProgramStatus::Next;
    } break;
    case BlockContent::Type::FuncCall:
    {    
        OnFuncCall(*content.Get<FuncCall>(), metas, ctx);
    } break; 
    case BlockContent::Type::RawBlock:
    {
        OnRawBlock(*content.Get<RawBlock>(), metas);
    } break;
    default:
        Expects(false);
        break;
    }
}

NailangRuntimeBase::ProgramStatus NailangRuntimeBase::ExecuteBlock(BlockContext ctx)
{
    for (size_t idx = 0; idx < ctx.BlockScope->Size();)
    {
        const auto& [metas, content] = (*ctx.BlockScope)[idx];
        if (HandleMetaFuncsBefore(metas, content, ctx))
        {
            ExecuteContent(content, metas, ctx);
        }
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

void NailangRuntimeBase::ExecuteBlock(const Block& block, common::span<const FuncCall> metas, const bool checkMetas)
{
    if (checkMetas)
    {

        BlockContext dummy;
        auto target = BlockContentItem::Generate(&block, 0, 0);
        if (!HandleMetaFuncsBefore(metas, target, dummy))
            return;
    }
    ExecuteBlock({ block, metas });
}



}
