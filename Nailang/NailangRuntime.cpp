#include "NailangPch.h"
#include "NailangRuntime.h"
#include "NailangParser.h"
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


#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTIONEX(NailangRuntimeException, __VA_ARGS__))

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

Arg LargeEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    if (const auto it = ArgMap.find(var.Name); it != ArgMap.end())
        return { &it->second, ArgAccess::ReadWrite };
    if (create)
    { // need create
        const auto [it, _] = ArgMap.insert_or_assign(std::u32string(var.Name), Arg{});
        return { &it->second, ArgAccess::WriteOnly };
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

Arg CompactEvaluateContext::LocateArg(const LateBindVar& var, const bool create) noexcept
{
    const HashedStrView hsv(var.Name);
    for (auto& [pos, val] : Args)
        if (ArgNames.GetHashedStr(pos) == hsv)
        {
            if (!val.IsEmpty())
                return { &val, ArgAccess::ReadWrite };
            if (create)
                return { &val, ArgAccess::WriteOnly };
            return {};
        }
    if (create)
    { // need create
        const auto piece = ArgNames.AllocateString(var.Name);
        Args.emplace_back(piece, Arg{});
        return { &Args.back().second, ArgAccess::WriteOnly };
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


size_t NailangHelper::BiDirIndexCheck(const size_t size, const Arg& idx, const Expr* src)
{
    if (!idx.IsInteger())
    {
        auto err = FMTSTR(u"Require integer as indexer, get[{}]"sv, idx.GetTypeName());
        if (src)
            COMMON_THROWEX(NailangRuntimeException, err, *src);
        else
            COMMON_THROWEX(NailangRuntimeException, err);
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
        COMMON_THROWEX(NailangRuntimeException,
            FMTSTR(u"index out of bound, access [{}{}] of length [{}]"sv, isReverse ? u"-"sv : u""sv, realIdx, size));
    return isReverse ? static_cast<size_t>(size - realIdx) : static_cast<size_t>(realIdx);
}

Arg NailangHelper::ExtractArg(Arg& target, SubQuery query, NailangExecutor& executor)
{
    Expects(query.Size() > 0);
    auto next = target.HandleQuery(query, executor);
    if (next.IsEmpty())
        COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
            Serializer::Stringify(query)));
    if (!HAS_FIELD(next.GetAccess(), ArgAccess::Readable))
        COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Can not access an get-host's [{}]",
            Serializer::Stringify(query)));
    if (query.Size() > 0)
    {
        next.Decay();
        return ExtractArg(next, query, executor);
    }
    else
    {
        next.Decay();
        return next;
    }
}
template<typename F>
bool NailangHelper::LocateWrite(Arg& target, SubQuery query, NailangExecutor& executor, const F& func)
{
    if (query.Size() > 0)
    {
        target.Decay();
        auto next = target.HandleQuery(query, executor);
        if (next.IsEmpty())
            COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                Serializer::Stringify(query)));
        if (!MATCH_FIELD(next.GetAccess(), ArgAccess::Mutable))
            COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Can not assgin to immutable's [{}]",
                Serializer::Stringify(query)));
        return LocateWrite(next, query, executor, func);
    }
    else
    {
        return func(target);
    }
}
template<bool IsWrite, typename F>
auto NailangHelper::LocateAndExecute(Arg& target, SubQuery query, NailangExecutor& executor, const F& func)
    -> decltype(func(std::declval<Arg&>()))
{
    if (query.Size() > 0)
    {
        target.Decay();
        auto next = target.HandleQuery(query, executor);
        if (next.IsEmpty())
            COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Does not exists [{}]",
                Serializer::Stringify(query)));
        if constexpr (IsWrite)
        {
            if (!MATCH_FIELD(next.GetAccess(), ArgAccess::Mutable))
                COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Can not assgin to immutable's [{}]",
                    Serializer::Stringify(query)));
        }
        else
        {
            if (!HAS_FIELD(next.GetAccess(), ArgAccess::Readable))
                COMMON_THROWEX(NailangRuntimeException, FMTSTR(u"Can not access an set-host's [{}]",
                    Serializer::Stringify(query)));
        }
        return LocateAndExecute<IsWrite>(next, query, executor, func);
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

NailangFrame::NailangFrame(NailangFrame* prev, NailangExecutor* executor, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag) noexcept :
    FuncInfo(nullptr), PrevFrame(prev), Executor(executor), Context(ctx), Flags(flag) 
{ }
NailangFrame::~NailangFrame() 
{ }
size_t NailangFrame::GetSize() const noexcept 
{ 
    return sizeof(NailangFrame);
}


NailangBlockFrame::NailangBlockFrame(NailangFrame* prev, NailangExecutor* executor, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag,
    const Block* block, common::span<const FuncCall> metas) noexcept :
    NailangFrame(prev, executor, ctx, flag), MetaScope(metas), BlockScope(block) 
{ }
NailangBlockFrame::~NailangBlockFrame() 
{ }
size_t NailangBlockFrame::GetSize() const noexcept
{
    return sizeof(NailangBlockFrame);
}


NailangRawBlockFrame::NailangRawBlockFrame(NailangFrame* prev, NailangExecutor* executor, const std::shared_ptr<EvaluateContext>& ctx, FrameFlags flag,
    const RawBlock* block, common::span<const FuncCall> metas) noexcept :
    NailangFrame(prev, executor, ctx, flag), MetaScope(metas), BlockScope(block)
{ }
NailangRawBlockFrame::~NailangRawBlockFrame()
{ }
size_t NailangRawBlockFrame::GetSize() const noexcept
{
    return sizeof(NailangRawBlockFrame);
}


NailangFrameStack::FrameHolderBase::~FrameHolderBase()
{
    if (!Frame)
        return;
    Expects(Host && Frame);
    Expects(Host->TopFrame == Frame);
    const auto size = Frame->GetSize();
    Frame->~NailangFrame();
    Host->TopFrame = Frame->PrevFrame;
    const auto ret = Host->TryDealloc({ reinterpret_cast<const std::byte*>(Frame), size });
    Ensures(ret);
}

NailangFrameStack::NailangFrameStack() : TrunckedContainer(4096, 4096)
{ }
NailangFrameStack::~NailangFrameStack()
{
    Expects(GetAllocatedSlot() == 0);
}

NailangBlockFrame* NailangFrameStack::SetReturn(Arg returnVal) const noexcept
{
    for (auto frame = TopFrame; frame; frame = frame->PrevFrame)
    {
        frame->Status = NailangFrame::ProgramStatus::End | NailangFrame::ProgramStatus::LoopEnd;
        if (HAS_FIELD(frame->Flags, NailangFrame::FrameFlags::FlowScope))
        {
            if (auto blkFrame = dynamic_cast<NailangBlockFrame*>(frame); blkFrame)
            {
                blkFrame->ReturnArg = std::move(returnVal);
                return blkFrame;
            }
            return nullptr;
        }
    }
    return nullptr;
}
NailangFrame* NailangFrameStack::SetBreak() const noexcept
{
    for (auto frame = TopFrame; frame; frame = frame->PrevFrame)
    {
        frame->Status = NailangFrame::ProgramStatus::End | NailangFrame::ProgramStatus::LoopEnd;
        if (HAS_FIELD(frame->Flags, NailangFrame::FrameFlags::LoopScope))
        {
            return frame;
        }
    }
    return nullptr;
}
NailangFrame* NailangFrameStack::SetContinue() const noexcept
{
    for (auto frame = TopFrame; frame; frame = frame->PrevFrame)
    {
        frame->Status = NailangFrame::ProgramStatus::End;
        if (HAS_FIELD(frame->Flags, NailangFrame::FrameFlags::LoopScope))
        {
            return frame;
        }
    }
    return nullptr;
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
std::vector<common::StackTraceItem> NailangFrameStack::CollectStacks() const noexcept
{
    auto GetFileName = [nameCache = boost::container::small_vector<std::pair<const std::u16string_view, common::SharedString<char16_t>>, 4>()]
    (const RawBlock* blk) mutable -> common::SharedString<char16_t>
    {
        if (!blk) return {};
        for (const auto& [name, str] : nameCache)
            if (name == blk->FileName)
                return str;
        common::SharedString<char16_t> str(blk->FileName);
        nameCache.emplace_back(blk->FileName, str);
        return str;
    };
    std::vector<common::StackTraceItem> stacks;
    for (auto frame = TopFrame; frame; frame = frame->PrevFrame)
    {
        common::SharedString<char16_t> fname;
        const auto blkFrame = dynamic_cast<const NailangBlockFrame*>(frame);
        if (blkFrame)
        {
            fname = GetFileName(blkFrame->BlockScope);
        }
        else if (const auto rawFrame = dynamic_cast<const NailangRawBlockFrame*>(frame); rawFrame)
        {
            fname = GetFileName(rawFrame->BlockScope);
        }
        const FuncCall* lastFunc = nullptr;
        for (auto finfo = frame->FuncInfo; finfo; finfo = finfo->PrevInfo)
        {
            stacks.emplace_back(CreateStack(finfo->Func, fname));
            lastFunc = finfo->Func;
        }
        if (blkFrame && blkFrame->CurContent && (*blkFrame->CurContent) != lastFunc)
        {
            stacks.emplace_back(blkFrame->CurContent->Visit([&](const auto* ptr) { return CreateStack(ptr, fname); }));
        }
    }
    return stacks;
}


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
    case FuncName::FuncInfo::ExprPart:  return U"Part of expr"sv;
    case FuncName::FuncInfo::Meta:      return U"MetaFunc"sv;
    case FuncName::FuncInfo::PostMeta:  return U"MetaFunc"sv;
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
    if (type == Arg::Type::Empty) return;
    if ((type & Arg::Type::CategoryMask) != Arg::Type::Empty) // require specific type
    {
        if (arg.TypeData == type) return;
    }
    else if ((type & Arg::Type::ExtendedMask) != Arg::Type::Empty) // require 
    {
        if ((arg.TypeData & type) == type) return;
    }
    else // allow custom
    {
        const auto type_ = arg.IsCustom() ? arg.GetCustom().Call<&CustomVar::Handler::QueryConvertSupport>() : arg.TypeData;
        if (HAS_FIELD(type_, type)) return;
    }
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
void NailangBase::ThrowIfStatement(const FuncCall& meta, const Statement& target, const Statement::Type type) const
{
    if (target.TypeData == type)
        NLRT_THROW_EX(FMTSTR(u"Metafunc [{}] cannot be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}
void NailangBase::ThrowIfNotStatement(const FuncCall& meta, const Statement& target, const Statement::Type type) const
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

std::u32string NailangBase::FormatString(const std::u32string_view formatter, common::span<const Arg> args)
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
        default: HandleException(CREATE_EXCEPTIONEX(NailangFormatException, formatter, arg, u"Unsupported DataType")); break;
        }
    }
    try
    {
        return fmt::vformat(formatter, store);
    }
    catch (const fmt::format_error& err)
    {
        HandleException(CREATE_EXCEPTIONEX(NailangFormatException, formatter, err));
    }
    return {};
}

TempFuncName NailangBase::CreateTempFuncName(std::u32string_view name, FuncName::FuncInfo info) const
{
    if (info == FuncName::FuncInfo::Meta && !name.empty() && name[0] == ':')
    {
        name.remove_prefix(1);
        info = FuncName::FuncInfo::PostMeta;
    }
    if (name.empty())
        NLRT_THROW_EX(u"Does not allow empty function name"sv);
    try
    {
        return FuncName::CreateTemp(name, info);
    }
    catch (const NailangPartedNameException&)
    {
        NLRT_THROW_EX(u"FuncCall's name not valid"sv);
    }
    return FuncName::CreateTemp(name, info);
}

LateBindVar NailangBase::DecideDynamicVar(const Expr& arg, const std::u16string_view reciever) const
{
    switch (arg.TypeData)
    {
    case Expr::Type::Var: 
        return arg.GetVar<Expr::Type::Var>();
    case Expr::Type::Str: 
    {
        const auto name = arg.GetVar<Expr::Type::Str>();
        const auto res  = NailangParser::VerifyVariableName(name);
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


NailangFrameStack::FrameHolder<NailangFrame> NailangExecutor::PushFrame(std::shared_ptr<EvaluateContext> ctx, NailangFrame::FrameFlags flag)
{
    return Runtime->FrameStack.PushFrame<NailangFrame>(this, ctx, flag);
}
NailangFrameStack::FrameHolder<NailangBlockFrame> NailangExecutor::PushBlockFrame(std::shared_ptr<EvaluateContext> ctx, NailangFrame::FrameFlags flag, 
    const Block* block, common::span<const FuncCall> metas)
{
    return Runtime->FrameStack.PushFrame<NailangBlockFrame>(this, ctx, flag, block, metas);
}
NailangFrameStack::FrameHolder<NailangRawBlockFrame> NailangExecutor::PushRawBlockFrame(std::shared_ptr<EvaluateContext> ctx, NailangFrame::FrameFlags flag, 
    const RawBlock* block, common::span<const FuncCall> metas)
{
    return Runtime->FrameStack.PushFrame<NailangRawBlockFrame>(this, ctx, flag, block, metas);
}

void NailangExecutor::EvalFuncArgs(const FuncCall& func, Arg* args, const Arg::Type* types, const size_t size, size_t offset)
{
    for (size_t i = 0, j = offset; i < size; ++i, ++j)
    {
        args[i] = EvaluateExpr(func.Args[j]);
        if (types != nullptr)
            ThrowByParamType(func, args[i], types[i], j);
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

bool NailangExecutor::HandleMetaFuncs(MetaSet& allMetas)
{
    std::u32string_view metaName;
    FuncName::FuncInfo metaFlag;
    size_t argOffset = 0;
    for (const auto& meta : allMetas.OriginalMetas)
    {
        Expects(meta.Name->IsMeta());
        metaName = meta.FullFuncName();
        metaFlag = meta.Name->Info();
        argOffset = 0;
        while (true)
        {
            const bool isPostMeta = metaFlag == FuncName::FuncInfo::PostMeta;
            if (!isPostMeta && metaName == U"MetaIf")
            {
                ThrowByArgTypes<2, ArgLimits::AtLeast>(meta, { Expr::Type::Empty, Expr::Type::Str }, argOffset);
                if (ThrowIfNotBool(EvaluateExpr(meta.Args[argOffset]), U""))
                {
                    metaName = meta.Args[argOffset + 1].GetVar<Expr::Type::Str>();
                    metaFlag = FuncName::PrepareFuncInfo(metaName, FuncName::FuncInfo::Meta);
                    if (metaName.empty())
                        NLRT_THROW_EX(u"Does not allow empty function name"sv, meta);
                    argOffset += 2;
                    continue;
                }
                else
                {
                    break;
                }
            }
            if (argOffset != 0)
                allMetas.AllocateMeta(isPostMeta, metaName, metaFlag, meta.Args.subspan(argOffset), meta.Position);
            else
                allMetas.AllocateMeta(isPostMeta, meta);
            break;
        }
    }
    allMetas.PrepareLiveMetas();
    for (auto meta : allMetas.LiveMetas())
    {
        if (meta.CheckFlag())
            continue;

        auto result = HandleMetaFunc(static_cast<const FuncCall&>(*meta), allMetas);
        if (result == MetaFuncResult::Unhandled)
        {
            allMetas.FillFuncPack(*meta, *this);
            result = HandleMetaFunc(*meta, allMetas);
        }
        meta.SetFlag(result != MetaFuncResult::Unhandled);
        switch (result)
        {
        case MetaFuncResult::Return:    Runtime->FrameStack.SetReturn({}); return false;
        case MetaFuncResult::Skip:      return false;
        default:                        break;
        }
    }
    // fill args of PostMeta
    for (auto& pack : allMetas.PostMetas())
    {
        allMetas.FillFuncPack(pack, *this);
    }
    return true;
}
NailangExecutor::MetaFuncResult NailangExecutor::HandleMetaFunc(const FuncCall& meta, MetaSet& allMetas)
{
    const auto metaName = meta.FullFuncName();
    if (metaName == U"While")
    {
        ThrowIfStatement(meta, allMetas.Target, Statement::Type::RawBlock);
        ThrowByArgCount(meta, 1);
        const auto& condition = meta.Args[0];

        auto frame = PushFrame(Runtime->ConstructEvalContext(), NailangFrame::FrameFlags::LoopScope);
        while (true)
        {
            if (const auto cond = EvaluateExpr(condition); ThrowIfNotBool(cond, U"Arg of MetaFunc[While]"))
            {
                Expects(frame->Status == NailangFrame::ProgramStatus::Next);
                HandleContent(allMetas.Target, {});
                if (frame->Has(NailangFrame::ProgramStatus::LoopEnd))
                    break;
                frame->Status = NailangFrame::ProgramStatus::Next; // reset state
            }
            else
                break;
        }
        return MetaFuncResult::Skip;
    }
    else if (metaName == U"DefFunc")
    {
        ThrowIfNotStatement(meta, allMetas.Target, Statement::Type::Block);
        for (const auto& arg : meta.Args)
        {
            if (arg.TypeData != Expr::Type::Var)
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg must be [LateBindVar]"sv, arg, meta);
            const auto& var = arg.GetVar<Expr::Type::Var>();
            if (HAS_FIELD(var.Info, LateBindVar::VarInfo::PrefixMask))
                NLRT_THROW_EX(u"MetaFunc[DefFunc]'s arg name must not contain [Root|Local] flag"sv, arg, meta);
        }
        std::vector<std::pair<std::u32string_view, Arg>> captures;
        for (auto m : allMetas.LiveMetas())
        {
            if (m->GetName() == U"Capture"sv)
            {
                ThrowByArgTypes<1, 2>(*m, { Expr::Type::Var, Expr::Type::Empty });
                auto argName = m->Args[0].GetVar<Expr::Type::Var>();
                if (m->Args.size() > 1 && HAS_FIELD(argName.Info, LateBindVar::VarInfo::PrefixMask))
                    NLRT_THROW_EX(u"MetaFunc[Capture]'s arg name must not contain [Root|Local] flag"sv, *m, meta);
                captures.emplace_back(argName.Name, EvaluateExpr(m->Args.back()));
                m.SetFlag(true);
            }
        }
        Runtime->SetFunc(allMetas.Target.Get<Block>(), captures, meta.Args);
        return MetaFuncResult::Skip;
    }
    return MetaFuncResult::Unhandled;
}
NailangExecutor::MetaFuncResult NailangExecutor::HandleMetaFunc(FuncPack& meta, MetaSet&)
{
    const auto metaName = meta.FullFuncName();
    if (metaName == U"Skip")
    {
        ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::BoolBit });
        return (meta.Params.empty() || meta.Params[0].GetBool().value()) ? MetaFuncResult::Skip : MetaFuncResult::Next;
    }
    else if (metaName == U"If")
    {
        ThrowByParamTypes<1>(meta, { Arg::Type::BoolBit });
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
        return EvaluateFunc(*fcall, nullptr);
    }
    case Type::Unary:
        return EvaluateUnaryExpr(*arg.GetVar<Type::Unary>());
    case Type::Binary:
        return EvaluateBinaryExpr(*arg.GetVar<Type::Binary>());
    case Type::Ternary:
        return EvaluateTernaryExpr(*arg.GetVar<Type::Ternary>());
    case Type::Query:
        return EvaluateQueryExpr(*arg.GetVar<Type::Query>());
    case Type::Var:     
        return Runtime->LookUpArg(arg.GetVar<Type::Var>());
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            assert(false); return {};
    }
}

Arg NailangExecutor::EvaluateFunc(const FuncCall& func, MetaSet* metas)
{
    Expects(func.Name->PartCount > 0);
    const auto fullName = func.FullFuncName();
    if (func.Name->IsMeta())
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
            const auto dst = Runtime->FrameStack.SetReturn(EvalFuncSingleArg(func, Arg::Type::Empty, ArgLimits::AtMost));
            if (!dst)
                NLRT_THROW_EX(u"[Return] can only be used inside FlowScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Break")
        {
            const auto dst = Runtime->FrameStack.SetBreak();
            if (!dst)
                NLRT_THROW_EX(u"[Break] can only be used inside LoopScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Continue")
        {
            const auto dst = Runtime->FrameStack.SetContinue();
            if (!dst)
                NLRT_THROW_EX(u"[Continue] can only be used inside LoopScope"sv, func);
            return {};
        }
        HashCase(fullName, U"Throw")
        {
            const auto arg = EvalFuncSingleArg(func, Arg::Type::Empty, ArgLimits::AtMost);
            Runtime->FrameStack.SetReturn({});
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
            return !Runtime->LocateArg(var, false).IsEmpty();
        }
        HashCase(name, U"Format")
        {
            ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::Empty });
            return FormatString(func.Params[0].GetStr().value(), func.Params.subspan(1));
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
        return EvaluateLocalFunc(lcFunc, func);
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
            const auto cmpRes = left.Compare(right);
            if (!cmpRes.HasOrderable())
                NLRT_THROW_EX(FMTSTR(u"Cannot get [orderable] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
            if (cmpRes.GetResult() == CompareResultCore::Less) 
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
            const auto cmpRes = left.Compare(right);
            if (!cmpRes.HasOrderable())
                NLRT_THROW_EX(FMTSTR(u"Cannot get [orderable] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
            if (cmpRes.GetResult() == CompareResultCore::Less)
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
    HashCase(mathName, U"ToFP")
    {
        ThrowByArgCount(func, 1);
        const auto arg = func.Params[0].GetFP();
        if (arg)
            return arg.value();
    } break;
    HashCase(mathName, U"ParseInt")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        int64_t ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToInt(str, ret).first)
            NLRT_THROW_EX(FMTSTR(u"Arg of [ParseInt] is not integer : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseUint")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        uint64_t ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToInt(str, ret).first)
            NLRT_THROW_EX(FMTSTR(u"Arg of [ParseUint] is not integer : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseFloat")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        double ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, false).first)
            NLRT_THROW_EX(FMTSTR(u"Arg of [ParseFloat] is not floatpoint : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseSciFloat")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        double ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, true).first)
            NLRT_THROW_EX(FMTSTR(u"Arg of [ParseFloat] is not scientific floatpoint : [{}]"sv, str), func);
        return ret;
    }
    default: return {};
    }
    NLRT_THROW_EX(FMTSTR(u"MathFunc [{}] cannot be evaluated.", func.FullFuncName()), func);
    //return {};
}
Arg NailangExecutor::EvaluateLocalFunc(const LocalFunc& func, FuncEvalPack& pack)
{
    ThrowByArgCount(pack, func.ArgNames.size() - func.CapturedArgs.size());
    auto newCtx = Runtime->ConstructEvalContext();
    size_t i = 0;
    for (; i < func.CapturedArgs.size(); ++i)
        newCtx->LocateArg(func.ArgNames[i], true).Set(func.CapturedArgs[i]);
    for (size_t j = 0; i < func.ArgNames.size(); ++i, ++j)
        newCtx->LocateArg(func.ArgNames[i], true).Set(std::move(pack.Params[j]));
    auto curFrame = PushBlockFrame(std::move(newCtx), NailangFrame::FrameFlags::FuncCall, func.Body, {});
    ExecuteFrame(*curFrame);
    return std::move(curFrame->ReturnArg);
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
        return !Runtime->LocateArg(expr.Operand.GetVar<Expr::Type::Var>(), false).IsEmpty();
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
        if (val.IsEmpty())
            return EvaluateExpr(expr.RightOperand);
        return val;
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
        return {};
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
        return {};
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
        return NailangHelper::ExtractArg(target, expr, *this);
    }
    catch (const NailangRuntimeException& nre)
    {
        Runtime->HandleException(nre);
        return {};
    }
}

void NailangExecutor::EvaluateAssign(const AssignExpr& assign, MetaSet*)
{
    const auto assignOp = assign.GetSelfAssignOp();
    std::variant<bool, EmbedOps> extra = false;
    if (assign.Queries.Size() > 0)
        extra = true;
    else if (assignOp)
        extra = assignOp.value();
    auto target = Runtime->LocateArgForWrite(assign.Target, assign.GetCheck(), extra);
    if (target.IsEmpty()) // skipped
        return;
    NailangHelper::LocateWrite(target, assign.Queries, *this, [&](Arg& dst)
        {
            if (!MATCH_FIELD(dst.GetAccess(), ArgAccess::Assignable))
                NLRT_THROW_EX(FMTSTR(u"Var [{}] is not assignable"sv, assign.Target));
            if (assign.IsSelfAssign)
            {
                if (!HAS_FIELD(dst.GetAccess(), ArgAccess::Readable))
                    NLRT_THROW_EX(FMTSTR(u"Var [{}] is not readable, request self assign on it"sv, assign.Target));
                auto tmp = dst;
                tmp.Decay();
                return dst.Set(tmp.HandleBinary(assignOp.value(), EvaluateExpr(assign.Statement)));
            }
            else
                return dst.Set(EvaluateExpr(assign.Statement));
        });
}

void NailangExecutor::EvaluateRawBlock(const RawBlock&, common::span<const FuncCall>)
{
    return; // just skip
}

void NailangExecutor::EvaluateBlock(const Block& block, common::span<const FuncCall> metas)
{
    auto curFrame = PushBlockFrame(Runtime->ConstructEvalContext(), NailangFrame::FrameFlags::Empty, &block, metas);
    ExecuteFrame(*curFrame);
}

void NailangExecutor::ExecuteFrame(NailangBlockFrame& frame)
{
    Expects(&GetFrame() == &frame);
    for (size_t idx = 0; idx < frame.BlockScope->Size();)
    {
        const auto& [metas, content] = (*frame.BlockScope)[idx];
        bool shouldExe = true;
        MetaSet allMetas(metas, content);
        if (!metas.empty())
        {
            shouldExe = HandleMetaFuncs(allMetas);
        }
        if (shouldExe)
        {
            frame.CurContent = &content;
            HandleContent(content, &allMetas);
        }
        frame.IfRecord >>= 2;
        if (frame.Has(NailangFrame::ProgramStatus::End))
            return;
        else
            idx++;
    }
    return;
}
void NailangExecutor::HandleContent(const Statement& content, MetaSet* metas)
{
    switch (content.TypeData)
    {
    case Statement::Type::Assign:
    {
        const auto& assign = *content.Get<AssignExpr>();
        EvaluateAssign(assign, metas);
    } break;
    case Statement::Type::Block:
    {
        EvaluateBlock(*content.Get<Block>(), MetaSet::GetOriginalMetas(metas));
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
        EvaluateRawBlock(*content.Get<RawBlock>(), MetaSet::GetOriginalMetas(metas));
    } break;
    default:
        assert(false); /*Expects(false);*/
        break;
    }
}

NailangExecutor::NailangExecutor(NailangRuntime* runtime) noexcept : Runtime(runtime)
{ }
NailangExecutor::~NailangExecutor() 
{ }
void NailangExecutor::HandleException(const NailangRuntimeException& ex) const
{
    Runtime->HandleException(ex);
}


NailangRuntime::NailangRuntime(std::shared_ptr<EvaluateContext> context) : 
    RootContext(std::move(context))
{ }
NailangRuntime::~NailangRuntime()
{ }

std::shared_ptr<EvaluateContext> NailangRuntime::GetContext(bool innerScope) const
{
    if (innerScope)
        return ConstructEvalContext();
    else if (auto frame = CurFrame(); frame)
        return frame->Context;
    else
        return RootContext;
}

void NailangRuntime::HandleException(const NailangRuntimeException& ex) const
{
    if (auto& info = ex.GetInfo(); !info.Scope)
    {
        if (const auto theFrame = dynamic_cast<NailangBlockFrame*>(CurFrame()); theFrame)
            info.Scope = *theFrame->CurContent;
    }
    auto stacks = FrameStack.CollectStacks();
    ex.Info->StackTrace.insert(ex.Info->StackTrace.begin(), 
        std::make_move_iterator(stacks.begin()), std::make_move_iterator(stacks.end()));
    throw ex;
}

Arg NailangRuntime::LocateArg(const LateBindVar& var, const bool create) const
{
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Root))
    {
        return RootContext->LocateArg(var, create);
    }
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Local))
    {
        if (auto theFrame = CurFrame(); theFrame)
            return theFrame->Context->LocateArg(var, create);
        else
            NLRT_THROW_EX(FMTSTR(u"LookUpLocalArg [{}] without frame", var));
        return {};
    }

    // Try find arg recursively
    for (auto frame = CurFrame(); frame; frame = frame->PrevFrame)
    {
        if (auto ret = frame->Context->LocateArg(var, create); !ret.IsEmpty())
            return ret;
        if (frame->Has(NailangFrame::FrameFlags::VarScope))
            break; // cannot beyond  
    }
    return RootContext->LocateArg(var, create);
}
Arg NailangRuntime::LocateArgForWrite(const LateBindVar& var, NilCheck nilCheck, std::variant<bool, EmbedOps> extra) const
{
    using Behavior = NilCheck::Behavior;
    if (auto ret = LocateArg(var, false); !ret.IsEmpty())
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
bool NailangRuntime::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const Expr> args)
{
    if (auto frame = CurFrame(); frame)
        return frame->Context->SetFunc(block, capture, args);
    NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    return false;
}
bool NailangRuntime::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args)
{
    if (auto frame = CurFrame(); frame)
        return frame->Context->SetFunc(block, capture, args);
    NLRT_THROW_EX(FMTSTR(u"SetFunc [{}] without frame", block->Name));
    return false;
}

std::shared_ptr<EvaluateContext> NailangRuntime::ConstructEvalContext() const
{
    return std::make_shared<CompactEvaluateContext>();
}

Arg NailangRuntime::LookUpArg(const LateBindVar& var, const bool checkNull) const
{
    auto ret = LocateArg(var, false);
    if (ret.IsEmpty())
    {
        if (checkNull)
            NLRT_THROW_EX(FMTSTR(u"Var [{}] does not exist", var));
        return {};
    }
    if (!HAS_FIELD(ret.GetAccess(), ArgAccess::Readable))
        NLRT_THROW_EX(FMTSTR(u"LookUpArg [{}] get a unreadable result", var));
    ret.Decay();
    return ret;
}

LocalFunc NailangRuntime::LookUpFunc(std::u32string_view name) const
{
    for (auto frame = CurFrame(); frame; frame = frame->PrevFrame)
    {
        auto func = frame->Context->LookUpFunc(name);
        if (func.Body != nullptr)
            return func;
    }
    return RootContext->LookUpFunc(name);
}


NailangBasicRuntime::NailangBasicRuntime(std::shared_ptr<EvaluateContext> context) :
    NailangRuntime(std::move(context)), Executor(this)
{ }
NailangBasicRuntime::~NailangBasicRuntime() 
{ }

void NailangBasicRuntime::ExecuteBlock(const Block& block, common::span<const FuncCall> metas, std::shared_ptr<EvaluateContext> ctx, const bool checkMetas)
{
    auto curFrame = Executor.PushBlockFrame(std::move(ctx), NailangFrame::FrameFlags::FlowScope, &block, metas);
    if (checkMetas && !metas.empty())
    {
        Statement target(&block);
        MetaSet allMetas(metas, target);
        if (!Executor.HandleMetaFuncs(allMetas))
            return;
    }
    curFrame->Execute();
}
Arg NailangBasicRuntime::EvaluateRawStatement(std::u32string_view content, const bool innerScope)
{
    common::parser::ParserContext context(content);
    const auto rawarg = NailangParser::ParseSingleExpr(MemPool, context, ""sv, U""sv);
    if (rawarg)
    {
        auto curFrame = Executor.PushFrame(GetContext(innerScope), NailangFrame::FrameFlags::FlowScope);
        return Executor.EvaluateExpr(rawarg);
    }
    else
        return {};
}


}
