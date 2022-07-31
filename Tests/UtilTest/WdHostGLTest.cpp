#include "TestRely.h"
#include "common/Linq2.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "ImageUtil/ImageUtil.h"
#include "WindowHost/WindowHost.h"
#include <thread>
#include <future>

using namespace std::string_view_literals;
using namespace common;
using namespace common::mlog;
using namespace xziar::img;
using namespace oglu;
using std::string;
using namespace xziar::gui;
using std::wstring;
using std::u16string;
using std::vector;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"WdGLTest", { GetConsoleBackend() });
    return logger;
}

struct Lutter
{
    oglu::oglDrawProgram LutGenerator;
    oglu::oglVBO ScreenBox;
    oglu::oglFBOLayered LUTFrame;
    oglu::oglVAO VAOScreen;
    oglu::oglTex3DS LutTex;
    uint32_t LutSize;
    Lutter(const uint32_t lutSize, const oglu::oglTex3DS tex) : LutTex(tex), LutSize(lutSize)
    {
        LutGenerator = oglDrawProgram_::Create(u"Lutter", LoadShaderFallback(u"ColorLUT.glsl", IDR_GL_FGLUT));
        ScreenBox = oglu::oglArrayBuffer_::Create();
        {
            const mbase::Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
            mbase::Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
            ScreenBox->WriteSpan(DatVert);
        }
        VAOScreen = oglu::oglVAO_::Create(VAODrawMode::Triangles);
        VAOScreen->Prepare(LutGenerator)
            .SetFloat(ScreenBox, "@VertPos",  sizeof(mbase::Vec4), 2, sizeof(float) * 0)
            .SetFloat(ScreenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        LUTFrame = oglu::oglLayeredFrameBuffer_::Create();
        LUTFrame->AttachColorTexture(LutTex);
        log().Info(u"LUT FBO status:{}\n", LUTFrame->CheckStatus() == FBOStatus::Complete ? u"complete" : u"not complete");
        LutGenerator->SetVal("step", 1.0f / (64 - 1));
        LutGenerator->SetVal("exposure", 1.0f);
        LutGenerator->SetVal("lutSize", 64);
    }
    void DoLut(const oglContext& ctx)
    {
        LUTFrame->Use();
        LUTFrame->ClearAll();
        //ctx->SetViewPort(0, 0, 64, 64);
        LutGenerator->Draw()
            .DrawInstance(VAOScreen, 64);

        ctx->ForceSync();
        const auto lutdata = LutTex->GetData(TextureFormat::RGBA8);
    }
};

static void TestErr()
{
    if (const auto e = oglUtil::GetError(); e.has_value())
        log().Warning(u"Here occurs error due to {}.\n", e.value());
}

static void RunTest(WindowBackend& backend)
{
    auto glType = GLType::Desktop;
    for (const auto& cmd : GetCmdArgs())
    {
        if (cmd == "renderdoc")
            oglu::oglUtil::InJectRenderDoc("");
        else if (cmd == "es")
            glType = GLType::ES;
    }
    
    oglContext context;
    oglDrawProgram drawer;
    oglVBO screenBox;
    oglVAO basicVAO;
    oglTex3DS lutTex;
    std::unique_ptr<Lutter> lutter;
    float lutZ = 0.5f;
    bool shouldLut = false;

#if COMMON_OS_WIN
    const auto loader = static_cast<WGLLoader*>(oglLoader::GetLoader("WGL"));
    using WdType = Win32Backend::Win32WdHost;
#elif COMMON_OS_LINUX
    const auto loader = static_cast<GLXLoader*>(oglLoader::GetLoader("GLX"));
    using WdType = XCBBackend::XCBWdHost;
    const auto& xcbBackend = static_cast<XCBBackend&>(backend);
    const auto host = loader->CreateHost(xcbBackend.GetDisplay(), xcbBackend.GetDefaultScreen());
#endif

    xziar::gui::CreateInfo wdInfo;
    wdInfo.Width = 1280, wdInfo.Height = 720, wdInfo.Title = u"WdHostGLTest";
    const auto window = std::dynamic_pointer_cast<WdType>(backend.Create(wdInfo));
    std::promise<void> closePms;
    window->Openning() += [&](const auto&) 
    {
        log().Info(u"opened.\n"); 
#if COMMON_OS_WIN
        const auto host = loader->CreateHost(window->GetHDC());
#elif COMMON_OS_LINUX
        host->InitDrawable(window->GetWindow());
#endif
        oglu::CreateInfo cinfo;
        cinfo.Type = glType;
        cinfo.PrintFuncLoadFail = cinfo.PrintFuncLoadSuccess = true;
        context = host->CreateContext(cinfo);
        context->UseContext();
        context->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        TestErr();

        log().Debug(u"{}\n", context->Capability->GenerateSupportLog());
        const auto fbo = oglu::oglDefaultFrameBuffer_::Get();
        fbo->SetWindowSize(1280, 720);
        log().Info(u"Def FBO is [{}]\n", fbo->IsSrgb() ? "SRGB" : "Linear");
        TestErr();
        try
        {
            drawer = oglDrawProgram_::Create(u"MainDrawer", LoadShaderFallback(u"fgTest.glsl", IDR_GL_FGTEST));
        }
        catch (const common::BaseException& be)
        {
            common::mlog::SyncConsoleBackend();
            log().Error(u"failed to load shader: [{}]\n", be.Message());
        }
        TestErr();
        screenBox = oglArrayBuffer_::Create();
        TestErr();
        basicVAO = oglVAO_::Create(VAODrawMode::Triangles);
        TestErr();
        lutTex = oglTex3DStatic_::Create(64, 64, 64, xziar::img::TextureFormat::RGBA8);
        lutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
        TestErr();
        if (oglComputeProgram_::CheckSupport())
        {
            try
            {
                const auto lutGenerator =
                    oglComputeProgram_::Create(u"ColorLut", LoadShaderFallback(u"ColorLUT.glsl", IDR_GL_FGLUT));
                TestErr();
                auto lutImg = oglImg3D_::Create(lutTex, TexImgUsage::WriteOnly);
                TestErr();
                lutGenerator->State()
                    .SetSubroutine("ToneMap", "ACES")
                    .SetImage(lutImg, "result");
                lutGenerator->SetVal("step", 1.0f / 64);
                lutGenerator->SetVal("exposure", 1.0f);
                lutGenerator->Run(64, 64, 64);
                TestErr();
                context->ForceSync();
                const auto lutdata = lutTex->GetData(TextureFormat::RGBA8);
                TestErr();
                Image img(ImageDataType::RGBA);
                img.SetSize(64, 64 * 64);
                memcpy_s(img.GetRawPtr(), img.GetSize(), lutdata.data(), lutdata.size());
                //xziar::img::WriteImage(img, u"lut.png");
            }
            catch (const common::BaseException& be)
            {
                common::mlog::SyncConsoleBackend();
                log().Warning(u"Failed to load LUT Generator:\n{}\n", be.Message());
            }
        }

        lutter = std::make_unique<Lutter>(64, lutTex);

        {
            const mbase::Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
            mbase::Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
            screenBox->WriteSpan(DatVert);
            basicVAO->Prepare(drawer)
                .SetFloat(screenBox, "@VertPos",  sizeof(mbase::Vec4), 2, sizeof(float) * 0)
                .SetFloat(screenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
                .SetDrawSize(0, 6);
            drawer->State().SetTexture(lutTex, "lut");
        }
    };
    window->Closed() += [&](const auto&)
    {
        drawer.reset();
        screenBox.reset();
        basicVAO.reset();
        lutTex.reset();
        lutter.release();
        context->Release();
        context = {};
        closePms.set_value();
    };
    window->Displaying() += [&](const auto& wd) 
    { 
        const auto defFBO = oglu::oglDefaultFrameBuffer_::Get();
        if (shouldLut)
        {
            lutter->DoLut(context);
            //context->SetViewPort(0, 0, width, height);
        }
        defFBO->Use();
        defFBO->ClearAll();
        drawer->Draw()
            .SetVal("lutZ", lutZ)
            .SetVal("shouldLut", shouldLut ? 1 : 0)
            .Draw(basicVAO); 
        context->SwapBuffer();
    };
    window->Resizing() += [&](const auto&, int32_t width, int32_t height)
    {
        window->InvokeUI([=](auto& wd)
        {
            oglDefaultFrameBuffer_::Get()->SetWindowSize(width, height);
            //wd.Invalidate();
            log().Info(u"resize to [{:4} x {:4}].\n", width, height);
        });
    };
    window->MouseDrag() += [&](const auto&, const auto& evt)
    {
        lutZ += evt.Delta.X / 1000.0f;
        log().Verbose("lutZ is now {}.\n", lutZ);
        window->Invalidate();
    };
    window->KeyDown() += [&](const auto&, const auto& evt)
    {
        switch (evt.ChangedKey.Key)
        {
        case event::CommonKeys::Enter:
            shouldLut = !shouldLut;
            window->SetTitle(shouldLut ? u"WdHostGLTest - LUT"sv : u"WdHostGLTest - NoLUT"sv);
            log().Verbose("shouldLut is now {}.\n", shouldLut ? "ON" : "OFF");
            break;
        default:
            return;
        }
        window->Invalidate();
    };
    window->Show([&](std::string_view name) -> std::any 
    {
        if (name == "background")
            return false;
#if COMMON_OS_LINUX
        else if (name == "visual")
            return host->GetVisualId();
#endif
        return {};
    });

    closePms.get_future().get();
}

static void WdGLTest()
{
    const auto backend = WindowBackend::GetBackends()[0];
    backend->Init();
    common::BasicPromise<void> pms;
    std::thread Thread([&]() 
        {
            pms.GetPromiseResult()->Get();
            RunTest(*backend);
        });
    backend->Run(true, &pms);
    if (Thread.joinable())
        Thread.join();
}

const static uint32_t ID = RegistTest("WdGLTest", &WdGLTest);




