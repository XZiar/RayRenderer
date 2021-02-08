#include "TestRely.h"
#include "XComputeBase/XCompNailang.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclNLCL.h"
#include "OpenCLUtil/oclNLCLRely.h"
#include "OpenCLUtil/oclKernelDebug.h"
#include "OpenCLUtil/oclException.h"
#include "SystemCommon/ConsoleEx.h"
#include "StringUtil/Convert.h"
#include "common/Linq2.hpp"
#include "common/StringLinq.hpp"
#include "common/CharConvs.hpp"
#include "common/StringEx.hpp"
#include <iostream>
#include <mutex>


using namespace common;
using namespace common::mlog;
using namespace oclu;
using namespace std::string_view_literals;
using std::string;
using std::u16string;
using std::u16string_view;
using std::cin;
using xziar::img::TexFormatUtil;
using common::str::to_u16string;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OCLStub", { GetConsoleBackend() });
    return logger;
}
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


namespace
{
using namespace xziar::nailang;

struct RunArgInfo
{
    enum class ArgType : uint16_t { Buffer, Image, Val8, Val16, Val32, Val64 };
    ArgFlags Flags;
    uint32_t Val0;
    uint32_t Val1;
    ArgType Type;
    constexpr RunArgInfo(const ArgFlags& flags) noexcept : Flags(flags), Val0(0), Val1(0), Type([](auto type) 
            {
            switch (type)
            {
            case KerArgType::Buffer: return ArgType::Buffer;
            case KerArgType::Image:  return ArgType::Image;
            case KerArgType::Simple:
            case KerArgType::Any:
            default:                 return ArgType::Val32;
            }
            }(Flags.ArgType)) 
    { }
    constexpr bool CheckType(ArgType type) const noexcept
    {
        switch (Flags.ArgType)
        {
        case KerArgType::Buffer: return type == ArgType::Buffer;
        case KerArgType::Image:  return type == ArgType::Image;
        case KerArgType::Simple: return type != ArgType::Buffer && type != ArgType::Image;
        case KerArgType::Any:    
        default:                 return false;
        }
    }
    static constexpr std::u16string_view GetTypeName(const ArgType type) noexcept
    {
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
};

struct RunConfig 
{
    oclKernel Kernel;
    std::vector<RunArgInfo> Args;
    std::array<size_t, 3> WgSize;
    std::array<size_t, 3> LcSize;
};
struct RunInfo
{
    std::vector<RunConfig> Configs;
};

struct ArgWrapperHandler : public CustomVar::Handler
{
    static ArgWrapperHandler Handler;
    static CustomVar CreateBuffer(uint32_t size)
    {
        return { &Handler, size, size, common::enum_cast(RunArgInfo::ArgType::Buffer) };
    }
    static CustomVar CreateImage(uint32_t w, uint32_t h)
    {
        return { &Handler, w, h, common::enum_cast(RunArgInfo::ArgType::Image) };
    }
    static CustomVar CreateVal8(uint8_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val8) };
    }
    static CustomVar CreateVal16(uint16_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val16) };
    }
    static CustomVar CreateVal32(uint32_t val)
    {
        return { &Handler, val, val, common::enum_cast(RunArgInfo::ArgType::Val32) };
    }
    static CustomVar CreateVal64(uint64_t val)
    {
        return { &Handler, val >> 32, static_cast<uint32_t>(val & UINT32_MAX), common::enum_cast(RunArgInfo::ArgType::Val64) };
    }
};
ArgWrapperHandler ArgWrapperHandler::Handler;


struct RunConfigVar : public AutoVarHandler<RunConfig>
{
    RunConfigVar() : AutoVarHandler<RunConfig>(U"RunConfig"sv)
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
            }, [](auto& argHandler) 
            {
                argHandler.SetAssigner([](RunArgInfo& arg, Arg val)
                {
                    if (!val.IsCustomType<ArgWrapperHandler>())
                        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Arg can only be set with ArgWrapper, get [{}]", val.GetTypeName()));
                    const auto& var = val.GetCustom();
                    const auto type = static_cast<RunArgInfo::ArgType>(var.Meta2);
                    if (!arg.CheckType(type))
                        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Arg is set with incompatible value, type [{}] get [{}]", 
                            arg.Flags.GetArgTypeName(), RunArgInfo::GetTypeName(type)));
                    arg.Val0 = static_cast<uint32_t>(var.Meta0);
                    arg.Val1 = static_cast<uint32_t>(var.Meta1);
                    arg.Type = type;
                });
            }).SetConst(false);
    }
    //size_t HandleSetter(CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime, Arg arg) override
    //{
    //    Expects(subq.Size() > 0);
    //    const auto [type, query] = subq[0];
    //    if (type != SubQuery::QueryType::Sub) return 0;
    //    const auto subf = query.GetVar<RawArg::Type::Str>();
    //    auto& config = reinterpret_cast<RunInfo*>(var.Meta0)->Configs[var.Meta2];
    //    const auto SetVec3 = [&](auto& dst, std::u16string_view name) -> size_t
    //    {
    //        if (subq.Size() == 1)
    //        {
    //            if (!arg.IsCustomType<xcomp::GeneralVecRef>())
    //                COMMON_THROW(NailangRuntimeException, FMTSTR(u"{} can only be set with vec, get [{}]", name, arg.GetTypeName()), var);
    //            const auto arr = xcomp::GeneralVecRef::ToArray(arg.GetCustom());
    //            dst[0] = arr.Get(0).GetUint().value();
    //            dst[1] = arr.Get(1).GetUint().value();
    //            dst[2] = arr.Get(2).GetUint().value();
    //            return 1;
    //        }
    //        return 1 + xcomp::GeneralVecRef::Create<size_t>(dst).Call<&CustomVar::Handler::HandleSetter>(subq.Sub(1), runtime, std::move(arg));
    //    };
    //    if (subf == U"WgSize")
    //        return SetVec3(config.WgSize, u"WgSize"sv);
    //    if (subf == U"LcSize")
    //        return SetVec3(config.WgSize, u"LcSize"sv);
    //    if (subf == U"Args")
    //    {
    //        if (subq.Size() < 2)
    //            COMMON_THROW(NailangRuntimeException, u"Field [Args] can not be assigned"sv, var);
    //        const auto& idx_ = subq.ExpectIndex(1);
    //        const auto idx = NailangHelper::BiDirIndexCheck(config.Args.size(), EvaluateArg(runtime, idx_), &idx_);
    //        const auto& argInfo = config.Kernel->ArgStore[idx];
    //        /*switch (argInfo.ArgType)
    //        {
    //            case KerArgType::Buffer
    //        }
    //        return { std::u32string(1, str[idx]), 1u };*/
    //    }
    //    return 0;
    //}
    Arg ConvertToCommon(const CustomVar&, Arg::Type type) noexcept override
    {
        if (type == Arg::Type::Bool)
            return true;
        return {};
    }
    static RunConfigVar Handler;
};
RunConfigVar RunConfigVar::Handler;

}


static void RunKernel(oclDevice dev, oclContext ctx, oclProgram prog, const std::unique_ptr<NLCLResult>& nlclRes)
{
    const auto que = oclCmdQue_::Create(ctx, dev);
    const std::vector<oclKernel> kernels = prog->GetKernels();
    common::mlog::SyncConsoleBackend();
    {
        size_t idx = 0;
        for (const auto& ker : kernels)
        {
            PrintColored(common::console::ConsoleColor::BrightWhite,
                FMTSTR(u"[{:3}] {}\n", idx++, ker->Name));
        }
    }
    while (true)
    {
        common::mlog::SyncConsoleBackend();
        PrintColored(common::console::ConsoleColor::BrightWhite, u"Enter command <kenrel|idx>[,repeat,wkX,wkY,wkZ] :\n"sv);
        const auto line = common::console::ConsoleEx::ReadLine();
        if (line == "break")
            break;
        if (line.empty())
            continue;
        const auto parts = common::str::Split(line, ',', true);
        Expects(parts.size() > 0);
        oclKernel kernel;
        for (const auto& ker : kernels)
        {
            if (ker->Name == parts[0])
            {
                kernel = ker; break;
            }
        }
        if (!kernel)
        {
            uint32_t idx = 0;
            if (common::StrToInt(parts[0], idx).first)
            {
                if (idx < kernels.size())
                    kernel = kernels[idx];
            }
        }
        if (!kernel)
        {
            log().warning(u"no kernel found for [{}]\n", parts[0]);
            continue;
        }
        const auto queryU32 = [&](std::string_view argName) -> std::optional<uint32_t>
        {
            if (nlclRes)
            {
                const auto name = fmt::format(U"clstub.{}.{}"sv, kernel->Name, argName);
                const auto result = nlclRes->QueryResult(name);
                switch (result.index())
                {
                case 1:  return std::get<1>(result) ? 1u : 0u;
                case 2:  return static_cast<uint32_t>(std::get<2>(result));
                case 3:  return static_cast<uint32_t>(std::get<3>(result));
                case 4:  return static_cast<uint32_t>(std::get<4>(result));
                case 0:
                case 5:
                default: return {};
                }
            }
            else
                return {};
        };
        const auto queryArgU32 = [&](size_t idx, std::string_view suffix = {}) -> std::optional<uint32_t>
        {
            const auto name = fmt::format(suffix.empty() ? "args.{}"sv : "args.{}.{}"sv, idx, suffix);
            return queryU32(name);
        };
        CallArgs args;
        {
            size_t idx = 0;
            for (const auto& arg : kernel->ArgStore)
            {
                if (arg.ArgType == KerArgType::Buffer)
                {
                    const auto bufSize = queryArgU32(idx).value_or(4096u);
                    const auto buf = oclBuffer_::Create(ctx, MemFlag::ReadWrite | MemFlag::HostNoAccess, bufSize);
                    args.PushArg(buf);
                }
                else if (arg.ArgType == KerArgType::Image)
                {
                    const auto w = queryArgU32(idx, "w").value_or(256u);
                    const auto h = queryArgU32(idx, "h").value_or(256u);
                    const auto img = oclImage2D_::Create(ctx, MemFlag::ReadWrite | MemFlag::HostNoAccess, w, h, xziar::img::TextureFormat::RGBA8);
                    args.PushArg(img);
                }
                else
                {
                    const auto val = queryArgU32(idx).value_or(0u);
                    args.PushSimpleArg(val);
                }
                idx++;
            }
        }
        constexpr auto NumOr = [](const auto& strs, const size_t idx, uint32_t num = 1) 
        {
            if (idx < strs.size())
                common::StrToInt(strs[idx], num);
            return num;
        };
        auto callsite = kernel->CallDynamic<3>(std::move(args));
        const uint32_t repeats   = NumOr(parts, 1);
        const uint32_t worksizeX = queryU32("wk.X").value_or(NumOr(parts, 2));
        const uint32_t worksizeY = queryU32("wk.Y").value_or(NumOr(parts, 3));
        const uint32_t worksizeZ = queryU32("wk.Z").value_or(NumOr(parts, 4));
        const SizeN<3> lcSize = kernel->WgInfo.CompiledWorkGroupSize;
        log().verbose(u"run kernel [{}] with [{}x{}x{}] for [{}] times\n",
            kernel->Name, worksizeX, worksizeY, worksizeZ, repeats);
        common::PromiseResult<CallResult> lastPms;
        std::vector<common::PromiseResult<CallResult>> pmss; pmss.reserve(repeats);
        for (uint32_t i = 0; i < repeats; ++i)
        {
            lastPms = callsite(lastPms, que, { worksizeX, worksizeY, worksizeZ }, lcSize);
            pmss.push_back(lastPms);
        }
        lastPms->WaitFinish();

        uint64_t timeMin = UINT64_MAX, timeMax = 0, total = 0;
        for (const auto& pms : pmss)
        {
            const auto clPms = dynamic_cast<oclu::oclPromiseCore*>(&pms->GetPromise());
            if (!clPms) continue;
            const auto timeB = clPms->QueryTime(oclu::oclPromiseCore::TimeType::Start);
            const auto timeE = clPms->QueryTime(oclu::oclPromiseCore::TimeType::End);
            timeMin = std::min(timeB, timeMin);
            timeMax = std::max(timeE, timeMax);
            total += timeE - timeB;
        }
        const auto e2eTime = timeMax - timeMin;
        const auto avgTime = total / repeats;
        log().success(FMT_STRING(u"Finish, E2ETime {:1.5f}ms, avg KernelTime {:1.5f}ms.\n"), e2eTime / 1e6f, avgTime / 1e6f);
    }
}

static void TestOCL(oclDevice dev, oclContext ctx, std::string fpath)
{
    bool exConfig = false, runnable = false;
    while (true)
    {
        const auto ch = fpath.back();
        if (ch == '#')
            exConfig = true;
        else if (ch == '@')
            runnable = true;
        else
            break;
        fpath.pop_back();
    }
    common::fs::path filepath = fpath;
    const bool isNLCL = filepath.extension().string() == ".nlcl";
    log().debug(u"loading cl file [{}]\n", filepath.u16string());
    try
    {
        const auto kertxt = common::file::ReadAllText(filepath);
        CLProgConfig config;
        config.Defines["LOC_MEM_SIZE"] = dev->LocalMemSize;
        if (exConfig)
        {
            string line;
            while (cin >> line)
            {
                ClearReturn();
                if (line.size() == 0) break;
                const auto parts = common::str::Split(line, '=');
                string key(parts[0].substr(1));
                switch (line.front())
                {
                case '#':
                    if (parts.size() > 1)
                        config.Defines[key] = string(parts[1].cbegin(), parts.back().cend());
                    else
                        config.Defines[key] = std::monostate{};
                    continue;
                case '@':
                    if (key == "version")
                        config.Version = (parts[1][0] - '0') * 10 + (parts[1][1] - '0');
                    else
                        config.Flags.insert(key);
                    continue;
                }
                break;
            }
        }
        std::unique_ptr<NLCLResult> nlclRes;
        oclu::oclProgram clProg;
        if (isNLCL)
        {
            static const NLCLProcessor NLCLProc;
            const auto prog = NLCLProc.Parse(common::as_bytes(common::to_span(kertxt)), filepath.u16string());
            nlclRes = NLCLProc.CompileProgram(prog, ctx, dev, {}, config);
            common::file::WriteAll(fpath + ".cl", nlclRes->GetNewSource());
            clProg = nlclRes->GetProgram();
        }
        else
        {
            clProg = oclProgram_::CreateAndBuild(ctx, kertxt, config, dev);
        }
        const auto kernels = clProg->GetKernels();
        log().success(u"loaded {} kernels:\n", kernels.Size());
        common::mlog::SyncConsoleBackend();
        for (const auto& ker : kernels)
        {
            const auto& wgInfo = ker->WgInfo;
            u16string txt;
            APPEND_FMT(txt, u"{}:\n-Pmem[{}], Smem[{}], Spill[{}], Size[{}]({}x), requireSize[{}x{}x{}]\n", ker->Name,
                wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize, wgInfo.SpillMemSize,
                wgInfo.WorkGroupSize, wgInfo.PreferredWorkGroupSizeMultiple,
                wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2]);
            if (const auto sgInfo = ker->GetSubgroupInfo(3, wgInfo.CompiledWorkGroupSize); sgInfo.has_value())
            {
                APPEND_FMT(txt, u"-Subgroup[{}] x[{}], requireSize[{}]\n", sgInfo->SubgroupSize, sgInfo->SubgroupCount, sgInfo->CompiledSubgroupSize);
            }
            txt.append(u"-Args:\n"sv);
            for (const auto& arg : ker->ArgStore)
            {
                if (arg.ArgType == KerArgType::Image)
                    APPEND_FMT(txt, u"---[Image ][{:9}]({:12})[{:12}][{}]\n", 
                        arg.GetImgAccessName(), arg.Type, arg.Name, arg.GetQualifierName());
                else
                    APPEND_FMT(txt, u"---[{:6}][{:9}]({:12})[{:12}][{}]\n", 
                        arg.GetArgTypeName(), arg.GetSpaceName(), arg.Type, arg.Name, arg.GetQualifierName());
            }
            txt.append(u"\n"sv);
            PrintColored(common::console::ConsoleColor::BrightWhite, txt);
        }
        const auto bin = clProg->GetBinary();
        if (!bin.empty())
            common::file::WriteAll(fpath + ".bin", bin);
        if (runnable)
        {
            RunKernel(dev, ctx, clProg, nlclRes);
        }
    }
    catch (const BaseException& be)
    {
        PrintException(be, u"Error here");
    }
}


static void OCLStub()
{
    const auto& plats = oclUtil::GetPlatforms();
    if (plats.size() == 0)
    {
        log().error(u"No OpenCL platform found!\n");
        return;
    }
    while (true)
    {
        const auto platidx = SelectIdx(plats, u"platform", [](const auto& plat)
            {
                return FMTSTR(u"{}  {{{}}}", plat->Name, plat->Ver);
            });
        const auto plat = plats[platidx];

        const auto devs = plat->GetDevices();
        if (devs.size() == 0)
        {
            log().error(u"No OpenCL device on the platform [{}]!\n", plat->Name);
            return;
        }
        const auto devidx = SelectIdx(devs, u"device", [](const auto& dev) 
            {
                return FMTSTR(u"{}  {{{} | {}}}\t[{} CU]", dev->Name, dev->Ver, dev->CVer, dev->ComputeUnits);
            });
        const auto dev = devs[devidx];

        const auto ctx = plat->CreateContext(dev);
        ctx->OnMessage += [](const auto& str) { log().debug(u"[MSG]{}\n", str); };
        auto que = oclCmdQue_::Create(ctx, dev);
        log().success(u"Create context with [{}] on [{}]!\n", dev->Name, plat->Name);
        //ClearReturn();
        //SimpleTest(ctx);
        while (true)
        {
            common::mlog::SyncConsoleBackend();
            string fpath = common::console::ConsoleEx::ReadLine("input opencl file:");
            if (fpath == "BREAK")
                break;
            if (fpath == "EXTENSION")
            {
                string exttxts("Extensions:\n");
                for (const auto& ext : dev->Extensions)
                    exttxts.append(ext).append("\n");
                log().verbose(u"{}\n", exttxts);
                continue;
            }
            else if (fpath == "IMAGE")
            {
                const auto proc = [](u16string str, auto& formats)
                {
                    for (const auto& format : formats)
                        str.append(TexFormatUtil::GetFormatName(format)).append(u"\t")
                        .append(to_u16string(TexFormatUtil::GetFormatDetail(format))).append(u"\n");
                    return str;
                };
                log().verbose(u"{}", proc(u"2DImage Supports:\n", ctx->Img2DFormatSupport));
                log().verbose(u"{}\n", proc(u"3DImage Supports:\n", ctx->Img3DFormatSupport));
                continue;
            }
            else if (fpath == "DEBUG")
            {
                const auto sgItemField = xcomp::debug::WGInfoHelper::Fields<oclu::debug::SubgroupWgInfo>();
                std::string str;
                for (const auto& field : sgItemField)
                {
                    fmt::format_to(std::back_inserter(str), FMT_STRING("[{:20}]: offset[{:3}] byte[{}] dim[{}]\n"),
                        field.Name, field.Offset, field.VecType.Bit / 8, field.VecType.Dim0);
                }
                log().verbose(u"Fields of SubgroupWgInfo:\n{}\n", str);
                continue;
            }
            else if (fpath == "INFO")
            {
                std::u16string infotxt;
#define ADD_INFO(info) fmt::format_to(std::back_inserter(infotxt), u"{}: [{}]\n"sv, PPCAT(u, STRINGIZE(info)), dev->info)
                ADD_INFO(Name);
                ADD_INFO(Vendor);
                ADD_INFO(Ver);
                ADD_INFO(CVer);
                ADD_INFO(ConstantBufSize);
                ADD_INFO(GlobalMemSize);
                ADD_INFO(LocalMemSize);
                ADD_INFO(MaxMemAllocSize);
                ADD_INFO(GlobalCacheSize);
                ADD_INFO(GlobalCacheLine);
                ADD_INFO(MemBaseAddrAlign);
                ADD_INFO(ComputeUnits);
                ADD_INFO(WaveSize);
                ADD_INFO(SupportProfiling);
                ADD_INFO(SupportOutOfOrder);
                ADD_INFO(SupportImplicitGLSync);
                ADD_INFO(SupportImage);
                ADD_INFO(LittleEndian);
                ADD_INFO(HasCompiler);
                fmt::format_to(std::back_inserter(infotxt), u"{}: [{}x{}x{}]\n"sv, "MaxWorkItemSize", dev->MaxWorkItemSize[0], dev->MaxWorkItemSize[1], dev->MaxWorkItemSize[2]);
                ADD_INFO(MaxWorkGroupSize);
#undef ADD_INFO
                log().verbose(u"Device Info:\n{}\n", infotxt);
                continue;
            }
            else if (fpath == "clear")
            {
                common::console::ConsoleEx::ClearConsole();
                continue;
            }
            else if (fpath.empty())
                continue;

            TestOCL(dev, ctx, fpath);
        }
    }
}

const static uint32_t ID = RegistTest("OCLStub", &OCLStub);
