#include "DxPch.h"
#include "NLDX.h"
#include "NLDXRely.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Detect.h"
#include "common/StrParsePack.hpp"

namespace dxu
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::ArgLocator;
using xziar::nailang::RawArg;
using xziar::nailang::CustomVar;
using xziar::nailang::SubQuery;
using xziar::nailang::LateBindVar;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::BlockContent;
using xziar::nailang::FuncCall;
using xziar::nailang::ArgLimits;
using xziar::nailang::FrameFlags;
using xziar::nailang::AutoVarHandler;
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



NLDXExtension::NLDXExtension(NLDXContext& context) : xcomp::XCNLExtension(context), Context(context)
{ }
NLDXExtension::~NLDXExtension() { }


struct NLDXContext::DXUVar : public AutoVarHandler<NLDXContext>
{
    DXUVar() : AutoVarHandler<NLDXContext>(U"NLDX"sv)
    {
        AddAutoMember<DxDevice_>(U"Dev"sv, U"DxDevice"sv, [](NLDXContext& ctx)
            {
                return ctx.Device;
            }, [](auto& argHandler)
            {
#define MEMBER(name) argHandler.AddSimpleMember(U"" STRINGIZE(name) ""sv, [](DxDevice_& dev) { return dev.name; })
#define MEMBERFUNC(name) argHandler.AddSimpleMember(U"" STRINGIZE(name) ""sv, [](DxDevice_& dev) { return dev.name(); })
                MEMBER(SMVer);
                MEMBER(WaveSize);
                MEMBER(AdapterName);
                MEMBERFUNC(IsTBR);
                MEMBERFUNC(IsUMA);
                MEMBERFUNC(IsUMACacheCoherent);
                MEMBERFUNC(IsIsolatedMMU);
                MEMBERFUNC(SupportFP64);
                MEMBERFUNC(SupportINT64);
                MEMBERFUNC(SupportFP16);
                MEMBERFUNC(SupportINT16);
#undef MEMBER
            });
    }
    Arg ConvertToCommon(const CustomVar&, Arg::Type type) noexcept override
    {
        if (type == Arg::Type::Bool)
            return true;
        return {};
    }
};


NLDXContext::NLDXContext(DxDevice dev, const common::CLikeDefines& info) :
    xcomp::XCNLContext(info), Device(dev), AllowDebug(false)
{
    AllowDebug = info["debug"].has_value();
    static DXUVar DXUVarHandler;
    DXUArg = DXUVarHandler.CreateVar(*this);
}
NLDXContext::~NLDXContext()
{ }

ArgLocator NLDXContext::LocateArg(const LateBindVar& var, bool create) noexcept
{
    if (var == U"dxu"sv)
        return { &DXUArg, 0 };
    return XCNLContext::LocateArg(var, create);
}

NLDXContext::VecTypeResult NLDXContext::ParseVecType(const std::u32string_view type) const noexcept
{
    auto [info, least] = xcomp::ParseVDataType(type);
    if (info.Bit == 0)
        return { info, false };
    bool typeSupport = true;
    if (info.Type == common::simd::VecDataInfo::DataTypes::Float) // FP ext handling
    {
        if (info.Bit == 16 && !Device->SupportFP16()) // FP16 check
        {
            if (least) // promotion
                info.Bit = 32;
            else
                typeSupport = false;
        }
        else if (info.Bit == 64 && !Device->SupportFP64())
        {
            typeSupport = false;
        }
    }
    return { info, typeSupport };
}

std::u32string_view NLDXContext::GetDXTypeName(common::simd::VecDataInfo info) noexcept
{
#define CASE(s, type, bit, n) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): return PPCAT(PPCAT(U,s),sv);
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),            type, bit, 1)  \
    CASE(STRINGIZE(PPCAT(pfx, 2)),  type, bit, 2)  \
    CASE(STRINGIZE(PPCAT(pfx, 3)),  type, bit, 3)  \
    CASE(STRINGIZE(PPCAT(pfx, 4)),  type, bit, 4)  \

    switch (static_cast<uint32_t>(info))
    {
    CASEV(uint16_t,     Unsigned, 16)
    CASEV(uint,         Unsigned, 32)
    CASEV(uint64_t,     Unsigned, 64)
    CASEV(int16_t,      Signed,   16)
    CASEV(int,          Signed,   32)
    CASEV(int64_t,      Signed,   64)
    CASEV(float16_t,    Float,    16)
    CASEV(float,        Float,    32)
    CASEV(double,       Float,    64)
    default: return {};
    }

#undef CASEV
#undef CASE
}

std::u32string_view NLDXContext::GetVecTypeName(common::simd::VecDataInfo info) const noexcept
{
    return GetDXTypeName(info);
}


NLDXRuntime::NLDXRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLDXContext> evalCtx) :
    XCNLRuntime(logger, evalCtx), Context(*evalCtx)
{ }
NLDXRuntime::~NLDXRuntime()
{ }

//void NLDXRuntime::StringifyKernelArg(std::u32string& out, const KernelArgInfo& arg)
//{
//    switch (arg.ArgType)
//    {
//    case KerArgType::Simple:
//        APPEND_FMT(out, U"{:8} {:5}  {} {}", ArgFlags::ToCLString(arg.Space),
//            HAS_FIELD(arg.Qualifier, KerArgFlag::Const) ? U"const"sv : U""sv,
//            common::str::to_u32string(arg.Type, Charset::UTF8),
//            common::str::to_u32string(arg.Name, Charset::UTF8));
//        break;
//    case KerArgType::Buffer:
//        APPEND_FMT(out, U"{:8} {:5} {} {} {} {}", ArgFlags::ToCLString(arg.Space),
//            HAS_FIELD(arg.Qualifier, KerArgFlag::Const) ? U"const"sv : U""sv,
//            HAS_FIELD(arg.Qualifier, KerArgFlag::Volatile) ? U"volatile"sv : U""sv,
//            common::str::to_u32string(arg.Type, Charset::UTF8),
//            HAS_FIELD(arg.Qualifier, KerArgFlag::Restrict) ? U"restrict"sv : U""sv,
//            common::str::to_u32string(arg.Name, Charset::UTF8));
//        break;
//    case KerArgType::Image:
//        APPEND_FMT(out, U"{:10} {} {}", ArgFlags::ToCLString(arg.Access),
//            common::str::to_u32string(arg.Type, Charset::UTF8),
//            common::str::to_u32string(arg.Name, Charset::UTF8));
//        break;
//    default:
//        NLRT_THROW_EX(u"Does not support KerArg of type[Any]");
//        break;
//    }
//}

void NLDXRuntime::OnReplaceFunction(std::u32string& output, void* cookie, const std::u32string_view func, const common::span<const std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtMost);
        output.append(U"[unroll]"sv);
        return;
    }
    
    return XCNLRuntime::OnReplaceFunction(output, cookie, func, args);
}

Arg NLDXRuntime::EvaluateFunc(const FuncCall& call, MetaFuncs metas)
{
    if (call.Name->PartCount == 2 && (*call.Name)[0] == U"dxu"sv)
    {
        switch (const auto subName = (*call.Name)[1]; common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"CompilerFlag")
        {
            const auto arg  = EvaluateFirstFuncArg(call, Arg::Type::String);
            const auto flag = common::str::to_string(arg.GetStr().value(), Charset::UTF8, Charset::UTF32);
            for (const auto& item : Context.CompilerFlags)
            {
                if (item == flag)
                    return {};
            }
            Context.CompilerFlags.push_back(flag);
        } return {};
        default: break;
        }
        auto ret = CommonFunc((*call.Name)[1], call, metas);
        if (ret.has_value())
            return std::move(ret.value());
    }
    return XCNLRuntime::EvaluateFunc(call, metas);
}

xcomp::OutputBlock::BlockType NLDXRuntime::GetBlockType(const RawBlock& block, MetaFuncs metas) const noexcept
{
    switch (common::DJBHash::HashC(block.Type))
    {
    HashCase(block.Type, U"dxu.Global")    return xcomp::OutputBlock::BlockType::Global;
    HashCase(block.Type, U"dxu.Struct")    return xcomp::OutputBlock::BlockType::Struct;
    HashCase(block.Type, U"dxu.Kernel")    return xcomp::OutputBlock::BlockType::Instance;
    HashCase(block.Type, U"dxu.Template")  return xcomp::OutputBlock::BlockType::Template;
    default:                                break;
    }
    return XCNLRuntime::GetBlockType(block, metas);
}

std::unique_ptr<xcomp::BlockCookie> NLDXRuntime::PrepareInstance(const xcomp::OutputBlock& block)
{
    return std::make_unique<KernelCookie>(block);
}


void NLDXRuntime::HandleInstanceMeta(const FuncCall& meta, xcomp::InstanceContext& ctx)
{
    auto& kerCtx = static_cast<KernelContext&>(ctx);
    if (meta.Name->PartCount == 2 && (*meta.Name)[0] == U"dxu"sv)
    {
        switch (const auto subName = (*meta.Name)[1]; common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"RequestWorkgroupSize")
        {
            const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::Integer, Arg::Type::Integer, Arg::Type::Integer });
            const auto x = args[0].GetUint().value_or(1),
                y = args[1].GetUint().value_or(1),
                z = args[2].GetUint().value_or(1);
            kerCtx.WorkgroupSize = gsl::narrow_cast<uint32_t>(x * y * z);
            kerCtx.AddAttribute(U"ReqWGSize"sv, FMTSTR(U"[numthreads({}, {}, {})]"sv, x, y, z));
        } return;
        HashCase(subName, U"UseGroupId")
        {
            const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::String, ArgLimits::AtMost);
            kerCtx.GroupIdVar = arg.GetStr().value_or(U"dxu_groupid"sv);
        } return;
        HashCase(subName, U"UseItemId")
        {
            const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::String, ArgLimits::AtMost);
            kerCtx.ItemIdVar = arg.GetStr().value_or(U"dxu_itemid"sv);
        } return;
        HashCase(subName, U"UseGlobalId")
        {
            const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::String, ArgLimits::AtMost);
            kerCtx.GlobalIdVar = arg.GetStr().value_or(U"dxu_globalid"sv);
        } return;
        HashCase(subName, U"UseTId")
        {
            const auto arg = EvaluateFirstFuncArg(meta, Arg::Type::String, ArgLimits::AtMost);
            kerCtx.TIdVar = arg.GetStr().value_or(U"dxu_threadid"sv);
        } return;
        //HashCase(subName, U"SimpleArg")
        //{
        //    const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
        //    const auto space_ = args[0].GetStr().value();
        //    const auto space = KerArgSpaceParser(space_);
        //    if (!space) NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgSpace [{}]"sv, space_), &meta);
        //    const auto argType = args[1].GetStr().value();
        //    // const bool isPointer = !argType.empty() && argType.back() == U'*';
        //    const auto name = args[2].GetStr().value();
        //    const auto flags = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
        //        .Reduce([&](KerArgFlag flag, std::u32string_view part)
        //            {
        //                if (part == U"const"sv)     return flag | KerArgFlag::Const;
        //                NLRT_THROW_EX(fmt::format(u"Unrecognized KerArgFlag [{}]"sv, part),
        //                    &meta);
        //                return flag;
        //            }, KerArgFlag::None);
        //    kerCtx.AddArg(KerArgType::Simple, space.value(), ImgAccess::None, flags,
        //        common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
        //        common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        //} return;
        //HashCase(subName, U"BufArg")
        //{
        //    const auto args = EvaluateFuncArgs<4>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String, Arg::Type::String });
        //    const auto space_ = args[0].GetStr().value();
        //    const auto space = KerArgSpaceParser(space_);
        //    if (!space)
        //        NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgSpace [{}]"sv, space_), &meta);
        //    const auto argType = std::u32string(args[1].GetStr().value()) + U'*';
        //    const auto name = args[2].GetStr().value();
        //    const auto flags = common::str::SplitStream(args[3].GetStr().value(), U' ', false)
        //        .Reduce([&](KerArgFlag flag, std::u32string_view part)
        //            {
        //                if (part == U"const"sv)     return flag | KerArgFlag::Const;
        //                if (part == U"restrict"sv)  return flag | KerArgFlag::Restrict;
        //                if (part == U"volatile"sv)  return flag | KerArgFlag::Volatile;
        //                NLRT_THROW_EX(FMTSTR(u"Unrecognized KerArgFlag [{}]"sv, part),
        //                    &meta);
        //                return flag;
        //            }, KerArgFlag::None);
        //    if (space.value() == KerArgSpace::Private)
        //        NLRT_THROW_EX(FMTSTR(u"BufArg [{}] cannot be in private space: [{}]"sv, name, space_), &meta);
        //    kerCtx.AddArg(KerArgType::Buffer, space.value(), ImgAccess::None, flags,
        //        common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
        //        common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        //} return;
        //HashCase(subName, U"ImgArg")
        //{
        //    const auto args = EvaluateFuncArgs<3>(meta, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
        //    const auto access_ = args[0].GetStr().value();
        //    const auto access = ImgArgAccessParser(access_);
        //    if (!access) NLRT_THROW_EX(fmt::format(u"Unrecognized ImgAccess [{}]"sv, access_), &meta);
        //    const auto argType = args[1].GetStr().value();
        //    const auto name = args[2].GetStr().value();
        //    kerCtx.AddArg(KerArgType::Image, KerArgSpace::Global, access.value(), KerArgFlag::None,
        //        common::str::to_string(name,    Charset::UTF8, Charset::UTF32),
        //        common::str::to_string(argType, Charset::UTF8, Charset::UTF32));
        //} return;
        default:
            break;
        }
    }
    return xcomp::XCNLRuntime::HandleInstanceMeta(meta, ctx);
}

void NLDXRuntime::OutputStruct(xcomp::BlockCookie& cookie, std::u32string& dst)
{
    dst.append(U"typedef struct \r\n{\r\n"sv);
    DirectOutput(cookie, dst);
    APPEND_FMT(dst, U"}} {};\r\n"sv, cookie.Block.Block->Name);
}

void NLDXRuntime::OutputInstance(xcomp::BlockCookie& cookie, std::u32string& dst)
{
    ProcessInstance(cookie);

    auto& kerCtx = *static_cast<KernelContext*>(cookie.GetInstanceCtx());
    // attributes
    for (const auto& item : kerCtx.Attributes)
    {
        dst.append(item.Content).append(U"\r\n"sv);
    }
    APPEND_FMT(dst, U"void {} ("sv, cookie.Block.Name());
    constexpr std::tuple<std::u32string KernelContext::*, std::u32string_view, std::u32string_view> Semantics[] =
    {
        { &KernelContext::GroupIdVar,   U"uint3"sv, U"SV_GroupID"sv },
        { &KernelContext::ItemIdVar,    U"uint3"sv, U"SV_GroupThreadID"sv },
        { &KernelContext::GlobalIdVar,  U"uint3"sv, U"SV_DispatchThreadID"sv },
        { &KernelContext::TIdVar,       U"uint"sv,  U"SV_GroupIndex"sv },
    };

    for (const auto& [accessor, type, sem] : Semantics)
    {
        if (const auto& name = kerCtx.*accessor; !name.empty())
        {
            APPEND_FMT(dst, U"\r\n    {:5} {} : {},", type, name, sem);
        }
    }

    if (dst.back() == U',')
        dst.pop_back(); // remove additional ','
    dst.append(U")\r\n{\r\n"sv);
    // prefixes
    kerCtx.WritePrefixes(dst);
    // content
    dst.append(kerCtx.Content);
    // suffixes
    kerCtx.WriteSuffixes(dst);
    dst.append(U"}\r\n"sv);
}

//void NLDXRuntime::BeforeFinishOutput(std::u32string& prefix, std::u32string&)
//{
//}


NLDXBaseResult::NLDXBaseResult(const std::shared_ptr<NLDXContext>& context) :
    TempRuntime(dxLog(), context), Context(context)
{ }
NLDXBaseResult::~NLDXBaseResult()
{ }
NLDXResult::ResultType NLDXBaseResult::QueryResult(std::u32string_view name) const
{
    const auto result = TempRuntime.EvaluateRawStatement(name, false);
    return result.Visit([](auto val) -> NLDXResult::ResultType
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>
                || std::is_same_v<T, double> || std::is_same_v<T, std::u32string_view>)
                return val;
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
                return {};
            else // var
                return std::any{};
        });
}

NLDXUnBuildResult::NLDXUnBuildResult(const std::shared_ptr<NLDXContext>& context, std::string&& source) :
    NLDXBaseResult(context), Source(std::move(source))
{ }
NLDXUnBuildResult::~NLDXUnBuildResult()
{ }
std::string_view NLDXUnBuildResult::GetNewSource() const noexcept
{
    return Source;
}
DxShader NLDXUnBuildResult::GetShader() const
{
    COMMON_THROW(common::BaseException, u"Shader has not been built!");
}

NLDXBuiltResult::NLDXBuiltResult(const std::shared_ptr<NLDXContext>& context, DxShader shader) :
    NLDXBaseResult(context), Shader(std::move(shader))
{ }
NLDXBuiltResult::~NLDXBuiltResult()
{ }
std::string_view NLDXBuiltResult::GetNewSource() const noexcept
{
    return Shader->GetSource();
}
DxShader NLDXBuiltResult::GetShader() const
{
    return Shader;
}

NLDXBuildFailResult::NLDXBuildFailResult(const std::shared_ptr<NLDXContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex) :
    NLDXUnBuildResult(context, std::move(source)), Exception(std::move(ex))
{ }
NLDXBuildFailResult::~NLDXBuildFailResult()
{ }
DxShader NLDXBuildFailResult::GetShader() const
{
    Exception->ThrowReal();
    return {};
}


NLDXProgStub::NLDXProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLDXContext> context,
    std::unique_ptr<NLDXRuntime>&& runtime) : XCNLProgStub(program, std::move(context), std::move(runtime))
{ }
NLDXProgStub::NLDXProgStub(const std::shared_ptr<const xcomp::XCNLProgram>& program, std::shared_ptr<NLDXContext> context,
    common::mlog::MiniLogger<false>& logger) : NLDXProgStub(program, std::move(context), std::make_unique<NLDXRuntime>(logger, context))
{ }


NLDXResult::~NLDXResult()
{ }


NLDXProcessor::NLDXProcessor() : TheLogger(&dxLog())
{ }
NLDXProcessor::NLDXProcessor(common::mlog::MiniLogger<false>&& logger) : TheLogger(std::move(logger)) 
{ }
NLDXProcessor::~NLDXProcessor()
{ }

void NLDXProcessor::ConfigureDX(NLDXProgStub& stub) const
{
    // Prepare
    stub.Program->ForEachBlockTypeName<true>(U"xcomp.Prepare"sv,
        [&](const Block& block, common::span<const FuncCall> metas)
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
    stub.Program->ForEachBlockTypeName<true>(U"dxu.Prepare"sv,
        [&](const Block& block, common::span<const FuncCall> metas) 
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
    // Collect
    stub.Program->ForEachBlockType<false>([&](const RawBlock& block, common::span<const FuncCall> metas)
        {
            if (IsBeginWith(block.Type, U"dxu."sv) || IsBeginWith(block.Type, U"xcomp."sv))
                stub.Runtime->ProcessRawBlock(block, metas);
        });
}

std::string NLDXProcessor::GenerateDX(NLDXProgStub& stub) const
{
    auto str = stub.Runtime->GenerateOutput();
    // PostAct
    stub.Program->ForEachBlockTypeName<true>(U"xcomp.PostAct"sv,
        [&](const Block& block, common::span<const FuncCall> metas)
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
    stub.Program->ForEachBlockTypeName<true>(U"dxu.PostAct"sv,
        [&](const Block& block, common::span<const FuncCall> metas)
        {
            stub.Runtime->ExecuteBlock(block, metas);
        });
    return str;
}

std::unique_ptr<NLDXResult> NLDXProcessor::CompileIntoProgram(NLDXProgStub& stub, const DxDevice dev, DxShaderConfig config) const
{
    auto str = GenerateDX(stub);
    try
    {
        auto& nldxCtx = *stub.GetContext().get();
        auto progStub = DxShader_::Create(dev, ShaderType::Compute, str);
        for (const auto& flag : nldxCtx.CompilerFlags)
        {
            config.Flags.insert(flag);
        }
        progStub.Build(config);
        return std::make_unique<NLDXBuiltResult>(stub.GetContext(), progStub.Finish());
    }
    catch (const common::BaseException& be)
    {
        return std::make_unique<NLDXBuildFailResult>(stub.GetContext(), std::move(str), be.InnerInfo());
    }
}

std::shared_ptr<xcomp::XCNLProgram> NLDXProcessor::Parse(common::span<const std::byte> source, std::u16string fileName) const
{
    auto& logger = Logger();
    const auto encoding = common::str::DetectEncoding(source);
    logger.info(u"File[{}], detected encoding[{}].\n", fileName, common::str::getCharsetName(encoding));
    auto src = common::str::to_u32string(source, encoding);
    logger.info(u"Translate into [utf32] for [{}] chars.\n", src.size());
    const auto prog = xcomp::XCNLProgram::Create(std::move(src), std::move(fileName));
    logger.verbose(u"Parse finished, get [{}] Blocks.\n", prog->GetProgram().Size());
    return prog;
}

std::unique_ptr<NLDXResult> NLDXProcessor::ProcessDX(const std::shared_ptr<xcomp::XCNLProgram>& prog, const DxDevice dev, const common::CLikeDefines& info) const
{
    NLDXProgStub stub(prog, std::make_shared<NLDXContext>(dev, info), Logger());
    ConfigureDX(stub);
    return std::make_unique<NLDXUnBuildResult>(stub.GetContext(), GenerateDX(stub));
}

std::unique_ptr<NLDXResult> NLDXProcessor::CompileShader(const std::shared_ptr<xcomp::XCNLProgram>& prog, DxDevice dev, const common::CLikeDefines& info, const DxShaderConfig& config) const
{
    NLDXProgStub stub(prog, std::make_shared<NLDXContext>(dev, info), Logger());
    ConfigureDX(stub);
    return CompileIntoProgram(stub, dev, config);
}


}