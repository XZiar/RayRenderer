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


std::pair<Arg, size_t> Arg::HandleGetter(SubQuery subq, NailangRuntimeBase& runtime) const
{
    Expects(subq.Size() > 0);
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleGetter>(subq, runtime);
    }
    if (IsArray())
    {
        const auto arr = GetVar<Type::Array>();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateArg(query);
            const auto idx = NailangHelper::BiDirIndexCheck(arr.Length, val, &query);
            return { arr.Get(idx), 1u };
        } break;
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<RawArg::Type::Str>();
            if (field == U"Length"sv)
                return { arr.Length, 1u };
        } break;
        default: break;
        }
    }
    if (IsStr())
    {
        const auto str = GetStr().value();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateArg(query);
            const auto idx = NailangHelper::BiDirIndexCheck(str.size(), val, &query);
            return { std::u32string(1, str[idx]), 1u };
        } break;
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<RawArg::Type::Str>();
            if (field == U"Length"sv)
                return { uint64_t(str.size()), 1u };
        } break;
        default: break;
        }
    }
    return { {}, 0u };
}

void Arg::HandleSetter(SubQuery subq, NailangRuntimeBase& runtime, Arg val)
{
    Expects(subq.Size() > 0);
    if (IsCustom())
    {
        auto& var = GetCustom();
        const auto used = var.Call<&CustomVar::Handler::HandleSetter>(subq, runtime, std::move(val));
        if (used != subq.Size())
        {
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"Failed to perform query [{}] on Custom type"sv, 
                Serializer::Stringify(subq.Sub(used))), *this);
        }
        return;
    }
    COMMON_THROW(NailangRuntimeException, FMTSTR(u"Cannot perform query [{}] on type [{}]"sv, 
        Serializer::Stringify(subq), GetTypeName()), *this);
}


Arg CustomVar::Handler::EvaluateArg(NailangRuntimeBase& runtime, const RawArg& arg)
{
    return runtime.EvaluateArg(arg);
}

void CustomVar::Handler::HandleException(NailangRuntimeBase& runtime, const NailangRuntimeException& ex)
{
    runtime.HandleException(ex);
}

std::pair<Arg, size_t> CustomVar::Handler::HandleGetter(const CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime)
{
    Expects(subq.Size() > 0);
    const auto [type, query] = subq[0];
    switch (type)
    {
    case SubQuery::QueryType::Index:
    {
        const auto idx = EvaluateArg(runtime, query);
        auto result = IndexerGetter(var, idx, &query);
        if (!result.IsEmpty())
            return { std::move(result), 1u };
    } break;
    case SubQuery::QueryType::Sub:
    {
        auto field = query.GetVar<RawArg::Type::Str>();
        auto result = SubfieldGetter(var, field);
        if (!result.IsEmpty())
            return { std::move(result), 1u };
    } break;
    default: break;
    }
    return { {}, 0u };
}

size_t CustomVar::Handler::HandleSetter(CustomVar&, SubQuery, NailangRuntimeBase&, Arg)
{
    return 0;
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

bool BasicEvaluateContext::SetFunc(const Block* block, common::span<const RawArg> args)
{
    const uint32_t offset = gsl::narrow_cast<uint32_t>(LocalFuncArgNames.size()),
        size = gsl::narrow_cast<uint32_t>(args.size());
    LocalFuncArgNames.reserve(static_cast<size_t>(offset) + size);
    for (const auto& arg : args)
        LocalFuncArgNames.emplace_back(arg.GetVar<RawArg::Type::Var>());
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

const Arg* LargeEvaluateContext::LocateArg(const LateBindVar& var) const noexcept
{
    if (const auto it = ArgMap.find(var.Name); it != ArgMap.end())
        if (!it->second.IsEmpty())
            return &it->second;
    return nullptr;
}

Arg* LargeEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    if (const auto it = ArgMap.find(var.Name); it != ArgMap.end())
        return &it->second;
    if (create)
    {
        const auto [it, _] = ArgMap.insert_or_assign(std::u32string(var.Name), Arg{});
        return &it->second;
    }
    return nullptr;
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

const Arg* CompactEvaluateContext::LocateArg(const LateBindVar& var) const noexcept
{
    const HashedStrView hsv(var.Name);
    for (const auto& [pos, val] : Args)
        if (ArgNames.GetHashedStr(pos) == hsv)
            return val.IsEmpty() ? nullptr : &val;
    return nullptr;
}

Arg* CompactEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    const HashedStrView hsv(var.Name);
    for (auto& [pos, val] : Args)
        if (ArgNames.GetHashedStr(pos) == hsv)
            return &val;
    if (create)
    {
        const auto piece = ArgNames.AllocateString(var.Name);
        Args.emplace_back(piece, Arg{});
        return &Args.back().second;
    }
    return nullptr;
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
std::optional<Arg> EmbedOpEval::Equal(const Arg& left, const Arg& right) noexcept
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
std::optional<Arg> EmbedOpEval::NotEqual(const Arg& left, const Arg& right) noexcept
{
    const auto ret = Equal(left, right);
    if (ret.has_value())
        return !ret->GetVar<Arg::Type::Bool>();
    return ret;
}
template<typename F>
forceinline static std::optional<Arg> NumOp(const Arg& left, const Arg& right, F func) noexcept
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

std::optional<Arg> EmbedOpEval::Less(const Arg& left, const Arg& right) noexcept
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
std::optional<Arg> EmbedOpEval::LessEqual(const Arg& left, const Arg& right) noexcept
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

std::optional<Arg> EmbedOpEval::And(const Arg& left, const Arg& right) noexcept
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ && *right_;
    return {};
}
std::optional<Arg> EmbedOpEval::Or(const Arg& left, const Arg& right) noexcept
{
    const auto left_ = left.GetBool(), right_ = right.GetBool();
    if (left_.has_value() && right_.has_value())
        return *left_ || *right_;
    return {};
}

std::optional<Arg> EmbedOpEval::Add(const Arg& left, const Arg& right) noexcept
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
std::optional<Arg> EmbedOpEval::Sub(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l - r; });
}
std::optional<Arg> EmbedOpEval::Mul(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l * r; });
}
std::optional<Arg> EmbedOpEval::Div(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) { return l / r; });
}
std::optional<Arg> EmbedOpEval::Rem(const Arg& left, const Arg& right) noexcept
{
    return NumOp(left, right, [](const auto l, const auto r) 
        {
            if constexpr (std::is_floating_point_v<decltype(l)> || std::is_floating_point_v<decltype(r)>)
                return std::fmod(static_cast<double>(l), static_cast<double>(r));
            else
                return l % r; 
        });
}

std::optional<Arg> EmbedOpEval::Not(const Arg& arg) noexcept
{
    const auto arg_ = arg.GetBool();
    if (arg_.has_value())
        return !*arg_;
    return {};
}
#undef LR_BOTH


size_t NailangHelper::BiDirIndexCheck(const size_t size, const Arg& idx, const RawArg* src)
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

NailangRuntimeBase::FCallHost::~FCallHost()
{
    Expects(Host.CurFrame);
    Expects(Host.CurFrame->CurFuncCall == &Frame);
    Host.CurFrame->CurFuncCall = Frame.PrevFrame;
}


NailangRuntimeBase::NailangRuntimeBase(std::shared_ptr<EvaluateContext> context) : 
    RootContext(std::move(context))
{ }
NailangRuntimeBase::~NailangRuntimeBase()
{ }


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
    NLRT_THROW_EX(FMTSTR(u"Func [{}] requires {} [{}] args, which gives [{}].", *call.Name, prefix, count, call.Args.size()),
        detail::ExceptionTarget{}, &call);
}

void NailangRuntimeBase::ThrowByArgType(const FuncCall& call, const RawArg::Type type, size_t idx) const
{
    const auto& arg = call.Args[idx];
    if (arg.TypeData != type)
        NLRT_THROW_EX(FMTSTR(u"Expected [{}] for [{}]'s {} rawarg, which gives [{}], source is [{}].",
            RawArg::TypeName(type), *call.Name, idx, arg.GetTypeName(), Serializer::Stringify(arg)),
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
                Arg::TypeName(type), *call.Name, idx, arg.GetTypeName(), Serializer::Stringify(call.Args[idx])),
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
        NLRT_THROW_EX(FMTSTR(u"Func [{}] only support as [{}], get[{}].", *call.Name, FuncTypeName(type), FuncTypeName(call.Name->Info())),
            call);
    }
}

void NailangRuntimeBase::ThrowIfBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const
{
    if (target.GetType() == type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] cannot be appllied to [{}].", *meta.Name, ContentTypeName(type)),
            meta);
}

void NailangRuntimeBase::ThrowIfNotBlockContent(const FuncCall& meta, const BlockContent target, const BlockContent::Type type) const
{
    if (target.GetType() != type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] can only be appllied to [{}].", *meta.Name, ContentTypeName(type)),
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

void NailangRuntimeBase::CheckFuncArgs(common::span<const RawArg::Type> types, const ArgLimits limit, const FuncCall& call)
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
        args[i] = EvaluateArg(call.Args[i]);
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
        CurFrame->CurContent = &content;
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
    if (*meta.Name == U"MetaIf"sv)
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

bool NailangRuntimeBase::HandleMetaFuncs(common::span<const FuncCall> metas, const BlockContent& target)
{
    FCallHost callHost(this);
    for (const auto& meta : metas)
    {
        Expects(meta.Name->Info() == FuncName::FuncInfo::Meta);
        callHost = &meta;
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

void NailangRuntimeBase::HandleContent(const BlockContent& content, common::span<const FuncCall> metas)
{
    switch (content.GetType())
    {
    case BlockContent::Type::Assignment:
    {
        const auto& assign = *content.Get<Assignment>();
        SetArg(assign.Target, assign.Queries, assign.Statement, assign.CheckNil);
    } break;
    case BlockContent::Type::Block:
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
    case BlockContent::Type::FuncCall:
    {
        const auto& fcall = content.Get<FuncCall>();
        FCallHost callHost(this, fcall);
        Expects(fcall->Name->Info() == FuncName::FuncInfo::Empty);
        EvaluateFunc(*fcall, metas);
    } break;
    case BlockContent::Type::RawBlock:
    {
        OnRawBlock(*content.Get<RawBlock>(), metas);
    } break;
    default:
        assert(false); /*Expects(false);*/
        break;
    }
}

void NailangRuntimeBase::OnLoop(const RawArg& condition, const BlockContent& target, common::span<const FuncCall> metas)
{
    const auto prevFrame = PushFrame(FrameFlags::InLoop);
    while (true)
    {
        const auto cond = EvaluateArg(condition);
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

std::u32string NailangRuntimeBase::FormatString(const std::u32string_view formatter, common::span<const RawArg> args)
{
    std::vector<Arg> results;
    results.reserve(args.size());
    for (const auto& arg : args)
        results.emplace_back(EvaluateArg(arg));
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
LateBindVar NailangRuntimeBase::DecideDynamicVar(const RawArg& arg, const std::u16string_view reciever) const
{
    switch (arg.TypeData)
    {
    case RawArg::Type::Var: 
        return arg.GetVar<RawArg::Type::Var>();
    case RawArg::Type::Str: 
    {
        const auto name = arg.GetVar<RawArg::Type::Str>();
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

const Arg* NailangRuntimeBase::LocateArgRead(const LateBindVar& var) const
{
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Root))
        return RootContext->LocateArg(var);
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Local))
    {
        if (!CurFrame)
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] without frame", var));
        else if (CurFrame->Has(FrameFlags::Virtual))
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] in a virtual frame", var));
        else
            return CurFrame->Context->LocateArg(var);
        return nullptr;
    }

    // Try find arg recursively
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        if (!frame->Has(FrameFlags::Virtual))
        {
            const auto ptr = frame->Context->LocateArg(var);
            if (ptr)
                return ptr;
        }
        //if (frame->Has(FrameFlags::CallScope))
        //    break; // cannot beyond  
    }
    return RootContext->LocateArg(var);
}

Arg* NailangRuntimeBase::LocateArgWrite(const LateBindVar& var, const bool create) const
{
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Root))
    {
        return RootContext->LocateArg(var, create);
    }
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Local))
    {
        if (!CurFrame)
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] without frame", var));
        else if (CurFrame->Has(FrameFlags::Virtual))
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] in a virtual frame", var));
        else
            return CurFrame->Context->LocateArg(var, create);
        return nullptr;
    }

    // Try find arg recursively
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        if (!frame->Has(FrameFlags::Virtual))
        {
            const auto ptr = frame->Context->LocateArg(var, create);
            if (ptr)
                return ptr;
        }
        //if (frame->Has(FrameFlags::CallScope))
        //    break; // cannot beyond  
    }
    return RootContext->LocateArg(var, create);
}

void NailangRuntimeBase::HandleException(const NailangRuntimeException& ex) const
{
    std::vector<std::pair<const std::u16string_view, common::SharedString<char16_t>>> nameCache;
    const auto GetFileName = [&](const Block* blk) -> common::SharedString<char16_t>
    {
        if (!blk) return {};
        for (const auto& [name, str] : nameCache)
            if (name == blk->FileName)
                return str;
        return blk->FileName;
    };
    constexpr auto FromBlock = [&](const Block* blk, common::SharedString<char16_t> fname) -> common::StackTraceItem
    {
        if (!blk)
            return { fname, u"<empty>"sv, 0 };
        return { fname, FMTSTR(u"<Block>{}", blk->Name), blk->Position.first };
    };
    constexpr auto FromFunc = [](const FuncFrame* frame, const common::SharedString<char16_t>& fname) -> common::StackTraceItem
    {
        if (frame->Func)
            return { fname, common::str::to_u16string(frame->Func->Name->FullName()), frame->Func->Position.first };
        return { fname, common::SharedString<char16_t>{}, 0 };
    };
    std::vector<common::StackTraceItem> traces;
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        auto fname = GetFileName(frame->BlockScope);
        for (auto call = CurFrame->CurFuncCall; call; call = call->PrevFrame)
        {
            traces.emplace_back(FromFunc(call, fname));
        }
        traces.emplace_back(FromBlock(frame->BlockScope, std::move(fname)));
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
    const auto ptr = LocateArgRead(var);
    if (ptr) return *ptr;
    return {};
}
bool NailangRuntimeBase::SetArg(const LateBindVar& var, SubQuery subq, std::variant<Arg, RawArg> arg, NilCheck nilCheck)
{
    const auto DirectSet = [&](Arg* ptr)
    {
        if (arg.index() == 0)
            *ptr = std::move(std::get<0>(arg));
        else
            *ptr = EvaluateArg(std::get<1>(arg));
    };
    if (nilCheck == NilCheck::ReqNull)
    {
        Expects(subq.Size() == 0);
        const auto ptr = LocateArgWrite(var, false);
        if (ptr) // not null, skip
            return false;
        else
        {
            DirectSet(LocateArgWrite(var, true));
            return true;
        }
    }
    // check if exists
    auto ptr = LocateArgWrite(var, false);
    if (!ptr)
    {
        if (nilCheck == NilCheck::ReqNotNull)
        {
            std::u16string msg;
            if (subq.Size() > 0)
            {
                msg = u", expect perform subquery on it"sv;
            }
            else if (arg.index() == 1 && std::get<1>(arg).TypeData == RawArg::Type::Binary)
            {
                const auto op = std::get<1>(arg).GetVar<RawArg::Type::Binary>()->Operator;
                msg = FMTSTR(u", expect perform [{}] on it"sv, EmbedOpHelper::GetOpName(op));
            }
            NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exists{}"sv, var, msg), std::get<1>(arg));
            return false;
        }
        ptr = LocateArgWrite(var, true);
    }
    Expects(ptr);
    if (subq.Size() == 0)
    {
        DirectSet(ptr);
        return true;
    }
    ptr->HandleSetter(subq, *this, arg.index() == 0 ? std::move(std::get<0>(arg)) : EvaluateArg(std::get<1>(arg)));
    return true;
}

LocalFunc NailangRuntimeBase::LookUpFunc(std::u32string_view name) const
{
    for (auto frame = CurFrame; frame; frame = frame->PrevFrame)
    {
        if (!frame->Has(FrameFlags::Virtual))
        {
            auto func = frame->Context->LookUpFunc(name);
            if (func.Body != nullptr)
                return func;
        }
    }
    return RootContext->LookUpFunc(name);
}
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<const RawArg> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else if (CurFrame->Has(FrameFlags::Virtual))
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] in a virtual frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, args);
    return false;
}
bool NailangRuntimeBase::SetFunc(const Block* block, common::span<const std::u32string_view> args)
{
    if (!CurFrame)
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    else if (CurFrame->Has(FrameFlags::Virtual))
        NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] in a virtual frame", block->Name));
    else
        return CurFrame->Context->SetFunc(block, args);
    return false;
}

NailangRuntimeBase::MetaFuncResult NailangRuntimeBase::HandleMetaFunc(const FuncCall& meta, const BlockContent& content, common::span<const FuncCall> allMetas)
{
    if (meta.Name->PartCount > 1)
        return MetaFuncResult::Unhandled;
    const auto metaName = meta.Name->FullName();
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
        ThrowIfNotBlockContent(meta, content, BlockContent::Type::Block);
        for (const auto& arg : meta.Args)
        {
            if (arg.TypeData != RawArg::Type::Var)
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv, arg, &meta);
            const auto& var = arg.GetVar<RawArg::Type::Var>();
            if (HAS_FIELD(var.Info, LateBindVar::VarInfo::PrefixMask))
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg name must not contain [Root|Local] flag"sv, arg, &meta);
        }
        SetFunc(content.Get<Block>(), meta.Args);
        return MetaFuncResult::Skip;
    }
    HashCase(metaName, U"While")
    {
        ThrowIfBlockContent(meta, content, BlockContent::Type::RawBlock);
        ThrowByArgCount(meta, 1);
        OnLoop(meta.Args[0], content, allMetas);
        return MetaFuncResult::Skip;
    }
    }
    return MetaFuncResult::Unhandled;
}

Arg NailangRuntimeBase::EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas)
{
    const auto fullName = call.Name->FullName();
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
    if (call.Name->PartCount == 2 && (*call.Name)[0] == U"Math"sv)
    {
        if (auto ret = EvaluateExtendMathFunc(call, metas); ret)
        {
            if (!ret->IsEmpty())
                return ret.value();
            NLRT_THROW_EX(FMTSTR(u"Math func's arg type does not match requirement, [{}]"sv, Serializer::Stringify(&call)), call);
        }
    }
    if (call.Name->PartCount == 1)
    {
        switch (DJBHash::HashC(fullName))
        {
        HashCase(fullName, U"Select")
        {
            ThrowByArgCount(call, 3);
            const auto& selected = ThrowIfNotBool(EvaluateArg(call.Args[0]), U""sv) ? call.Args[1] : call.Args[2];
            return EvaluateArg(selected);
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
                return EvaluateArg(call.Args[1]);
        }
        HashCase(fullName, U"Format")
        {
            ThrowByArgCount(call, 2, ArgLimits::AtLeast);
            const auto fmtStr = EvaluateArg(call.Args[0]).GetStr();
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

Arg NailangRuntimeBase::EvaluateLocalFunc(const LocalFunc& func, const FuncCall& call, common::span<const FuncCall>)
{
    ThrowByArgCount(call, func.ArgNames.size());

    auto newFrame = PushFrame(FrameFlags::FuncCall);
    CurFrame->BlockScope = func.Body;
    CurFrame->MetaScope = {};
    for (const auto& [varName, rawarg] : common::linq::FromIterable(func.ArgNames).Pair(common::linq::FromIterable(call.Args)))
    {
        *CurFrame->Context->LocateArg(varName, true) = EvaluateArg(rawarg);
    }
    ExecuteFrame();
    return std::move(CurFrame->ReturnArg);
}

Arg NailangRuntimeBase::EvaluateUnknwonFunc(const FuncCall& call, common::span<const FuncCall>)
{
    NLRT_THROW_EX(FMTSTR(u"Func [{}] with [{}] args cannot be resolved.", *call.Name, call.Args.size()), call);
    return {};
}

std::optional<Arg> NailangRuntimeBase::EvaluateExtendMathFunc(const FuncCall& call, common::span<const FuncCall>)
{
    using Type = Arg::Type;
    const auto mathName = (*call.Name)[1];
    switch (DJBHash::HashC(mathName))
    {
    HashCase(mathName, U"Max")
    {
        ThrowByArgCount(call, 1, ArgLimits::AtLeast);
        auto ret = EvaluateArg(call.Args[0]);
        for (size_t i = 1; i < call.Args.size(); ++i)
        {
            auto ret2 = EvaluateArg(call.Args[i]);
            const auto isLess = EmbedOpEval::Less(ret, ret2);
            if (!isLess.has_value()) return Arg{};
            if (isLess->GetBool().value()) ret = std::move(ret2);
        }
        return ret;
    }
    HashCase(mathName, U"Min")
    {
        ThrowByArgCount(call, 1, ArgLimits::AtLeast);
        auto ret = EvaluateArg(call.Args[0]);
        for (size_t i = 1; i < call.Args.size(); ++i)
        {
            auto ret2 = EvaluateArg(call.Args[i]);
            const auto isLess = EmbedOpEval::Less(ret2, ret);
            if (!isLess.has_value()) return Arg{};
            if (isLess->GetBool().value()) ret = std::move(ret2);
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
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
        return Arg{};
    }
    HashCase(mathName, U"Pow")
    {
        const auto args = EvaluateFuncArgs<2>(call, { Arg::Type::Number, Arg::Type::Number });
        const auto x = args[0].GetFP();
        const auto y = args[1].GetFP();
        if (x && y)
            return std::pow(x.value(), y.value());
        return Arg{};
    }
    HashCase(mathName, U"LeadZero")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.LeadZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
        return Arg{};
    }
    HashCase(mathName, U"TailZero")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.TailZero(arg.GetVar<Type::Int>()));
        default:         break;
        }
        return Arg{};
    }
    HashCase(mathName, U"PopCount")
    {
        const auto arg = EvaluateFirstFuncArg(call, Arg::Type::Integer);
        switch (arg.TypeData)
        {
        case Type::Uint: return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Uint>()));
        case Type::Int:  return static_cast<uint64_t>(common::MiscIntrin.PopCount(arg.GetVar<Type::Int>()));
        default:         break;
        }
        return Arg{};
    }
    HashCase(mathName, U"ToUint")
    {
        const auto arg = EvaluateFirstFuncArg(call).GetUint();
        if (arg)
            return arg;
        return Arg{};
    }
    HashCase(mathName, U"ToInt")
    {
        const auto arg = EvaluateFirstFuncArg(call).GetInt();
        if (arg)
            return arg;
        return Arg{};
    }
    default: break;
    }
    return {};
}

Arg NailangRuntimeBase::EvaluateArg(const RawArg& arg)
{
    using Type = RawArg::Type;
    switch (arg.TypeData)
    {
    case Type::Func:
    {
        const auto& fcall = arg.GetVar<Type::Func>();
        FCallHost callHost(this, fcall);
        Expects(fcall->Name->Info() == FuncName::FuncInfo::ExprPart);
        return EvaluateFunc(*fcall, {});
    }
    case Type::Unary:
        if (auto ret = EvaluateUnaryExpr(*arg.GetVar<Type::Unary>()); ret.has_value())
            return std::move(ret.value());
        else
            NLRT_THROW_EX(u"Unary expr's arg type does not match requirement"sv, arg);
        break;
    case Type::Binary:
        if (auto ret = EvaluateBinaryExpr(*arg.GetVar<Type::Binary>()); ret.has_value())
            return std::move(ret.value());
        else
            NLRT_THROW_EX(u"Binary expr's arg type does not match requirement"sv, arg);
        break;
    case Type::Query:
        if (auto ret = EvaluateQueryExpr(*arg.GetVar<Type::Query>()); ret.has_value())
            return std::move(ret.value());
        else
            NLRT_THROW_EX(u"Query expr's arg type does not match requirement"sv, arg);
        break;
    case Type::Var:     return LookUpArg(arg.GetVar<Type::Var>());
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            assert(false); /*Expects(false);*/ break;
    }
    return {};
}

std::optional<Arg> NailangRuntimeBase::EvaluateUnaryExpr(const UnaryExpr& expr)
{
    if (expr.Operator == EmbedOps::Not)
        return EmbedOpEval::Not(EvaluateArg(expr.Oprend));
    NLRT_THROW_EX(u"Unexpected unary op"sv, RawArg(&expr));
    return {};
}

std::optional<Arg> NailangRuntimeBase::EvaluateBinaryExpr(const BinaryExpr& expr)
{
    switch (expr.Operator)
    {
#define EVAL_BIN_OP(type) case EmbedOps::type: return EmbedOpEval::type(EvaluateArg(expr.LeftOprend), EvaluateArg(expr.RightOprend))
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
#undef EVAL_BIN_OP
    case EmbedOps::And: 
    {
        const auto left = EvaluateArg(expr.LeftOprend).GetBool();
        if (!left.has_value()) return {};
        if (!left.value()) return false;
        const auto right = EvaluateArg(expr.RightOprend).GetBool();
        if (!right.has_value()) return {};
        if (!right.value()) return false;
        return true;
    }
    case EmbedOps::Or:
    {
        const auto left = EvaluateArg(expr.LeftOprend).GetBool();
        if (!left.has_value()) return {};
        if (left.value()) return true;
        const auto right = EvaluateArg(expr.RightOprend).GetBool();
        if (!right.has_value()) return {};
        if (right.value()) return true;
        return false;
    }
    default:
        NLRT_THROW_EX(u"Unexpected binary op"sv, RawArg(&expr));
        return {};
    }
}

std::optional<Arg> NailangRuntimeBase::EvaluateQueryExpr(const QueryExpr& expr)
{
    auto target = EvaluateArg(expr.Target);
    for (size_t i = 0; i < expr.Size(); ++i)
    {
        size_t step = 0;
        const auto subq = expr.Sub(i);
        try 
        {
            std::tie(target, step) = target.HandleGetter(subq, *this);
        }
        catch (const NailangRuntimeException& nre)
        {
            HandleException(nre);
        }
        if (step == 0)
            NLRT_THROW_EX(FMTSTR(u"Fail to evaluate query [{}]"sv, Serializer::Stringify(subq)), target, RawArg{ &expr });
        i += step;
    }
    return target;
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
        auto target = BlockContentItem::Generate(&block, 0, 0);
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
    const auto rawarg = ComplexArgParser::ParseSingleStatement(MemPool, context);
    if (rawarg.has_value())
    {
        auto frame = PushFrame(innerScope, FrameFlags::FlowScope);
        return EvaluateArg(rawarg.value());
    }
    else
        return {};
}

Arg NailangRuntimeBase::EvaluateRawStatements(std::u32string_view content, const bool innerScope)
{
    struct EmbedBlkParser : public BlockParser
    {
        using BlockParser::BlockParser;
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
