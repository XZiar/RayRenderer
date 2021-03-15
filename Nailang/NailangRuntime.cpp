#include "NailangRuntime.h"
#include "NailangParser.h"
#include "ParserRely.h"
#include "SystemCommon/MiscIntrins.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Format.h"
#include "common/StringEx.hpp"
#include "common/Linq2.hpp"
#include "common/StrParsePack.hpp"
#include "common/CharConvs.hpp"
#include <cmath>
#include <cassert>


namespace xziar::nailang
{
using namespace std::string_view_literals;
using common::DJBHash;
using common::str::HashedStrView;


#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))

NailangFormatException::NailangFormatException(const std::u32string_view formatter, const std::runtime_error& err)
    : NailangRuntimeException(T_<ExceptionInfo>{}, FMTSTR(u"Error when formating string: {}"sv, err.what()), formatter)
{ }
NailangFormatException::NailangFormatException(const std::u32string_view formatter, const Arg& arg, const std::u16string_view reason)
    : NailangRuntimeException(T_<ExceptionInfo>{}, FMTSTR(u"Error when formating string: {}"sv, reason), formatter, arg)
{ }

NailangCodeException::ExceptionInfo::ExceptionInfo(const char* type, const std::u32string_view msg,
    detail::ExceptionTarget target, detail::ExceptionTarget scope)
    : NailangCodeException::TPInfo(type, common::str::to_u16string(msg), std::move(target), std::move(scope))
{ }
NailangCodeException::NailangCodeException(const std::u32string_view msg, detail::ExceptionTarget target, detail::ExceptionTarget scope) :
    NailangRuntimeException(T_<ExceptionInfo>{}, msg, std::move(target), std::move(scope))
{ }


std::u32string_view SubQuery::ExpectSubField(size_t idx) const
{
    Expects(idx < Queries.size());
    if (static_cast<QueryType>(Queries[idx].ExtraFlag) != QueryType::Sub)
        COMMON_THROW(NailangRuntimeException, u"Expect [Subfield] as sub-query, get [Index]"sv);
    return Queries[idx].GetVar<Expr::Type::Str>();
}
const Expr& SubQuery::ExpectIndex(size_t idx) const
{
    Expects(idx < Queries.size());
    if (static_cast<QueryType>(Queries[idx].ExtraFlag) != QueryType::Index)
        COMMON_THROW(NailangRuntimeException, u"Expect [Index] as sub-query, get [Subfield]"sv);
    return Queries[idx];
}


ArgLocator Arg::HandleQuery(SubQuery subq, NailangExecutor& runtime)
{
    Expects(subq.Size() > 0);
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleQuery>(subq, runtime);
    }
    if (IsArray())
    {
        const auto arr = GetVar<Type::Array>();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateExpr(query);
            const auto idx = NailangHelper::BiDirIndexCheck(arr.Length, val, &query);
            return arr.Access(idx);
        }
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length"sv)
                return { arr.Length, 1u };
        } break;
        default: break;
        }
        return {};
    }
    if (IsStr())
    {
        const auto str = GetStr().value();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateExpr(query);
            const auto idx = NailangHelper::BiDirIndexCheck(str.size(), val, &query);
            if (TypeData == Type::U32Str)
                return { std::u32string(1, str[idx]), 1u };
            else // (TypeData == Type::U32Sv)
                return { str.substr(idx, 1), 1u };
        }
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length"sv)
                return { uint64_t(str.size()), 1u };
        } break;
        default: break;
        }
        return {};
    }
    return {};
}
Arg Arg::HandleUnary(const EmbedOps op)
{
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleUnary>(op);
    }
    if (op == EmbedOps::Not)
        return EmbedOpEval::Not(*this);
    return {};
}
Arg Arg::HandleBinary(const EmbedOps op, const Arg& right)
{
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleBinary>(op, right);
    }
    switch (op)
    {
#define EVAL_BIN_OP(type) case EmbedOps::type: return EmbedOpEval::type(*this, right)
        EVAL_BIN_OP(Equal);
        EVAL_BIN_OP(NotEqual);
        EVAL_BIN_OP(Less);
        EVAL_BIN_OP(LessEqual);
        EVAL_BIN_OP(Greater);
        EVAL_BIN_OP(GreaterEqual);
        EVAL_BIN_OP(Add);
        EVAL_BIN_OP(Sub);
        EVAL_BIN_OP(Mul);
        EVAL_BIN_OP(Div);
        EVAL_BIN_OP(Rem);
        EVAL_BIN_OP(And);
        EVAL_BIN_OP(Or);
#undef EVAL_BIN_OP
    default: return {};
    }
}


ArgLocator CustomVar::Handler::HandleQuery(CustomVar& var, SubQuery subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const auto [type, query] = subq[0];
    switch (type)
    {
    case SubQuery::QueryType::Index:
    {
        const auto idx = executor.EvaluateExpr(query);
        auto result = IndexerGetter(var, idx, query);
        if (!result.IsEmpty())
            return { std::move(result), 1u };
    } break;
    case SubQuery::QueryType::Sub:
    {
        auto field = query.GetVar<Expr::Type::Str>();
        auto result = SubfieldGetter(var, field);
        if (!result.IsEmpty())
            return { std::move(result), 1u };
    } break;
    default: break;
    }
    return {};
}
Arg CustomVar::Handler::HandleUnary(const CustomVar&, const EmbedOps)
{
    return {};
}
Arg CustomVar::Handler::HandleBinary(const CustomVar& var, const EmbedOps op, const Arg& right)
{
    switch (op)
    {
    case EmbedOps::Equal:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsEqual();
        break;
    case EmbedOps::NotEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsNotEqual();
        break;
    case EmbedOps::Less:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsLess();
        break;
    case EmbedOps::LessEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsLessEqual();
        break;
    case EmbedOps::Greater:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsGreater();
        break;
    case EmbedOps::GreaterEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsGreaterEqual();
        break;
    default:
        break;
    }
    return {};
}


ArgLocator AutoVarHandlerBase::HandleQuery(CustomVar& var, SubQuery subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    const auto [type, query] = subq[0];
    if (var.Meta1 < UINT32_MAX) // is array
    {
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto idx  = executor.EvaluateExpr(query);
            size_t tidx = 0;
            if (ExtendIndexer)
            {
                const auto idx_ = ExtendIndexer(reinterpret_cast<void*>(var.Meta0), var.Meta1, idx);
                if (idx_.has_value())
                    tidx = *idx_;
                else
                    COMMON_THROW(NailangRuntimeException,
                        FMTSTR(u"cannot find element for index [{}] in array of [{}]"sv, idx.ToString().StrView(), var.Meta1), query);
            }
            else
            {
                tidx = NailangHelper::BiDirIndexCheck(var.Meta1, idx, &query);
            }
            const auto ptr  = static_cast<uintptr_t>(var.Meta0) + tidx * TypeSize;
            const auto flag = nonConst ? ArgLocator::LocateFlags::ReadWrite : ArgLocator::LocateFlags::ReadOnly;
            return { CustomVar{ var.Host, ptr, UINT32_MAX, var.Meta2 }, 1, flag };
        }
        case SubQuery::QueryType::Sub:
        {
            auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length")
                return { uint64_t(var.Meta1), 1 };
        } break;
        default: break;
        }
    }
    else
    {
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto idx = executor.EvaluateExpr(query);
            auto result = IndexerGetter(var, idx, query);
            if (!result.IsEmpty())
                return { std::move(result), 1u };
        } break;
        case SubQuery::QueryType::Sub:
        {
            auto field = query.GetVar<Expr::Type::Str>();
            if (const auto accessor = FindMember(field); accessor)
            {
                const auto ptr = reinterpret_cast<void*>(var.Meta0);
                const auto isConst = !nonConst || accessor->IsConst;
                if (accessor->IsSimple)
                    return { [=]() { return accessor->SimpleMember.first(ptr); },
                        isConst ? ArgLocator::Setter{} : [=](Arg val) { return accessor->SimpleMember.second(ptr, std::move(val)); },
                        1 };
                else
                    return { accessor->AutoMember(ptr), 1, isConst ? ArgLocator::LocateFlags::ReadOnly : ArgLocator::LocateFlags::ReadWrite };
            }
        } break;
        default: break;
        }
    }
    return {};
}

std::u32string_view AutoVarHandlerBase::GetTypeName() noexcept
{
    return TypeName;
}


EvaluateContext::~EvaluateContext()
{ }


BasicEvaluateContext::~BasicEvaluateContext()
{ }

std::pair<uint32_t, uint32_t> BasicEvaluateContext::InsertCaptures(
    common::span<std::pair<std::u32string_view, Arg>> capture)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncCapturedArgs.size()),
        size = gsl::narrow_cast<uint32_t>(capture.size());
    LocalFuncCapturedArgs.reserve(static_cast<size_t>(offset) + size);
    for (auto& [name, arg] : capture)
    {
        LocalFuncCapturedArgs.push_back(std::move(arg));
    }
    return { offset, size };
}
template<typename T>
std::pair<uint32_t, uint32_t> BasicEvaluateContext::InsertNames(
    common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const T> argNames)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(capture.size() + argNames.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
    for (const auto& [name, arg] : capture)
    {
        LocalFuncArgNames.push_back(name);
    }
    for (const auto& argName : argNames)
    {
        if constexpr(std::is_same_v<T, Expr>)
            LocalFuncArgNames.push_back(argName.template GetVar<Expr::Type::Var>().Name);
        else
            LocalFuncArgNames.push_back(argName);
    }
    return { offset, size };
}

LocalFunc BasicEvaluateContext::LookUpFunc(std::u32string_view name) const
{
    const auto [ptr, argRange, nameRange] = LookUpFuncInside(name);
    return 
    { 
        ptr, 
        common::to_span(LocalFuncArgNames).subspan(nameRange.first, nameRange.second),
        common::to_span(LocalFuncCapturedArgs).subspan(argRange.first, argRange.second)
    };
}

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args)
{
    const auto argRange  = InsertCaptures(capture);
    const auto nameRange = InsertNames(capture, args);
    return SetFuncInside(block->Name, { block, argRange, nameRange });
}

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args)
{
    const auto argRange = InsertCaptures(capture);
    const auto nameRange = InsertNames(capture, args);
    return SetFuncInside(block->Name, { block, argRange, nameRange });
}


LargeEvaluateContext::~LargeEvaluateContext()
{ }

bool LargeEvaluateContext::SetFuncInside(std::u32string_view name, LocalFuncHolder func)
{
    return LocalFuncMap.insert_or_assign(name, func).second;
}

BasicEvaluateContext::LocalFuncHolder LargeEvaluateContext::LookUpFuncInside(std::u32string_view name) const
{
    const auto it = LocalFuncMap.find(name);
    if (it == LocalFuncMap.end())
        return { nullptr, {0,0}, {0,0} };
    return it->second;
}

ArgLocator LargeEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    if (const auto it = ArgMap.find(var.Name); it != ArgMap.end())
        return { &it->second, 0, ArgLocator::LocateFlags::ReadWrite };
    if (create)
    { // need create
        const auto [it, _] = ArgMap.insert_or_assign(std::u32string(var.Name), Arg{});
        return { &it->second, 0, ArgLocator::LocateFlags::WriteOnly };
    }
    return {};
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

BasicEvaluateContext::LocalFuncHolder CompactEvaluateContext::LookUpFuncInside(std::u32string_view name) const
{
    for (auto& [key, val] : LocalFuncs)
        if (key == name)
        {
            return val;
        }
    return { nullptr, {0,0}, {0,0} };
}

ArgLocator CompactEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    const HashedStrView hsv(var.Name);
    for (auto& [pos, val] : Args)
        if (ArgNames.GetHashedStr(pos) == hsv)
        {
            if (!val.IsEmpty())
                return { &val, 0, ArgLocator::LocateFlags::ReadWrite };
            if (create)
                return { &val, 0, ArgLocator::LocateFlags::WriteOnly };
            return {};
        }
    if (create)
    { // need create
        const auto piece = ArgNames.AllocateString(var.Name);
        Args.emplace_back(piece, Arg{});
        return { &Args.back().second, 0, ArgLocator::LocateFlags::WriteOnly };
    }
    return {};
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
        return left.GetVar<Arg::Type::Bool>() == right.GetVar<Arg::Type::Bool>();
    if (LR_BOTH(left, right, IsStr))
        return left.GetStr() == right.GetStr();
    if (LR_BOTH(left, right, IsFloatPoint))
        return left.GetVar<Arg::Type::FP>() == right.GetVar<Arg::Type::FP>();
    if (LR_BOTH(left, right, IsNumber))
        return left.GetFP() == right.GetFP();
    return {};
}
Arg EmbedOpEval::NotEqual(const Arg& left, const Arg& right) noexcept
{
    const auto ret = Equal(left, right);
    if (ret.IsBool())
        return !ret.GetVar<Arg::Type::Bool>();
    return ret;
}
template<typename F>
forceinline static Arg NumOp(const Arg& left, const Arg& right, F func) noexcept
{
    using Type = Arg::Type;
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


size_t NailangHelper::BiDirIndexCheck(const size_t size, const Arg& idx, const Expr* src)
{
    if (!idx.IsInteger())
    {
        auto err = FMTSTR(u"Require integer as indexer, get[{}]"sv, idx.GetTypeName());
        if (src)
            COMMON_THROW(NailangRuntimeException, err, *src);
        else
            COMMON_THROW(NailangRuntimeException, err);
    }
    bool isReverse = false;
    uint64_t realIdx = 0;
    if (idx.TypeData == Arg::Type::Uint)
        realIdx = idx.GetUint().value();
    else
    {
        const auto tmp = idx.GetInt().value();
        isReverse = tmp < 0;
        realIdx = std::abs(tmp);
    }
    if (realIdx >= size)
        COMMON_THROW(NailangRuntimeException,
            FMTSTR(u"index out of bound, access [{}{}] of length [{}]"sv, isReverse ? u"-"sv : u""sv, realIdx, size));
    return isReverse ? static_cast<size_t>(size - realIdx) : static_cast<size_t>(realIdx);
}

template<typename F>
bool NailangHelper::LocateWrite(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func)
{
    if (query.Size() > 0)
    {
        auto next = target.HandleQuery(query, executor);
        auto subq = query.Sub(next.GetConsumed());
        if (!next)
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                Serializer::Stringify(subq)));
        if (!next.IsMutable())
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not assgin to immutable's [{}]",
                Serializer::Stringify(subq)));
        return LocateWrite(next, subq, executor, func);
    }
    else
    {
        return func(target);
    }
}
template<typename F>
Arg NailangHelper::LocateRead(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func)
{
    if (query.Size() > 0)
    {
        auto next = target.HandleQuery(query, executor);
        auto subq = query.Sub(next.GetConsumed());
        if (!next)
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                Serializer::Stringify(subq)));
        if (!next.CanRead())
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not access an set-host's [{}]",
                Serializer::Stringify(subq)));
        return LocateRead(next, subq, executor, func);
    }
    else
    {
        return func(target);
    }
}
template<typename F>
Arg NailangHelper::LocateRead(Arg& target, SubQuery query, NailangExecutor& executor, const F& func)
{
    Expects(query.Size() > 0);
    auto next = target.HandleQuery(query, executor);
    auto subq = query.Sub(next.GetConsumed());
    if (!next)
        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
            Serializer::Stringify(subq)));
    if (!next.CanRead())
        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not access an set-host's [{}]",
            Serializer::Stringify(subq)));
    return LocateRead(next, subq, executor, func);
}
template<bool IsWrite, typename F>
auto NailangHelper::LocateAndExecute(ArgLocator& target, SubQuery query, NailangExecutor& executor, const F& func)
    -> decltype(func(std::declval<ArgLocator&>()))
{
    if (query.Size() > 0)
    {
        auto next = target.HandleQuery(query, executor);
        auto subq = query.Sub(next.GetConsumed());
        if constexpr (IsWrite)
        {
            if (!next.IsMutable())
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not assgin to immutable's [{}]",
                    Serializer::Stringify(subq)));
        }
        else
        {
            if (!next.CanRead())
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not access an set-host's [{}]",
                    Serializer::Stringify(subq)));
        }
        return LocateAndExecute<IsWrite>(next, subq, executor, func);
    }
    else
    {
        return func(target);
    }
}


class NailangFrame::FuncInfoHolder
{
    friend NailangFrame;
    friend NailangExecutor;
    NailangFrame& Host;
    FuncStackInfo Info;
    constexpr FuncInfoHolder(NailangFrame* host, const FuncCall* func) : Host(*host), Info{ Host.FuncInfo, func }
    {
        Host.FuncInfo = &Info;
    }
public:
    COMMON_NO_COPY(FuncInfoHolder)
    COMMON_NO_MOVE(FuncInfoHolder)
    ~FuncInfoHolder()
    {
        Expects(Host.FuncInfo == &Info);
        Host.FuncInfo = Info.PrevInfo;
    }
};


static constexpr auto ContentTypeName(const Statement::Type type) noexcept
{
    switch (type)
    {
    case Statement::Type::Assign:   return U"assignment"sv;
    case Statement::Type::FuncCall: return U"funccall"sv;
    case Statement::Type::RawBlock: return U"rawblock"sv;
    case Statement::Type::Block:    return U"block"sv;
    default:                        return U"error"sv;
    }
}
static constexpr std::u32string_view FuncTypeName(const FuncName::FuncInfo type)
{
    switch (type)
    {
    case FuncName::FuncInfo::Empty:     return U"Plain"sv;
    case FuncName::FuncInfo::Meta:      return U"MetaFunc"sv;
    case FuncName::FuncInfo::ExprPart:  return U"Part of expr"sv;
    default:                            return U"Unknown"sv;
    }
}


void NailangBase::ThrowByArgCount(const FuncCall& call, const size_t count, const ArgLimits limit) const
{
    std::u32string_view prefix;
    switch (limit)
    {
    case ArgLimits::Exact:
        if (call.Args.size() == count) return;
        prefix = U""; break;
    case ArgLimits::AtMost:
        if (call.Args.size() <= count) return;
        prefix = U"at most"; break;
    case ArgLimits::AtLeast:
        if (call.Args.size() >= count) return;
        prefix = U"at least"; break;
    }
    NLRT_THROW_EX(FMTSTR(u"Func [{}] requires {} [{}] args, which gives [{}].", call.FullFuncName(), prefix, count, call.Args.size()),
        detail::ExceptionTarget{}, call);
}
void NailangBase::ThrowByArgType(const FuncCall& func, const Expr::Type type, size_t idx) const
{
    const auto& arg = func.Args[idx];
    if (arg.TypeData != type && type != Expr::Type::Empty)
        NLRT_THROW_EX(FMTSTR(u"Expected [{}] for [{}]'s {} arg-expr, which gives [{}], source is [{}].",
            Expr::TypeName(type), func.FullFuncName(), idx, arg.GetTypeName(), Serializer::Stringify(arg)),
            arg);
}
void NailangBase::ThrowByArgTypes(const FuncCall& func, common::span<const Expr::Type> types, const ArgLimits limit, size_t offset) const
{
    ThrowByArgCount(func, offset + types.size(), limit);
    Ensures(func.Args.size() >= offset);
    const auto size = std::min(types.size(), func.Args.size() - offset);
    for (size_t i = 0; i < size; ++i)
        ThrowByArgType(func, types[i], offset + i);
}
void NailangBase::ThrowByParamType(const FuncCall& func, const Arg& arg, const Arg::Type type, size_t idx) const
{
    if ((arg.TypeData & type) != type)
        NLRT_THROW_EX(FMTSTR(u"Expected [{}] for [{}]'s {} arg, which gives [{}], source is [{}].",
            Arg::TypeName(type), func.FullFuncName(), idx, arg.GetTypeName(), Serializer::Stringify(func.Args[idx])),
            arg);
}
void NailangBase::ThrowByParamTypes(const FuncPack& func, common::span<const Arg::Type> types, const ArgLimits limit, size_t offset) const
{
    ThrowByArgCount(func, types.size() + offset, limit);
    Ensures(func.Params.size() >= offset);
    const auto size = std::min(types.size() + offset, func.Params.size()) - offset;
    for (size_t i = 0; i < size; ++i)
        ThrowByParamType(func, func.Params[offset + i], types[i], offset + i);
}
void NailangBase::ThrowIfNotFuncTarget(const FuncCall& call, const FuncName::FuncInfo type) const
{
    if (call.Name->Info() != type)
    {
        NLRT_THROW_EX(FMTSTR(u"Func [{}] only support as [{}], get[{}].", call.FullFuncName(), FuncTypeName(type), FuncTypeName(call.Name->Info())),
            call);
    }
}
void NailangBase::ThrowIfStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const
{
    if (target.TypeData == type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] cannot be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}
void NailangBase::ThrowIfNotStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const
{
    if (target.TypeData != type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] can only be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}
bool NailangBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        NLRT_THROW_EX(FMTSTR(u"[{}] should be bool-able, which is [{}].", varName, arg.GetTypeName()),
            arg);
    return ret.value();
}


void NailangExecutor::EvalFuncArgs(const FuncCall& func, Arg* args, const Arg::Type* types, const size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        args[i] = EvaluateExpr(func.Args[i]);
        if (types != nullptr)
            ThrowByParamType(func, args[i], types[i], i);
    }
}
std::vector<Arg> NailangExecutor::EvalFuncAllArgs(const FuncCall& func)
{
    std::vector<Arg> params;
    params.reserve(func.Args.size());
    for (const auto& arg : func.Args)
        params.emplace_back(EvaluateExpr(arg));
    return params;
}

bool NailangExecutor::HandleMetaFuncs(MetaSet& allMetas, const Statement& target)
{
    for (auto meta : allMetas)
    {
        if (meta.IsUsed())
            continue;
        Expects(meta->Name->Info() == FuncName::FuncInfo::Meta);
        //ExprHolder callHost(this, &meta);
        const auto result = HandleMetaFunc(*meta, target, allMetas);
        if (result != MetaFuncResult::Unhandled)
            meta.SetUsed();
        switch (result)
        {
        case MetaFuncResult::Return:    GetFrame().SetReturn({}); return false;
        case MetaFuncResult::Skip:      return false;
        default:                        break;
        }
    }
    return true;
}
NailangExecutor::MetaFuncResult NailangExecutor::HandleMetaFunc(const FuncCall& meta, const Statement& content, MetaSet& allMetas)
{
    const auto metaName = meta.FullFuncName();
    if (metaName == U"While")
    {
        ThrowIfStatement(meta, content, Statement::Type::RawBlock);
        ThrowByArgCount(meta, 1);
        const auto& condition = meta.Args[0];

        const auto prevFrame = Runtime->PushFrame(NailangFrame::FrameFlags::LoopScope);
        auto& frame = GetFrame();
        frame.BlockScope = prevFrame->BlockScope;
        frame.CurContent = &content;
        frame.Executor = this;
        while (true)
        {
            if (const auto cond = EvaluateExpr(condition); ThrowIfNotBool(cond, U"Arg of MetaFunc[While]"))
            {
                Expects(frame.Status == NailangFrame::ProgramStatus::Next);
                HandleContent(*frame.CurContent, {});
                if (frame.Has(NailangFrame::ProgramStatus::LoopEnd))
                    break;
                frame.Status = NailangFrame::ProgramStatus::Next; // reset state
            }
            else
                break;
        }
        //Runtime->OnLoop(meta.Args[0], content);
        return MetaFuncResult::Skip;
    }
    else if (metaName == U"MetaIf")
    {
        const auto args = EvalFuncArgs<2, ArgLimits::AtLeast>(meta, { Arg::Type::Boolable, Arg::Type::String });
        if (args[0].GetBool().value())
        {
            const auto name = args[1].GetStr().value();
            const auto funcName = Runtime->CreateTempFuncName(name, FuncName::FuncInfo::Meta);
            const FuncCall newMeta(funcName.Ptr(), meta.Args.subspan(2), meta.Position);
            return HandleMetaFunc(newMeta, content, allMetas);
        }
        return MetaFuncResult::Next;
    }
    else if (metaName == U"DefFunc")
    {
        ThrowIfNotStatement(meta, content, Statement::Type::Block);
        for (const auto& arg : meta.Args)
        {
            if (arg.TypeData != Expr::Type::Var)
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv, arg, meta);
            const auto& var = arg.GetVar<Expr::Type::Var>();
            if (HAS_FIELD(var.Info, LateBindVar::VarInfo::PrefixMask))
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg name must not contain [Root|Local] flag"sv, arg, meta);
        }
        std::vector<std::pair<std::u32string_view, Arg>> captures;
        for (auto m : allMetas)
        {
            if (m->GetName() == U"Capture"sv)
            {
                ThrowByArgTypes<1, 2>(*m, { Expr::Type::Var, Expr::Type::Empty });
                auto argName = m->Args[0].GetVar<Expr::Type::Var>();
                if (m->Args.size() > 1 && HAS_FIELD(argName.Info, LateBindVar::VarInfo::PrefixMask))
                    NLRT_THROW_EX(u"MetaFunc[Capture]'s arg name must not contain [Root|Local] flag"sv, *m, meta);
                captures.emplace_back(argName.Name, EvaluateExpr(m->Args.back()));
                m.SetUsed();
            }
        }
        Runtime->SetFunc(content.Get<Block>(), captures, meta.Args);
        return MetaFuncResult::Skip;
    }
    auto params = EvalFuncAllArgs(meta);
    MetaEvalPack pack(meta, params, allMetas, content);
    return HandleMetaFunc(pack);
}
NailangExecutor::MetaFuncResult NailangExecutor::HandleMetaFunc(MetaEvalPack& meta)
{
    const auto metaName = meta.FullFuncName();
    if (metaName == U"Skip")
    {
        ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::Boolable });
        return (meta.Params.empty() || meta.Params[0].GetBool().value()) ? MetaFuncResult::Skip : MetaFuncResult::Next;
    }
    else if (metaName == U"If")
    {
        ThrowByParamTypes<1>(meta, { Arg::Type::Boolable });
        const auto check = meta.Params[0].GetBool().value();
        GetFrame().IfRecord = check ? 0b1100u : 0b1000u; // expecting to be right shifted
        return check ? MetaFuncResult::Next : MetaFuncResult::Skip;
    }
    else if (metaName == U"Else")
    {
        ThrowByArgCount(meta, 0);
        switch (GetFrame().IfRecord)
        {
        case 0b11:  return MetaFuncResult::Skip;
        case 0b10:  return MetaFuncResult::Next;
        default:
            NLRT_THROW_EX(u"MetaFunc [Else] can not be followed by [If].", meta);
        }
    }
    return MetaFuncResult::Unhandled;
}

Arg NailangExecutor::EvaluateExpr(const Expr& arg)
{
    using Type = Expr::Type;
    switch (arg.TypeData)
    {
    case Type::Func:
    {
        const auto& fcall = arg.GetVar<Type::Func>();
        Expects(fcall->Name->Info() == FuncName::FuncInfo::ExprPart);
        return EvaluateFunc(*fcall, {});
    }
    case Type::Unary:
        return EvaluateUnaryExpr(*arg.GetVar<Type::Unary>());
    case Type::Binary:
        return EvaluateBinaryExpr(*arg.GetVar<Type::Binary>());
    case Type::Ternary:
        return EvaluateTernaryExpr(*arg.GetVar<Type::Ternary>());
    case Type::Query:
        return EvaluateQueryExpr(*arg.GetVar<Type::Query>());
    case Type::Var:     return Runtime->LookUpArg(arg.GetVar<Type::Var>());
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            assert(false); return {};
    }
}

Arg NailangExecutor::EvaluateFunc(const FuncCall& func, common::span<const FuncCall> metas)
{
    Expects(func.Name->PartCount > 0);
    const auto fullName = func.FullFuncName();
    if (func.Name->Info() == FuncName::FuncInfo::Meta)
    {
        NLRT_THROW_EX(FMTSTR(u"MetaFunc [{}] can not be evaluated here.", fullName), func);
    }
    NailangFrame::FuncInfoHolder holder(&GetFrame(), &func);
    // only for plain function
    if (func.Name->Info() == FuncName::FuncInfo::Empty && func.Name->PartCount == 1)
    {
        switch (DJBHash::HashC(fullName))
        {
        HashCase(fullName, U"Return")
        {
            const auto dst = GetFrame().SetReturn(EvalFuncSingleArg(func, Arg::Type::Empty, ArgLimits::AtMost));
            if (!dst)
                NLRT_THROW_EX(u"[Return] can only be used inside FlowScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Break")
        {
            const auto dst = GetFrame().SetBreak();
            if (!dst)
                NLRT_THROW_EX(u"[Break] can only be used inside LoopScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Continue")
        {
            const auto dst = GetFrame().SetContinue();
            if (!dst)
                NLRT_THROW_EX(u"[Continue] can only be used inside LoopScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Throw")
        {
            const auto arg = EvalFuncSingleArg(func, Arg::Type::Empty, ArgLimits::AtMost);
            GetFrame().SetReturn({});
            Runtime->HandleException(CREATE_EXCEPTION(NailangCodeException, arg.ToString().StrView(), func));
            return {};
        }
        default: break;
        }
    }
    auto params = EvalFuncAllArgs(func);
    FuncEvalPack pack(func, params, metas);
    return EvaluateFunc(pack);
}
Arg NailangExecutor::EvaluateFunc(FuncEvalPack& func)
{
    Expects(func.Name->PartCount > 0);
    if (func.Name->PartCount == 1)
    {
        switch (const auto name = func.NamePart(0); DJBHash::HashC(name))
        {
        HashCase(name, U"Exists")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            const LateBindVar var(func.Params[0].GetStr().value());
            return static_cast<bool>(Runtime->LocateArg(var, false));
        }
        HashCase(name, U"Format")
        {
            ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::Empty });
            return Runtime->FormatString(func.Params[0].GetStr().value(), func.Params.subspan(1));
        }
        HashCase(name, U"ParseInt")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            int64_t ret = 0;
            if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToInt(str, ret).first)
                NLRT_THROW_EX(FMTSTR(u"Arg of [ParseInt] is not integer : [{}]"sv, str), func);
            return ret;
        }
        HashCase(name, U"ParseUint")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            uint64_t ret = 0;
            if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToInt(str, ret).first)
                NLRT_THROW_EX(FMTSTR(u"Arg of [ParseUint] is not integer : [{}]"sv, str), func);
            return ret;
        }
        HashCase(name, U"ParseFloat")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            double ret = 0;
            if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, false).first)
                NLRT_THROW_EX(FMTSTR(u"Arg of [ParseFloat] is not floatpoint : [{}]"sv, str), func);
            return ret;
        }
        HashCase(name, U"ParseSciFloat")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            double ret = 0;
            if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, true).first)
                NLRT_THROW_EX(FMTSTR(u"Arg of [ParseFloat] is not scientific floatpoint : [{}]"sv, str), func);
            return ret;
        }
        default: break;
        }
    }
    else
    {
        if (func.NamePart(0) == U"Math"sv)
        {
            if (auto ret = EvaluateExtendMathFunc(func); !ret.IsEmpty())
            {
                return ret;
            }
        }
    }
    if (const auto lcFunc = Runtime->LookUpFunc(func.FullFuncName()); lcFunc)
    {
        return Runtime->OnLocalFunc(lcFunc, func);
    }
    return EvaluateUnknwonFunc(func);
}
Arg NailangExecutor::EvaluateExtendMathFunc(FuncEvalPack& func)
{
    using Type = Arg::Type;
    const auto mathName = func.NamePart(1);
    switch (DJBHash::HashC(mathName))
    {
    HashCase(mathName, U"Max")
    {
        ThrowByArgCount(func, 1, ArgLimits::AtLeast);
        size_t maxIdx = 0;
        for (size_t i = 1; i < func.Params.size(); ++i)
        {
            const auto& left = func.Params[maxIdx], &right = func.Params[i];
            const auto isLess = EmbedOpEval::Less(left, right);
            if (isLess.IsEmpty())
                NLRT_THROW_EX(FMTSTR(u"Cannot perform [less] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
            if (isLess.GetBool().value()) 
                maxIdx = i;
        }
        return func.Params[maxIdx];
    }
    HashCase(mathName, U"Min")
    {
        ThrowByArgCount(func, 1, ArgLimits::AtLeast);
        size_t minIdx = 0;
        for (size_t i = 1; i < func.Params.size(); ++i)
        {
            const auto& left = func.Params[i], & right = func.Params[minIdx];
            const auto isLess = EmbedOpEval::Less(left, right);
            if (isLess.IsEmpty())
                NLRT_THROW_EX(FMTSTR(u"Cannot perform [less] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
            if (isLess.GetBool().value())
                minIdx = i;
        }
        return func.Params[minIdx];
    }
    HashCase(mathName, U"Sqrt")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::sqrt(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::sqrt(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::sqrt(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Ceil")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::ceil(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Floor")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::floor(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Round")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::round(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::log(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log2")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::log2(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log2(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log2(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log10")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Number });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::FP:      return std::log10(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log10(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log10(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Lerp")
    {
        ThrowByParamTypes<3>(func, { Arg::Type::Number, Arg::Type::Number, Arg::Type::Number });
        const auto x = func.Params[0].GetFP();
        const auto y = func.Params[1].GetFP();
        const auto a = func.Params[2].GetFP();
        if (x && y && a)
#if defined(__cpp_lib_interpolate) && __cpp_lib_interpolate >= 201902L
            return std::lerp(x.value(), y.value(), a.value());
#else
            return x.value() * (1. - a.value()) + y.value() * a.value();
#endif
    } break;
    HashCase(mathName, U"Pow")
    {
        ThrowByParamTypes<2>(func, { Arg::Type::Number, Arg::Type::Number });
        const auto x = func.Params[0].GetFP();
        const auto y = func.Params[1].GetFP();
        if (x && y)
            return std::pow(x.value(), y.value());
    } break;
    HashCase(mathName, U"LeadZero")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Integer });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"TailZero")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Integer });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"PopCount")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::Integer });
        switch (const auto& arg = func.Params[0]; arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"ToUint")
    {
        ThrowByArgCount(func, 1);
        const auto arg = func.Params[0].GetUint();
        if (arg)
            return arg.value();
    } break;
    HashCase(mathName, U"ToInt")
    {
        ThrowByArgCount(func, 1);
        const auto arg = func.Params[0].GetInt();
        if (arg)
            return arg.value();
    } break;
    default: return {};
    }
    NLRT_THROW_EX(FMTSTR(u"MathFunc [{}] cannot be evaluated.", func.FullFuncName()), func);
    //return {};
}
Arg NailangExecutor::EvaluateUnknwonFunc(FuncEvalPack& func)
{
    NLRT_THROW_EX(FMTSTR(u"Func [{}] with [{}] args cannot be resolved.", func.FullFuncName(), func.Args.size()), func);
    //return {};
}

Arg NailangExecutor::EvaluateUnaryExpr(const UnaryExpr& expr)
{
    Expects(EmbedOpHelper::IsUnaryOp(expr.Operator));
    switch (expr.Operator)
    {
    case EmbedOps::CheckExist:
    {
        Ensures(expr.Operand.TypeData == Expr::Type::Var);
        return static_cast<bool>(Runtime->LocateArg(expr.Operand.GetVar<Expr::Type::Var>(), false));
    }
    case EmbedOps::Not:
    {
        auto val = EvaluateExpr(expr.Operand);
        auto ret = val.HandleUnary(expr.Operator);
        if (ret.IsEmpty())
            NLRT_THROW_EX(FMTSTR(u"Cannot perform unary expr [{}] on type [{}]"sv,
                EmbedOpHelper::GetOpName(expr.Operator), val.GetTypeName()), Expr(&expr));
        return ret;
    }
    default:
        NLRT_THROW_EX(FMTSTR(u"Unexpected unary op [{}]"sv, EmbedOpHelper::GetOpName(expr.Operator)), Expr(&expr));
        //return {};
    }
}

Arg NailangExecutor::EvaluateBinaryExpr(const BinaryExpr& expr)
{
    if (expr.Operator == EmbedOps::ValueOr)
    {
        Expects(expr.LeftOperand.TypeData == Expr::Type::Var);
        auto val = Runtime->LookUpArg(expr.LeftOperand.GetVar<Expr::Type::Var>(), false); // skip null check, but require read
        return val.IsEmpty() ? EvaluateExpr(expr.RightOperand) : val;
    }
    Arg left = EvaluateExpr(expr.LeftOperand), right;
    if (expr.Operator == EmbedOps::And)
    {
        if (const auto l = left.GetBool(); l.has_value())
        {
            if (!l.value()) return false;
            right = EvaluateExpr(expr.RightOperand);
            if (const auto r = right.GetBool(); r.has_value())
                return r.value();
        }
    }
    else if (expr.Operator == EmbedOps::Or)
    {
        if (const auto l = left.GetBool(); l.has_value())
        {
            if (l.value()) return true;
            right = EvaluateExpr(expr.RightOperand);
            if (const auto r = right.GetBool(); r.has_value())
                return r.value();
        }
    }
    if (right.IsEmpty())
        right = EvaluateExpr(expr.RightOperand);
    Arg ret = left.HandleBinary(expr.Operator, right);
    if (!ret.IsEmpty())
        return ret;
    NLRT_THROW_EX(FMTSTR(u"Cannot perform binary expr [{}] on type [{}],[{}]"sv,
        EmbedOpHelper::GetOpName(expr.Operator), left.GetTypeName(), right.GetTypeName()), Expr(&expr));
    //return {};
}

Arg NailangExecutor::EvaluateTernaryExpr(const TernaryExpr& expr)
{
    const auto cond = ThrowIfNotBool(EvaluateExpr(expr.Condition), U"condition of ternary operator"sv);
    const auto& selected = cond ? expr.LeftOperand : expr.RightOperand;
    return EvaluateExpr(selected);
}

Arg NailangExecutor::EvaluateQueryExpr(const QueryExpr& expr)
{
    Expects(expr.Size() > 0);
    auto target = EvaluateExpr(expr.Target);
    try
    {
        return NailangHelper::LocateRead(target, expr, *this, [](ArgLocator& ret) { return ret.ExtractGet(); });
    }
    catch (const NailangRuntimeException& nre)
    {
        Runtime->HandleException(nre);
        return {};
    }
}

void NailangExecutor::ExecuteFrame()
{
    auto& frame = GetFrame();
    frame.Executor = this;
    for (size_t idx = 0; idx < frame.BlockScope->Size();)
    {
        const auto& [metas, content] = (*frame.BlockScope)[idx];
        bool shouldExe = true;
        if (!metas.empty())
        {
            MetaSet allMetas(metas);
            shouldExe = HandleMetaFuncs(allMetas, content);
        }
        if (shouldExe)
        {
            HandleContent(content, metas);
        }
        frame.IfRecord >>= 2;
        if (frame.Has(NailangFrame::ProgramStatus::End))
            return;
        else
            idx++;
    }
    return;
}
void NailangExecutor::HandleContent(const Statement& content, common::span<const FuncCall> metas)
{
    auto& frame = GetFrame();
    frame.CurContent = &content;
    switch (content.TypeData)
    {
    case Statement::Type::Assign:
    {
        const auto& assign = *content.Get<AssignExpr>();
        Runtime->SetArg(assign.Target, assign.Queries, assign.Statement, assign.Check);
    } break;
    case Statement::Type::Block:
    {
        Runtime->OnBlock(*content.Get<Block>(), metas);
    } break;
    case Statement::Type::FuncCall:
    {
        const auto fcall = content.Get<FuncCall>();
        Expects(fcall->Name->Info() == FuncName::FuncInfo::Empty);
        //ExprHolder callHost(this, fcall);
        [[maybe_unused]] const auto dummy = EvaluateFunc(*fcall, metas);
    } break;
    case Statement::Type::RawBlock:
    {
        Runtime->OnRawBlock(*content.Get<RawBlock>(), metas);
    } break;
    default:
        assert(false); /*Expects(false);*/
        break;
    }
}

NailangExecutor::NailangExecutor(NailangRuntimeBase* runtime) noexcept : Runtime(runtime)
{ }
NailangExecutor::~NailangExecutor() 
{ }
void NailangExecutor::HandleException(const NailangRuntimeException& ex) const
{
    Runtime->HandleException(ex);
}


NailangRuntimeBase::FrameHolder::FrameHolder(NailangRuntimeBase* host, const std::shared_ptr<EvaluateContext>& ctx, const NailangFrame::FrameFlags flags) :
    Host(*host), Frame(host->CurFrame, ctx, flags)
{
    Expects(ctx);
    Host.CurFrame = &Frame;
}
NailangRuntimeBase::FrameHolder::~FrameHolder()
{
    Expects(Host.CurFrame == &Frame);
    Host.CurFrame = Frame.PrevFrame;
}


NailangRuntimeBase::NailangRuntimeBase(std::shared_ptr<EvaluateContext> context) : 
    RootContext(std::move(context)), BasicExecutor(std::make_unique<NailangExecutor>(this))
{ }
NailangRuntimeBase::~NailangRuntimeBase()
{ }


NailangRuntimeBase::FrameHolder NailangRuntimeBase::PushFrame(std::shared_ptr<EvaluateContext> ctx, const NailangFrame::FrameFlags flags)
{
    return FrameHolder(this, std::move(ctx), flags);
}

static common::StackTraceItem CreateStack(const AssignExpr* assign, const common::SharedString<char16_t>& fname) noexcept
{
    if (!assign)
        return { fname, u"<empty assign>"sv, 0 };
    std::u32string target = U"<Assign>";
    Serializer::Stringify(target, assign->Target);
    Serializer::Stringify(target, assign->Queries);
    return { fname, common::str::to_u16string(target), assign->Position.first };
}
static common::StackTraceItem CreateStack(const FuncCall* fcall, const common::SharedString<char16_t>& fname) noexcept
{
    if (!fcall)
        return { fname, u"<empty func>"sv, 0 };
    return { fname, common::str::to_u16string(fcall->FullFuncName()), fcall->Position.first };
}
static common::StackTraceItem CreateStack(const RawBlock* block, const common::SharedString<char16_t>& fname) noexcept
{
    if (!block)
        return { fname, u"<empty raw block>"sv, 0 };
    return { fname, FMTSTR(u"<Raw>{}"sv, block->Name), block->Position.first };
}
static common::StackTraceItem CreateStack(const Block* block, const common::SharedString<char16_t>& fname) noexcept
{
    if (!block)
        return { fname, u"<empty block>"sv, 0 };
    return { fname, FMTSTR(u"<Block>{}"sv, block->Name), block->Position.first };
}

void NailangRuntimeBase::HandleException(const NailangRuntimeException& ex) const
{
    if (auto& info = ex.GetInfo(); !info.Scope && CurFrame && CurFrame->CurContent)
        info.Scope = *CurFrame->CurContent;

    auto GetFileName = [nameCache = std::vector<std::pair<const std::u16string_view, common::SharedString<char16_t>>>()]
        (const Block* blk) mutable -> common::SharedString<char16_t>
    {
        if (!blk) return {};
        for (const auto& [name, str] : nameCache)
            if (name == blk->FileName)
                return str;
        common::SharedString<char16_t> str(blk->FileName);
        nameCache.emplace_back(blk->FileName, str);
        return str;
    };
    std::vector<common::StackTraceItem> traces;
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        auto fname = GetFileName(frame->BlockScope);
        const FuncCall* lastFunc = nullptr;
        for (auto finfo = frame->FuncInfo; finfo; finfo = finfo->PrevInfo)
        {
            traces.emplace_back(CreateStack(finfo->Func, fname));
            lastFunc = finfo->Func;
        }
        if (frame->CurContent && (*frame->CurContent) != lastFunc)
        {
            traces.emplace_back(frame->CurContent->Visit([&](const auto* ptr) { return CreateStack(ptr, fname); }));
        }
    }
    ex.Info->StackTrace.insert(ex.Info->StackTrace.begin(), traces.begin(), traces.end());
    throw ex;
}

std::u32string NailangRuntimeBase::FormatString(const std::u32string_view formatter, common::span<const Arg> args)
{
    fmt::dynamic_format_arg_store<fmt::u32format_context> store;
    for (const auto& arg : args)
    {
        switch (arg.TypeData)
        {
        case Arg::Type::Bool:   store.push_back(arg.GetVar<Arg::Type::Bool >()); break;
        case Arg::Type::Uint:   store.push_back(arg.GetVar<Arg::Type::Uint >()); break;
        case Arg::Type::Int:    store.push_back(arg.GetVar<Arg::Type::Int  >()); break;
        case Arg::Type::FP:     store.push_back(arg.GetVar<Arg::Type::FP   >()); break;
        case Arg::Type::U32Sv:  store.push_back(arg.GetVar<Arg::Type::U32Sv>()); break;
        case Arg::Type::U32Str: store.push_back(std::u32string(arg.GetVar<Arg::Type::U32Str>())); break;
        default: HandleException(CREATE_EXCEPTION(NailangFormatException, formatter, arg, u"Unsupported DataType")); break;
        }
    }
    try
    {
        return fmt::vformat(formatter, store);
    }
    catch (const fmt::format_error& err)
    {
        HandleException(CREATE_EXCEPTION(NailangFormatException, formatter, err));
        return {};
    }
}

TempFuncName NailangRuntimeBase::CreateTempFuncName(std::u32string_view name, FuncName::FuncInfo info) const
{
    try
    {
        return FuncName::CreateTemp(name, info);
    }
    catch (const NailangPartedNameException&)
    {
        HandleException(CREATE_EXCEPTION(NailangRuntimeException, u"FuncCall's name not valid"sv));
    }
    return FuncName::CreateTemp(name, info);
}
LateBindVar NailangRuntimeBase::DecideDynamicVar(const Expr& arg, const std::u16string_view reciever) const
{
    switch (arg.TypeData)
    {
    case Expr::Type::Var: 
        return arg.GetVar<Expr::Type::Var>();
    case Expr::Type::Str: 
    {
        const auto name = arg.GetVar<Expr::Type::Str>();
        const auto res  = tokenizer::VariableTokenizer::CheckName(name);
        if (res.has_value())
            NLRT_THROW_EX(FMTSTR(u"LateBindVar's name not valid at [{}] with len[{}]"sv, res.value(), name.size()), 
                arg);
        return name;
    }
    default:
        NLRT_THROW_EX(FMTSTR(u"[{}] only accept [Var]/[String], which gives [{}].", reciever, arg.GetTypeName()),
            arg);
        break;
    }
    Expects(false);
    return U""sv;
}

ArgLocator NailangRuntimeBase::LocateArg(const LateBindVar& var, const bool create) const
{
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Root))
    {
        return RootContext->LocateArg(var, create);
    }
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Local))
    {
        if (CurFrame)
            return CurFrame->Context->LocateArg(var, create);
        else
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] without frame", var));
        return {};
    }

    // Try find arg recursively
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        if (auto ret = frame->Context->LocateArg(var, create); ret)
            return std::move(ret);
        if (frame->Has(NailangFrame::FrameFlags::CallScope))
            break; // cannot beyond  
    }
    return RootContext->LocateArg(var, create);
}
ArgLocator NailangRuntimeBase::LocateArgForWrite(const LateBindVar& var, NilCheck nilCheck, std::variant<bool, EmbedOps> extra) const
{
    using Behavior = NilCheck::Behavior;
    auto ret = LocateArg(var, false);
    if (ret)
    {
        switch (nilCheck.WhenNotNull())
        {
        case Behavior::Skip:
            return {};
        case Behavior::Throw:
            NLRT_THROW_EX(FMTSTR(u"Var [{}] already exists"sv, var));
            return {};
        default:
            return ret;
        }
    }
    else
    {
        switch (nilCheck.WhenNull())
        {
        case Behavior::Skip:
            return {};
        case Behavior::Throw:
            if (extra.index() == 1)
                NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exists, expect perform [{}] on it"sv, 
                    var, EmbedOpHelper::GetOpName(std::get<1>(extra))));
            else
                NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exists{}"sv, 
                    var, std::get<0>(extra) ? u""sv : u", expect perform subquery on it"sv));
            return {};
        default:
            Expects(extra.index() == 1 || std::get<0>(extra) == false);
            return LocateArg(var, true);
        }
    }
}
bool NailangRuntimeBase::SetArg(const LateBindVar& var, SubQuery subq, Arg arg, NilCheck nilCheck)
{
    auto target = LocateArgForWrite(var, nilCheck, subq.Size() != 0);
    return NailangHelper::LocateWrite(target, subq, *CurFrame->Executor, [&](ArgLocator& dst)
        {
            if (!dst.CanAssign())
                NLRT_THROW_EX(FMTSTR(u"Var [{}] is not assignable"sv, var));
            return dst.Set(std::move(arg));
        });
}
bool NailangRuntimeBase::SetArg(const LateBindVar& var, SubQuery subq, const Expr& expr, NilCheck nilCheck)
{
    std::variant<bool, EmbedOps> extra = false;
    if (subq.Size() > 0)
        extra = true;
    else if (expr.TypeData == Expr::Type::Binary)
        extra = expr.GetVar<Expr::Type::Binary>()->Operator;
    auto target = LocateArgForWrite(var, nilCheck, extra);
    return NailangHelper::LocateWrite(target, subq, *CurFrame->Executor, [&](ArgLocator& dst)
        {
            if (!dst.CanAssign())
                NLRT_THROW_EX(FMTSTR(u"Var [{}] is not assignable"sv, var));
            return dst.Set(CurFrame->Executor->EvaluateExpr(expr));
        });
}
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, capture, args);
    return false;
}
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, capture, args);
    return false;
}

std::shared_ptr<EvaluateContext> NailangRuntimeBase::ConstructEvalContext() const
{
    return std::make_shared<CompactEvaluateContext>();
}

Arg NailangRuntimeBase::LookUpArg(const LateBindVar& var, const bool checkNull) const
{
    auto ret = LocateArg(var, false);
    if (!ret)
    {
        if (checkNull)
            NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exist", var));
        return {};
    }
    if (!ret.CanRead())
        NLRT_THROW_EX(FMTSTR(u"LookUpArg [{}] get a unreadable result", var));
    return ret.ExtractGet();
}

LocalFunc NailangRuntimeBase::LookUpFunc(std::u32string_view name) const
{
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        auto func = frame->Context->LookUpFunc(name);
        if (func.Body != nullptr)
            return func;
    }
    return RootContext->LookUpFunc(name);
}

void NailangRuntimeBase::OnRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}

void NailangRuntimeBase::OnBlock(const Block& block, common::span<const FuncCall> metas)
{
    const auto prevFrame = PushFrame();
    CurFrame->BlockScope = &block;
    CurFrame->MetaScope = metas;
    prevFrame->Executor->ExecuteFrame();
}

Arg NailangRuntimeBase::OnLocalFunc(const LocalFunc& func, FuncEvalPack& pack)
{
    ThrowByArgCount(pack, func.ArgNames.size() - func.CapturedArgs.size());
    auto newCtx = ConstructEvalContext();
    size_t i = 0;
    for (; i < func.CapturedArgs.size(); ++i)
        newCtx->LocateArg(func.ArgNames[i], true).Set(func.CapturedArgs[i]);
    for (size_t j = 0; i < func.ArgNames.size(); ++i, ++j)
        newCtx->LocateArg(func.ArgNames[i], true).Set(std::move(pack.Params[j]));
    auto prevFrame = PushFrame(std::move(newCtx), NailangFrame::FrameFlags::FuncCall);
    CurFrame->BlockScope = func.Body;
    CurFrame->MetaScope = pack.Metas;
    prevFrame->Executor->ExecuteFrame();
    return std::move(CurFrame->ReturnArg);
}


void NailangRuntimeBase::ExecuteBlock(const Block& block, common::span<const FuncCall> metas, std::shared_ptr<EvaluateContext> ctx, const bool checkMetas)
{
    auto frame = PushFrame(ctx, NailangFrame::FrameFlags::FlowScope);
    CurFrame->MetaScope = metas;
    CurFrame->BlockScope = &block;
    if (checkMetas)
    {
        Statement target(&block);
        MetaSet allMetas(metas);
        if (!BasicExecutor->HandleMetaFuncs(allMetas, target))
            return;
    }
    BasicExecutor->ExecuteFrame();
}

Arg NailangRuntimeBase::EvaluateRawStatement(std::u32string_view content, const bool innerScope)
{
    common::parser::ParserContext context(content);
    const auto rawarg = NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
    if (rawarg)
    {
        auto frame = PushFrame(innerScope, NailangFrame::FrameFlags::FlowScope);
        return BasicExecutor->EvaluateExpr(rawarg);
    }
    else
        return {};
}

Arg NailangRuntimeBase::EvaluateRawStatements(std::u32string_view content, const bool innerScope)
{
    struct EmbedBlkParser : public NailangParser
    {
        using NailangParser::NailangParser;
        static Block GetBlock(MemoryPool& pool, const std::u32string_view src)
        {
            common::parser::ParserContext context(src);
            EmbedBlkParser parser(pool, context);
            Block ret;
            parser.ParseContentIntoBlock(true, ret);
            return ret;
        }
    };

    common::parser::ParserContext context(content);
    const auto blk = EmbedBlkParser::GetBlock(MemPool, content);

    auto frame = PushFrame(innerScope, NailangFrame::FrameFlags::FlowScope);
    CurFrame->BlockScope = &blk;
    BasicExecutor->ExecuteFrame();
    return std::move(CurFrame->ReturnArg);
}



}
