#include "TestRely.h"
#include "XCompCommon.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclNLCL.h"
#include "OpenCLUtil/oclNLCLRely.h"
#include "OpenCLUtil/oclKernelDebug.h"
#include "OpenCLUtil/oclException.h"
#include "SystemCommon/ConsoleEx.h"
#include "SystemCommon/StringConvert.h"
#include "common/Linq2.hpp"
#include "common/StringLinq.hpp"
#include "common/StrParsePack.hpp"
#include "common/CharConvs.hpp"
#include "common/StringEx.hpp"
#include <iostream>
#include <deque>


using namespace common;
using namespace common::mlog;
using namespace oclu;
using namespace std::string_view_literals;
using std::string;
using std::u16string;
using std::u16string_view;
using std::cin;
using xziar::img::TexFormatUtil;
using common::str::Encoding;
using common::str::to_u16string;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OCLStub", { GetConsoleBackend() });
    return logger;
}
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


namespace
{

struct CLStubHelper : public XCStubHelper
{
    struct NLCLContext_ : public NLCLContext
    {
        friend CLStubHelper;
    };
    ~CLStubHelper() override { }
    static KernelArgInfo GetArgInfo(const RunArgInfo& info) noexcept
    {
        const auto& args = *reinterpret_cast<const KernelArgStore*>(static_cast<uintptr_t>(info.Meta0));
        return args[info.Meta1];
    }
    std::any TryFindKernel(xcomp::XCNLContext& context, std::string_view name) const final
    {
        auto& ctx = static_cast<NLCLContext_&>(context);
        for (const auto& [kname, karg] : ctx.CompiledKernels)
        {
            if (name == kname)
            {
                return &karg;
            }
        }
        return {};
    }
    void FillArgs(std::vector<RunArgInfo>& dst, const std::any& cookie) const final
    {
        const auto args = std::any_cast<const KernelArgStore*>(cookie);
        const uint64_t meta0 = reinterpret_cast<uintptr_t>(args);
        dst.reserve(args->GetSize());
        uint32_t idx = 0;
        for (const auto arg : *args)
        {
            dst.emplace_back(meta0, idx++, [](const auto type) 
                {
                    switch (type)
                    {
                    case KerArgType::Buffer: return RunArgInfo::ArgType::Buffer;
                    case KerArgType::Image:  return RunArgInfo::ArgType::Image;
                    case KerArgType::Simple:
                    case KerArgType::Any:
                    default:                 return RunArgInfo::ArgType::Val32;
                    }
                }(arg.ArgType));
        }
    }
    std::optional<size_t> FindArgIdx(common::span<const RunArgInfo> args, std::string_view name) const final
    {
        size_t idx = 0;
        for (const auto& arg : args)
        {
            if (GetArgInfo(arg).Name == name)
                return idx;
            idx++;
        }
        return {};
    }
    bool CheckType(const RunArgInfo& dst, RunArgInfo::ArgType type) const noexcept final
    {
        switch (GetArgInfo(dst).ArgType)
        {
        case KerArgType::Buffer: return type == RunArgInfo::ArgType::Buffer;
        case KerArgType::Image:  return type == RunArgInfo::ArgType::Image;
        case KerArgType::Simple: return type != RunArgInfo::ArgType::Buffer && type != RunArgInfo::ArgType::Image;
        case KerArgType::Any:
        default:                 return false;
        }
    }
    std::string_view GetRealTypeName(const RunArgInfo& info) const noexcept final
    {
        return GetArgInfo(info).GetArgTypeName();
    }
};
static CLStubHelper StubHelper;

}


static void RunKernel(oclDevice dev, oclContext ctx, oclProgram prog, const RunInfo& info)
{
    const auto que = oclCmdQue_::Create(ctx, dev);
    const std::vector<oclKernel> kernels = prog->GetKernels();
    common::mlog::SyncConsoleBackend();
    {
        size_t idx = 0;
        for (const auto& conf : info.Configs)
        {
            PrintColored(common::console::ConsoleColor::BrightWhite,
                FMTSTR(u"[{:3}] {}\n", idx++, conf.Name));
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
        const RunConfig* config = nullptr;
        {
            std::string_view kerName;
            const auto name32 = common::str::to_u32string(parts[0]);
            for (const auto& conf : info.Configs)
            {
                if (conf.Name == name32)
                {
                    config = &conf;
                    break;
                }
            }
            if (kerName.empty())
            {
                if (uint32_t idx = 0; common::StrToInt(parts[0], idx).first)
                {
                    if (idx < info.Configs.size())
                    {
                        config = &info.Configs[idx];
                    }
                }
            }
        }
        if (!config)
        {
            log().warning(u"no config found for [{}]\n", parts[0]);
            continue;
        }
        oclKernel kernel;
        for (const auto& ker : kernels)
        {
            if (ker->Name == config->KernelName)
            {
                kernel = ker; break;
            }
        }
        CallArgs args;
        {
            for (const auto& arg : config->Args)
            {
                switch (arg.Type)
                {
                case RunArgInfo::ArgType::Buffer:
                {
                    const auto buf = oclBuffer_::Create(ctx, MemFlag::ReadWrite | MemFlag::HostNoAccess, arg.Val0);
                    args.PushArg(buf);
                } break;
                case RunArgInfo::ArgType::Image:
                {
                    const auto img = oclImage2D_::Create(ctx, MemFlag::ReadWrite | MemFlag::HostNoAccess, 
                        arg.Val0, arg.Val1, xziar::img::TextureFormat::RGBA8);
                    args.PushArg(img);
                } break;
                case RunArgInfo::ArgType::Val8:
                {
                    args.PushSimpleArg(static_cast<uint8_t>(arg.Val0));
                } break;
                case RunArgInfo::ArgType::Val16:
                {
                    args.PushSimpleArg(static_cast<uint16_t>(arg.Val0));
                } break;
                case RunArgInfo::ArgType::Val32:
                {
                    args.PushSimpleArg(static_cast<uint32_t>(arg.Val0));
                } break;
                case RunArgInfo::ArgType::Val64:
                {
                    args.PushSimpleArg((static_cast<uint64_t>(arg.Val0) << 32) + arg.Val1);
                } break;
                default: break;
                }
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
        const uint32_t worksizeX = NumOr(parts, 2, static_cast<uint32_t>(config->WgSize[0]));
        const uint32_t worksizeY = NumOr(parts, 3, static_cast<uint32_t>(config->WgSize[1]));
        const uint32_t worksizeZ = NumOr(parts, 4, static_cast<uint32_t>(config->WgSize[2]));
        SizeN<3> lcSize = config->LcSize;
        if (!lcSize.GetData(true))
            lcSize = kernel->WgInfo.CompiledWorkGroupSize;
        //const SizeN<3> lcSize = kernel->WgInfo.CompiledWorkGroupSize;
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
        RunInfo runInfo;
        if (isNLCL)
        {
            static const NLCLProcessor NLCLProc;
            const auto prog = NLCLProc.Parse(common::as_bytes(common::to_span(kertxt)), filepath.u16string());
            prog->AttachExtension([&](auto&, auto& ctx) { return StubHelper.GenerateExtension(ctx, runInfo); });
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
            for (const auto arg : ker->ArgStore)
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
            RunKernel(dev, ctx, clProg, runInfo);
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
                return FMTSTR(u"[{:04X}:{:02X}.{:<2X}]{}  {{{} | {}}}\t[{} CU]", 
                    dev->PCIEBus, dev->PCIEDev, dev->PCIEFunc,
                    dev->Name, dev->Ver, dev->CVer, dev->ComputeUnits);
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
                for (const auto ext : dev->Extensions)
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
#define ADD_INFO(info) APPEND_FMT(infotxt, u"{}: [{}]\n"sv, PPCAT(u, STRINGIZE(info)), dev->info)
                ADD_INFO(Name);
                ADD_INFO(Vendor);
                APPEND_FMT(infotxt, u"VendorId: [{:#08x}]\n"sv, dev->VendorId);
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
                APPEND_FMT(infotxt, u"MaxWorkItemSize: [{}x{}x{}]\n"sv, dev->MaxWorkItemSize[0], dev->MaxWorkItemSize[1], dev->MaxWorkItemSize[2]);
                ADD_INFO(MaxWorkGroupSize);
                APPEND_FMT(infotxt, u"F64Caps: [ {} ]\n"sv, oclDevice_::GetFPCapabilityStr(dev->F64Caps));
                APPEND_FMT(infotxt, u"F32Caps: [ {} ]\n"sv, oclDevice_::GetFPCapabilityStr(dev->F32Caps));
                APPEND_FMT(infotxt, u"F16Caps: [ {} ]\n"sv, oclDevice_::GetFPCapabilityStr(dev->F16Caps));
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
