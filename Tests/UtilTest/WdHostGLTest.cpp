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
        LutGenerator->SetVal("curStep", 1.0f / (64 - 1));
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

static std::shared_ptr<GLHost> CreateHostAndinit(WindowBackend& backend, xziar::gui::WindowHost_& window, oglLoader* loader)
{
    const auto name = loader->Name();
    if (name == "WGL")
    {
        Ensures(backend.Name() == "Win32");
        return static_cast<WGLLoader*>(loader)->CreateHost(static_cast<Win32Backend::Win32WdHost&>(window).GetHDC());
    }
    else if (name == "GLX")
    {
        if (backend.Name() == "XCB")
        {
            const auto& xcbBackend = static_cast<XCBBackend&>(backend);
            auto host = static_cast<GLXLoader*>(loader)->CreateHost(xcbBackend.GetDisplay(), xcbBackend.GetDefaultScreen());
            host->InitDrawable(static_cast<XCBBackend::XCBWdHost&>(window).GetWindow());
            return host;
        }
    }
    else if (name == "EGL")
    {
        auto& ld = *static_cast<EGLLoader*>(loader);
        if (backend.Name() == "XCB")
        {
            const auto& xcbBackend = static_cast<XCBBackend&>(backend);
            const auto wd = static_cast<XCBBackend::XCBWdHost&>(window).GetWindow();
            if (auto host = ld.CreateHostFromXcb(xcbBackend.GetConnection(), xcbBackend.GetDefaultScreen(), false); host)
            {
                host->InitSurface(wd);
                return host;
            }
#if COMMON_OS_ANDROID
            if (auto host = ld.CreateHostFromAndroid(true); host)
            {
                host->InitSurface(wd);
                return host;
            }
#endif
            if (auto host = ld.CreateHost(xcbBackend.GetDisplay(), false); host)
            {
                host->InitSurface(wd);
                return host;
            }
        }
        else if (backend.Name() == "Win32")
        {
            auto& wd = static_cast<Win32Backend::Win32WdHost&>(window);
            if (auto host = ld.CreateHost(wd.GetHDC(), false); host)
            {
                host->InitSurface(reinterpret_cast<uintptr_t>(wd.GetHWND()));
                return host;
            }
        }
        else if (backend.Name() == "Wayland")
        {
            const auto& wlBackend = static_cast<WaylandBackend&>(backend);
            const auto wd = static_cast<WaylandBackend::WaylandWdHost&>(window).GetEGLWindow();
            if (auto host = ld.CreateHostFromWayland(wlBackend.GetDisplay(), false); host)
            {
                host->InitSurface(reinterpret_cast<uintptr_t>(wd));
                return host;
            }
        }
    }
    return {};
}

static void RunTest(WindowBackend& backend)
{
    auto glType = GLType::Desktop;
    std::vector<std::string_view> loaderPref;
    for (const auto& cmd : GetCmdArgs())
    {
        if (cmd == "renderdoc")
            oglu::oglUtil::InJectRenderDoc("");
        else if (cmd == "es")
            glType = GLType::ES;
        else if (cmd.starts_with("glbe="))
            loaderPref.emplace_back(cmd.substr(5));
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
    loaderPref.emplace_back("WGL");
    loaderPref.emplace_back("EGL");
#elif COMMON_OS_ANDROID
    glType = GLType::ES;
    loaderPref.emplace_back("EGL");
    loaderPref.emplace_back("GLX");
#elif COMMON_OS_LINUX
    loaderPref.emplace_back("GLX");
    loaderPref.emplace_back("EGL");
#endif
    const auto loaders_ = oglLoader::GetLoaders();
    std::vector<oglLoader*> loaders;
    for (const auto& pref : loaderPref)
    {
        for (const auto& loader : loaders_)
        {
            if (loader->Name() == pref)
                loaders.emplace_back(loader.get());
        }
    }
    if (loaders.size() == 0)
    {
        log().Error(u"No OpenGL loader found!\n");
        return;
    }

    xziar::gui::CreateInfo wdInfo;
    wdInfo.Width = 1280, wdInfo.Height = 720, wdInfo.Title = u"WdHostGLTest";
    const auto window = backend.Create(wdInfo);
    std::promise<void> closePms;
    window->Openning() += [&](const auto&) 
    {
        log().Info(u"opened.\n");
        oglu::CreateInfo cinfo;
        cinfo.Type = glType;
        cinfo.PrintFuncLoadFail = cinfo.PrintFuncLoadSuccess = true;

        for (const auto loader : loaders)
        {
            auto host = CreateHostAndinit(backend, *window, loader);
            if (host)
            {
                const auto name = loader->Name();
                log().Info(u"GLhost created using [{}].\n", name);
                if (name == "GLX")
                    window->SetWindowData("visual", static_cast<GLXLoader::GLXHost&>(*host).GetVisualId());
                else if (name == "EGL")
                    window->SetWindowData("visual", static_cast<EGLLoader::EGLHost&>(*host).GetVisualId());
                context = host->CreateContext(cinfo);
                break;
            }
        }
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
                lutGenerator->SetVal("curStep", 1.0f / 64);
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
        log().Verbose(u"lutZ is now {}.\n", lutZ);
        window->Invalidate();
    };
    window->KeyDown() += [&](const auto&, const auto& evt)
    {
        switch (evt.ChangedKey.Key)
        {
        case event::CommonKeys::Enter:
            shouldLut = !shouldLut;
            window->SetTitle(shouldLut ? u"WdHostGLTest - LUT"sv : u"WdHostGLTest - NoLUT"sv);
            log().Verbose(u"shouldLut is now {}.\n", shouldLut ? "ON" : "OFF");
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
        else if (name == "visual")
        {
            const auto dat = window->GetWindowData<int>("visual");
            if (dat) return *dat;
        }
        return {};
    });

    closePms.get_future().get();
}

static void WdGLTest()
{
    std::string_view wdpref;
    for (const auto& cmd : GetCmdArgs())
    {
        if (cmd.starts_with("wdbe="))
            wdpref = cmd.substr(5);
    }
    std::vector<WindowBackend*> backends;
    std::optional<uint32_t> whbidx;
    for (const auto backend : WindowBackend::GetBackends())
    {
        if (!backend->CheckFeature("OpenGL") && !backend->CheckFeature("OpenGLES"))
            continue;
        auto& fmter = GetLogFmt();
        try
        {
            backend->Init();
            if (backend->Name() == wdpref)
                whbidx = static_cast<uint32_t>(backends.size());
            backends.emplace_back(backend);
            continue;
        }
        catch (common::BaseException& be)
        {
            fmter.FormatToStatic(fmter.Str, FmtString(u"{@<Y}Failed to init [{}]: {}{@<w}\n"), backend->Name(), be);
        }
        catch(...)
        {
            fmter.FormatToStatic(fmter.Str, FmtString(u"{@<Y}Failed to init [{}]{@<w}\n"), backend->Name());
        }
        common::mlog::SyncConsoleBackend();
        PrintToConsole(fmter);
    }
    if (!whbidx && IsAutoMode())
        whbidx = 0;
    whbidx = SelectIdx(backends, u"backend", [&](const auto& backend)
    {
        return FMTSTR2(u"[{}] {:2}|{:4}|{:2}|{:2}|{:2}", backend->Name(),
            backend->CheckFeature("OpenGL")     ? u"GL"   : u"",
            backend->CheckFeature("OpenGLES")   ? u"GLES" : u"",
            backend->CheckFeature("DirectX")    ? u"DX"   : u"",
            backend->CheckFeature("Vulkan")     ? u"VK"   : u"",
            backend->CheckFeature("NewThread")  ? u"NT"   : u"");
    }, whbidx);
    const auto backend = backends[*whbidx];
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




