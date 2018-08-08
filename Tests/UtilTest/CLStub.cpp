#include "TestRely.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclException.h"


using namespace common;
using namespace common::mlog;
using namespace oclu;
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


static void OCLStub()
{
    oclUtil::init();
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
    while (true)
    {
        log().info(u"input opencl file:");
        string fpath;
        std::getline(cin, fpath);
        common::fs::path filepath(fpath);
        log().debug(u"loading cl file [{}]\n", filepath.u16string());
        try
        {
            const auto kertxt = common::file::ReadAllText(fpath);
            oclProgram clProg(ctx, kertxt);
            try
            {
                string options = "-cl-fast-relaxed-math";
                if (ctx->vendor == Vendor::NVIDIA)
                    options += " -cl-kernel-arg-info -cl-nv-verbose -DNVIDIA";
                options += " -DLOC_MEM_SIZE=" + std::to_string(ctx->Devices[0]->LocalMemSize);
                clProg->Build(options);
                string names;
                for (const auto& name : clProg->GetKernelNames())
                    names.append(name).push_back('\t');
                log().success(u"loaded! kernels:\n{}\n\n", names);
            }
            catch (OCLException& cle)
            {
                log().error(u"Fail to build opencl Program:\n{}\n", cle.message);
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
