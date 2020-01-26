#include "TestRely.h"
#include "common/Linq2.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "ImageUtil/ImageUtil.h"
#include "OpenGLUtil/oglException.h"
#include "FreeGLUTView/FreeGLUTView.h"

using namespace common;
using namespace common::mlog;
using namespace xziar::img;
using namespace oglu;
using std::string;
using std::cin;
using namespace glutview;
using namespace b3d;
using std::wstring;
using std::u16string;
using std::vector;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"FGTest", { GetConsoleBackend() });
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
            const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
            Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
            ScreenBox->WriteSpan(DatVert);
        }
        VAOScreen = oglu::oglVAO_::Create(VAODrawMode::Triangles);
        VAOScreen->Prepare(LutGenerator)
            .SetFloat(ScreenBox, "@VertPos",  sizeof(Vec4), 2, sizeof(float) * 0)
            .SetFloat(ScreenBox, "@VertTexc", sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        LUTFrame = oglu::oglLayeredFrameBuffer_::Create();
        LUTFrame->AttachColorTexture(LutTex);
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

        oglUtil::ForceSyncGL()->Wait();
        const auto lutdata = LutTex->GetData(TextureFormat::RGBA8);
    }
};

static void TestErr()
{
    if (const auto e = oglUtil::GetError(); e.has_value())
        log().warning(u"Here occurs error due to {}.\n", e.value());
}

static void FGTest()
{
    printf("miniBLAS intrin:%s\n", miniBLAS::miniBLAS_intrin());
    if (!common::linq::FromIterable(GetCmdArgs())
        .Where([](const auto arg) { return arg == "--renderdoc"; })
        .Empty())
        oglu::oglUtil::InJectRenderDoc("");
    FreeGLUTViewInit();
    auto window = std::make_shared<glutview::detail::_FreeGLUTView>();
    oglUtil::InitLatestVersion();
    TestErr();
    const auto ctx = oglContext_::NewContext(oglContext_::CurrentContext(), false, oglu::oglContext_::GetLatestVersion());
    TestErr();
    ctx->UseContext();
    log().debug(u"{}\n", ctx->Capability->GenerateSupportLog());
    log().info(u"Def FBO is [{}]\n", oglu::oglDefaultFrameBuffer_::Get()->IsSrgb() ? "SRGB" : "Linear");
    window->SetTitle("FGTest");
    TestErr();
    auto drawer = oglDrawProgram_::Create(u"MainDrawer", LoadShaderFallback(u"fgTest.glsl", IDR_GL_FGTEST));
    TestErr();
    auto screenBox = oglArrayBuffer_::Create();
    TestErr();
    auto basicVAO = oglVAO_::Create(VAODrawMode::Triangles);
    TestErr();
    auto lutTex = oglTex3DStatic_::Create(64, 64, 64, xziar::img::TextureFormat::RGBA8);
    lutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
    if (oglComputeProgram_::CheckSupport())
    {
        try
        {
            const auto lutGenerator =
                oglComputeProgram_::Create(u"ColorLut", LoadShaderFallback(u"ColorLUT.glsl", IDR_GL_FGLUT));
            auto lutImg = oglImg3D_::Create(lutTex, TexImgUsage::WriteOnly);
            lutGenerator->State()
                .SetSubroutine("ToneMap", "ACES")
                .SetImage(lutImg, "result");
            lutGenerator->SetVal("step", 1.0f / 64);
            lutGenerator->SetVal("exposure", 1.0f);
            lutGenerator->Run(64, 64, 64);
            oglUtil::ForceSyncGL()->Wait();
            const auto lutdata = lutTex->GetData(TextureFormat::RGBA8);
            Image img(ImageDataType::RGBA);
            img.SetSize(64, 64 * 64);
            memcpy_s(img.GetRawPtr(), img.GetSize(), lutdata.data(), lutdata.size());
            //xziar::img::WriteImage(img, u"lut.png");
        }
        catch (const oglu::OGLException & gle)
        {
            log().warning(u"Failed to load LUT Generator:\n{}\n", gle.message);
        }
    }

    auto lutter = std::make_unique<Lutter>(64, lutTex);
    int32_t width = 0, height = 0;


    float lutZ = 0.5f;
    bool shouldLut = false;
    {
        const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
        screenBox->WriteSpan(DatVert);
        basicVAO->Prepare(drawer)
            .SetFloat(screenBox, "@VertPos",  sizeof(Vec4), 2, sizeof(float) * 0)
            .SetFloat(screenBox, "@VertTexc", sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        drawer->State().SetTexture(lutTex, "lut");
    }
    window->funDisp = [&](FreeGLUTView) 
    { 
        ctx->UseContext(true); 
        const auto defFBO = oglu::oglDefaultFrameBuffer_::Get();
        if (shouldLut)
        {
            lutter->DoLut(ctx);
            //ctx->SetViewPort(0, 0, width, height);
        }
        defFBO->Use();
        defFBO->ClearAll();
        drawer->Draw()
            .SetVal("lutZ", lutZ)
            .SetVal("shouldLut", shouldLut ? 1 : 0)
            .Draw(basicVAO); 
    };
    window->funReshape = [&](FreeGLUTView, const int32_t w, const int32_t h)
    {
        ctx->UseContext(true);
        oglDefaultFrameBuffer_::Get()->SetWindowSize(w, h);
        width = w, height = h;
        log().verbose(u"Resize to [{},{}].\n", w, h);
    };
    window->funMouseEvent = [&](FreeGLUTView wd, MouseEvent evt)
    {
        switch (evt.type)
        {
        case MouseEventType::Moving:
            lutZ += evt.dx / 1000.0f;
            log().verbose("lutZ is now {}.\n", lutZ);
            break;
        default:
            return;
        }
        wd->Refresh();
    };
    window->funKeyEvent = [&](FreeGLUTView wd, KeyEvent evt)
    {
        switch (evt.key)
        {
        case 13:
            shouldLut = !shouldLut;
            log().verbose("shouldLut is now {}.\n", shouldLut ? "ON" : "OFF");
            break;
        default:
            return;
        }
        wd->Refresh();
    };
    // window->setTimerCallback(onTimer, 20);
    // window->funDropFile = onDropFile;
    window->funOnClose = [&](FreeGLUTView) 
    { 
        ctx->UseContext();
        drawer.reset();
        screenBox.reset();
        basicVAO.reset();
        lutTex.reset();
        lutter.release();
        ctx->Release();
    };
    FreeGLUTViewRun();
    window.reset();
}

const static uint32_t ID = RegistTest("FGTest", &FGTest);




