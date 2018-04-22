#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "TextureUtil/TexCompressor.h"
#include <thread>
#include <future>

namespace rayr
{

struct Init
{
    Init()
    {
        basLog().verbose(u"BasicTest Static Init\n");
        oglUtil::init();
        oclu::oclUtil::init();
    }
};


static string LoadShaderFallback(const fs::path& shaderPath, int32_t id)
{
    if (shaderPath.empty())
        return getShaderFromDLL(id);
    try
    {
        return common::file::ReadAllText(shaderPath);
    }
    catch (const FileException& fe)
    {
        basLog().error(u"unable to load shader from [{}] : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), fe.message);
        return getShaderFromDLL(id);
    }
}

void BasicTest::init2d(const fs::path& shaderPath)
{
    prog2D.reset(u"Prog 2D");
    const string shaderSrc = LoadShaderFallback(shaderPath / u"2d.glsl", IDR_SHADER_2D);
    try
    {
        prog2D->AddExtShaders(shaderSrc);
    }
    catch (const OGLException& gle)
    {
        basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"OpenGL compile fail");
    }
    try
    {
        prog2D->Link();
    }
    catch (const OGLException& gle)
    {
        basLog().error(u"Fail to link Program:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"link Program error");
    }
    picVAO.reset(VAODrawMode::Triangles);
    screenBox.reset();
    {
        Vec3 DatVert[] = { { -1.0f, -1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f },{ -1.0f, 1.0f, 0.0f },{ 1.0f, -1.0f, 0.0f } };
        screenBox->Write(DatVert, sizeof(DatVert));
        picVAO->Prepare()
            .SetFloat(screenBox, prog2D->Attr_Vert_Pos, sizeof(Vec3), 3, 0)
            .SetDrawSize(0, 6);
    }
}

void BasicTest::init3d(const fs::path& shaderPath)
{
    cam.position = Vec3(0.0f, 0.0f, 4.0f);
    {
        oglProgram progBasic(u"3D Prog");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"3d.glsl", IDR_SHADER_3D);
        try
        {
            progBasic->AddExtShaders(shaderSrc);
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, L"OpenGL compile fail");
        }
        try
        {
            progBasic->Link();
            progBasic->State().SetSubroutine("lighter", "tex0").SetSubroutine("getNorm", "vertedNormal");
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"Fail to link Program:\n{}\n", gle.message);
            COMMON_THROW(BaseException, L"link Program error");
        }
        progBasic->SetCamera(cam);
        Prog3Ds.insert(progBasic);
    }
    {
        oglProgram progPBR(u"3D-pbr");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"3d_pbr.glsl", IDR_SHADER_3DPBR);
        try
        {
            progPBR->AddExtShaders(shaderSrc);
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, L"OpenGL compile fail");
        }
        try
        {
            progPBR->Link();
            progPBR->State()
                .SetSubroutine("lighter", "albedoOnly")
                .SetSubroutine("getNorm", "bothNormal")
                .SetSubroutine("getAlbedo", "bothAlbedo");
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"Fail to link Program:\n{}\n", gle.message);
            COMMON_THROW(BaseException, L"link Program error");
        }
        progPBR->SetCamera(cam);
        Prog3Ds.insert(progPBR);
        prog3D = progPBR;
    }
    {
        Wrapper<Pyramid> pyramid(1.0f);
        pyramid->Name = u"Pyramid";
        pyramid->position = { 0,0,0 };
        AddObject(pyramid);
        Wrapper<Sphere> ball(0.75f);
        ball->Name = u"Ball";
        ball->position = { 1,0,0 };
        AddObject(ball);
        Wrapper<Box> box(0.5f, 1.0f, 2.0f);
        box->Name = u"Box";
        box->position = { 0,1,0 };
        AddObject(box);
        Wrapper<Plane> ground(500.0f, 50.0f);
        ground->Name = u"Ground";
        ground->position = { 0,-2,0 };
        AddObject(ground);
    }
}

void BasicTest::initTex()
{
    picTex.reset(128, 128, TextureInnerFormat::RGBA8);
    picBuf.reset();
    //tmpTex.reset(128, 128, TextureInnerFormat::RGBA8);
    {
        picTex->SetProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
        Vec4 empty[128][128];
        for (int a = 0; a < 128; ++a)
        {
            for (int b = 0; b < 128; ++b)
            {
                auto& obj = empty[a][b];
                obj.x = a / 128.0f;
                obj.y = b / 128.0f;
                obj.z = (a + b) / 256.0f;
                obj.w = 1.0f;
            }
        }
        empty[0][0] = empty[0][127] = empty[127][0] = empty[127][127] = Vec4(0, 0, 1, 1);
        picTex->SetData(TextureDataFormat::RGBAf, empty);
        picBuf->Write(nullptr, 128 * 128 * 4, BufferWriteMode::DynamicDraw);
        //const auto outimg = picTex->GetImage(ImageDataType::RGBA);
        //tmpTex = oglu::texcomp::CompressToTex(outimg, TextureInnerFormat::BC7, false)->wait();
    }
    chkTex = MultiMaterialHolder::GetCheckTex();
    chkTex->SetProperty(TextureFilterVal::Nearest, TextureWrapVal::Repeat);
    {
        oglu::oglTex2DArray tex2darr(128, 128, 1, TextureInnerFormat::RGBA8);
        tex2darr->SetTextureLayer(0, chkTex);
    }
    {
        MiddleFrame.reset();
        tmpTex.reset(1280, 720, TextureInnerFormat::RGBA8);
        MiddleFrame->AttachColorTexture(tmpTex, 0);
        oglRBO mainRBO(1280, 720, oglu::RBOFormat::Depth24Stencil8);
        MiddleFrame->AttachDepthStencilBuffer(mainRBO);
        basLog().info(u"FBO status:{}\n", MiddleFrame->CheckIsComplete() ? u"complete" : u"not complete");
    }
    {
        //oglu::oglUtil::TryTask();
    }
}

void BasicTest::initUBO()
{
    uint16_t size = 0;
    for (const auto& prog : Prog3Ds)
    {
        if (auto lubo = prog->GetResource("lightBlock"))
            size = common::max(size, lubo->size);
    }
    lightUBO.reset(size);
    LightBuf.resize(size);
    prepareLight();
}

void BasicTest::prepareLight()
{
    size_t pos = 0;
    uint32_t onCnt = 0;
    for (const auto& lgt : lights)
    {
        if (!lgt->isOn)
            continue;
        onCnt++;
        pos += lgt->WriteData(&LightBuf[pos]);
        if (pos >= lightUBO->Size())
            break;
    }
    prog3D->SetUniform("lightCount", onCnt);
    lightUBO->Write(LightBuf.data(), pos, BufferWriteMode::StreamDraw);
}

void BasicTest::fontTest(const char32_t word)
{
    try
    {
        auto fonttex = fontCreator->getTexture();
        if (word == 0x0)
        {
            const auto imgShow = fontCreator->clgraysdfs(U'°¡', 16);
            oglUtil::invokeSyncGL([&imgShow, &fonttex](const common::asyexe::AsyncAgent& agent) 
            {
                fonttex->SetData(TextureInnerFormat::R8, imgShow);
                agent.Await(oglu::oglUtil::SyncGL());
            })->wait();
            img::WriteImage(imgShow, Basepath / (u"Show.png"));
            /*SimpleTimer timer;
            for (uint32_t cnt = 0; cnt < 65536; cnt += 4096)
            {
                const auto imgShow = fontCreator->clgraysdfs((char32_t)cnt, 4096);
                img::WriteImage(imgShow, basepath / (u"Show-" + std::to_u16string(cnt) + u".png"));
                basLog().success(u"successfully processed words begin from {}\n", cnt);
            }
            timer.Stop();
            basLog().success(u"successfully processed 65536 words, cost {}ms\n", timer.ElapseMs());*/
        }
        else
        {
            fontCreator->setChar(L'G', false);
            const auto imgG = fonttex->GetImage(ImageDataType::GRAY);
            img::WriteImage(imgG, Basepath / u"G.png");
            fontCreator->setChar(word, false);
            const auto imgA = fonttex->GetImage(ImageDataType::GRAY);
            img::WriteImage(imgA, Basepath / u"A.png");
            const auto imgShow = fontCreator->clgraysdfs(U'°¡', 16);
            fonttex->SetData(TextureInnerFormat::R8, imgShow);
            img::WriteImage(imgShow, Basepath / u"Show.png");
            fonttex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
            fontViewer->BindTexture(fonttex);
        }
    }
    catch (BaseException& be)
    {
        basLog().error(u"Font Construct failure:\n{}\n", be.message);
    }
}

BasicTest::BasicTest(const fs::path& shaderPath)
{
    static Init _init;
    glContext = oglu::oglContext::CurrentContext();
    glContext->SetDepthTest(DepthTestType::LessEqual);
    //glContext->SetFaceCulling(FaceCullingType::CullCW);
    fontViewer.reset();
    fontCreator.reset(oclu::Vendor::Intel);
    Basepath = u"D:\\Programs Temps\\RayRenderer";
    if (!fs::exists(Basepath))
        Basepath = u"C:\\Programs Temps\\RayRenderer";
    fontCreator->reloadFont(Basepath / u"test.ttf");

    fontTest(/*L'‡å'*/);
    initTex();
    init2d(shaderPath);
    init3d(shaderPath);
    //prog2D->State().SetTexture(fontCreator->getTexture(), "tex");
    prog2D->State().SetTexture(tmpTex, "tex");
    initUBO();
    for (const auto& prog : Prog3Ds)
    {
        prog->State()
            //.SetTexture(chkTex, "tex")
            .SetUBO(lightUBO, "lightBlock");
    }
    glProgs.insert(prog2D);
    glProgs.insert(Prog3Ds.cbegin(), Prog3Ds.cend());
    glProgs.insert(fontViewer->prog);
}

void BasicTest::Draw()
{
    {
        const auto changed = (ChangableUBO)IsUBOChanged.exchange(0, std::memory_order::memory_order_relaxed);
        if (HAS_FIELD(changed, ChangableUBO::Light))
            prepareLight();
    }
    if (mode)
    {
        prog3D->SetCamera(cam);
        auto drawcall = prog3D->Draw();
        for (const auto& d : drawables)
        {
            if (!d->ShouldRender)
                continue;
            d->Draw(drawcall);
            drawcall.Restore();
        }
    }
    else
    {
        //fontViewer->Draw();
        prog2D->Draw().Draw(picVAO);
    }
}

void BasicTest::Resize(const int w, const int h)
{
    glContext->SetViewPort(0, 0, w, h);
    cam.resize(w, h);
    prog2D->SetProject(cam);
    prog3D->SetProject(cam);
}

void BasicTest::ReloadFontLoader(const u16string& fname)
{
    auto clsrc = file::ReadAllText(fs::path(fname));
    fontCreator->reload(clsrc);
    fontTest(0);
}

void BasicTest::ReloadFontLoaderAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(const BaseException&)> onError)
{
    std::thread([this, onFinish, onError](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for FontTest");
        try
        {
            auto clsrc = file::ReadAllText(fs::path(name));
            fontCreator->reload(clsrc);
            fontTest(0);
            onFinish([]() { return true; });
        }
        catch (BaseException& be)
        {
            basLog().error(u"failed to reload font test\n");
            if (onError)
                onError(be);
            else
                onFinish([]() { return false; });
        }
    }, fname).detach();
}


void BasicTest::LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError)
{
    std::thread([onFinish, onError](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for Model");
        try
        {
            Wrapper<Model> mod(name, true);
            mod->Name = u"model";
            onFinish(mod);
        }
        catch (BaseException& be)
        {
            basLog().error(u"failed to load model by file {}\n", name);
            if (onError)
                onError(be);
            else
                onFinish(Wrapper<Model>());
        }
    }, fname).detach();
}

void BasicTest::LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(oglProgram)> onFinish, std::function<void(const BaseException&)> onError /*= nullptr*/)
{
    using common::asyexe::StackSize;
    const auto loadPms = std::make_shared<std::promise<oglProgram>>();
    auto fut = loadPms->get_future();
    oglUtil::invokeSyncGL([fname, shdName, loadPms](const common::asyexe::AsyncAgent& agent)
    {
        oglProgram prog(shdName);
        try
        {
            prog->AddExtShaders(common::file::ReadAllText(fname));
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
            loadPms->set_exception(std::current_exception());
            return;
        }
        try
        {
            prog->Link();
            prog->State()
                .SetSubroutine("lighter", "tex0")
                .SetSubroutine("getNorm", "bothNormal")
                .SetSubroutine("getAlbedo", "bothAlbedo");
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"Fail to link Program:\n{}\n", gle.message);
            loadPms->set_exception(std::current_exception());
            return;
        }
        loadPms->set_value(prog);
    }, u"load shader " + shdName, StackSize::Big);
    std::thread([onFinish, onError](std::future<oglProgram>&& fut)
    {
        common::SetThreadName(u"AsyncLoader for Shader");
        try
        {
            onFinish(fut.get());
        }
        catch (const BaseException& be)
        {
            onError(be);
        }
    }, std::move(fut)).detach();
}

bool BasicTest::AddObject(const Wrapper<Drawable>& drawable)
{
    for(const auto& prog : Prog3Ds)
        drawable->PrepareGL(prog);
    drawables.push_back(drawable);
    basLog().success(u"Add an Drawable [{}][{}]:  {}\n", drawables.size() - 1, drawable->GetType(), drawable->Name);
    return true;
}

bool BasicTest::AddShader(const oglProgram& prog)
{
    const auto isAdd = Prog3Ds.insert(prog).second;
    if (isAdd)
    {
        for (const auto& d : drawables)
            d->PrepareGL(prog);
        prog->State()
            .SetTexture(chkTex, "tex")
            .SetUBO(lightUBO, "lightBlock");
    }
    return isAdd;
}

bool BasicTest::AddLight(const Wrapper<Light>& light)
{
    lights.push_back(light);
    prepareLight();
    basLog().success(u"Add a Light [{}][{}]:  {}\n", lights.size() - 1, (int32_t)light->type, light->name);
    return true;
}

void BasicTest::ChangeShader(const oglProgram& prog)
{
    if (Prog3Ds.count(prog))
    {
        prog3D = prog;
        prog3D->SetProject(cam);
        prog3D->SetCamera(cam);
        prepareLight();
    }
    else
        basLog().warning(u"change to an unknown shader [{}], ignored.\n", prog ? prog->Name : u"null");
}

void BasicTest::DelAllLight()
{
    lights.clear();
    prepareLight();
}

void BasicTest::ReportChanged(const ChangableUBO target)
{
    IsUBOChanged |= (uint32_t)target;
}


}