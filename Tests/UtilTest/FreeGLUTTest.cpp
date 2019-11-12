#include "TestRely.h"
#include "OpenGLUtil/OpenGLUtil.h"
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

static void FGTest()
{
    printf("miniBLAS intrin:%s\n", miniBLAS::miniBLAS_intrin());
    FreeGLUTViewInit();
    auto window = std::make_shared<glutview::detail::_FreeGLUTView>();
    oglUtil::InitLatestVersion();
    const auto ctx = oglContext_::NewContext(oglContext_::CurrentContext(), false, oglu::oglContext_::GetLatestVersion());
    ctx->UseContext();
    window->SetTitle("FGTest");
    auto drawer = oglDrawProgram_::Create(u"MainDrawer", LoadShaderFallback(u"fgTest.glsl", IDR_GL_FGTEST));
    auto screenBox = oglArrayBuffer_::Create();
    auto basicVAO = oglVAO_::Create(VAODrawMode::Triangles);
    auto lutTex = oglTex3DStatic_::Create(64, 64, 64, xziar::img::TextureFormat::RGBA8);
    lutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
    auto lutImg = oglImg3D_::Create(lutTex, TexImgUsage::WriteOnly);
    try 
    {
        const auto lutGenerator = 
            oglComputeProgram_::Create(u"ColorLut", LoadShaderFallback(u"fgTest.glsl", IDR_GL_FGTEST));
        lutGenerator->State()
            .SetSubroutine("ToneMap", "ACES")
            .SetImage(lutImg, "result");
        lutGenerator->SetUniform("step", 1.0f / 64);
        lutGenerator->SetUniform("exposure", 1.0f);
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
    float lutZ = 0.5f;
    bool shouldLut = false;
    {
        const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
        screenBox->WriteSpan(DatVert);
        basicVAO->Prepare()
            .SetFloat(screenBox, drawer->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(screenBox, drawer->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        drawer->State().SetTexture(lutTex, "lut");
    }
    window->funDisp = [&](FreeGLUTView) 
    { 
        ctx->UseContext(true); 
        drawer->Draw()
            .SetUniform("lutZ", lutZ)
            .SetUniform("shouldLut", shouldLut ? 1 : 0)
            .Draw(basicVAO); 
    };
    window->funReshape = [&](FreeGLUTView, const int32_t w, const int32_t h)
    {
        ctx->UseContext(true);
        ctx->SetViewPort(0, 0, w, h);
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
        lutImg.reset();
        ctx->Release();
    };
    FreeGLUTViewRun();
    window.reset();
}

const static uint32_t ID = RegistTest("FGTest", &FGTest);




