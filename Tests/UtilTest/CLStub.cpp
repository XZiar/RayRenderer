#include "TestRely.h"
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
            const auto prog = NLCLProc.Parse(common::as_bytes(common::to_span(kertxt)));
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
