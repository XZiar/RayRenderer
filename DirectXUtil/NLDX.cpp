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
using xziar::nailang::Expr;
using xziar::nailang::CustomVar;
using xziar::nailang::SubQuery;
using xziar::nailang::LateBindVar;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::Statement;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncPack;
using xziar::nailang::MetaSet;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::ArgLimits;
using xziar::nailang::AutoVarHandler;
using xziar::nailang::NailangRuntime;
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

size_t NLDXContext::GetKernelCount() const noexcept
{
    return common::linq::FromIterable(OutputBlocks)
        .Where([](const auto& blk) { return blk.Type == xcomp::OutputBlock::BlockType::Instance; })
        .Count();
}

void NLDXContext::AddResource(const Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype,
    uint16_t space, uint16_t reg, uint16_t count, xcomp::InstanceArgInfo::TexTypes texType, BoundedResourceType type)
{
    const auto resId = BindResoures.size();
    //Expects(resId < UINT32_MAX);
    BindResoures.push_back({ {}, StrPool.AllocateString(name), StrPool.AllocateString(dtype), space, reg, count, texType, type });
    BindResoures.back().KernelIds.push_back(static_cast<char>(kerId));
    if (source)
        ReusableResIds.emplace_back(*source, static_cast<uint32_t>(resId));
}
void NLDXContext::AddConstant(const Arg* source, uint8_t kerId, std::string_view name, std::string_view dtype, uint16_t count)
{
    const auto resId = ShaderConstants.size();
    //Expects(resId < UINT32_MAX);
    ShaderConstants.push_back({ {}, StrPool.AllocateString(name), StrPool.AllocateString(dtype), count });
    ShaderConstants.back().KernelIds.push_back(static_cast<char>(kerId));
    if (source)
        ReusableSCIds.emplace_back(*source, static_cast<uint32_t>(resId));
}


NLDXExecutor::NLDXExecutor(NLDXRuntime* runtime) : XCNLExecutor(runtime)
{ }


NLDXConfigurator::NLDXConfigurator(NLDXRuntime* runtime) : NLDXExecutor(runtime) 
{ }
NLDXConfigurator::~NLDXConfigurator() 
{ }
xcomp::XCNLExecutor& NLDXConfigurator::GetExecutor() noexcept
{
    return *this;
}

NLDXConfigurator::MetaFuncResult NLDXConfigurator::HandleMetaFunc(const FuncCall& meta, MetaSet& allMetas)
{
    const auto ret = NLDXExecutor::HandleMetaFunc(meta, allMetas);
    if (ret != MetaFuncResult::Unhandled)
        return ret;
    if (const auto frame = TryGetOutputFrame(); frame)
        return XCNLConfigurator::HandleRawBlockMeta(meta, frame->Block, allMetas) ? MetaFuncResult::Next : MetaFuncResult::Unhandled;
    return MetaFuncResult::Unhandled;
}

Arg NLDXConfigurator::EvaluateFunc(FuncEvalPack& func)
{
    auto& runtime = GetRuntime();
    const auto& fname = func.GetName();
    if (func.Name->PartCount == 2 && fname[0] == U"dxu"sv)
    {
        const auto subName = fname[1];
        switch (common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"CompilerFlag")
        {
            ThrowByParamTypes<1>(func, { Arg::Type::String });
            const auto flag = common::str::to_string(func.Params[0].GetStr().value(), Charset::UTF8, Charset::UTF32);
            for (const auto& item : runtime.Context.CompilerFlags)
            {
                if (item == flag)
                    return {};
            }
            runtime.Context.CompilerFlags.push_back(flag);
        } return {};
        HashCase(subName, U"Enable16Bit")
        {
            if (runtime.Context.Device->SMVer < 62)
                NLRT_THROW_EX(FMTSTR(u"16Bit support requires SM6.2, but Device [{}] only support [SM{}].", 
                    runtime.Context.Device->AdapterName, runtime.Context.Device->SMVer / 10.f), func);
            runtime.Context.MinSMVer = std::max(runtime.Context.MinSMVer, 62u);
            runtime.Context.Enable16Bit = true;
        } return {};
        default: break;
        }
        auto ret = runtime.CommonFunc(fname[1], func);
        if (ret.has_value())
            return std::move(ret.value());
    }
    return XCNLExecutor::EvaluateFunc(func);
}

void NLDXConfigurator::EvaluateRawBlock(const RawBlock& block, xcomp::MetaFuncs metas)
{
    GetRuntime().CollectRawBlock(block, metas);
}


NLDXRawExecutor::NLDXRawExecutor(NLDXRuntime* runtime) : NLDXExecutor(runtime)
{ }
NLDXRawExecutor::~NLDXRawExecutor() 
{ }
xcomp::XCNLExecutor& NLDXRawExecutor::GetExecutor() noexcept
{
    return *this;
}
const xcomp::XCNLExecutor& NLDXRawExecutor::GetExecutor() const noexcept
{
    return *this;
}

std::unique_ptr<xcomp::InstanceContext> NLDXRawExecutor::PrepareInstance(const xcomp::OutputBlock& block)
{
    auto& runtime = GetRuntime();
    const auto kerId = runtime.Context.KernelNames.size();
    if (kerId >= 250)
        NLRT_THROW_EX(u"Too many kernels being defined, at most 250"sv);
    runtime.Context.KernelNames.emplace_back(block.Name());
    return std::make_unique<KernelContext>(static_cast<uint8_t>(kerId));
}

void NLDXRawExecutor::OutputInstance(const xcomp::OutputBlock& block, std::u32string& output)
{
    const auto kerCount = static_cast<NLDXContext&>(GetContext()).GetKernelCount();
    auto& kerCtx = GetCurInstance();

    if (kerCount > 1)
        APPEND_FMT(output, U"#if defined(DXU_KERNEL_{})\r\n"sv, block.Name());

    /*Context.AddPatchedBlock(*this, FMTSTR(U"Resource for [{}]"sv, cookie.Block.Name()), 
        &NLDXRuntime::StringifyKernelResource, kerCtx, cookie.Block.Name());*/

    // attributes
    for (const auto& item : kerCtx.Attributes)
    {
        output.append(item.Content).append(U"\r\n"sv);
    }
    APPEND_FMT(output, U"void {} ("sv, block.Name());
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
            APPEND_FMT(output, U"\r\n    {:5} {} : {},", type, name, sem);
        }
    }

    if (output.back() == U',')
        output.pop_back(); // remove additional ','
    output.append(U")\r\n{\r\n"sv);
    // prefixes
    kerCtx.WritePrefixes(output);
    // content
    output.append(kerCtx.Content);
    // suffixes
    kerCtx.WriteSuffixes(output);
    output.append(U"}\r\n"sv);
    if (kerCount > 1)
        output.append(U"#endif\r\n"sv);
}

void NLDXRawExecutor::ProcessStruct(const xcomp::OutputBlock& block, std::u32string& output)
{
    auto frame = XCNLRawExecutor::PushFrame(block, CreateContext(), nullptr);

    APPEND_FMT(output, U"struct {}"sv, block.Block->Name);
    output.append(U"\r\n{\r\n"sv);
    DirectOutput(block, output);
    output.append(U"};\r\n"sv);
}

bool NLDXRawExecutor::HandleInstanceMeta(FuncPack& meta)
{
    auto& kerCtx = GetCurInstance();
    const auto& fname = meta.GetName();
    if (meta.Name->PartCount == 2 && fname[0] == U"dxu"sv)
    {
        switch (const auto subName = fname[1]; common::DJBHash::HashC(subName))
        {
        HashCase(subName, U"RequestWorkgroupSize")
        {
            ThrowByParamTypes<3>(meta, { Arg::Type::Integer, Arg::Type::Integer, Arg::Type::Integer });
            const auto x = meta.Params[0].GetUint().value_or(1),
                y = meta.Params[1].GetUint().value_or(1),
                z = meta.Params[2].GetUint().value_or(1);
            kerCtx.WorkgroupSize = gsl::narrow_cast<uint32_t>(x * y * z);
            kerCtx.AddAttribute(U"ReqWGSize"sv, FMTSTR(U"[numthreads({}, {}, {})]"sv, x, y, z));
        } return true;
        HashCase(subName, U"UseGroupId")
        {
            ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::String });
            kerCtx.GroupIdVar = meta.Params.empty() ? U"dxu_groupid"sv : meta.Params[0].GetStr().value();
        } return true;
        HashCase(subName, U"UseItemId")
        {
            ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::String });
            kerCtx.ItemIdVar = meta.Params.empty() ? U"dxu_itemid"sv : meta.Params[0].GetStr().value();
        } return true;
        HashCase(subName, U"UseGlobalId")
        {
            ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::String });
            kerCtx.GlobalIdVar = meta.Params.empty() ? U"dxu_globalid"sv : meta.Params[0].GetStr().value();
        } return true;
        HashCase(subName, U"UseTId")
        {
            ThrowByParamTypes<1, ArgLimits::AtMost>(meta, { Arg::Type::String });
            kerCtx.TIdVar = meta.Params.empty() ? U"dxu_threadid"sv : meta.Params[0].GetStr().value();
        } return true;
        default:
            break;
        }
    }
    return XCNLRawExecutor::HandleInstanceMeta(meta);
}

NLDXRawExecutor::MetaFuncResult NLDXRawExecutor::HandleMetaFunc(FuncPack& meta, MetaSet& allMetas)
{
    if (HandleInstanceMeta(meta))
        return MetaFuncResult::Next;
    return NLDXExecutor::HandleMetaFunc(meta, allMetas);
}

void NLDXRawExecutor::OnReplaceFunction(std::u32string& output, void* cookie, std::u32string_view func, common::span<const std::u32string_view> args)
{
    if (func == U"unroll"sv)
    {
        ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtMost);
        output.append(U"[unroll]"sv);
        return;
    }
    return XCNLRawExecutor::OnReplaceFunction(output, cookie, func, args);
}


NLDXStructHandler::NLDXStructHandler(NLDXRuntime* runtime) : NLDXExecutor(runtime)
{ }
NLDXStructHandler::~NLDXStructHandler()
{ }
xcomp::XCNLExecutor& NLDXStructHandler::GetExecutor() noexcept
{
    return *this;
}
const xcomp::XCNLExecutor& NLDXStructHandler::GetExecutor() const noexcept
{
    return *this;
}

std::unique_ptr<xcomp::XCNLStruct> NLDXStructHandler::GenerateStruct(std::u32string_view name, MetaSet& metas)
{
    auto ret = std::make_unique<NLDXStruct>(name);
    for (auto& meta : metas.PostMetas())
    {
        if (meta.FullFuncName() == U"dxu.Layout"sv)
        {
            ThrowByParamTypes<1>(meta, { Arg::Type::String });
            if (const auto layout = meta.Params[0].GetStr().value(); layout == U"cbuf"sv)
                ret->Layout = NLDXStruct::LayoutTarget::ConstBuf;
            else if (layout == U"structbuf"sv)
                ret->Layout = NLDXStruct::LayoutTarget::StructBuf;
            else
                NLRT_THROW_EX(FMTSTR(u"Unrecognized layout [{}] for struct [{}]."sv,
                    layout, name), meta);
        }
    }
    return ret;
}

void NLDXStructHandler::OnNewField(const xcomp::XCNLStruct& target_, xcomp::XCNLStruct::Field& field, MetaSet&)
{
    auto& target = static_cast<const NLDXStruct&>(target_);
    if (target.Layout == NLDXStruct::LayoutTarget::ConstBuf)
    {
        if (field.Length > 1)
        {
            if (const auto size = field.GetElementSize(GetCustomStructs()); size % 16 != 0)
            {
                GetLogger().warning(u"Field [{}] of cbuffer struct [{}] is an array with element size [{}], may causing unexpected padding.\n",
                    target.StrPool.GetStringView(field.Name), target.GetName(), size);
            }
        }
    }
}

void NLDXStructHandler::FillFieldOffsets(xcomp::XCNLStruct& target_)
{
    auto& target = static_cast<NLDXStruct&>(target_);
    if (target.Layout == NLDXStruct::LayoutTarget::StructBuf)
        return XCNLStructHandler::FillFieldOffsets(target_);
    // cbuffer has more strict layout requirement
    // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
    uint32_t offset = 0;
    for (auto& field : target.Fields)
    {
        const auto [align_, eleSize] = GetBasicFieldAlignment(field);
        const auto align = std::max<uint32_t>(field.Offset == 0 ? align_ : field.Offset, 4);
        const auto newOffset = (offset + align - 1) / align * align;
        if (field.Length == 1 && newOffset % 16 + eleSize <= 16) // fit into the slot
        {
            field.Offset = static_cast<uint16_t>(newOffset);
            offset = newOffset + eleSize;
        }
        else // start new slot
        {
            field.Offset = static_cast<uint16_t>((newOffset / 16 + 1) * 16);
            const auto EleSlots = (eleSize + 15) / 16;
            field.MetaInfo = EleSlots;
            offset = field.Offset + (EleSlots * 16 * (field.Length - 1)) + eleSize;
        }
    }
    target.Alignment = 16; // vec4
    target.Size = offset;
}

void NLDXStructHandler::OutputStructContent(const NLDXStruct& target, std::u32string& output) const
{
    output.append(U"{\r\n"sv);
    for (const auto& field : target.Fields)
    {
        output.append(U"\t");
        StringifyBasicField(field, target, output);
        output.append(U";\r\n");
    }
    output.append(U"};\r\n"sv);
}

void NLDXStructHandler::OutputStruct(const xcomp::XCNLStruct& target_, std::u32string& output)
{
    auto& target = static_cast<const NLDXStruct&>(target_);
    /*if (target.Layout == NLDXStruct::LayoutTarget::ConstBuf)
        return;*/
    APPEND_FMT(output, U"struct {}\r\n"sv, target.GetName());
    OutputStructContent(target, output);
}

Arg NLDXStructHandler::EvaluateFunc(FuncEvalPack& func)
{
    //auto& runtime = GetRuntime();
    //const auto& fname = func.GetName();
    if (EvaluateStructFunc(func))
        return {};
    return XCNLExecutor::EvaluateFunc(func);
}


NLDXRuntime::NLDXRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLDXContext> evalCtx) :
    XCNLRuntime(logger, evalCtx), Context(*evalCtx), Configurator(this), RawExecutor(this), StructHandler(this)
{ }
NLDXRuntime::~NLDXRuntime()
{ }

xcomp::XCNLConfigurator& NLDXRuntime::GetConfigurator() noexcept { return Configurator; }
xcomp::XCNLRawExecutor& NLDXRuntime::GetRawExecutor() noexcept { return RawExecutor; }
xcomp::XCNLStructHandler& NLDXRuntime::GetStructHandler() noexcept { return StructHandler; }

std::u32string_view NLDXRuntime::GetDXTypeName(xcomp::VTypeInfo info) noexcept
{
#define CASE(s, type, bit, minBits, n) case static_cast<uint32_t>(xcomp::VTypeInfo{ \
    xcomp::VTypeInfo::DataTypes::type, n, 0, bit, minBits ? xcomp::VTypeInfo::TypeFlags::MinBits : xcomp::VTypeInfo::TypeFlags::Empty}):    \
        return U"" STRINGIZE(s) ""sv;
#define CASEV(pfx, type, bit, minBits) \
    CASE(pfx,            type, bit, minBits, 1)  \
    CASE(PPCAT(pfx, 2),  type, bit, minBits, 2)  \
    CASE(PPCAT(pfx, 3),  type, bit, minBits, 3)  \
    CASE(PPCAT(pfx, 4),  type, bit, minBits, 4)  \

    switch (static_cast<uint32_t>(info))
    {
    CASEV(min16uint,    Unsigned, 16, true)
    CASEV(uint16_t,     Unsigned, 16, false)
    CASEV(uint,         Unsigned, 32, false)
    CASEV(uint64_t,     Unsigned, 64, false)
    CASEV(min16int,     Signed,   16, true)
    CASEV(int16_t,      Signed,   16, false)
    CASEV(int,          Signed,   32, false)
    CASEV(int64_t,      Signed,   64, false)
    CASEV(min16float,   Float,    16, true)
    CASEV(float16_t,    Float,    16, false)
    CASEV(float,        Float,    32, false)
    CASEV(double,       Float,    64, false)
    default: return {};
    }

#undef CASEV
#undef CASE
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

void NLDXRuntime::HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const FuncPack& meta, const Arg* source)
{
    using xcomp::InstanceArgInfo;
    using TexTypes = xcomp::InstanceArgInfo::TexTypes;
    using common::str::to_string;
    using common::str::Charset;
    auto& kerCtx = static_cast<KernelContext&>(ctx);

    if (source)
    {
        const auto HandleReusable = [&](auto& pool, const auto& idList, const Arg* source)
        {
            for (const auto& [holder, idx] : idList)
            {
                if (holder.GetCustom().Call<&CustomVar::Handler::CompareSameClass>(source->GetCustom()).IsEqual())
                {
                    auto& target = pool[idx];
                    const char kerIdChar = static_cast<char>(kerCtx.KernelId);
                    if (target.KernelIds.find(kerIdChar) != std::string::npos)
                        NLRT_THROW_EX(FMTSTR(u"Arg [{}] has already been referenced for kernel [{}]."sv,
                            arg.Name, kerCtx.InsatnceName), meta);
                    target.KernelIds.push_back(kerIdChar);
                    return true;
                }
            }
            return false;
        };
        const bool exist = arg.Type == InstanceArgInfo::Types::Simple ?
            HandleReusable(Context.ShaderConstants, Context.ReusableSCIds,  source) :
            HandleReusable(Context.BindResoures,    Context.ReusableResIds, source);
        if (exist)
            return;
    }

    xziar::nailang::FlaggedItems<const std::u32string_view> extras(arg.Extra);
    const auto ParseCommonInfo = [&]()
    {
        constexpr auto ParseNumber = [](std::u32string_view str, uint16_t& target) 
        {
            uint32_t num = 0;
            if (str.empty())
                return false;
            for (const auto ch : str)
            {
                if (ch >= U'0' && ch <= '9')
                    num = num * 10 + (ch - U'0');
                else
                    return false;
            }
            target = gsl::narrow_cast<uint16_t>(num);
            return true;
        };
        using common::str::IsBeginWith;
        std::u32string unknwonExtra;
        uint16_t space = UINT16_MAX, reg = UINT16_MAX, count = 1;
        for (auto extra : extras)
        {
            if (extra.CheckFlag())
                continue;
#define CHECK_FLAG(name)                                                                    \
    if (const auto pfx_##name = U"" STRINGIZE(name) "="sv; IsBeginWith(*extra, pfx_##name)) \
    {                                                                                       \
        if (!ParseNumber(extra->substr(pfx_##name.size()), name))                           \
            NLRT_THROW_EX(FMTSTR(u"BufArg [{}] has invalid flag on [{}]: [{}]."sv,          \
                arg.Name, U"" STRINGIZE(name) ""sv, *extra), meta);                         \
            extra.SetFlag(true);                                                            \
    }
            CHECK_FLAG(space)
            else CHECK_FLAG(reg)
            else CHECK_FLAG(count)
            else unknwonExtra.append(*extra).append(U"], [");
        }
        if (!unknwonExtra.empty())
        {
            Expects(unknwonExtra.size() > 4);
            unknwonExtra.resize(unknwonExtra.size() - 4);
            dxLog().warning(u"BufArg [{}] has unrecoginzed flags: [{}].\n", arg.Name, unknwonExtra);
        }
        return std::make_tuple(space, reg, count);
    };
    switch (arg.Type)
    {
    case InstanceArgInfo::Types::RawBuf:
    {
        bool isCBuffer = false;
        for (auto extra : extras)
        {
            if (*extra == U"cbuffer"sv)
            {
                isCBuffer = true;
                extra.SetFlag(true);
            }
        }
        const bool isWritable = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write);
        BoundedResourceType type = isCBuffer ? BoundedResourceType::CBuffer : 
            (isWritable ? BoundedResourceType::RWStructBuf : BoundedResourceType::StructBuf);
        const auto [space, reg, count] = ParseCommonInfo();
        if (isCBuffer)
        {
            if (isWritable)
                NLRT_THROW_EX(FMTSTR(u"RawBufArg [{}] cannot be writable since it's in shader constants"sv, arg.Name), meta);
            // cbuffer array supported by SM5.1
            // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#constant-buffers
            /*if (count > 1)
                NLRT_THROW_EX(FMTSTR(u"RawBufArg [{}] cannot be array since it's flagged as cbuffer"sv, arg.Name), meta);*/
            if (Context.FindStruct(arg.DataType) == SIZE_MAX)
                NLRT_THROW_EX(FMTSTR(u"RawBufArg [{}] has unrecoginized struct datatype [{}]"sv, arg.Name, arg.DataType), meta);
        }
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    case InstanceArgInfo::Types::TypedBuf:
    {
        BoundedResourceType type = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write) ?
            BoundedResourceType::RWTyped : BoundedResourceType::Typed;
        const auto [space, reg, count] = ParseCommonInfo();
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    case InstanceArgInfo::Types::Simple:
    {
        if (HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write))
            NLRT_THROW_EX(FMTSTR(u"SimpleArg [{}] cannot be writable since it's in shader constants"sv, arg.Name), meta);
        const auto [space, reg, count] = ParseCommonInfo();
        Context.AddConstant(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8), count);
    } return;
    case InstanceArgInfo::Types::Texture:
    {
        BoundedResourceType type = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write) ?
            BoundedResourceType::RWTyped : BoundedResourceType::Typed;
        const auto [space, reg, count] = ParseCommonInfo();
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    default:
        return;
    }
}

void NLDXRuntime::BeforeFinishOutput(std::u32string&, std::u32string& structs, std::u32string&, std::u32string&)
{
    std::u32string bindings = U"\r\n/* Bounded Resources */\r\n"s;

    // categorize resources, ids are natually sorted
    std::string allKernelName;
    for (size_t i = 0; i < Context.KernelNames.size(); ++i)
        allKernelName.push_back(static_cast<char>(static_cast<uint8_t>(i)));
    std::map<std::string_view, std::vector<uint32_t>, std::less<>> categoryMap;
    {
        uint32_t idx = 0;
        for (const auto& res : Context.BindResoures)
        {
            categoryMap[res.KernelIds].push_back(idx++);
        }
    }
    for (const auto& [cat, resIdxs] : categoryMap)
    {
        bindings.append(U"/* resources for "sv);
        const bool isAllKernel = cat == allKernelName;
        if (isAllKernel)
        {
            bindings.append(U"all kernels */\r\n"sv);
        }
        else
        {
            std::u32string defNames = U"#if ";
            for (const auto ch : cat)
            {
                const auto name = Context.KernelNames[static_cast<uint8_t>(ch)];
                APPEND_FMT(bindings, U"[{}], "sv, name);
                APPEND_FMT(defNames, U"defined(DXU_KERNEL_{}) || "sv, name);
            }
            bindings.resize(bindings.size() - 2);
            bindings.append(U" */\r\n"sv).append(defNames, 0, defNames.size() - 4).append(U"\r\n"sv);
        }

        constexpr auto TypeResolver = [](const ResourceInfo& info)
        {
            using TexTypes = xcomp::InstanceArgInfo::TexTypes;
            if (info.TexType != TexTypes::Empty)
            {
                const bool isRW = HAS_FIELD(info.Type, BoundedResourceType::RWMask);
                switch (info.TexType)
                {
                case TexTypes::Tex1D:       return isRW ? U"RWTexture1D"sv      : U"Texture1D"sv;
                case TexTypes::Tex1DArray:  return isRW ? U"RWTexture1DArray"sv : U"Texture1DArray"sv;
                case TexTypes::Tex2D:       return isRW ? U"RWTexture2D"sv      : U"Texture2D"sv;
                case TexTypes::Tex2DArray:  return isRW ? U"RWTexture2DArray"sv : U"Texture2DArray"sv;
                case TexTypes::Tex3D:       return isRW ? U"RWTexture3D"sv      : U"Texture3D"sv;
                default:                    return U""sv;
                }
            }
            else
            {
                switch (info.Type)
                {
                case BoundedResourceType::StructBuf:    return U"StructuredBuffer"sv;
                case BoundedResourceType::RWStructBuf:  return U"RWStructuredBuffer"sv;
                case BoundedResourceType::Typed:        return U"Buffer"sv;
                case BoundedResourceType::RWTyped:      return U"RWBuffer"sv;
                case BoundedResourceType::CBuffer:      return U"ConstantBuffer"sv;
                default:                                return U""sv;
                }
            }
        };
        for (const auto idx : resIdxs)
        {
            const auto& res = Context.BindResoures[idx];
            const auto type = TypeResolver(res);
            APPEND_FMT(bindings, U"{}<{}> {}"sv, type, Context.StrPool.GetStringView(res.DataType), Context.StrPool.GetStringView(res.Name));
            if (res.Count != 1)
            {
                APPEND_FMT(bindings, U"[{}]", res.Count);
            }
            if (res.BindReg != UINT16_MAX)
            {
                const bool isRW = HAS_FIELD(res.Type, BoundedResourceType::RWMask);
                const bool isCBuffer = res.Type == BoundedResourceType::CBuffer;
                APPEND_FMT(bindings, U" : register({}{}", isCBuffer ? U"b"sv : (isRW ? U"u"sv : U"t"sv), res.BindReg);
                if (res.Space != UINT16_MAX)
                {
                    APPEND_FMT(bindings, U", space{}", res.Space);
                }
                bindings.push_back(U')');
            }
            bindings.append(U";\r\n");
        }
        if (!isAllKernel)
            bindings.append(U"#endif\r\n"sv);
        bindings.append(U"\r\n"sv);
    }

    // output cbuffer
    if (!Context.ShaderConstants.empty())
    {
        bindings.append(U"cbuffer dxu_cbuf\r\n{\r\n"sv);
        for (const auto& res : Context.ShaderConstants)
        {
            APPEND_FMT(bindings, U"\t{} {}; // for ", Context.StrPool.GetStringView(res.DataType), Context.StrPool.GetStringView(res.Name));
            for (const auto ch : res.KernelIds)
            {
                const auto name = Context.KernelNames[static_cast<uint8_t>(ch)];
                APPEND_FMT(bindings, U"[{}], "sv, name);
            }
            bindings.resize(bindings.size() - 2);
            bindings.append(U"\r\n"sv);
        }
        bindings.append(U"};\r\n\r\n");
    }

    structs.append(bindings);
}

xcomp::VTypeInfo NLDXRuntime::TryParseVecType(const std::u32string_view type, bool allowMinBits) const noexcept
{
    auto info = xcomp::ParseVDataType(type);
    if (!info)
        info;
    const bool isMinBits = info.HasFlag(xcomp::VTypeInfo::TypeFlags::MinBits);
    bool typeSupport = true;
    if (info.Type == xcomp::VTypeInfo::DataTypes::BFloat)
        typeSupport = false;
    else if (info.Bits == 16) // 16bit handling
    {
        if (Context.Enable16Bit)
            info.Flag &= ~xcomp::VTypeInfo::TypeFlags::MinBits; // simply remove minBits
        else if (allowMinBits)
            info.Flag |= xcomp::VTypeInfo::TypeFlags::MinBits; // simply add minBits
        else if (isMinBits) // disallow minbits, promote
            info.Bits = 32;
        else // disallow minbits, cannot support
            typeSupport = false;
    }
    else if (info.Bits == 64) // 64bit handling
    {
        if (info.Type == xcomp::VTypeInfo::DataTypes::Float) // FP64
            typeSupport = Context.Device->SupportFP64();
        else // INT64
            typeSupport = Context.Device->SupportINT64();
    }
    if (info.Dim0() > 4)
        typeSupport = false;
    if (!allowMinBits)
        info.Flag &= ~xcomp::VTypeInfo::TypeFlags::MinBits;
    if (!typeSupport)
        info.Flag |= xcomp::VTypeInfo::TypeFlags::Unsupport;
    return info;
}
std::u32string_view NLDXRuntime::GetVecTypeName(xcomp::VTypeInfo info) const noexcept
{
    return GetDXTypeName(info);
}


NLDXBaseResult::NLDXBaseResult(const std::shared_ptr<NLDXContext>& context) :
    Runtime(context)
{ }
NLDXBaseResult::~NLDXBaseResult()
{ }
NLDXResult::ResultType NLDXBaseResult::QueryResult(std::u32string_view name) const
{
    const auto result = Runtime.EvaluateRawStatement(name);
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
common::span<const std::pair<std::string, DxShader>> NLDXUnBuildResult::GetShaders() const
{
    COMMON_THROW(common::BaseException, u"Shader has not been built!");
}

NLDXBuiltResult::NLDXBuiltResult(const std::shared_ptr<NLDXContext>& context, std::string source, std::vector<std::pair<std::string, DxShader>> shaders) :
    NLDXBaseResult(context), Source(std::move(source)), Shaders(std::move(shaders))
{ }
NLDXBuiltResult::~NLDXBuiltResult()
{ }
std::string_view NLDXBuiltResult::GetNewSource() const noexcept
{
    return Source;
}
common::span<const std::pair<std::string, DxShader>> NLDXBuiltResult::GetShaders() const
{
    return Shaders;
}

NLDXBuildFailResult::NLDXBuildFailResult(const std::shared_ptr<NLDXContext>& context, std::string&& source, std::shared_ptr<common::ExceptionBasicInfo> ex) :
    NLDXUnBuildResult(context, std::move(source)), Exception(std::move(ex))
{ }
NLDXBuildFailResult::~NLDXBuildFailResult()
{ }
common::span<const std::pair<std::string, DxShader>> NLDXBuildFailResult::GetShaders() const
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
    constexpr std::u32string_view preapres[] = { U"xcomp.Prepare"sv, U"dxu.Prepare"sv, U"xcomp.Struct" };
    stub.Prepare(preapres);
    constexpr std::u32string_view collects[] = { U"dxu."sv, U"xcomp."sv };
    stub.Collect(collects);
}
std::string NLDXProcessor::GenerateDX(NLDXProgStub& stub) const
{
    auto str = stub.Runtime->GenerateOutput();
    constexpr std::u32string_view postacts[] = { U"xcomp.PostAct"sv, U"dxu.PostAct"sv };
    stub.PostAct(postacts);
    return str;
}

std::unique_ptr<NLDXResult> NLDXProcessor::CompileIntoProgram(NLDXProgStub& stub, const DxDevice dev, DxShaderConfig config) const
{
    auto str = GenerateDX(stub);
    try
    {
        auto& nldxCtx = *stub.GetContext().get();
        config.SMVersion = std::max(nldxCtx.MinSMVer, config.SMVersion);
        if (nldxCtx.Enable16Bit)
            config.Flags.insert("-enable-16bit-types");
        for (const auto& flag : nldxCtx.CompilerFlags)
        {
            config.Flags.insert(flag);
        }
        std::vector<std::pair<std::string, DxShader>> shaders;
        for (const auto kerName : nldxCtx.KernelNames)
        {
            config.EntryPoint = common::str::to_u16string(kerName);
            auto kerNameU8 = common::str::to_string(kerName, common::str::Charset::UTF8);
            const auto defName = "DXU_KERNEL_" + kerNameU8;
            config.Defines.Add(defName);
            auto progStub = DxShader_::Create(dev, ShaderType::Compute, str);
            progStub.Build(config);
            shaders.emplace_back(std::move(kerNameU8), progStub.Finish());
            config.Defines.Remove(defName);
        }
        return std::make_unique<NLDXBuiltResult>(stub.GetContext(), std::move(str), std::move(shaders));
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