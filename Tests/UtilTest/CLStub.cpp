#include "TestRely.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclException.h"


using namespace common;
using namespace common::mlog;
using namespace oclu;
using namespace oglu;
using std::string;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OCLStub", { GetConsoleBackend() });
    return logger;
}

static auto LoadCtx()
{
    const auto& plats = oclu::oclUtil::getPlatforms();
    uint32_t platidx = 0;
    for (const auto& plat : plats)
    {
        log().info(u"option[{}] {}\t{}\n", platidx++, plat->Name, plat->Ver);
    }
    std::cin >> platidx;
    auto ctx = plats[platidx]->CreateContext();
    ctx->onMessage = [](const auto& str) { log().debug(u"[MSG]{}\n", str); };
    return ctx;
}

static void SimpleTest(const oclContext& ctx)
{
    oclImg3D img3d(ctx, MemFlag::WriteOnly | MemFlag::HostReadOnly, 64, 64, 64, TextureDataFormat::RGB10A2);
}

static void OCLStub()
{
    oclUtil::init(false);
    auto ctx = LoadCtx();
    oclCmdQue que;
    for (const auto& dev : ctx->Devices)
        if (dev->Type == DeviceType::GPU)
        {
            que.reset(ctx, dev);
            break;
        }
    if (!que)
        que.reset(ctx, ctx->Devices[0]);
    const auto dev = ctx->Devices[0];
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
            for(const auto& ext : dev->Extensions)
                exttxts.append(ext).append("\n");
            log().verbose(u"{}\n", exttxts);
            continue;
        }
        else if (fpath == "IMAGE")
        {
            string img2d("2DImage Supports:\n");
            for(const auto& dformat : ctx->Img2DFormatSupport)
                img2d.append(oglu::TexFormatUtil::GetFormatDetail(dformat)).append("\n");
            string img3d("3DImage Supports:\n");
            for(const auto& dformat : ctx->Img3DFormatSupport)
                img3d.append(oglu::TexFormatUtil::GetFormatDetail(dformat)).append("\n");
            log().verbose(u"{}{}\n", img2d, img3d);
            continue;
        }
        common::fs::path filepath = FindPath() / fpath;
        log().debug(u"loading cl file [{}]\n", filepath.u16string());
        try
        {
            const auto kertxt = common::file::ReadAllText(filepath);
            oclProgram clProg(ctx, kertxt);
            try
            {
                oclu::CLProgConfig config;
                config.Defines.insert_or_assign("LOC_MEM_SIZE", ctx->Devices[0]->LocalMemSize);
                clProg->Build(config);
                log().success(u"loaded! kernels:\n");
                for (const auto& ker : clProg->GetKernels())
                {
                    const auto wgInfo = ker->GetWorkGroupInfo(dev);
                    log().info(u"{}:\nPmem[{}], Smem[{}], Size[{}]({}x), requireSize[{}x{}x{}]\n", ker->Name, 
                        wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize, wgInfo.WorkGroupSize, wgInfo.PreferredWorkGroupSizeMultiple,
                        wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2]);
                    for (const auto& arg : ker->ArgsInfo)
                    {
                        log().verbose(u"---[{:8}][{:9}]({:12})[{}][{}]\n", arg.GetSpace(), arg.GetImgAccess(), arg.Type, arg.Name, arg.GetQualifier());
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
                COMMON_THROW(BaseException, u"build Program error");
            }
        }
        catch (const BaseException& be)
        {
            log().error(u"Error here:\n{}\n\n", be.message);
        }
    }
    
}

const static uint32_t ID = RegistTest("OCLStub", &OCLStub);
