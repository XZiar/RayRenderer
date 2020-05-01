#include "TestRely.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclNLCL.h"
#include "OpenCLUtil/oclException.h"
#include "SystemCommon/ConsoleEx.h"
#include "StringCharset/Convert.h"
#include "common/Linq2.hpp"
#include "common/StringLinq.hpp"
#include "common/StringEx.hpp"


using namespace common;
using namespace common::mlog;
using namespace oclu;
using std::string;
using std::u16string;
using std::u16string_view;
using std::cin;
using xziar::img::TexFormatUtil;
using common::strchset::to_u16string;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OCLStub", { GetConsoleBackend() });
    return logger;
}


template<typename T>
uint32_t SelectIdx(const T& container, u16string_view name)
{
    if (container.size() <= 1)
        return 0;
    log().info(u"Select {} to use:\n", name);
    uint32_t idx = UINT32_MAX;
    do
    {
        const auto ch = common::console::ConsoleEx::ReadCharImmediate(false);
        if (ch >= '0' && ch <= '9')
            idx = ch - '0';
    } while (idx >= container.size());
    return idx;
}

static void OCLStub()
{
    const auto& plats = oclUtil::GetPlatforms();
    if (plats.size() == 0)
    {
        log().error(u"No OpenCL platform found!\n");
        return;
    }
    common::linq::FromIterable(plats)
        .ForEach([](const auto& plat, size_t idx) 
            { log().info(FMT_STRING(u"platform[{}] {}  {{{}}}\n"), idx, plat->Name, plat->Ver); });
    const auto platidx = SelectIdx(plats, u"platform");
    const auto plat = plats[platidx];

    const auto devs = plat->GetDevices();
    if (devs.size() == 0)
    {
        log().error(u"No OpenCL device on the platform [{}]!\n", plat->Name);
        return;
    }
    common::linq::FromIterable(devs)
        .ForEach([](const auto& dev, size_t idx)
            { log().info(FMT_STRING(u"device[{}] {}  {{{} | {}}}\t[{} CU]\n"), idx, dev->Name, dev->Ver, dev->CVer, dev->ComputeUnits); });
    const auto devidx = SelectIdx(devs, u"device");
    const auto dev = devs[devidx];

    const auto ctx = plat->CreateContext(dev);
    ctx->OnMessage += [](const auto& str) { log().debug(u"[MSG]{}\n", str); };
    auto que = oclCmdQue_::Create(ctx, dev);
    log().success(u"Create context with [{}] on [{}]!\n", dev->Name, plat->Name);
    //ClearReturn();
    //SimpleTest(ctx);
    while (true)
    {
        log().info(u"input opencl file:");
        string fpath;
        std::getline(cin, fpath);
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
        else if (fpath == "clear")
        {
            common::console::ConsoleEx::ClearConsole();
            continue;
        }
        else if (fpath.empty())
            continue;
        bool exConfig = false;
        if (fpath.back() == '#')
            fpath.pop_back(), exConfig = true;
        common::fs::path filepath = FindPath() / fpath;
        log().debug(u"loading cl file [{}]\n", filepath.u16string());
        try
        {
            auto kertxt = common::file::ReadAllText(filepath);
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
                    }
                    break;
                }
            }
            if (common::str::IsEndWith(fpath, ".nlcl"))
            {
                static const NLCLProcessor NLCLProc;
                const auto prog = NLCLProc.Parse(common::as_bytes(common::to_span(kertxt)));
                auto result = NLCLProc.ProcessCL(prog, dev);
                common::file::WriteAll(FindPath() / (fpath + ".cl"), result);
                kertxt = result;
            }
            auto clProg = oclProgram_::CreateAndBuild(ctx, kertxt, config);
            log().success(u"loaded! kernels:\n");
            for (const auto& ker : clProg->GetKernels())
            {
                const auto wgInfo = ker->GetWorkGroupInfo(dev);
                log().info(u"{}:\nPmem[{}], Smem[{}], Spill[{}], Size[{}]({}x), requireSize[{}x{}x{}]\n", ker->Name,
                    wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize, wgInfo.SpillMemSize,
                    wgInfo.WorkGroupSize, wgInfo.PreferredWorkGroupSizeMultiple,
                    wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2]);
                const auto sgInfo = ker->GetSubgroupInfo(dev, 3, wgInfo.CompiledWorkGroupSize);
                if (sgInfo.has_value())
                {
                    const auto& info = sgInfo.value();
                    log().info(u"{}:\nSubgroup[{}] x[{}], requireSize[{}]\n", ker->Name, info.SubgroupSize, info.SubgroupCount, info.CompiledSubgroupSize);
                }
                for (const auto& arg : ker->ArgsInfo)
                {
                    log().verbose(u"---[{:8}][{:9}]({:12})[{:12}][{}]\n", arg.GetSpace(), arg.GetImgAccess(), arg.Type, arg.Name, arg.GetQualifier());
                }
            }
            log().info(u"\n\n");
        }
        catch (OCLException& cle)
        {
            u16string buildLog;
            if (cle.data.has_value())
                buildLog = std::any_cast<u16string>(cle.data);
            log().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
        }
        catch (const BaseException& be)
        {
            log().error(u"Error here:\n{}\n\n", be.message);
        }
    }

}

const static uint32_t ID = RegistTest("OCLStub", &OCLStub);
