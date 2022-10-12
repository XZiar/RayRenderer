#include "XCompCommon.h"
#include "TestRely.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/ConsoleEx.h"
#include "Nailang/NailangAutoVar.h"

#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)
#define APPEND_FMT(dst, syntax, ...) common::str::Formatter<typename std::decay_t<decltype(dst)>::value_type>{}\
    .FormatToStatic(dst, FmtString(syntax), __VA_ARGS__)

using namespace xziar::nailang;
using namespace xcomp;
using namespace std::string_view_literals;
using common::str::Encoding;


void PrintCommonDevice()
{
    [[maybe_unused]] static auto dummy = []() 
    {
        auto& fmter = GetLogFmt();
        fmter.FormatToStatic(fmter.Str, FmtString(u"{@<w}Common Devices:\n"sv));
        for (const auto& dev : xcomp::ProbeDevice())
        {
            fmter.FormatToStatic(fmter.Str, FmtString(u"{@<W}[{}][VID {:#010x} DID {:#010x}][Disp{:>4}]{@<G}{} {@<w}[{}][{}]\n"sv),
                dev.PCIEAddress, dev.VendorId, dev.DeviceId, std::min(dev.GetDisplay(), 9999u), dev.Name,
                common::MiscIntrin.HexToStr(dev.Guid), common::MiscIntrin.HexToStr(dev.Luid));
        }
        PrintToConsole(fmter);
        return 0;
    }();
}


std::u16string_view RunArgInfo::GetTypeName(const ArgType type) noexcept
{
    using namespace std::string_view_literals;
    switch (type)
    {
    case ArgType::Buffer:   return u"Buffer"sv;
    case ArgType::Image:    return u"Image"sv;
    case ArgType::Val8:     return u"Val8"sv;
    case ArgType::Val16:    return u"Val16"sv;
    case ArgType::Val32:    return u"Val32"sv;
    case ArgType::Val64:    return u"Val64"sv;
    default:                return u"unknwon"sv;
    }
}

ArgWrapperHandler ArgWrapperHandler::Handler;


struct XCStubHelper::RunConfigVar : public xziar::nailang::AutoVarHandler<RunConfig>
{
    const XCStubHelper& Helper;
    RunConfigVar(const XCStubHelper& helper) : AutoVarHandler<RunConfig>(U"RunConfig"sv), Helper(helper)
    {
        AddCustomMember(U"WgSize"sv, [](RunConfig& conf)
            {
                return xcomp::GeneralVecRef::Create<size_t>(conf.WgSize);
            }).SetConst(false);
            AddCustomMember(U"LcSize"sv, [](RunConfig& conf)
                {
                    return xcomp::GeneralVecRef::Create<size_t>(conf.LcSize);
                }).SetConst(false);
                AddAutoMember<RunArgInfo>(U"Args"sv, U"RunArg"sv, [](RunConfig& conf)
                    {
                        return common::to_span(conf.Args);
                    }, [&](auto& argHandler)
                    {
                        argHandler.SetAssigner([&](RunArgInfo& arg, Arg val)
                            {
                                if (!val.IsCustomType<ArgWrapperHandler>())
                                    COMMON_THROW(xziar::nailang::NailangRuntimeException, FMTSTR2(u"Arg can only be set with ArgWrapper, get [{}]", val.GetTypeName()));
                                const auto& var = val.GetCustom();
                                const auto type = static_cast<RunArgInfo::ArgType>(var.Meta2);
                                if (!Helper.CheckType(arg, type))
                                    COMMON_THROW(xziar::nailang::NailangRuntimeException, FMTSTR2(u"Arg is set with incompatible value, type [{}] get [{}]",
                                        Helper.GetRealTypeName(arg), RunArgInfo::GetTypeName(type)));
                                arg.Val0 = static_cast<uint32_t>(var.Meta0);
                                arg.Val1 = static_cast<uint32_t>(var.Meta1);
                                arg.Type = type;
                            });
                        argHandler.SetExtendIndexer([&](common::span<const RunArgInfo> all, const Arg& idx) -> std::optional<size_t>
                            {
                                if (idx.IsStr())
                                    return Helper.FindArgIdx(all, common::str::to_string(idx.GetStr().value(), Encoding::UTF8));
                                return {};
                            });
                    }).SetConst(false);
    }

    Arg ConvertToCommon(const CustomVar&, Arg::Type type) noexcept final
    {
        if (type == xziar::nailang::Arg::Type::Bool)
            return true;
        return {};
    }
};



struct XCStubHelper::XCStubExtension : public XCNLExtension
{
private:
    const XCStubHelper& Helper;
    RunInfo& Info;
public:
    XCStubExtension(const XCStubHelper& helper, XCNLContext& context, RunInfo& info) :
        XCNLExtension(context), Helper(helper), Info(info)
    { }
    ~XCStubExtension() override { }

    void  BeginXCNL(XCNLRuntime&) override { }
    void FinishXCNL(XCNLRuntime&) override { }
    std::optional<Arg> ConfigFunc(XCNLExecutor& executor, xziar::nailang::FuncEvalPack& func) override
    {
        const auto& fname = func.GetName();
        if (fname[0] == U"xcstub"sv)
        {
            const auto sub = fname[1];
            if (sub == U"AddRun"sv)
            {
                executor.ThrowByParamTypes<2>(func, { Arg::Type::String, Arg::Type::String });
                const auto name = func.Params[0].GetStr().value();
                const auto kerName = common::str::to_string(func.Params[1].GetStr().value(), Encoding::UTF8);
                const auto cookie = Helper.TryFindKernel(Context, kerName);
                if (!cookie.has_value())
                    COMMON_THROW(NailangRuntimeException, FMTSTR2(u"Does not found kernel [{}]", kerName));
                auto& config = Info.Configs.emplace_back(name, kerName);
                Helper.FillArgs(config.Args, cookie);
                return Helper.RunConfigHandler->CreateVar(config);
            }
            else if (sub == U"BufArg"sv)
            {
                executor.ThrowByParamTypes<1>(func, { Arg::Type::Integer });
                const auto size = func.Params[0].GetUint().value();
                return ArgWrapperHandler::CreateBuffer(gsl::narrow_cast<uint32_t>(size));
            }
            else if (sub == U"ImgArg"sv)
            {
                executor.ThrowByParamTypes<2>(func, { Arg::Type::Integer, Arg::Type::Integer });
                const auto width  = gsl::narrow_cast<uint32_t>(func.Params[0].GetUint().value());
                const auto height = gsl::narrow_cast<uint32_t>(func.Params[1].GetUint().value());
                return ArgWrapperHandler::CreateImage(width, height);
            }
            else if (sub == U"ValArg"sv)
            {
                const auto type = fname[2];
                executor.ThrowByParamTypes<1>(func, { Arg::Type::Integer });
                if (type == U"u8")
                {
                    const auto val = func.Params[0].GetUint().value();
                    return ArgWrapperHandler::CreateVal8(gsl::narrow_cast<uint8_t>(val));
                }
                else if (type == U"u16")
                {
                    const auto val = func.Params[0].GetUint().value();
                    return ArgWrapperHandler::CreateVal16(gsl::narrow_cast<uint16_t>(val));
                }
                else if (type == U"u32")
                {
                    const auto val = func.Params[0].GetUint().value();
                    return ArgWrapperHandler::CreateVal32(gsl::narrow_cast<uint32_t>(val));
                }
                else if (type == U"u64")
                {
                    const auto val = func.Params[0].GetUint().value();
                    return ArgWrapperHandler::CreateVal64(gsl::narrow_cast<uint64_t>(val));
                }
                executor.HandleException(CREATE_EXCEPTION(NailangRuntimeException, FMTSTR2(u"Unknown type for ValArg [{}]", type)));
            }
        }
        return {};
    }
};


XCStubHelper::XCStubHelper() : RunConfigHandler(std::make_unique<RunConfigVar>(*this))
{ }
XCStubHelper::~XCStubHelper()
{ }
std::unique_ptr<XCNLExtension> XCStubHelper::GenerateExtension(XCNLContext& context, RunInfo& info) const
{
    return std::make_unique<XCStubExtension>(*this, context, info);
}
