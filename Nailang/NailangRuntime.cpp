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


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))

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


ArgLocator Arg::HandleQuery(SubQuery subq, NailangRuntimeBase& runtime)
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


Arg CustomVar::Handler::EvaluateExpr(NailangRuntimeBase& runtime, const Expr& arg)
{
    return runtime.EvaluateExpr(arg);
}

void CustomVar::Handler::HandleException(NailangRuntimeBase& runtime, const NailangRuntimeException& ex)
{
    runtime.HandleException(ex);
}

ArgLocator CustomVar::Handler::HandleQuery(CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime)
{
    Expects(subq.Size() > 0);
    const auto [type, query] = subq[0];
    switch (type)
    {
    case SubQuery::QueryType::Index:
    {
        const auto idx = EvaluateExpr(runtime, query);
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


ArgLocator AutoVarHandlerBase::HandleQuery(CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime)
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
            const auto idx  = EvaluateExpr(runtime, query);
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
            const auto idx = EvaluateExpr(runtime, query);
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

LocalFunc BasicEvaluateContext::LookUpFunc(std::u32string_view name) const
{
    const auto [ptr, offset, size] = LookUpFuncInside(name);
    return { ptr, common::to_span(LocalFuncArgNames).subspan(offset, size) };
}

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<const Expr> args)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(args.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
    for (const auto& arg : args)
        LocalFuncArgNames.emplace_back(arg.GetVar<Expr::Type::Var>());
    return SetFuncInside(block->Name, { block, offset, size });
}

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<const std::u32string_view> args)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(args.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
    for (const auto& name : args)
        LocalFuncArgNames.emplace_back(name);
    return SetFuncInside(block->Name, { block, offset, size });
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
        return { nullptr, 0, 0 };
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
    return { nullptr, 0, 0 };
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


constexpr bool NailangRuntimeBase::BlockFrame::Has(FrameFlags flag) const noexcept
{ 
    return HAS_FIELD(Flags, flag);
}
constexpr bool NailangRuntimeBase::BlockFrame::IsInLoop() const noexcept
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
constexpr NailangRuntimeBase::BlockFrame* NailangRuntimeBase::BlockFrame::GetCallScope() noexcept
{
    for (auto frame = this; frame; frame = frame->PrevFrame)
    {
        if (HAS_FIELD(frame->Flags, FrameFlags::CallScope))
            return frame;
    }
    return nullptr;
}


NailangRuntimeBase::FrameHolder::FrameHolder(NailangRuntimeBase* host, const std::shared_ptr<EvaluateContext>& ctx, const FrameFlags flags) :
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


class NailangRuntimeBase::ExprHolder
{
    friend class NailangRuntimeBase;
    NailangRuntimeBase& Host;
    Expr PrevExpr;
    constexpr ExprHolder(NailangRuntimeBase* host, const Expr& expr) noexcept :
        Host(*host), PrevExpr(Host.CurFrame->CurExpr)
    {
        Host.CurFrame->CurExpr = expr;
    }
    COMMON_NO_COPY(ExprHolder)
    COMMON_NO_MOVE(ExprHolder)
public:
    ~ExprHolder()
    {
        Expects(Host.CurFrame);
        Host.CurFrame->CurExpr = PrevExpr;
    }
};


struct ArgQuery : private ArgLocator
{
    template<typename F>
    static bool LocateWrite(ArgLocator& target, SubQuery query, NailangRuntimeBase& runtime, const F& func)
    {
        auto next = target.HandleQuery(query, runtime);
        if (query.Size() > 1)
        {
            auto subq = query.Sub(1);
            if (!next)
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                    Serializer::Stringify(subq)));
            if (!next.IsMutable())
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not assgin to immutable's [{}]",
                    Serializer::Stringify(subq)));
            return LocateWrite(next, subq, runtime, func);
        }
        else
        {
            return func(std::move(next));
        }
    }
    template<typename T, typename F>
    static Arg LocateRead(T& target, SubQuery query, NailangRuntimeBase& runtime, const F& func)
    {
        auto next = target.HandleQuery(query, runtime);
        if (query.Size() > 1)
        {
            auto subq = query.Sub(1);
            if (!next)
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                    Serializer::Stringify(subq)));
            if (!next.CanRead())
                COMMON_THROW(NailangRuntimeException, FMTSTR(u"Can not access an set-host's [{}]",
                    Serializer::Stringify(subq)));
            return LocateRead(next, subq, runtime, func);
        }
        else
        {
            return func(std::move(next));
        }
    }
    template<bool IsWrite, typename F>
    static auto LocateAndExecute(ArgLocator& target, SubQuery query, NailangRuntimeBase& runtime, const F& func) 
        -> decltype(func(std::declval<ArgLocator>()))
    {
        auto next = target.HandleQuery(query, runtime);
        if (query.Size() > 1)
        {
            auto subq = query.Sub(1);
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
            return LocateAndExecute<IsWrite>(next, subq, runtime, func);
        }
        else
        {
            return func(std::move(next));
        }
    }
};


NailangRuntimeBase::NailangRuntimeBase(std::shared_ptr<EvaluateContext> context) : 
    RootContext(std::move(context))
{ }
NailangRuntimeBase::~NailangRuntimeBase()
{ }


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

void NailangRuntimeBase::ThrowByArgCount(const FuncCall& call, const size_t count, const ArgLimits limit) const
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
        detail::ExceptionTarget{}, &call);
}

void NailangRuntimeBase::ThrowByArgType(const FuncCall& call, const Expr::Type type, size_t idx) const
{
    const auto& arg = call.Args[idx];
    if (arg.TypeData != type)
        NLRT_THROW_EX(FMTSTR(u"Expected [{}] for [{}]'s {} rawarg, which gives [{}], source is [{}].",
            Expr::TypeName(type), call.FullFuncName(), idx, arg.GetTypeName(), Serializer::Stringify(arg)),
            arg);
}

void NailangRuntimeBase::ThrowByArgType(const Arg& arg, const Arg::Type type) const
{
    if ((arg.TypeData & type) != type)
        NLRT_THROW_EX(FMTSTR(u"Expected arg of [{}], which gives [{}].", Arg::TypeName(type), arg.GetTypeName()),
            arg);
}

void NailangRuntimeBase::ThrowByArgType(const FuncCall& call, const Arg& arg, const Arg::Type type, size_t idx) const
{
    if ((arg.TypeData & type) != type)
        NLRT_THROW_EX(FMTSTR(u"Expected [{}] for [{}]'s {} arg, which gives [{}], source is [{}].", 
                Arg::TypeName(type), call.FullFuncName(), idx, arg.GetTypeName(), Serializer::Stringify(call.Args[idx])),
            arg);
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

void NailangRuntimeBase::ThrowIfNotFuncTarget(const FuncCall& call, const FuncName::FuncInfo type) const
{
    if (call.Name->Info() != type)
    {
        NLRT_THROW_EX(FMTSTR(u"Func [{}] only support as [{}], get[{}].", call.FullFuncName(), FuncTypeName(type), FuncTypeName(call.Name->Info())),
            call);
    }
}

void NailangRuntimeBase::ThrowIfStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const
{
    if (target.TypeData == type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] cannot be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}

void NailangRuntimeBase::ThrowIfNotStatement(const FuncCall& meta, const Statement target, const Statement::Type type) const
{
    if (target.TypeData != type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] can only be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}

bool NailangRuntimeBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        NLRT_THROW_EX(FMTSTR(u"[{}] should be bool-able, which is [{}].", varName, arg.GetTypeName()),
            arg);
    return ret.value();
}

void NailangRuntimeBase::CheckFuncArgs(common::span<const Expr::Type> types, const ArgLimits limit, const FuncCall& call)
{
    ThrowByArgCount(call, types.size(), limit);
    const auto size = std::min(types.size(), call.Args.size());
    for (size_t i = 0; i < size; ++i)
        ThrowByArgType(call, types[i], i);
}

void NailangRuntimeBase::EvaluateFuncArgs(Arg* args, const Arg::Type* types, const size_t size, const FuncCall& call)
{
    for (size_t i = 0; i < size; ++i)
    {
        args[i] = EvaluateExpr(call.Args[i]);
        if (types != nullptr)
            ThrowByArgType(call, args[i], types[i], i);
    }
}

NailangRuntimeBase::FrameHolder NailangRuntimeBase::PushFrame(std::shared_ptr<EvaluateContext> ctx, const FrameFlags flags)
{
    return FrameHolder(this, std::move(ctx), flags);
}

void NailangRuntimeBase::ExecuteFrame()
{
    for (size_t idx = 0; idx < CurFrame->BlockScope->Size();)
    {
        const auto& [metas, content] = (*CurFrame->BlockScope)[idx];
        if (HandleMetaFuncs(metas, content))
        {
            HandleContent(content, metas);
        }
        switch (CurFrame->Status)
        {
        case ProgramStatus::Return:
        case ProgramStatus::End:
        case ProgramStatus::Break:
            return;
        case ProgramStatus::Next:
            idx++; break;
        }
    }
    return;
}

std::variant<std::monostate, bool, xziar::nailang::TempFuncName> NailangRuntimeBase::HandleMetaIf(const FuncCall& meta)
{
    if (meta.GetName() == U"MetaIf"sv)
    {
        const auto args = EvaluateFuncArgs<2, ArgLimits::AtLeast>(meta, { Arg::Type::Boolable, Arg::Type::String });
        if (args[0].GetBool().value())
        {
            const auto name = args[1].GetStr().value();
            return CreateTempFuncName(name, FuncName::FuncInfo::Meta);
        }
        return false;
    }
    return {};
}

bool NailangRuntimeBase::HandleMetaFuncs(common::span<const FuncCall> metas, const Statement& target)
{
    for (const auto& meta : metas)
    {
        Expects(meta.Name->Info() == FuncName::FuncInfo::Meta);
        ExprHolder callHost(this, &meta);
        auto result = MetaFuncResult::Unhandled;
        switch (const auto newMeta = HandleMetaIf(meta); newMeta.index())
        {
        case 0: 
            result = HandleMetaFunc(meta, target, metas); break;
        case 2:
            result = HandleMetaFunc({ std::get<2>(newMeta).Ptr(), meta.Args.subspan(2) }, target, metas); break;
        default:
            break;
        }
        switch (result)
        {
        case MetaFuncResult::Return:
            CurFrame->Status = ProgramStatus::Return; return false;
        case MetaFuncResult::Skip:
            CurFrame->Status = ProgramStatus::Next;   return false;
        case MetaFuncResult::Next:
        default:
            CurFrame->Status = ProgramStatus::Next;   break;
        }
    }
    return true;
}

void NailangRuntimeBase::HandleContent(const Statement& content, common::span<const FuncCall> metas)
{
    CurFrame->CurContent = &content;
    switch (content.TypeData)
    {
    case Statement::Type::Assign:
    {
        const auto& assign = *content.Get<AssignExpr>();
        SetArg(assign.Target, assign.Queries, assign.Statement, assign.Check);
    } break;
    case Statement::Type::Block:
    {
        const auto prevFrame = PushFrame();
        CurFrame->BlockScope = content.Get<Block>();
        CurFrame->MetaScope = metas;
        ExecuteFrame();
        switch (CurFrame->Status)
        {
        case ProgramStatus::Return:
            prevFrame->Status = ProgramStatus::Return;
            break;
        case ProgramStatus::Break:
            prevFrame->Status = ProgramStatus::Break;
            break;
        default:
            break;
        }
    } break;
    case Statement::Type::FuncCall:
    {
        const auto fcall = content.Get<FuncCall>();
        Expects(fcall->Name->Info() == FuncName::FuncInfo::Empty);
        ExprHolder callHost(this, fcall);
        EvaluateFunc(*fcall, metas);
    } break;
    case Statement::Type::RawBlock:
    {
        OnRawBlock(*content.Get<RawBlock>(), metas);
    } break;
    default:
        assert(false); /*Expects(false);*/
        break;
    }
}

void NailangRuntimeBase::OnLoop(const Expr& condition, const Statement& target, common::span<const FuncCall> metas)
{
    const auto prevFrame = PushFrame(FrameFlags::InLoop);
    while (true)
    {
        const auto cond = EvaluateExpr(condition);
        if (ThrowIfNotBool(cond, U"Arg of MetaFunc[While]"))
        {
            HandleContent(target, metas);
            switch (CurFrame->Status)
            {
            case ProgramStatus::Return:
                prevFrame->Status = ProgramStatus::Return;
                return;
            case ProgramStatus::Break:
                return;
            default:
                continue;
            }
        }
        else
            return;
    }
}

std::u32string NailangRuntimeBase::FormatString(const std::u32string_view formatter, common::span<const Expr> args)
{
    std::vector<Arg> results;
    results.reserve(args.size());
    for (const auto& arg : args)
        results.emplace_back(EvaluateExpr(arg));
    return FormatString(formatter, results);
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
FuncName* NailangRuntimeBase::CreateFuncName(std::u32string_view name, FuncName::FuncInfo info)
{
    try
    {
        return FuncName::Create(MemPool, name, info);
    }
    catch (const NailangPartedNameException&)
    {
        HandleException(CREATE_EXCEPTION(NailangRuntimeException, u"FuncCall's name not valid"sv));
    }
    return nullptr;
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
        if (frame->Has(FrameFlags::CallScope))
            break; // cannot beyond  
    }
    return RootContext->LocateArg(var, create);
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
    if (auto& info = ex.GetInfo(); !info.Scope && CurFrame && CurFrame->CurExpr)
        info.Scope = CurFrame->CurExpr;

    auto GetFileName = [nameCache = std::vector<std::pair<const std::u16string_view, common::SharedString<char16_t>>>()]
        (const Block* blk) mutable -> common::SharedString<char16_t>
    {
        if (!blk) return {};
        for (const auto& [name, str] : nameCache)
            if (name == blk->FileName)
                return str;
        return blk->FileName;
    };
    std::vector<common::StackTraceItem> traces;
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        auto fname = GetFileName(frame->BlockScope);
        /*for (const auto expr = frame->CurExpr; call; call = call->PrevFrame)
        {
            traces.emplace_back(CreateStack(call->Func, fname));
        }*/
        if (frame->CurContent)
            traces.emplace_back(frame->CurContent->Visit([&](const auto* ptr) { return CreateStack(ptr, fname); }));
    }
    ex.Info->StackTrace.insert(ex.Info->StackTrace.begin(), traces.begin(), traces.end());
    throw ex;
}

std::shared_ptr<EvaluateContext> NailangRuntimeBase::ConstructEvalContext() const
{
    return std::make_shared<CompactEvaluateContext>();
}

Arg NailangRuntimeBase::LookUpArg(const LateBindVar& var) const
{
    auto ret = LocateArg(var, false);
    if (!ret)
        return {};
    if (!ret.CanRead())
        NLRT_THROW_EX(FMTSTR(u"LookUpArg [{}] get a unreadable result", var));
    return ret.ExtractGet();
}
bool NailangRuntimeBase::SetArg(const LateBindVar& var, SubQuery subq, std::variant<Arg, Expr> arg, NilCheck nilCheck)
{
    using Behavior = NilCheck::Behavior;
    const auto DirectSet = [&](auto&& target)
    {
        if (!target.CanAssign())
            NLRT_THROW_EX(FMTSTR(u"Var [{}] is not assignable"sv, var));
        if (arg.index() == 0)
            return target.Set(std::get<0>(std::move(arg)));
        else
            return target.Set(EvaluateExpr(std::get<1>(arg)));
    };
    auto ret = LocateArg(var, false);
    if (ret)
    {
        switch (nilCheck.WhenNotNull())
        {
        case Behavior::Skip:
            return false;
        case Behavior::Throw:
        {
            NLRT_THROW_EX(FMTSTR(u"Var [{}] already exists"sv, var), arg);
            return false;
        }
        default:
            if (subq.Size() == 0)
                return DirectSet(ret);
            else
                return ArgQuery::LocateWrite(ret, subq, *this, DirectSet);
        }
    }
    else
    {
        switch (nilCheck.WhenNull())
        {
        case Behavior::Skip:
            return false;
        case Behavior::Throw:
        {
            std::u16string msg;
            if (subq.Size() > 0)
            {
                msg = u", expect perform subquery on it"sv;
            }
            else if (arg.index() == 1 && std::get<1>(arg).TypeData == Expr::Type::Binary)
            {
                const auto op = std::get<1>(arg).GetVar<Expr::Type::Binary>()->Operator;
                msg = FMTSTR(u", expect perform [{}] on it"sv, EmbedOpHelper::GetOpName(op));
            }
            NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exists{}"sv, var, msg), arg);
            return false;
        }
        default:
            Expects(subq.Size() == 0);
            return DirectSet(LocateArg(var, true));
        }
    }
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
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<const Expr> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, args);
    return false;
}
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<const std::u32string_view> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, args);
    return false;
}

NailangRuntimeBase::MetaFuncResult NailangRuntimeBase::HandleMetaFunc(const FuncCall& meta, const Statement& content, common::span<const FuncCall> allMetas)
{
    if (meta.Name->PartCount > 1)
        return MetaFuncResult::Unhandled;
    const auto metaName = meta.FullFuncName();
    switch (DJBHash::HashC(metaName))
    {
    HashCase(metaName, U"Skip")
    {
        const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::Boolable, ArgLimits::AtMost);
        return arg.IsEmpty() || arg.GetBool().value() ? MetaFuncResult::Skip : MetaFuncResult::Next;
    }
    HashCase(metaName, U"If")
    {
        const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::Boolable);
        return arg.GetBool().value() ? MetaFuncResult::Next : MetaFuncResult::Skip;
    }
    HashCase(metaName, U"DefFunc")
    {
        ThrowIfNotStatement(meta, content, Statement::Type::Block);
        for (const auto& arg : meta.Args)
        {
            if (arg.TypeData != Expr::Type::Var)
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv, arg, &meta);
            const auto& var = arg.GetVar<Expr::Type::Var>();
            if (HAS_FIELD(var.Info, LateBindVar::VarInfo::PrefixMask))
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg name must not contain [Root|Local] flag"sv, arg, &meta);
        }
        SetFunc(content.Get<Block>(), meta.Args);
        return MetaFuncResult::Skip;
    }
    HashCase(metaName, U"While")
    {
        ThrowIfStatement(meta, content, Statement::Type::RawBlock);
        ThrowByArgCount(meta, 1);
        OnLoop(meta.Args[0], content, allMetas);
        return MetaFuncResult::Skip;
    }
    }
    return MetaFuncResult::Unhandled;
}

Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas)
{
    const auto fullName = call.FullFuncName();
    // only for plain function
    if (call.Name->Info() == FuncName::FuncInfo::Empty && call.Name->PartCount == 1)
    {
        if (fullName == U"Break"sv)
        {
            ThrowByArgCount(call, 0);
            if (!CurFrame->IsInLoop())
                NLRT_THROW_EX(u"[Break] can only be used inside While"sv, call);
            CurFrame->Status = ProgramStatus::Break;
            return {};
        }
        else if (fullName == U"Return"sv)
        {
            if (const auto scope = CurFrame->GetCallScope(); scope)
                scope->ReturnArg = EvaluateFirstFuncArg(call, ArgLimits::AtMost);
            else
                NLRT_THROW_EX(u"[Return] can only be used inside FlowScope"sv, call);
            CurFrame->Status = ProgramStatus::Return;
            return {};
        }
        else if (fullName == U"Throw"sv)
        {
            const auto arg = EvaluateFirstFuncArg(call);
            this->HandleException(CREATE_EXCEPTION(NailangCodeException, arg.ToString().StrView(), call));
            CurFrame->Status = ProgramStatus::Return;
            return {};
        }
    }
    // suitable for all
    if (call.Name->PartCount == 2 && call.Name->GetPart(0) == U"Math"sv)
    {
        if (auto ret = EvaluateExtendMathFunc(call, metas); !ret.IsEmpty())
        {
            return ret;
        }
    }
    if (call.Name->PartCount == 1)
    {
        switch (DJBHash::HashC(fullName))
        {
        HashCase(fullName, U"Select")
        {
            ThrowByArgCount(call, 3);
            const auto& selected = ThrowIfNotBool(EvaluateExpr(call.Args[0]), U""sv) ? call.Args[1] : call.Args[2];
            return EvaluateExpr(selected);
        }
        HashCase(fullName, U"Exists")
        {
            ThrowByArgCount(call, 1);
            const auto var = DecideDynamicVar(call.Args[0], u"Exists"sv);
            return !LookUpArg(var).IsEmpty();
        }
        HashCase(fullName, U"ExistsDynamic")
        {
            const auto arg = EvaluateFirstFuncArg(call, Arg::Type::String);
            const LateBindVar var(arg.GetStr().value());
            return !LookUpArg(var).IsEmpty();
        }
        HashCase(fullName, U"ValueOr")
        {
            ThrowByArgCount(call, 2);
            const auto var = DecideDynamicVar(call.Args[0], u"ValueOr"sv);
            if (auto ret = LookUpArg(var); !ret.IsEmpty())
                return ret;
            else
                return EvaluateExpr(call.Args[1]);
        }
        HashCase(fullName, U"Format")
        {
            ThrowByArgCount(call, 2, ArgLimits::AtLeast);
            const auto fmtStr = EvaluateExpr(call.Args[0]).GetStr();
            if (!fmtStr)
                NLRT_THROW_EX(u"Arg[0] of [Format] should be string"sv, call);
            return FormatString(fmtStr.value(), call.Args.subspan(1));
        }
        HashCase(fullName, U"ParseInt")
        {
            const auto arg = EvaluateFirstFuncArg(call, Arg::Type::String);
            const auto str = common::str::to_string(arg.GetStr().value());
            int32_t ret = 0;
            if (!common::StrToInt(str, ret).first)
                NLRT_THROW_EX(u"Arg of [ParseInt] is not integer"sv, call);
            return static_cast<int64_t>(ret);
        }
        HashCase(fullName, U"ParseUint")
        {
            const auto arg = EvaluateFirstFuncArg(call, Arg::Type::String);
            const auto str = common::str::to_string(arg.GetStr().value());
            uint32_t ret = 0;
            if (!common::StrToInt(str, ret).first)
                NLRT_THROW_EX(u"Arg of [ParseUint] is not unsigned integer"sv, call);
            return static_cast<uint64_t>(ret);
        }
        HashCase(fullName, U"ParseFloat")
        {
            const auto arg = EvaluateFirstFuncArg(call, Arg::Type::String);
            const auto str = common::str::to_string(arg.GetStr().value());
            double ret = 0;
            if (!common::StrToFP(str, ret, false).first)
                NLRT_THROW_EX(u"Arg of [ParseFloat] is not floatpoint"sv, call);
            return ret;
        }
        HashCase(fullName, U"ParseSciFloat")
        {
            const auto arg = EvaluateFirstFuncArg(call, Arg::Type::String);
            const auto str = common::str::to_string(arg.GetStr().value());
            double ret = 0;
            if (!common::StrToFP(str, ret, true).first)
                NLRT_THROW_EX(u"Arg of [ParseSciFloat] is not scientific floatpoint"sv, call);
            return ret;
        }
        default: break;
        }
    }
    if (const auto lcFunc = LookUpFunc(fullName); lcFunc)
    {
        return EvaluateLocalFunc(lcFunc, call, metas);
    }
    return EvaluateUnknwonFunc(call, metas);
}

Arg NailangRuntimeBase::EvaluateLocalFunc(const LocalFunc& func, const FuncCall& call, common::span<const FuncCall> metas)
{
    ThrowByArgCount(call, func.ArgNames.size());

    auto newCtx = ConstructEvalContext();
    for (const auto& [varName, rawarg] : common::linq::FromIterable(func.ArgNames).Pair(common::linq::FromIterable(call.Args)))
    {
        newCtx->LocateArg(varName, true).Set(EvaluateExpr(rawarg));
    }
    auto newFrame = PushFrame(std::move(newCtx), FrameFlags::FuncCall);
    CurFrame->BlockScope = func.Body;
    CurFrame->MetaScope = metas;
    ExecuteFrame();
    return std::move(CurFrame->ReturnArg);
}

Arg NailangRuntimeBase::EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall>)
{
    NLRT_THROW_EX(FMTSTR(u"Func [{}] with [{}] args cannot be resolved.", call.FullFuncName(), call.Args.size()), call);
    return {};
}

Arg NailangRuntimeBase::EvaluateExtendMathFunc(const FuncCall& call, common::span<const FuncCall>)
{
    using Type = Arg::Type;
    const auto mathName = call.Name->GetPart(1);
    switch (DJBHash::HashC(mathName))
    {
    HashCase(mathName, U"Max")
    {
        ThrowByArgCount(call, 1, ArgLimits::AtLeast);
        auto ret = EvaluateExpr(call.Args[0]);
        for (size_t i = 1; i < call.Args.size(); ++i)
        {
            auto ret2 = EvaluateExpr(call.Args[i]);
            const auto isLess = EmbedOpEval::Less(ret, ret2);
            if (isLess.IsEmpty())
                NLRT_THROW_EX(FMTSTR(u"Cannot perform [less] on type [{}],[{}]"sv, ret.GetTypeName(), ret2.GetTypeName()), call);
            if (isLess.GetBool().value()) 
                ret = std::move(ret2);
        }
        return ret;
    }
    HashCase(mathName, U"Min")
    {
        ThrowByArgCount(call, 1, ArgLimits::AtLeast);
        auto ret = EvaluateExpr(call.Args[0]);
        for (size_t i = 1; i < call.Args.size(); ++i)
        {
            auto ret2 = EvaluateExpr(call.Args[i]);
            const auto isLess = EmbedOpEval::Less(ret2, ret);
            if (isLess.IsEmpty())
                NLRT_THROW_EX(FMTSTR(u"Cannot perform [less] on type [{}],[{}]"sv, ret.GetTypeName(), ret2.GetTypeName()), call);
            if (isLess.GetBool().value()) 
                ret = std::move(ret2);
        }
        return ret;
    }
    HashCase(mathName, U"Sqrt")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::sqrt(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::sqrt(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::sqrt(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Ceil")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::ceil(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Floor")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::floor(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Round")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::round(arg.GetVar<Type::FP>());
        case Type::Uint:
        case Type::Int:     return arg;
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::log(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log2")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::log2(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log2(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log2(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Log10")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Number);
        switch (arg.TypeData)
        {
        case Type::FP:      return std::log10(arg.GetVar<Type::FP>());
        case Type::Uint:    return std::log10(arg.GetVar<Type::Uint>());
        case Type::Int:     return std::log10(arg.GetVar<Type::Int>());
        default:            break;
        }
    } break;
    HashCase(mathName, U"Lerp")
    {
        const auto args = EvaluateFuncArgs<3>(call, { Arg::Type::Number, Arg::Type::Number, Arg::Type::Number });
        const auto x = args[0].GetFP();
        const auto y = args[1].GetFP();
        const auto a = args[2].GetFP();
        if (x && y && a)
#if defined(__cpp_lib_interpolate) && __cpp_lib_interpolate >= 201902L
            return std::lerp(x.value(), y.value(), a.value());
#else
            return x.value() * (1. - a.value()) + y.value() * a.value();
#endif
    } break;
    HashCase(mathName, U"Pow")
    {
        const auto args = EvaluateFuncArgs<2>(call, { Arg::Type::Number, Arg::Type::Number });
        const auto x = args[0].GetFP();
        const auto y = args[1].GetFP();
        if (x && y)
            return std::pow(x.value(), y.value());
    } break;
    HashCase(mathName, U"LeadZero")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"TailZero")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"PopCount")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Int>()));
        default:         break;
        }
    } break;
    HashCase(mathName, U"ToUint")
    {
        const auto arg = EvaluateFirstFuncArg(call).GetUint();
        if (arg)
            return arg.value();
    } break;
    HashCase(mathName, U"ToInt")
    {
        const auto arg = EvaluateFirstFuncArg(call).GetInt();
        if (arg)
            return arg.value();
    } break;
    default: return {};
    }
    NLRT_THROW_EX(FMTSTR(u"MathFunc [{}] cannot be evaluated.", call.FullFuncName()), call);
    return {};
}

Arg NailangRuntimeBase::EvaluateExpr(const Expr& arg)
{
    using Type = Expr::Type;
    ExprHolder callHost(this, arg);
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
    case Type::Var:     return LookUpArg(arg.GetVar<Type::Var>());
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            assert(false); return {};
    }
    
}

Arg NailangRuntimeBase::EvaluateUnaryExpr(const UnaryExpr& expr)
{
    if (expr.Operator == EmbedOps::Not)
    {
        auto val = EvaluateExpr(expr.Operand);
        auto ret = val.HandleUnary(expr.Operator);
        if (!ret.IsEmpty())
            return std::move(ret);
        NLRT_THROW_EX(FMTSTR(u"Cannot perform unary expr [{}] on type [{}]"sv, 
            EmbedOpHelper::GetOpName(expr.Operator), val.GetTypeName()), Expr(&expr));
    }
    else
        NLRT_THROW_EX(FMTSTR(u"Unexpected unary op [{}]"sv, EmbedOpHelper::GetOpName(expr.Operator)), Expr(&expr));
    return {};
}

Arg NailangRuntimeBase::EvaluateBinaryExpr(const BinaryExpr& expr)
{
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
    else if(expr.Operator == EmbedOps::Or)
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
    return {};
}

Arg NailangRuntimeBase::EvaluateTernaryExpr(const TernaryExpr& expr)
{
    const auto cond = ThrowIfNotBool(EvaluateExpr(expr.Condition), U"condition of ternary operator"sv);
    const auto& selected = cond ? expr.LeftOperand : expr.RightOperand;
    return EvaluateExpr(selected);
}

Arg NailangRuntimeBase::EvaluateQueryExpr(const QueryExpr& expr)
{
    Expects(expr.Size() > 0);
    auto target = EvaluateExpr(expr.Target);
    try
    {
        return ArgQuery::LocateRead(target, expr, *this, [](ArgLocator ret) { return ret.ExtractGet(); });
    }
    catch (const NailangRuntimeException& nre)
    {
        HandleException(nre);
        return {};
    }
}

void NailangRuntimeBase::OnRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}


void NailangRuntimeBase::ExecuteBlock(const Block& block, common::span<const FuncCall> metas, std::shared_ptr<EvaluateContext> ctx, const bool checkMetas)
{
    auto frame = PushFrame(ctx, FrameFlags::FlowScope);
    if (checkMetas)
    {
        Statement target(&block);
        if (!HandleMetaFuncs(metas, target))
            return;
    }
    CurFrame->MetaScope  = metas;
    CurFrame->BlockScope = &block;
    ExecuteFrame();
}

Arg NailangRuntimeBase::EvaluateRawStatement(std::u32string_view content, const bool innerScope)
{
    common::parser::ParserContext context(content);
    const auto rawarg = NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
    if (rawarg)
    {
        auto frame = PushFrame(innerScope, FrameFlags::FlowScope);
        return EvaluateExpr(rawarg);
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

    auto frame = PushFrame(innerScope, FrameFlags::FlowScope);
    CurFrame->BlockScope = &blk;
    ExecuteFrame();
    return std::move(CurFrame->ReturnArg);
}



}
