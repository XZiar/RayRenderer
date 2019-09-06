#include "TestRely.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclException.h"
#include "common/Linq.hpp"


using namespace common;
using namespace common::mlog;
using namespace oclu;
using namespace oglu;
using std::string;
using std::cin;
using common::linq::Linq;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OCLStub", { GetConsoleBackend() });
    return logger;
}

static void OCLStub()
{
    oclUtil::Init(false);

    Linq::FromIterable(oclUtil::GetPlatforms())
        .ForEach([i = 0u](const auto& plat) mutable { log().info(u"option[{}] {}\t{}\n", i++, plat->Name, plat->Ver); });
    uint32_t platidx = 0;
    std::cin >> platidx;
    const auto plat = oclUtil::GetPlatforms()[platidx];

    auto thedev = Linq::FromIterable(plat->GetDevices())
        .Where([](const auto& dev) { return dev->Type == DeviceType::GPU; })
        .TryGetFirst().value_or(plat->GetDefaultDevice());
    const auto ctx = plat->CreateContext(thedev);
    ctx->onMessage = [](const auto& str) { log().debug(u"[MSG]{}\n", str); };
    oclCmdQue que(ctx, thedev);
    ClearReturn();
    //SimpleTest(ctx);
    while (true)
    {
        log().info(u"input opencl file:");
        string fpath;
        std::getline(cin, fpath);
        if (fpath == "EXTENSION")
        {
            string exttxts("Extensions:\n");
            for (const auto& ext : thedev->Extensions)
                exttxts.append(ext).append("\n");
            log().verbose(u"{}\n", exttxts);
            continue;
        }
        else if (fpath == "IMAGE")
        {
            string img2d("2DImage Supports:\n");
            for (const auto& dformat : ctx->Img2DFormatSupport)
                img2d.append(oglu::TexFormatUtil::GetFormatDetail(dformat)).append("\n");
            string img3d("3DImage Supports:\n");
            for (const auto& dformat : ctx->Img3DFormatSupport)
                img3d.append(oglu::TexFormatUtil::GetFormatDetail(dformat)).append("\n");
            log().verbose(u"{}{}\n", img2d, img3d);
            continue;
        }
        bool exConfig = false;
        if (fpath.size() > 0 && fpath.back() == '#')
            fpath.pop_back(), exConfig = true;
        common::fs::path filepath = FindPath() / fpath;
        log().debug(u"loading cl file [{}]\n", filepath.u16string());
        try
        {
            const auto kertxt = common::file::ReadAllText(filepath);
            oclProgram clProg(ctx, kertxt);
            CLProgConfig config;
            config.Defines["LOC_MEM_SIZE"] = thedev->LocalMemSize;
            if (exConfig)
            {
                string line;
                while (cin >> line)
                {
                    ClearReturn();
                    if (line.size() == 0) break;
                    const auto parts = common::str::Split(&line[1], line.size() - 1, '=');
                    switch (line.front())
                    {
                    case '#':
                        if (parts.size() > 1)
                            config.Defines[string(parts[0])] = string(parts[1].cbegin(), parts.back().cend());
                        else
                            config.Defines[string(parts[0])] = std::monostate{};
                        continue;
                    }
                    break;
                }
            }
            clProg->Build(config);
            log().success(u"loaded! kernels:\n");
            for (const auto& ker : clProg->GetKernels())
            {
                const auto wgInfo = ker->GetWorkGroupInfo(thedev);
                log().info(u"{}:\nPmem[{}], Smem[{}], Spill[{}], Size[{}]({}x), requireSize[{}x{}x{}]\n", ker->Name,
                    wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize, wgInfo.SpillMemSize,
                    wgInfo.WorkGroupSize, wgInfo.PreferredWorkGroupSizeMultiple,
                    wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2]);
                const auto sgInfo = ker->GetSubgroupInfo(thedev, 3, wgInfo.CompiledWorkGroupSize);
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
