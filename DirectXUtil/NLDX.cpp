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


KernelCookie::KernelCookie(const xcomp::OutputBlock& block, uint8_t kerId) noexcept :
    xcomp::BlockCookie(block), Context(kerId)
{
    Context.InsatnceName = this->Block.Name();
}


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


NLDXRuntime::NLDXRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<NLDXContext> evalCtx) :
    XCNLRuntime(logger, evalCtx), Context(*evalCtx)
{ }
NLDXRuntime::~NLDXRuntime()
{ }

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
    const auto kerId = Context.KernelNames.size();
    if (kerId >= 250)
        NLRT_THROW_EX(u"Too many kernels being defined, at most 250"sv);
    Context.KernelNames.emplace_back(block.Name());
    return std::make_unique<KernelCookie>(block, static_cast<uint8_t>(kerId));
}

void NLDXRuntime::HandleInstanceArg(const xcomp::InstanceArgInfo& arg, xcomp::InstanceContext& ctx, const FuncCall& meta, const Arg* source)
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

    const auto ParseCommonInfo = [&](const std::vector<std::u32string_view>& extras)
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
        for (const auto extra : extras)
        {
#define CHECK_FLAG(name)                                                                    \
    if (const auto pfx_##name = U"" STRINGIZE(name) "="sv; IsBeginWith(extra, pfx_##name))  \
    {                                                                                       \
        if (!ParseNumber(extra.substr(pfx_##name.size()), name))                            \
            NLRT_THROW_EX(FMTSTR(u"BufArg [{}] has invalid flag on [{}]: [{}]."sv,          \
                arg.Name, U"" STRINGIZE(name) ""sv, extra), &meta);                         \
    }
            CHECK_FLAG(space)
            else CHECK_FLAG(reg)
            else CHECK_FLAG(count)
            else unknwonExtra.append(extra).append(U"], [");
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
        BoundedResourceType type = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write) ?
            BoundedResourceType::RWStructBuf : BoundedResourceType::StructBuf;
        const auto [space, reg, count] = ParseCommonInfo(arg.Extra);
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    case InstanceArgInfo::Types::TypedBuf:
    {
        BoundedResourceType type = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write) ?
            BoundedResourceType::RWTyped : BoundedResourceType::Typed;
        const auto [space, reg, count] = ParseCommonInfo(arg.Extra);
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    case InstanceArgInfo::Types::Simple:
    {
        if (HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write))
            NLRT_THROW_EX(FMTSTR(u"SimpleArg [{}] cannot be writable since it's in shader constants"sv, arg.Name), &meta);
        const auto [space, reg, count] = ParseCommonInfo(arg.Extra);
        Context.AddConstant(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8), count);
    } return;
    case InstanceArgInfo::Types::Texture:
    {
        BoundedResourceType type = HAS_FIELD(arg.Flag, InstanceArgInfo::Flags::Write) ?
            BoundedResourceType::RWTyped : BoundedResourceType::Typed;
        const auto [space, reg, count] = ParseCommonInfo(arg.Extra);
        Context.AddResource(source, kerCtx.KernelId, to_string(arg.Name, Charset::UTF8), to_string(arg.DataType, Charset::UTF8),
            space, reg, count, arg.TexType, type);
    } return;
    default:
        return;
    }
}

//std::u32string NLDXRuntime::StringifyKernelResource(const KernelContext& ctx, std::u32string_view kerName)
//{
//    std::u32string txt;
//    for (const auto& res : ctx.BindResoures)
//    {
//        const auto type = [](const KernelContext::ResourceInfo& info)
//        {
//            using TexTypes = xcomp::InstanceArgInfo::TexTypes;
//            if (info.TexType != TexTypes::Empty)
//            {
//                const bool isRW = HAS_FIELD(info.Type, BoundedResourceType::RWMask);
//                switch (info.TexType)
//                {
//                case TexTypes::Tex1D:       return isRW ? U"RWTexture1D"sv      : U"Texture1D"sv;
//                case TexTypes::Tex1DArray:  return isRW ? U"RWTexture1DArray"sv : U"Texture1DArray"sv;
//                case TexTypes::Tex2D:       return isRW ? U"RWTexture2D"sv      : U"Texture2D"sv;
//                case TexTypes::Tex2DArray:  return isRW ? U"RWTexture2DArray"sv : U"Texture2DArray"sv;
//                case TexTypes::Tex3D:       return isRW ? U"RWTexture3D"sv      : U"Texture3D"sv;
//                default:                    return U""sv;
//                }
//            }
//            else
//            {
//                switch (info.Type)
//                {
//                case BoundedResourceType::StructBuf:    return U"StructuredBuffer"sv;
//                case BoundedResourceType::RWStructBuf:  return U"RWStructuredBuffer"sv;
//                case BoundedResourceType::Typed:        return U"Buffer"sv;
//                case BoundedResourceType::RWTyped:      return U"RWBuffer"sv;
//                default:                                return U""sv;
//                }
//            }
//        }(res);
//        APPEND_FMT(txt, U"{}<{}> {}"sv, type, ctx.StrPool.GetStringView(res.DataType), ctx.StrPool.GetStringView(res.Name));
//        if (res.Count != 1)
//        {
//            APPEND_FMT(txt, U"[{}]", res.Count);
//        }
//        if (res.BindReg != UINT16_MAX)
//        {
//            const bool isRW = HAS_FIELD(res.Type, BoundedResourceType::RWMask);
//            APPEND_FMT(txt, U" : register({}{}", isRW ? U"u"sv : U"t"sv, res.BindReg);
//            if (res.Space != UINT16_MAX)
//            {
//                APPEND_FMT(txt, U", space{}", res.Space);
//            }
//            txt.push_back(U')');
//        }
//        txt.append(U";\r\n");
//    }
//    if (!ctx.ShaderConstants.empty())
//    {
//        APPEND_FMT(txt, U"\r\ncbuffer dxu_cbuf_{}\r\n{{\r\n"sv, kerName);
//        for (const auto& res : ctx.ShaderConstants)
//        {
//            APPEND_FMT(txt, U"\t{} {};\r\n", ctx.StrPool.GetStringView(res.DataType), ctx.StrPool.GetStringView(res.Name));
//        }
//        txt.append(U"};\r\n");
//    }
//    return txt;
//}

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

    /*Context.AddPatchedBlock(*this, FMTSTR(U"Resource for [{}]"sv, cookie.Block.Name()), 
        &NLDXRuntime::StringifyKernelResource, kerCtx, cookie.Block.Name());*/

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

void NLDXRuntime::BeforeFinishOutput(std::u32string& prefix, std::u32string&)
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
                APPEND_FMT(bindings, U" : register({}{}", isRW ? U"u"sv : U"t"sv, res.BindReg);
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

    prefix.insert(prefix.begin(), bindings.begin(), bindings.end());
}


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
        for (const auto& flag : nldxCtx.CompilerFlags)
        {
            config.Flags.insert(flag);
        }
        std::vector<std::pair<std::string, DxShader>> shaders;
        for (const auto kerName : nldxCtx.KernelNames)
        {
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