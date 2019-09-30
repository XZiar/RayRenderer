#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "TextureLoader.h"
#include "OpenGLUtil/oglWorker.h"
#include "TextureUtil/TexUtilWorker.h"
#include "TextureUtil/TexMipmap.h"
#include <thread>
#include <future>

namespace rayr
{

using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;
using xziar::img::TextureFormat;

struct Init
{
    Init()
    {
        dizzLog().verbose(u"BasicTest Static Init\n");
        oglContext::Refresh();
        oclUtil::GetPlatforms();
    }
};


static string LoadShaderFallback(const fs::path& shaderPath, int32_t id)
{
    if (!shaderPath.empty())
    {
        try
        {
            return common::file::ReadAllText(shaderPath);
        }
        catch (const FileException& fe)
        {
            dizzLog().error(u"unable to load shader from [{}] : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), fe.message);
        }
    }
    return getShaderFromDLL(id);
}

static int32_t JudgeVendor(const Vendor vendor)
{
    switch (vendor)
    {
    case Vendor::NVIDIA: return 1;
    case Vendor::AMD:    return 2;
    case Vendor::Intel:  return 3;
    default:             return 4;
    }
}

static std::pair<oclContext, oclContext> CreateOCLContext(const Vendor vendor, const oglContext glContext)
{
    const auto venderClPlat = Linq::FromIterable(oclUtil::GetPlatforms())
        .Where([](const auto& plat) { return Linq::FromIterable(plat->GetDevices())
            .ContainsIf([](const auto& dev) { return dev->Type == DeviceType::GPU; }); })
        .Select([&](const auto& plat) { return std::pair{ plat->PlatVendor == vendor ? 0 : JudgeVendor(plat->PlatVendor), plat }; })
        .OrderBy<common::container::PairLess>().Select([](const auto& p) { return p.second; })
        .TryGetFirst().value_or(oclPlatform{});
    if (!venderClPlat)
        COMMON_THROW(BaseException, u"No avaliable OpenCL Platform found");
    const auto glPlat = Linq::FromIterable(oclUtil::GetPlatforms())
        .Where([](const auto& plat) { return Linq::FromIterable(plat->GetDevices())
            .ContainsIf([](const auto& dev) { return dev->Type == DeviceType::GPU; }); })
        .Where([&](const auto& plat) { return GLInterop::CheckIsGLShared(*plat, glContext); })
        .TryGetFirst().value_or(oclPlatform{});
    oclContext defCtx, sharedCtx;
    if (glPlat)
    {
        sharedCtx = GLInterop::CreateGLSharedContext(*glPlat, glContext);
        dizzLog().success(u"Created Shared OCLContext in platform {}!\n", glPlat->Name);
        sharedCtx->onMessage = [](const u16string& errtxt) { dizzLog().error(u"Error from shared CLContext:\t{}\n", errtxt); };
    }
    defCtx = glPlat == venderClPlat ? sharedCtx : venderClPlat->CreateContext();
    dizzLog().success(u"Created OCLContext in platform {}!\n", venderClPlat->Name);
    defCtx->onMessage = [](const u16string& errtxt) { dizzLog().error(u"Error from CLContext:\t{}\n", errtxt); };
    return { defCtx, sharedCtx };
}

BasicTest::BasicTest(const fs::path& shaderPath)
{
    static Init _init;
    const auto oriCtx = oglu::oglContext::CurrentContext();
    //oriCtx->SetRetain(true);
    glContext = oglu::oglContext::NewContext(oriCtx);
    glContext->UseContext();
    //glContext = oglu::oglContext::CurrentContext();
    {
        const auto ctxs = CreateOCLContext(Vendor::NVIDIA, glContext);
        ClContext = ctxs.first; ClSharedContext = ctxs.second;
        ClQue = oclCmdQue_::Create(ClSharedContext, ClSharedContext->GetGPUDevice());
        if (!ClQue)
            COMMON_THROW(BaseException, u"clQueue initialized failed!");
    }
    GLWorker = std::make_shared<oglu::oglWorker>(u"Core");
    GLWorker->Start();
    TexWorker = std::make_shared<oglu::texutil::TexUtilWorker>(oglu::oglContext::NewContext(glContext, true), ClSharedContext);
    MipMapper = std::make_shared<oglu::texutil::TexMipmap>(TexWorker);
    TexLoader = std::make_shared<TextureLoader>(MipMapper);
    MipMapper->Test2();
    ThumbMan.reset(TexWorker, GLWorker);
    PostProc.reset(ClSharedContext, ClQue);
    //for reverse-z
    glContext->SetDepthClip(true);
    glContext->SetDepthTest(DepthTestType::GreaterEqual);
    //glContext->SetFaceCulling(FaceCullingType::CullCW);
    WindowWidth = 1280, WindowHeight = 720;
    cam.zNear = 0.01f;
    fontViewer.reset();
    fontCreator.reset(ClContext);
    Basepath = u"C:\\Programs Temps\\RayRenderer";
    if (!fs::exists(Basepath))
        Basepath = u"D:\\ProgramsTemps\\RayRenderer";
    fontCreator->reloadFont(Basepath / u"test.ttf");

    fontTest(/*U'啊'*/);
    initTex();
    init2d(shaderPath);
    init3d(shaderPath);
    {
        Wrapper<Pyramid> pyramid(1.0f);
        pyramid->Name = u"Pyramid";
        pyramid->Position = { 0,0,0 };
        AddObject(pyramid);
        Wrapper<Sphere> ball(0.75f);
        ball->Name = u"Ball";
        ball->Position = { 1,0,0 };
        AddObject(ball);
        Wrapper<Box> box(0.5f, 1.0f, 2.0f);
        box->Name = u"Box";
        box->Position = { 0,1,0 };
        AddObject(box);
        Wrapper<Plane> ground(500.0f, 50.0f);
        ground->Name = u"Ground";
        ground->Position = { 0,-2,0 };
        AddObject(ground);
    }
    MiddleFrame.reset();
    ResizeFBO(1280, 720, true);
    //prog2D->State().SetTexture(fontCreator->getTexture(), "tex");
    initUBO();
    for (const auto& shader : glProgs)
        glProgs2.insert(shader->Program);
    glProgs2.insert(fontViewer->prog);
}

void BasicTest::init2d(const fs::path& shaderPath)
{
    {
        screenBox.reset();
        const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
        screenBox->Write(DatVert, sizeof(DatVert));
    }
    {
        Prog2D = Wrapper<GLShader>(u"Prog 2D", LoadShaderFallback(shaderPath / u"2d.glsl", IDR_SHADER_2D));
        glProgs.insert(Prog2D);
        prog2D = Prog2D->Program;
        picVAO.reset(VAODrawMode::Triangles);
        picVAO->Prepare()
            .SetFloat(screenBox, prog2D->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(screenBox, prog2D->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        prog2D->State().SetTexture(picTex, "tex");
    }
    {
        //ProgPost = Wrapper<GLShader>(u"PostProcess", LoadShaderFallback(shaderPath / u"postprocess.glsl", IDR_SHADER_POSTPROC));
        ProgPost = PostProc->GetShader();
        glProgs.insert(ProgPost);
        progPost = ProgPost->Program;
        /*ppVAO.reset(VAODrawMode::Triangles);
        ppVAO->Prepare()
            .SetFloat(screenBox, progPost->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(screenBox, progPost->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        progPost->State()
            .SetSubroutine("ToneMap", "ACES")
            .SetTexture(PostProc->GetLut(), "lut");*/
    }
}

void BasicTest::init3d(const fs::path& shaderPath)
{
    cam.Position = Vec3(0.0f, 0.0f, 4.0f);
    {
        Prog3D = Wrapper<GLShader>(u"3D Prog", LoadShaderFallback(shaderPath / u"3d.glsl", IDR_SHADER_3D));
        glProgs.insert(Prog3D);
        const auto progBasic = Prog3D->Program;
        progBasic->State().SetSubroutine("lighter", "tex0").SetSubroutine("getNorm", "vertedNormal");
        progBasic->SetView(cam.GetView());
        progBasic->SetVec("vecCamPos", cam.Position);
        Prog3Ds.insert(progBasic);
    }
    {
        oglu::ShaderConfig config;
        config.Routines["getNorm"] = "bothNormal";
        config.Routines["getAlbedo"] = "bothAlbedo";
        Prog3DPBR = Wrapper<GLShader>(u"3D-pbr", LoadShaderFallback(shaderPath / u"3d_pbr.glsl", IDR_SHADER_3DPBR), config);
        glProgs.insert(Prog3DPBR);
        const auto progPBR = Prog3DPBR->Program;
        progPBR->State()
            .SetSubroutine("lighter", "albedoOnly")
            .SetSubroutine("getNorm", "bothNormal")
            .SetSubroutine("getAlbedo", "bothAlbedo");
        progPBR->SetView(cam.GetView());
        progPBR->SetVec("vecCamPos", cam.Position);
        Prog3Ds.insert(progPBR);
        prog3D = progPBR;
    }
}

void BasicTest::initTex()
{
    picTex.reset(128, 128, xziar::img::TextureFormat::RGBA8);
    picBuf.reset();
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
        picTex->SetData(TextureFormat::RGBAf, empty);
        picBuf->Write(nullptr, 128 * 128 * 4, BufferWriteMode::DynamicDraw);
    }
    chkTex = MultiMaterialHolder::GetCheckTex();
    chkTex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
    {
        oglu::oglTex2DArray tex2darr(128, 128, 1, xziar::img::TextureFormat::SRGBA8);
        tex2darr->SetTextureLayer(0, chkTex);
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
    for (const auto& prog : Prog3Ds)
    {
        prog->State().SetUBO(lightUBO, "lightBlock");
    }
    prepareLight();
}

void BasicTest::prepareLight()
{
    size_t pos = 0;
    uint32_t onCnt = 0;
    for (const auto& lgt : lights)
    {
        if (!lgt->IsOn)
            continue;
        onCnt++;
        lgt->WriteData(&LightBuf[pos]);
        pos += Light::WriteSize;
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
            const auto imgShow = fontCreator->clgraysdfs(U'啊', 16);
            GLWorker->InvokeShare([&imgShow, &fonttex](const common::asyexe::AsyncAgent& agent) 
            {
                fonttex->SetData(xziar::img::TextureFormat::R8, imgShow);
                agent.Await(oglu::oglUtil::SyncGL());
            })->Wait();
            img::WriteImage(imgShow, Basepath / (u"Show.png"));
            /*SimpleTimer timer;
            for (uint32_t cnt = 0; cnt < 65536; cnt += 4096)
            {
                const auto imgShow = fontCreator->clgraysdfs((char32_t)cnt, 4096);
                img::WriteImage(imgShow, basepath / (u"Show-" + std::to_u16string(cnt) + u".png"));
                dizzLog().success(u"successfully processed words begin from {}\n", cnt);
            }
            timer.Stop();
            dizzLog().success(u"successfully processed 65536 words, cost {}ms\n", timer.ElapseMs());*/
        }
        else
        {
            fontCreator->setChar(L'G');
            const auto imgG = fonttex->GetImage(ImageDataType::GRAY, false);
            img::WriteImage(imgG, Basepath / u"G.png");
            fontCreator->setChar(word);
            const auto imgA = fonttex->GetImage(ImageDataType::GRAY, false);
            img::WriteImage(imgA, Basepath / u"A.png");
            auto imgShow = fontCreator->clgraysdfs(U'啊', 16);
            img::WriteImage(imgShow, Basepath / u"Show.png");
            imgShow.FlipVertical(); // pre-flip
            fonttex->SetData(xziar::img::TextureFormat::R8, imgShow, true, false);
            fonttex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
            fontViewer->BindTexture(fonttex);
        }
    }
    catch (BaseException& be)
    {
        dizzLog().error(u"Font Construct failure:\n{}\n", be.message);
    }
}

void BasicTest::RefreshContext() const
{
    oglu::oglContext::Refresh();
    glContext->UseContext();
}


void BasicTest::Draw()
{
    RefreshContext();
    {
        const auto changed = (ChangableUBO)IsUBOChanged.exchange(0, std::memory_order::memory_order_relaxed);
        if (HAS_FIELD(changed, ChangableUBO::Light))
            prepareLight();
    }
    /*if (PostProc->UpdateLut())
    {
        const auto lutdata = PostProc->GetLut()->GetData(TextureFormat::RGB10A2);
        Image img(ImageDataType::RGBA);
        const auto lutSize = PostProc->GetLutSize();
        img.SetSize(lutSize, lutSize * lutSize);
        memcpy_s(img.GetRawPtr(), img.GetSize(), lutdata.data(), lutdata.size());
        xziar::img::WriteImage(img, fs::temp_directory_path() / u"lut.png");
    }*/
    RenderPassContext rpContext({}, WindowWidth, WindowHeight);
    {
        PostProc->OnPrepare(rpContext);
    }
    if (mode)
    {
        glContext->SetSRGBFBO(true);
        oglu::oglFBO::UseDefault();
        prog3D->SetView(cam.GetView());
        prog3D->SetVec("vecCamPos", cam.Position);
        auto drawcall = prog3D->Draw();
        for (const auto& d : drawables)
        {
            if (!d->ShouldRender)
                continue;
            d->Draw(drawcall);
            drawcall.Restore(true);
        }
    }
    else
    {
        const auto[w, h] = fboTex->GetSize();
        MiddleFrame->Use();
        glContext->SetSRGBFBO(false);
        glContext->ClearFBO();
        Resize(w, h, false);
        prog3D->SetView(cam.GetView());
        prog3D->SetVec("vecCamPos", cam.Position);
        {
            auto drawcall = prog3D->Draw();
            for (const auto& d : drawables)
            {
                if (!d->ShouldRender)
                    continue;
                d->Draw(drawcall);
                drawcall.Restore(true);
            }
        }
        oglu::oglFBO::UseDefault();
        //glContext->SetSRGBFBO(true);
        Resize(WindowWidth, WindowHeight, false);
        {
            progPost->Draw().Draw(PostProc->VAOScreen);
        }
        //fontViewer->Draw();
    }
}

void BasicTest::Resize(const uint32_t w, const uint32_t h, const bool changeWindow)
{
    RefreshContext();
    glContext->SetViewPort(0, 0, w, h);
    prog3D->SetProject(cam.GetProjection((float)w / h));
    if (changeWindow)
        WindowWidth = w, WindowHeight = h;
}

void BasicTest::ResizeFBO(const uint32_t w, const uint32_t h, const bool isFloatDepth)
{
    RefreshContext();
    fboTex.reset(w, h, xziar::img::TextureFormat::RG11B10);
    fboTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    MiddleFrame->AttachColorTexture(fboTex, 0);
    oglRBO mainRBO(w, h, isFloatDepth ? oglu::RBOFormat::Depth32Stencil8 : oglu::RBOFormat::Depth24Stencil8);
    MiddleFrame->AttachDepthStencilBuffer(mainRBO);
    dizzLog().info(u"FBO resize to [{}x{}], status:{}\n", w, h, MiddleFrame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
    progPost->State().SetTexture(fboTex, "scene");
    PostProc->SetMidFrame((uint16_t)w, (uint16_t)h, isFloatDepth);
}

void BasicTest::ReloadFontLoader(const u16string& fname)
{
    RefreshContext();
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
            dizzLog().error(u"failed to reload font test\n");
            if (onError)
                onError(be);
            else
                onFinish([]() { return false; });
        }
    }, fname).detach();
}


void BasicTest::LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError)
{
    std::thread([onFinish, onError, this](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for Model");
        try
        {
            Wrapper<Model> mod(name, TexLoader, GLWorker);
            mod->Name = u"model";
            onFinish(mod);
        }
        catch (BaseException& be)
        {
            dizzLog().error(u"failed to load model by file {}\n", name);
            if (onError)
                onError(be);
            else
                onFinish(Wrapper<Model>());
        }
    }, fname).detach();
}

common::PromiseResult<Wrapper<Model>> BasicTest::LoadModelAsync2(const u16string& fname) const
{
    auto fut = std::async(std::launch::async, [&](const u16string name)
        {
            common::SetThreadName(u"AsyncLoader for Model");
            try
            {
                Wrapper<Model> mod(name, TexLoader, GLWorker);
                mod->Name = u"model";
                return mod;
            }
            catch (BaseException& be)
            {
                dizzLog().error(u"failed to load model by file {}\n", name);
                throw be;
            }
        }, fname);
    return PromiseResultSTD<Wrapper<Model>>::Get(std::move(fut));
}

void BasicTest::LoadShaderAsync(const u16string& fpath, const u16string& shdName, std::function<void(Wrapper<GLShader>)> onFinish, std::function<void(const BaseException&)> onError /*= nullptr*/)
{
    auto pms = GLWorker->InvokeShare([fpath, shdName](const common::asyexe::AsyncAgent&)
    {
        auto shader = Wrapper<GLShader>(shdName, common::file::ReadAllText(fpath));
        shader->Program->State()
            .SetSubroutine("lighter", "tex0")
            .SetSubroutine("getNorm", "bothNormal")
            .SetSubroutine("getAlbedo", "bothAlbedo");
        return shader;
    }, u"load shader " + shdName, common::asyexe::StackSize::Big);
    std::thread([this, onFinish, onError](common::PromiseResult<Wrapper<GLShader>>&& pms)
    {
        common::SetThreadName(u"AsyncLoader for Shader");
        try
        {
            auto shader = pms->Wait();
            glProgs.insert(shader);
            onFinish(shader);
        }
        catch (const BaseException& be)
        {
            onError(be);
        }
    }, std::move(pms)).detach();
}

common::PromiseResult<Wrapper<GLShader>> BasicTest::LoadShaderAsync2(const u16string& fname, const u16string& shdName) const
{
    return GLWorker->InvokeShare([fname, shdName](const common::asyexe::AsyncAgent&)
        {
            auto shader = Wrapper<GLShader>(shdName, common::file::ReadAllText(fname));
            shader->Program->State()
                .SetSubroutine("lighter", "tex0")
                .SetSubroutine("getNorm", "bothNormal")
                .SetSubroutine("getAlbedo", "bothAlbedo");
            return shader;
        }, u"load shader " + shdName, common::asyexe::StackSize::Big);
}


bool BasicTest::AddObject(const Wrapper<Drawable>& drawable)
{
    RefreshContext();
    drawable->PrepareMaterial();
    drawable->AssignMaterial();
    for(const auto& prog : Prog3Ds)
        drawable->PrepareGL(prog);
    drawables.push_back(drawable);
    dizzLog().success(u"Add an Drawable [{}][{}]:  {}\n", drawables.size() - 1, drawable->GetType(), drawable->Name);
    return true;
}

bool BasicTest::AddShader(const oglDrawProgram& prog)
{
    RefreshContext();
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
    RefreshContext();
    lights.push_back(light);
    prepareLight();
    dizzLog().success(u"Add a Light [{}][{}]:  {}\n", lights.size() - 1, (int32_t)light->Type, light->Name);
    return true;
}

void BasicTest::ChangeShader(const oglDrawProgram& prog)
{
    RefreshContext();
    if (Prog3Ds.count(prog))
    {
        prog3D = prog;
        prog3D->SetProject(cam.GetProjection((float)WindowWidth / WindowHeight));
        prog3D->SetView(cam.GetView());
        prepareLight();
    }
    else
        dizzLog().warning(u"change to an unknown shader [{}], ignored.\n", prog ? prog->Name : u"null");
}

void BasicTest::DelAllLight()
{
    RefreshContext();
    lights.clear();
    prepareLight();
}

void BasicTest::ReportChanged(const ChangableUBO target)
{
    IsUBOChanged |= (uint32_t)target;
}

xziar::img::Image BasicTest::Screenshot()
{
    RefreshContext();
    const auto width = WindowWidth & 0xfffc, height = WindowHeight & 0xfffc;
    oglu::oglFBO ssFBO(std::in_place);
    oglu::oglTex2DS ssTex(width, height, xziar::img::TextureFormat::SRGBA8);
    ssTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    ssFBO->AttachColorTexture(ssTex, 0);
    dizzLog().info(u"Screenshot FBO [{}x{}], status:{}\n", width, height, ssFBO->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
    ssFBO->BlitColorFrom({}, { 0, 0, (int32_t)width, (int32_t)height });
    return ssTex->GetImage(xziar::img::ImageDataType::RGBA);
}

void BasicTest::Serialize(const fs::path & fpath) const
{
    SerializeUtil serializer(fpath);
    serializer.IsPretty = true;
    serializer.AddFilter([](const string_view& serType)
    {
        return serType == detail::_ModelMesh::SERIALIZE_TYPE ? "/meshes" : "";
    });
    {
        auto jlights = serializer.NewArray();
        for (const auto& lgt : lights)
            serializer.AddObject(jlights, *lgt);
        serializer.AddObject("lights", jlights);
    }
    {
        auto jdrawables = serializer.NewArray();
        for (const auto& drw : drawables)
            serializer.AddObject(jdrawables, *drw);
        serializer.AddObject("drawables", jdrawables);
    }
    {
        auto jprogs = serializer.NewArray();
        for (const auto& prog : { Prog2D,Prog3D, Prog3DPBR, ProgPost })
        {
            serializer.AddObject(jprogs, *prog);
        }
        serializer.AddObject("shaders", jprogs);
    }
    serializer.AddObject(serializer.Root, "camera", cam);
    serializer.Finish();
}

void BasicTest::DeSerialize(const fs::path & fpath)
{
    DeserializeUtil deserializer(fpath);
    {
        lights.clear();
        const auto jlights = deserializer.Root.GetArray("lights");
        for (const auto ele : jlights)
        {
            const ejson::JObjectRef<true> jlgt(ele);
            lights.push_back(deserializer.DeserializeShare<Light>(jlgt));
        }
        ReportChanged(ChangableUBO::Light);
    }
    {
        //drawables.clear();
        const auto jdrawables = deserializer.Root.GetArray("drawables");
        for (const auto ele : jdrawables)
        {
            const ejson::JObjectRef<true> jdrw(ele);
            const auto k = deserializer.DeserializeShare<Drawable>(jdrw);
            //drawables.push_back(deserializer.DeserializeShare<Drawable>(jdrw));
        }
    }
    {
        vector<Wrapper<GLShader>> tmpShaders;
        const auto jprogs = deserializer.Root.GetArray("shaders");
        for (const auto ele : jprogs)
        {
            const ejson::JObjectRef<true> jdrw(ele);
            const auto k = deserializer.DeserializeShare<GLShader>(jdrw);
            tmpShaders.push_back(k);
        }
    }
    Camera cam0 = *deserializer.Deserialize<Camera>(deserializer.Root.GetObject("camera"));
}

}