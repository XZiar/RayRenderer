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
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::DJBHash;
using common::str::HashedStrView;


#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTIONEX(NailangRuntimeException, __VA_ARGS__))

COMMON_EXCEPTION_IMPL(NailangRuntimeException)

static std::u16string AppendString(std::u16string_view base, std::string_view add) noexcept
{
    return std::u16string(base).append(add.begin(), add.end());
}

COMMON_EXCEPTION_IMPL(NailangFormatException)
NailangFormatException::NailangFormatException(const std::u32string_view formatter, const std::runtime_error& err)
    : NailangRuntimeException(T_<ExceptionInfo>{}, AppendString(u"Error when formating string: {}"sv, err.what()), formatter)
{ }
NailangFormatException::NailangFormatException(const std::u32string_view formatter, const Arg* arg, const std::u16string_view reason)
    : NailangRuntimeException(T_<ExceptionInfo>{}, u"Error when formating string: {}"s.append(reason), formatter, 
        arg ? *arg : detail::ExceptionTarget{})
{ }

COMMON_EXCEPTION_IMPL(NailangCodeException)
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
        auto err = FMTSTR2(u"Require integer as indexer, get[{}]"sv, idx.GetTypeName());
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
    const auto ret = Host->TryDealloc({ reinterpret_cast<const std::byte*>(Frame), size }, true);
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
    return { fname, FMTSTR2(u"<Raw>{}"sv, block->Name), block->Position.first };
}
static common::StackTraceItem CreateStack(const Block* block, const common::SharedString<char16_t>& fname) noexcept
{
    if (!block)
        return { fname, u"<empty block>"sv, 0 };
    return { fname, FMTSTR2(u"<Block>{}"sv, block->Name), block->Position.first };
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
    NLRT_THROW_EX(FMTSTR2(u"Func [{}] requires {} [{}] args, which gives [{}].", call.FullFuncName(), prefix, count, call.Args.size()),
        detail::ExceptionTarget{}, call);
}
void NailangBase::ThrowByArgType(const FuncCall& func, const Expr::Type type, size_t idx) const
{
    const auto& arg = func.Args[idx];
    if (arg.TypeData != type && type != Expr::Type::Empty)
        NLRT_THROW_EX(FMTSTR2(u"Expected [{}] for [{}]'s {} arg-expr, which gives [{}], source is [{}].",
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
    NLRT_THROW_EX(FMTSTR2(u"Expected [{}] for [{}]'s {} arg, which gives [{}], source is [{}].",
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
        NLRT_THROW_EX(FMTSTR2(u"Func [{}] only support as [{}], get[{}].", call.FullFuncName(), FuncTypeName(type), FuncTypeName(call.Name->Info())),
            call);
    }
}
void NailangBase::ThrowIfStatement(const FuncCall& meta, const Statement& target, const Statement::Type type) const
{
    if (target.TypeData == type)
        NLRT_THROW_EX(FMTSTR2(u"Metafunc [{}] cannot be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}
void NailangBase::ThrowIfNotStatement(const FuncCall& meta, const Statement& target, const Statement::Type type) const
{
    if (target.TypeData != type)
        NLRT_THROW_EX(FMTSTR2(u"Metafunc [{}] can only be appllied to [{}].", meta.FullFuncName(), ContentTypeName(type)),
            meta);
}
bool NailangBase::ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const
{
    const auto ret = arg.GetBool();
    if (!ret.has_value())
        NLRT_THROW_EX(FMTSTR2(u"[{}] should be bool-able, which is [{}].", varName, arg.GetTypeName()),
            arg);
    return ret.value();
}

struct NailangFormatExecutor final : public common::str::CombinedExecutor<char32_t, common::str::Formatter<char32_t>>
{
    using Fmter = common::str::Formatter<char32_t>;
    using Base  = common::str::CombinedExecutor<char32_t, Fmter>;
    using CTX   = common::str::FormatterExecutor::Context;
    struct Context : public Base::Context
    {
        common::span<const Arg> Args;
        constexpr Context(std::u32string& dst, std::u32string_view fmtstr, common::span<const Arg> args) noexcept : 
            Base::Context(dst, fmtstr), Args(args) { }
    };
    /*void OnColor(CTX& ctx, common::ScreenColor color) final
    {
        auto& context = static_cast<Context&>(ctx);
        PutColor(context.Dst, color);
    }*/
    void OnArg(CTX& ctx, uint8_t argIdx, bool isNamed, const common::str::FormatSpec* spec) final
    {
        Expects(!isNamed);
        auto& context = static_cast<Context&>(ctx);
        const auto& arg = context.Args[argIdx];
        switch (arg.TypeData)
        {
        case Arg::Type::Bool:   Fmter::PutString (context.Dst, arg.GetVar<Arg::Type::Bool>() ? U"true"sv : U"false"sv, spec); break;
        case Arg::Type::Uint:   Fmter::PutInteger(context.Dst, arg.GetVar<Arg::Type::Uint, false>(), false, spec); break;
        case Arg::Type::Int:    Fmter::PutInteger(context.Dst, arg.GetVar<Arg::Type::Uint, false>(),  true, spec); break;
        case Arg::Type::FP:     Fmter::PutFloat  (context.Dst, arg.GetVar<Arg::Type::FP>(), spec); break;
        case Arg::Type::U32Sv:  Fmter::PutString (context.Dst, arg.GetVar<Arg::Type::U32Sv>(), spec); break;
        case Arg::Type::U32Str: Fmter::PutString (context.Dst, arg.GetVar<Arg::Type::U32Str>(), spec); break;
        default: break;
        }
    }
    using FormatterBase::Execute;
};
static NailangFormatExecutor NLFmtExecutor;

std::u32string NailangBase::FormatString(const std::u32string_view formatter, common::span<const Arg> args)
{
    using namespace common::str;
    const auto result = ParseResult::ParseString<char32_t>(formatter);
    try
    {
        ParseResult::CheckErrorRuntime(result.ErrorPos, result.OpCount);
        const auto strInfo = result.ToInfo(formatter);
        ArgInfo argsInfo;
        for (const auto& arg : args)
        {
            switch (arg.TypeData)
            {
            case Arg::Type::Bool:   ArgInfo::ParseAnArg<bool                >(argsInfo); break;
            case Arg::Type::Uint:   ArgInfo::ParseAnArg<uint64_t            >(argsInfo); break;
            case Arg::Type::Int:    ArgInfo::ParseAnArg<int64_t             >(argsInfo); break;
            case Arg::Type::FP:     ArgInfo::ParseAnArg<double              >(argsInfo); break;
            case Arg::Type::U32Sv:  ArgInfo::ParseAnArg<std::u32string_view >(argsInfo); break;
            case Arg::Type::U32Str: ArgInfo::ParseAnArg<std::u32string_view >(argsInfo); break;
            default: HandleException(CREATE_EXCEPTIONEX(NailangFormatException, formatter, &arg, u"Unsupported DataType")); break;
            }
        }
        /*const auto mapping = */ArgChecker::CheckDD(strInfo, argsInfo);
        std::u32string dst;
        NailangFormatExecutor::Context ctx{ dst, formatter, args };
        uint32_t opOffset = 0;
        while (opOffset < strInfo.Opcodes.size())
        {
            NailangFormatExecutor::Execute<common::str::FormatterExecutor>(strInfo.Opcodes, opOffset, NLFmtExecutor, ctx);
        }
        return dst;
    }
    catch (const common::BaseException& be)
    {
        HandleException(CREATE_EXCEPTIONEX(NailangFormatException, formatter, nullptr, be.Message()));
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
        NLRT_THROW_EX(FMTSTR2(u"[{}] only accept [Var]/[String], which gives [{}].", reciever, arg.GetTypeName()),
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
    EvalTempStore store;
    for (size_t i = 0, j = offset; i < size; ++i, ++j)
    {
        args[i] = EvaluateExpr(func.Args[j], store);
        if (types != nullptr)
            ThrowByParamType(func, args[i], types[i], j);
        store.Reset();
    }
}
std::vector<Arg> NailangExecutor::EvalFuncAllArgs(const FuncCall& func)
{
    EvalTempStore store;
    std::vector<Arg> params;
    params.reserve(func.Args.size());
    for (const auto& arg : func.Args)
    {
        params.emplace_back(EvaluateExpr(arg, store));
        store.Reset();
    }
    return params;
}

bool NailangExecutor::HandleMetaFuncs(MetaSet& allMetas)
{
    std::u32string_view metaName;
    FuncName::FuncInfo metaFlag;
    size_t argOffset = 0;
    EvalTempStore store;
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
                if (ThrowIfNotBool(EvaluateExpr(meta.Args[argOffset], store), U""))
                {
                    metaName = meta.Args[argOffset + 1].GetVar<Expr::Type::Str>();
                    metaFlag = FuncName::PrepareFuncInfo(metaName, FuncName::FuncInfo::Meta);
                    if (metaName.empty())
                        NLRT_THROW_EX(u"Does not allow empty function name"sv, meta);
                    argOffset += 2;
                    store.Reset();
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
        store.Reset();
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
        EvalTempStore store;
        while (true)
        {
            if (const auto cond = EvaluateExpr(condition, store); ThrowIfNotBool(cond, U"Arg of MetaFunc[While]"))
            {
                Expects(frame->Status == NailangFrame::ProgramStatus::Next);
                HandleContent(allMetas.Target, store, nullptr);
                if (frame->Has(NailangFrame::ProgramStatus::LoopEnd))
                    break;
                frame->Status = NailangFrame::ProgramStatus::Next; // reset state
            }
            else
                break;
            store.Reset();
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
        EvalTempStore store;
        for (auto m : allMetas.LiveMetas())
        {
            if (m->GetName() == U"Capture"sv)
            {
                ThrowByArgTypes<1, 2>(*m, { Expr::Type::Var, Expr::Type::Empty });
                auto argName = m->Args[0].GetVar<Expr::Type::Var>();
                if (m->Args.size() > 1 && HAS_FIELD(argName.Info, LateBindVar::VarInfo::PrefixMask))
                    NLRT_THROW_EX(u"MetaFunc[Capture]'s arg name must not contain [Root|Local] flag"sv, *m, meta);
                captures.emplace_back(argName.Name, EvaluateExpr(m->Args.back(), store));
                m.SetFlag(true);
                store.Reset();
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

Arg NailangExecutor::EvaluateExpr(const Expr& arg, EvalTempStore& store, bool forWrite)
{
    using Type = Expr::Type;
    switch (arg.TypeData)
    {
    case Type::Assign:
        NLRT_THROW_EX(u"Should not evluate assign expr as plain expr"sv);
    case Type::Func:
    {
        const auto& fcall = arg.GetVar<Type::Func>();
        Expects(fcall->Name->Info() == FuncName::FuncInfo::ExprPart);
        return EvaluateFunc(*fcall, store, nullptr);
    }
    case Type::Query:
    {
        const auto& query = *arg.GetVar<Type::Query>();
        auto source = EvaluateExpr(query.Target, store);
        if (forWrite && query.Target.TypeData == Expr::Type::Query) // query expr under [forWrite] always return non-decayed
            source.Decay(); 
        if (query.TypeData == QueryExpr::QueryType::Index)
        {
            boost::container::small_vector<Arg, 4> indexes;
            indexes.reserve(query.Count);
            const auto rawQueries = query.GetQueries();
            for (const auto& item : rawQueries)
                indexes.emplace_back(EvaluateExpr(item, store));
            return EvaluateQuery<Arg, &Arg::HandleIndexes>(std::move(source), { rawQueries, indexes }, store, forWrite);
        }
        else
        {
            const auto rawQueries = query.GetQueries();
            return EvaluateQuery<const Expr, &Arg::HandleSubFields>(std::move(source), { rawQueries, rawQueries }, store, forWrite);
        }
    }
    case Type::Unary:
        return EvaluateUnaryExpr(*arg.GetVar<Type::Unary>(), store);
    case Type::Binary:
        return EvaluateBinaryExpr(*arg.GetVar<Type::Binary>(), store);
    case Type::Ternary:
        return EvaluateTernaryExpr(*arg.GetVar<Type::Ternary>(), store);
    case Type::Var:
        // direct-assign has been handled seperately, always no need too create new var
        return Runtime->LookUpArg(arg.GetVar<Type::Var>());
    case Type::Str:     return arg.GetVar<Type::Str>();
    case Type::Uint:    return arg.GetVar<Type::Uint>();
    case Type::Int:     return arg.GetVar<Type::Int>();
    case Type::FP:      return arg.GetVar<Type::FP>();
    case Type::Bool:    return arg.GetVar<Type::Bool>();
    default:            assert(false); return {};
    }
}

Arg NailangExecutor::EvaluateFunc(const FuncCall& func, EvalTempStore& store, MetaSet* metas)
{
    Expects(func.Name->PartCount > 0);
    const auto fullName = func.FullFuncName();
    if (func.Name->IsMeta())
    {
        NLRT_THROW_EX(FMTSTR2(u"MetaFunc [{}] can not be evaluated here.", fullName), func);
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
                NLRT_THROW_EX(FMTSTR2(u"Cannot get [orderable] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
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
                NLRT_THROW_EX(FMTSTR2(u"Cannot get [orderable] on type [{}],[{}]"sv, left.GetTypeName(), right.GetTypeName()), func);
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
            NLRT_THROW_EX(FMTSTR2(u"Arg of [ParseInt] is not integer : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseUint")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        uint64_t ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToInt(str, ret).first)
            NLRT_THROW_EX(FMTSTR2(u"Arg of [ParseUint] is not integer : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseFloat")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        double ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, false).first)
            NLRT_THROW_EX(FMTSTR2(u"Arg of [ParseFloat] is not floatpoint : [{}]"sv, str), func);
        return ret;
    }
    HashCase(mathName, U"ParseSciFloat")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        double ret = 0;
        if (const auto str = common::str::to_string(func.Params[0].GetStr().value()); !common::StrToFP(str, ret, true).first)
            NLRT_THROW_EX(FMTSTR2(u"Arg of [ParseFloat] is not scientific floatpoint : [{}]"sv, str), func);
        return ret;
    }
    default: return {};
    }
    NLRT_THROW_EX(FMTSTR2(u"MathFunc [{}] cannot be evaluated.", func.FullFuncName()), func);
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
    NLRT_THROW_EX(FMTSTR2(u"Func [{}] with [{}] args cannot be resolved.", func.FullFuncName(), func.Args.size()), func);
    //return {};
}

Arg NailangExecutor::EvaluateUnaryExpr(const UnaryExpr& expr, EvalTempStore& store)
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
        auto val = EvaluateExpr(expr.Operand, store);
        auto ret = val.HandleUnary(expr.Operator);
        if (ret.IsEmpty())
            NLRT_THROW_EX(FMTSTR2(u"Cannot perform unary expr [{}] on type [{}]"sv,
                EmbedOpHelper::GetOpName(expr.Operator), val.GetTypeName()), Expr(&expr));
        return ret;
    }
    default:
        NLRT_THROW_EX(FMTSTR2(u"Unexpected unary op [{}]"sv, EmbedOpHelper::GetOpName(expr.Operator)), Expr(&expr));
        //return {};
    }
}

Arg NailangExecutor::EvaluateBinaryExpr(const BinaryExpr& expr, EvalTempStore& store)
{
    if (expr.Operator == EmbedOps::ValueOr)
    {
        Expects(expr.LeftOperand.TypeData == Expr::Type::Var);
        auto val = Runtime->LookUpArg(expr.LeftOperand.GetVar<Expr::Type::Var>(), false); // skip null check, but require read
        if (val.IsEmpty())
            return EvaluateExpr(expr.RightOperand, store);
        return val;
    }
    Arg left = EvaluateExpr(expr.LeftOperand, store), right;
    if (expr.Operator == EmbedOps::And)
    {
        if (const auto l = left.GetBool(); l.has_value())
        {
            if (!l.value()) return false;
            right = EvaluateExpr(expr.RightOperand, store);
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
            right = EvaluateExpr(expr.RightOperand, store);
            if (const auto r = right.GetBool(); r.has_value())
                return r.value();
        }
        return {};
    }
    if (right.IsEmpty())
        right = EvaluateExpr(expr.RightOperand, store);
    Arg ret = left.HandleBinary(expr.Operator, right);
    if (!ret.IsEmpty())
        return ret;
    NLRT_THROW_EX(FMTSTR2(u"Cannot perform binary expr [{}] on type [{}],[{}]"sv,
        EmbedOpHelper::GetOpName(expr.Operator), left.GetTypeName(), right.GetTypeName()), Expr(&expr));
    //return {};
}

Arg NailangExecutor::EvaluateTernaryExpr(const TernaryExpr& expr, EvalTempStore& store)
{
    const auto cond = ThrowIfNotBool(EvaluateExpr(expr.Condition, store), U"condition of ternary operator"sv);
    const auto& selected = cond ? expr.LeftOperand : expr.RightOperand;
    return EvaluateExpr(selected, store);
}

static std::u16string_view EvaluateQueryCheck(Arg& next, bool forWrite)
{
    if (next.IsEmpty())
        return u"Does not exists"sv;
    if (forWrite && !MATCH_FIELD(next.GetAccess(), ArgAccess::Mutable))
        return u"Can not assgin to immutable's";
    else if (!forWrite && !HAS_FIELD(next.GetAccess(), ArgAccess::Readable))
        return u"Can not access an get-host's";
    return {};
}

template<typename T, Arg(Arg::* F)(SubQuery<T>&)>
Arg NailangExecutor::EvaluateQuery(Arg target, SubQuery<T> query, EvalTempStore& store, bool forWrite)
{
    Expects(query.Size() > 0);
    while (true)
    {
        auto next = (target.*F)(query);
        if (const auto errMsg = EvaluateQueryCheck(next, forWrite); !errMsg.empty())
        {
            NLRT_THROW_EX(FMTSTR2(u"{} [{}]", errMsg, Serializer::Stringify(query)));
            /*const auto queryStr = Serializer::Stringify(query);
            ThrowEvaluateQueryException(*this, errMsg, queryStr);*/
        }
        if (query.Size() > 0)
        {
            next.Decay();
            store.Put(std::move(target));
            target = std::move(next);
        }
        else
        {
            if (!forWrite)
                next.Decay();
            return next;
        }
    }
}

void NailangExecutor::EvaluateAssign(const AssignExpr& assign, EvalTempStore& store, MetaSet*)
{
    const auto assignOp = assign.GetSelfAssignOp();
    Arg target;
    if (assign.Target.TypeData == Expr::Type::Var)
    {
        target = Runtime->LocateArgForWrite(assign.Target.GetVar<Expr::Type::Var>(), assign.GetCheck(), {});
        if (target.IsEmpty()) // skipped
            return;
    }
    else
    {
        target = EvaluateExpr(assign.Target, store, true);
    }
    if (!MATCH_FIELD(target.GetAccess(), ArgAccess::Assignable))
        NLRT_THROW_EX(FMTSTR2(u"Target [{}] is not assignable"sv, Serializer::Stringify(assign.Target)));
    bool assignRet = false;
    if (assign.IsSelfAssign)
    {
        if (!HAS_FIELD(target.GetAccess(), ArgAccess::Readable))
            NLRT_THROW_EX(FMTSTR2(u"Target [{}] is not readable, request self assign on it"sv, Serializer::Stringify(assign.Target)));
        auto tmp = target;
        tmp.Decay();
        assignRet = target.Set(tmp.HandleBinary(assignOp.value(), EvaluateExpr(assign.Statement, store)));
    }
    else
        assignRet = target.Set(EvaluateExpr(assign.Statement, store));
    if (!assignRet)
        NLRT_THROW_EX(FMTSTR2(u"Assign to target [{}] fails"sv, Serializer::Stringify(assign.Target)));
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
    EvalTempStore store;
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
            store.Reset();
            HandleContent(content, store, &allMetas);
        }
        frame.IfRecord >>= 2;
        if (frame.Has(NailangFrame::ProgramStatus::End))
            return;
        else
            idx++;
    }
    return;
}
void NailangExecutor::HandleContent(const Statement& content, EvalTempStore& store, MetaSet* metas)
{
    switch (content.TypeData)
    {
    case Statement::Type::Assign:
    {
        const auto& assign = *content.Get<AssignExpr>();
        EvaluateAssign(assign, store, metas);
    } break;
    case Statement::Type::Block:
    {
        EvaluateBlock(*content.Get<Block>(), metas ? metas->OriginalMetas : common::span<const FuncCall>{});
    } break;
    case Statement::Type::FuncCall:
    {
        const auto fcall = content.Get<FuncCall>();
        Expects(fcall->Name->Info() == FuncName::FuncInfo::Empty);
        //ExprHolder callHost(this, fcall);
        [[maybe_unused]] const auto dummy = EvaluateFunc(*fcall, store, metas);
    } break;
    case Statement::Type::RawBlock:
    {
        EvaluateRawBlock(*content.Get<RawBlock>(), metas ? metas->OriginalMetas : common::span<const FuncCall>{});
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
    auto& info = ex.GetInfo();
    if (!info.Scope)
    {
        if (const auto theFrame = dynamic_cast<NailangBlockFrame*>(CurFrame()); theFrame)
            info.Scope = *theFrame->CurContent;
    }
    auto stacks = FrameStack.CollectStacks();
    info.StackTrace.insert(info.StackTrace.begin(), std::make_move_iterator(stacks.begin()), std::make_move_iterator(stacks.end()));
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
            NLRT_THROW_EX(FMTSTR2(u"LookUpLocalArg [{}] without frame", var.Name));
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
            NLRT_THROW_EX(FMTSTR2(u"Var [{}] already exists"sv, var.Name));
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
                NLRT_THROW_EX(FMTSTR2(u"Var [{}] does not exists, expect perform [{}] on it"sv, 
                    var.Name, EmbedOpHelper::GetOpName(std::get<1>(extra))));
            else
                NLRT_THROW_EX(FMTSTR2(u"Var [{}] does not exists{}"sv, 
                    var.Name, std::get<0>(extra) ? u""sv : u", expect perform subquery on it"sv));
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
    NLRT_THROW_EX(FMTSTR2(u"SetFunc [{}] without frame", block->Name));
    return false;
}
bool NailangRuntime::SetFunc(const Block* block, common::span<std::pair<std::u32string_view, Arg>> capture, common::span<const std::u32string_view> args)
{
    if (auto frame = CurFrame(); frame)
        return frame->Context->SetFunc(block, capture, args);
    NLRT_THROW_EX(FMTSTR2(u"SetFunc [{}] without frame", block->Name));
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
            NLRT_THROW_EX(FMTSTR2(u"Var [{}] does not exist", var.Name));
        return {};
    }
    if (!HAS_FIELD(ret.GetAccess(), ArgAccess::Readable))
        NLRT_THROW_EX(FMTSTR2(u"LookUpArg [{}] get a unreadable result", var.Name));
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
        EvalTempStore store;
        auto curFrame = Executor.PushFrame(GetContext(innerScope), NailangFrame::FrameFlags::FlowScope);
        return Executor.EvaluateExpr(rawarg, store);
    }
    else
        return {};
}


}
