#include "XCompNailang.h"
#include "StringUtil/Convert.h"
#include "common/StrParsePack.hpp"

namespace xcomp
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::RawArg;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::BlockContent;
using xziar::nailang::FuncCall;
using xziar::nailang::ArgLimits;
using xziar::nailang::FrameFlags;
using xziar::nailang::NailangRuntimeBase;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::mlog::LogLevel;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;
using FuncInfo = xziar::nailang::FuncName::FuncInfo;


#define NLRT_THROW_EX(...) this->HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)



struct TemplateBlockInfo : public OutputBlock::BlockInfo
{
    std::vector<std::u32string_view> ReplaceArgs;
    TemplateBlockInfo(common::span<const xziar::nailang::RawArg> args)
    {
        ReplaceArgs.reserve(args.size());
        for (const auto& arg : args)
        {
            ReplaceArgs.emplace_back(*arg.GetVar<RawArg::Type::Var>());
        }
    }
    ~TemplateBlockInfo() override {}
};


std::u32string_view OutputBlock::BlockTypeName(const BlockType type) noexcept
{
    switch (type)
    {
    case BlockType::Global:     return U"Global"sv;
    case BlockType::Struct:     return U"Struct"sv;
    case BlockType::Instance:   return U"Instance"sv;
    case BlockType::Template:   return U"Template"sv;
    default:                    return U"Unknwon"sv;
    }
}


bool InstanceContext::Add(std::vector<NamedText>& container, const std::u32string_view id, std::u32string_view content)
{
    for (const auto& item : container)
        if (item.ID == id)
            return false;
    container.push_back(NamedText{ std::u32string(id), std::u32string(content) });
    return true;
}

void InstanceContext::Write(std::u32string& output, const NamedText& item)
{
    APPEND_FMT(output, U"    //vvvvvvvv below injected by {}  vvvvvvvv\r\n"sv, item.ID);
    output.append(item.Content).append(U"\r\n"sv);
    APPEND_FMT(output, U"    //^^^^^^^^ above injected by {}  ^^^^^^^^\r\n\r\n"sv, item.ID);
}



XCNLExtension::XCNLExtension(XCNLContext& context) : Context(context), ID(UINT32_MAX)
{ }
XCNLExtension::~XCNLExtension() { }




static uint32_t RegId() noexcept
{
    static std::atomic<uint32_t> ID = 0;
    return ID++;
}
static auto& XCNLExtGenerators() noexcept
{
    static std::vector<std::pair<XCNLExtension::XCNLExtGen, uint32_t>> container;
    return container;
}
uint32_t XCNLExtension::RegisterXCNLExtension(XCNLExtGen creator) noexcept
{
    const auto id = RegId();
    XCNLExtGenerators().emplace_back(std::move(creator), id);
    return id;
}


XCNLContext::XCNLContext(const common::CLikeDefines& info)
{
    for (const auto [key, val] : info)
    {
        const auto varName = common::str::to_u32string(key, Charset::UTF8);
        const auto var = xziar::nailang::LateBindVar::CreateSimple(varName);
        switch (val.index())
        {
        case 1: SetArg(var, std::get<1>(val), true); break;
        case 2: SetArg(var, std::get<2>(val), true); break;
        case 3: SetArg(var, std::get<3>(val), true); break;
        case 4: SetArg(var, common::str::to_u32string(std::get<4>(val), Charset::UTF8), true); break;
        case 0:
        default:
            break;
        }
    }
}
XCNLContext::~XCNLContext()
{ }


XCNLRuntime::XCNLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<XCNLContext> evalCtx) :
    NailangRuntimeBase(evalCtx), XCContext(*evalCtx), Logger(logger)
{ }
XCNLRuntime::~XCNLRuntime()
{ }

void XCNLRuntime::HandleException(const xziar::nailang::NailangParseException& ex) const
{
    Logger.error(u"{}\n", ex.Message());
    ReplaceEngine::HandleException(ex);
}

void XCNLRuntime::HandleException(const NailangRuntimeException& ex) const
{
    Logger.error(u"{}\n", ex.Message());
    NailangRuntimeBase::HandleException(ex);
}

void XCNLRuntime::ThrowByReplacerArgCount(const std::u32string_view call, const common::span<const std::u32string_view> args,
    const size_t count, const ArgLimits limit) const
{
    std::u32string_view prefix;
    switch (limit)
    {
    case ArgLimits::Exact:
        if (args.size() == count) return;
        prefix = U""; break;
    case ArgLimits::AtMost:
        if (args.size() <= count) return;
        prefix = U"at most"; break;
    case ArgLimits::AtLeast:
        if (args.size() >= count) return;
        prefix = U"at least"; break;
    }
    NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [{}] requires {} [{}] args, which gives [{}]."sv, call, prefix, count, args.size()));
}

void XCNLRuntime::InnerLog(common::mlog::LogLevel level, std::u32string_view str)
{
    Logger.log(level, u"[XCNL]{}\n", str);
}

common::simd::VecDataInfo XCNLRuntime::ParseVecType(const std::u32string_view var,
    std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo) const
{
    const auto vtype = XCContext.ParseVecType(var);
    if (!vtype)
    {
        switch (extraInfo.index())
        {
        case 0:
        {
            const auto str = std::get<0>(extraInfo);
            if (str.size() == 0)
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType."sv, var));
            else
                NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType when {}."sv, var, str));
        } break;
        case 1:
        {
            const auto str = std::get<1>(extraInfo)();
            NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized as VecType, {}."sv, var, str));
        } break;
        }
    }
    if (!vtype.TypeSupport)
        Logger.warning(u"Potential use of unsupported type[{}] with [{}].\n", StringifyVDataType(vtype.Info), var);

    return vtype.Info;
}

std::u32string_view XCNLRuntime::GetVecTypeName(const std::u32string_view vname) const
{
    const auto vtype = ParseVecType(vname, u"call [GetVecTypeName]"sv);
    const auto str = XCContext.GetVecTypeName(vtype);
    if (!str.empty())
        return str;
    else
        NLRT_THROW_EX(FMTSTR(u"Type [{}] not supported"sv, vname));
    return {};
}

constexpr auto LogLevelParser = SWITCH_PACK(Hash, 
    (U"error",   common::mlog::LogLevel::Error), 
    (U"success", common::mlog::LogLevel::Success),
    (U"warning", common::mlog::LogLevel::Warning),
    (U"info",    common::mlog::LogLevel::Info),
    (U"verbose", common::mlog::LogLevel::Verbose), 
    (U"debug",   common::mlog::LogLevel::Debug));
std::optional<common::mlog::LogLevel> ParseLogLevel(const Arg& arg) noexcept
{
    const auto str = arg.GetStr();
    if (str)
        return LogLevelParser(str.value());
    return {};
}
std::optional<Arg> XCNLRuntime::CommonFunc(const std::u32string_view name, const FuncCall& call, MetaFuncs)
{
    switch (hash_(name))
    {
    HashCase(name, U"GetVecTypeName")
    {
        const auto arg = EvaluateFuncArgs<1>(call, { Arg::Type::String })[0];
        return GetVecTypeName(arg.GetStr().value());
    }
    HashCase(name, U"Log")
    {
        ThrowIfNotFuncTarget(call, FuncInfo::Empty);
        const auto args2 = EvaluateFuncArgs<2, ArgLimits::AtLeast>(call, { Arg::Type::String, Arg::Type::String });
        const auto logLevel = ParseLogLevel(args2[0]);
        if (!logLevel)
            NLRT_THROW_EX(u"Arg[0] of [Log] should be LogLevel"sv, call);
        const auto fmtStr = args2[1].GetStr().value();
        if (call.Args.size() == 2)
            InnerLog(logLevel.value(), fmtStr);
        else
        {
            try
            {
                const auto str = FormatString(fmtStr, call.Args.subspan(2));
                InnerLog(logLevel.value(), str);
            }
            catch (const xziar::nailang::NailangFormatException& nfe)
            {
                Logger.error(u"Error when formating inner log: {}\n", nfe.Message());
            }
        }
    } return Arg{};
    default: break;
    }
    return {};
}

std::optional<common::str::StrVariant<char32_t>> XCNLRuntime::CommonReplaceFunc(const std::u32string_view name, const std::u32string_view call,
    const common::span<const std::u32string_view> args, BlockCookie& cookie)
{
    switch (hash_(name))
    {
    HashCase(name, U"GetVecTypeName")
    {
        ThrowByReplacerArgCount(call, args, 1, ArgLimits::Exact);
        return GetVecTypeName(args[0]);
    }
    HashCase(name, U"CodeBlock")
    {
        ThrowByReplacerArgCount(call, args, 1, ArgLimits::AtLeast);
        const OutputBlock* block = nullptr;
        for (const auto& blk : XCContext.TemplateBlocks)
            if (blk.Block->Name == args[0])
            {
                block = &blk; break;
            }
        if (block)
        {
            const auto& extra = static_cast<const TemplateBlockInfo&>(*block->ExtraInfo);
            ThrowByReplacerArgCount(call, args, extra.ReplaceArgs.size() + 1, ArgLimits::AtLeast);
            auto frame = PushFrame(extra.ReplaceArgs.size() > 0, FrameFlags::FlowScope);
            for (size_t i = 0; i < extra.ReplaceArgs.size(); ++i)
            {
                const auto var_ = DecideDynamicVar(extra.ReplaceArgs[i], u"CodeBlock"sv);
                {
                    using xziar::nailang::LateBindVar;
                    auto& var2 = const_cast<LateBindVar&>(var_.Get());
                    var2.Info() |= LateBindVar::VarInfo::Local;
                }
                const auto ret = LookUpArg(var_);
                SetArg(var_, args[i + 1]);
            }
            auto output = FMTSTR(U"// template block [{}]\r\n"sv, args[0]);
            NestedCookie newCookie(cookie, *block);
            DirectOutput(newCookie, output);
            return output;
        }
        break;
    }
    default: break;
    }
    return {};
}

void XCNLRuntime::OutputConditions(MetaFuncs metas, std::u32string& dst) const
{
    // std::set<std::u32string_view> lateVars;
    for (const auto& meta : metas)
    {
        std::u32string_view prefix;
        if (*meta.Name == U"If"sv)
            prefix = U"If "sv;
        else if (*meta.Name == U"Skip"sv)
            prefix = U"Skip if "sv;
        else
            continue;
        if (meta.Args.size() == 1)
            APPEND_FMT(dst, U"    /* {:7} :  {} */\r\n"sv, prefix, xziar::nailang::Serializer::Stringify(meta.Args[0]));
    }
}

void XCNLRuntime::DirectOutput(BlockCookie& cookie, std::u32string& dst)
{
    Expects(CurFrame != nullptr);
    const auto& block = cookie.Block;
    OutputConditions(block.MetaFunc, dst);
    common::str::StrVariant<char32_t> source(block.Block->Source);
    for (const auto& [var, arg] : block.PreAssignArgs)
    {
        SetArg(*var, EvaluateArg(arg));
    }
    if (block.ReplaceVar || block.ReplaceFunc)
        source = ProcessOptBlock(source.StrView(), U"$$@"sv, U"@$$"sv);
    if (block.ReplaceVar)
        source = ProcessVariable(source.StrView(), U"$$!{"sv, U"}"sv);
    if (block.ReplaceFunc)
        source = ProcessFunction(source.StrView(), U"$$!"sv, U""sv, &cookie);
    dst.append(source.StrView());
}

void XCNLRuntime::OnReplaceOptBlock(std::u32string& output, void*, const std::u32string_view cond, const std::u32string_view content)
{
    if (cond.empty())
    {
        NLRT_THROW_EX(u"Empty condition occurred when replace-opt-block"sv);
        return;
    }
    Arg ret;
    if (cond.back() == U';')
    {
        ret = EvaluateRawStatement(cond);
    }
    else
    {
        std::u32string str(cond);
        str.push_back(U';');
        ret = EvaluateRawStatement(str);
    }
    if (ret.IsEmpty())
    {
        NLRT_THROW_EX(u"No value returned as condition when replace-opt-block"sv);
        return;
    }
    else if (!HAS_FIELD(ret.TypeData, Arg::Type::Boolable))
    {
        NLRT_THROW_EX(u"Evaluated condition is not boolable when replace-opt-block"sv);
        return;
    }
    else if (ret.GetBool().value())
    {
        output.append(content);
    }
}

void XCNLRuntime::OnReplaceVariable(std::u32string& output, [[maybe_unused]] void* cookie, const std::u32string_view var)
{
    if (var.size() > 0 && var[0] == U'@')
    {
        const auto vtype = ParseVecType(var.substr(1), u"replace-variable"sv);
        const auto str = XCContext.GetVecTypeName(vtype);
        if (!str.empty())
            output.append(str);
        else
            NLRT_THROW_EX(FMTSTR(u"Type [{}] not supported when replace-variable"sv, var));
    }
    else // '@' not allowed in var anyway
    {
        const auto var_ = DecideDynamicVar(var, u"ReplaceVariable"sv);
        const auto ret = LookUpArg(var_);
        if (ret.IsEmpty() || ret.TypeData == Arg::Type::Var)
        {
            NLRT_THROW_EX(FMTSTR(u"Arg [{}] not found when replace-variable"sv, var));
            return;
        }
        output.append(ret.ToString().StrView());
    }
}

void XCNLRuntime::OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (IsBeginWith(func, U"xcomp."sv))
    {
        const auto subName = func.substr(5);
        auto ret = CommonReplaceFunc(subName, func, args, *reinterpret_cast<BlockCookie*>(cookie));
        if (ret.has_value())
        {
            output.append(ret->StrView());
            return;
        }
    }
    
    for (const auto& ext : XCContext.Extensions)
    {
        if (!ext) continue;
        const auto ret = ext->ReplaceFunc(*this, func, args);
        if (ret)
        {
            output.append(ret.GetStr());
            return;
        }
        else
        {
            const auto str = ret.GetStr();
            if (!ret.CheckAllowFallback())
                NLRT_THROW_EX(FMTSTR(u"replace-func [{}] with [{}]args error: {}", func, args.size(), str));
            if (!str.empty())
                Logger.warning(FMT_STRING(u"when replace-func [{}]: {}"), func, str);
        }
    }
    NLRT_THROW_EX(FMTSTR(u"replace-func [{}] with [{}]args is not resolved", func, args.size()));
}

void XCNLRuntime::OnRawBlock(const RawBlock& block, MetaFuncs metas)
{
    ProcessRawBlock(block, metas);
}

Arg XCNLRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas)
{
    if (call.Name->PartCount == 2 && (*call.Name)[0] == U"xcomp"sv)
    {
        auto ret = CommonFunc(call.Name[1], call, metas);
        if (ret.has_value())
            return std::move(ret.value());
    }
    for (const auto& ext : XCContext.Extensions)
    {
        auto ret = ext->XCNLFunc(*this, call, metas);
        if (ret)
            return std::move(ret.value());
    }
    return NailangRuntimeBase::EvaluateFunc(call, metas);
}

InstanceContext XCNLRuntime::ProcessInstance(InstanceCookie& cookie)
{
    auto& kerCtx = cookie.Context;
    for (const auto& ext : XCContext.Extensions)
    {
        ext->BeginInstance(*this, kerCtx);
    }

    for (const auto& meta : cookie.Block.MetaFunc)
    {
        switch (const auto newMeta = HandleMetaIf(meta); newMeta.index())
        {
        case 0:     HandleInstanceMeta(meta, kerCtx); break;
        case 2:     HandleInstanceMeta({ std::get<2>(newMeta).Ptr(), meta.Args.subspan(2) }, kerCtx); break;
        default:    break;
        }
    }

    DirectOutput(cookie, kerCtx.Content);

    for (const auto& ext : XCContext.Extensions)
    {
        ext->FinishInstance(*this, kerCtx);
    }

    return std::move(kerCtx);
}

OutputBlock::BlockType XCNLRuntime::GetBlockType(const RawBlock& block, MetaFuncs) const noexcept
{
    switch (hash_(block.Type))
    {
    HashCase(block.Type, U"xcomp.Global")   return OutputBlock::BlockType::Global;
    HashCase(block.Type, U"xcomp.Struct")   return OutputBlock::BlockType::Struct;
    HashCase(block.Type, U"xcomp.Kernel")   return OutputBlock::BlockType::Instance;
    HashCase(block.Type, U"xcomp.Template") return OutputBlock::BlockType::Template;
    default:                                break;
    }
    return OutputBlock::BlockType::None;
}

std::unique_ptr<OutputBlock::BlockInfo> XCNLRuntime::PrepareBlockInfo(OutputBlock& blk)
{
    if (blk.Type == OutputBlock::BlockType::Template)
    {
        common::span<const RawArg> tpArgs;
        for (const auto& meta : blk.MetaFunc)
        {
            if (*meta.Name == U"xcomp.TemplateArgs")
            {
                for (uint32_t i = 0; i < meta.Args.size(); ++i)
                {
                    if (meta.Args[i].TypeData != RawArg::Type::Var)
                        NLRT_THROW_EX(FMTSTR(u"TemplateArgs's arg[{}] is [{}]. not [Var]"sv,
                            i, meta.Args[i].GetTypeName()), meta, blk.Block);
                }
                tpArgs = meta.Args;
                break;
            }
        }
        return std::make_unique<TemplateBlockInfo>(tpArgs);
    }
    return std::move(blk.ExtraInfo);
}

void XCNLRuntime::HandleInstanceMeta(const FuncCall&, InstanceContext&)
{ }

void XCNLRuntime::BeforeOutputBlock(const OutputBlock& block, std::u32string& dst) const
{
    APPEND_FMT(dst, U"\r\n/* From {} Block [{}] */\r\n"sv, block.GetBlockTypeName(), block.Block->Name);

    // std::set<std::u32string_view> lateVars;

    OutputConditions(block.MetaFunc, dst);
}

void XCNLRuntime::BeforeFinishOutput(std::u32string&, std::u32string&)
{ }

void XCNLRuntime::ProcessRawBlock(const xziar::nailang::RawBlock& block, MetaFuncs metas)
{
    auto frame = PushFrame(FrameFlags::FlowScope);

    if (!HandleMetaFuncs(metas, BlockContent::Generate(&block)))
        return;
    
    const auto type = GetBlockType(block, metas);
    if (type == OutputBlock::BlockType::None)
        return;
    auto& dst = type == OutputBlock::BlockType::Template ? XCContext.TemplateBlocks : XCContext.OutputBlocks;
    dst.emplace_back(&block, metas, type);
    auto& blk = dst.back();

    for (const auto& fcall : metas)
    {
        if (*fcall.Name == U"xcomp.ReplaceVariable"sv)
            blk.ReplaceVar = true;
        else if (*fcall.Name == U"xcomp.ReplaceFunction"sv)
            blk.ReplaceFunc = true;
        else if (*fcall.Name == U"xcomp.Replace"sv)
            blk.ReplaceVar = blk.ReplaceFunc = true;
        else if (*fcall.Name == U"xcomp.PreAssign"sv)
        {
            ThrowByArgCount(fcall, 2, ArgLimits::Exact);
            ThrowByArgType(fcall, RawArg::Type::Var, 0);
            blk.PreAssignArgs.emplace_back(fcall.Args[0].GetVar<RawArg::Type::Var>(), fcall.Args[1]);
        }
    }

    blk.ExtraInfo = PrepareBlockInfo(blk);
}

std::string XCNLRuntime::GenerateOutput()
{
    std::u32string prefix, output;

    for (const auto& block : XCContext.OutputBlocks)
    {
        if (block.Type == OutputBlock::BlockType::None || block.Type == OutputBlock::BlockType::Template)
        {
            Logger.warning(u"Unexpected RawBlock (Type[{}], Name[{}]) when generating output.\n", block.Block->Type, block.Block->Name); break;
            continue;
        }
        BeforeOutputBlock(block, output);
        auto frame = PushFrame(FrameFlags::FlowScope);
        if (block.Type == OutputBlock::BlockType::Instance)
        {
            auto cookie = PrepareInstance(block);
            OutputInstance(cookie, output);
        }
        else
        {
            BlockCookie cookie(block);
            switch (block.Type)
            {
            case OutputBlock::BlockType::Struct:    DirectOutput(cookie, output); continue;
            case OutputBlock::BlockType::Instance:  OutputStruct(cookie, output); continue;
            default:                                Expects(false); break;
            }
        }
    }
    for (const auto& ext : XCContext.Extensions)
    {
        ext->FinishXCNL(*this);
    }

    BeforeFinishOutput(prefix, output);

    // Output patched blocks
    for (const auto& [id, src] : XCContext.PatchedBlocks)
    {
        APPEND_FMT(prefix, U"/* Patched Block [{}] */\r\n"sv, id);
        prefix.append(src);
        prefix.append(U"\r\n\r\n"sv);
    }

    return common::str::to_string(prefix + output, Charset::UTF8, Charset::UTF32);
}


XCNLProgram::XCNLProgram(std::u32string&& source) : Source(std::move(source)) 
{ }
XCNLProgram::~XCNLProgram()
{ }


XCNLProgStub::XCNLProgStub(const std::shared_ptr<const XCNLProgram>& program, std::shared_ptr<XCNLContext>&& context,
    std::unique_ptr<XCNLRuntime>&& runtime) : Program(program), Context(std::move(context)), Runtime(std::move(runtime))
{
    Prepare(Runtime->Logger);
}
XCNLProgStub::~XCNLProgStub()
{ }
void XCNLProgStub::Prepare(common::mlog::MiniLogger<false>& logger)
{
    Context->Extensions.clear();
    for (const auto& [creator, id] : XCNLExtGenerators())
    {
        auto ext = creator(logger, *Context, Program->Program);
        if (ext)
        {
            ext->ID = id;
            Context->Extensions.push_back(std::move(ext));
        }
    }
}


}