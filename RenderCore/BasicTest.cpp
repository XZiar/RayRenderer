#include "RenderCoreRely.h"
#include "resource.h"
#include "BasicTest.h"
#include "OpenGLUtil/oglWorker.h"
#include <thread>
#include <future>

namespace rayr
{

using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;

struct Init
{
    Init()
    {
        basLog().verbose(u"BasicTest Static Init\n");
        oglUtil::Init();
        oclu::oclUtil::init();
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
            basLog().error(u"unable to load shader from [{}] : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), fe.message);
        }
    }
    return getShaderFromDLL(id);
}

static oclPlatform FindPlatform(const std::vector<oclPlatform>& platforms, const Vendor vendor)
{
    const auto FindHasGPU = [](const oclPlatform& plat) 
    {
        return std::any_of(plat->GetDevices().cbegin(), plat->GetDevices().cend(),
            [](const oclDevice& dev) { return dev->Type == DeviceType::GPU; });
    };
    for (const auto& plt : platforms)
        if (plt->PlatVendor == vendor && FindHasGPU(plt))
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::NVIDIA && FindHasGPU(plt))
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::AMD && FindHasGPU(plt))
            return plt;
    for (const auto& plt : platforms)
        if (plt->PlatVendor == Vendor::Intel && FindHasGPU(plt))
            return plt;
    for (const auto& plt : platforms)
        if (FindHasGPU(plt))
            return plt;
    return oclPlatform();
}
static std::pair<oclContext, oclContext> CreateOCLContext(const Vendor vendor, const oglContext glContext)
{
    const auto venderClPlat = FindPlatform(oclUtil::getPlatforms(), vendor);
    const auto glPlat = std::find_if(oclUtil::getPlatforms().cbegin(), oclUtil::getPlatforms().cend(), [&](const oclPlatform& plat) 
    {
        return plat->IsGLShared(glContext);
    });
    oclContext defCtx, sharedCtx;
    if (!venderClPlat)
        COMMON_THROW(BaseException, u"No avaliable OpenCL Platform found");
    if (glPlat != oclUtil::getPlatforms().cend())
    {
        sharedCtx = (*glPlat)->CreateContext(glContext);
        basLog().success(u"Created Shared OCLContext in platform {}!\n", (*glPlat)->Name);
        sharedCtx->onMessage = [](const u16string& errtxt) { basLog().error(u"Error from shared CLContext:\t{}\n", errtxt); };
        if (*glPlat == venderClPlat)
            defCtx = sharedCtx;
    }
    defCtx = venderClPlat->CreateContext();
    basLog().success(u"Created OCLContext in platform {}!\n", venderClPlat->Name);
    defCtx->onMessage = [](const u16string& errtxt) { basLog().error(u"Error from CLContext:\t{}\n", errtxt); };
    return { defCtx, sharedCtx };
}

BasicTest::BasicTest(const fs::path& shaderPath)
{
    static Init _init;
    const auto oriCtx = oglu::oglContext::CurrentContext();
    oriCtx->SetRetain(true);
    glContext = oglu::oglContext::NewContext(oriCtx);
    glContext->UseContext();
    //glContext = oglu::oglContext::CurrentContext();
    {
        const auto ctxs = CreateOCLContext(Vendor::NVIDIA, glContext);
        ClContext = ctxs.first; ClSharedContext = ctxs.second;
    }
    ThumbMan.reset(glContext, ClSharedContext);
    GLWorker.reset(u"Core");
    GLWorker->Start();
    //for reverse-z
    glContext->SetDepthClip(true);
    glContext->SetDepthTest(DepthTestType::GreaterEqual);
    //glContext->SetFaceCulling(FaceCullingType::CullCW);
    cam.Resize(1280, 720);
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
    MiddleFrame.reset();
    ResizeFBO(1280, 720, true);
    //prog2D->State().SetTexture(fontCreator->getTexture(), "tex");
    initUBO();
    glProgs.insert(prog2D);
    glProgs.insert(progPost);
    glProgs.insert(Prog3Ds.cbegin(), Prog3Ds.cend());
    glProgs.insert(fontViewer->prog);
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
        prog2D.reset(u"Prog 2D");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"2d.glsl", IDR_SHADER_2D);
        try
        {
            prog2D->AddExtShaders(shaderSrc);
            prog2D->Link();
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"2D OpenGL shader fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, u"2D OpenGL shader fail");
        }
        picVAO.reset(VAODrawMode::Triangles);
        picVAO->Prepare()
            .SetFloat(screenBox, prog2D->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(screenBox, prog2D->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        prog2D->State().SetTexture(picTex, "tex");
    }
    {
        progPost.reset(u"PostProcess");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"postprocess.glsl", IDR_SHADER_POSTPROC);
        try
        {
            progPost->AddExtShaders(shaderSrc);
            progPost->Link();
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"2D OpenGL shader fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, u"2D OpenGL shader fail");
        }
        ppVAO.reset(VAODrawMode::Triangles);
        ppVAO->Prepare()
            .SetFloat(screenBox, progPost->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(screenBox, progPost->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        prog2D->State().SetSubroutine("ToneMap", "NoTone");
    }
}

void BasicTest::init3d(const fs::path& shaderPath)
{
    cam.Position = Vec3(0.0f, 0.0f, 4.0f);
    {
        oglDrawProgram progBasic(u"3D Prog");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"3d.glsl", IDR_SHADER_3D);
        try
        {
            progBasic->AddExtShaders(shaderSrc);
            progBasic->Link();
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"3D OpenGL shader fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, u"3D OpenGL shader fail");
        }
        progBasic->State().SetSubroutine("lighter", "tex0").SetSubroutine("getNorm", "vertedNormal");
        progBasic->SetView(cam.GetView());
        progBasic->SetVec("vecCamPos", cam.Position);
        Prog3Ds.insert(progBasic);
    }
    {
        oglDrawProgram progPBR(u"3D-pbr");
        const string shaderSrc = LoadShaderFallback(shaderPath / u"3d_pbr.glsl", IDR_SHADER_3DPBR);
        try
        {
            oglu::ShaderConfig config;
            config.Routines["getNorm"] = "bothNormal";
            config.Routines["getAlbedo"] = "bothAlbedo";
            progPBR->AddExtShaders(shaderSrc, config);
            progPBR->Link();
        }
        catch (const OGLException& gle)
        {
            basLog().error(u"3D OpenGL shader fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, u"3D OpenGL shader fail");
        }
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
    picTex.reset(128, 128, TextureInnerFormat::RGBA8);
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
            const auto imgShow = fontCreator->clgraysdfs(U'啊', 16);
            GLWorker->InvokeShare([&imgShow, &fonttex](const common::asyexe::AsyncAgent& agent) 
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
            const auto imgG = fonttex->GetImage(ImageDataType::GRAY, false);
            img::WriteImage(imgG, Basepath / u"G.png");
            fontCreator->setChar(word, false);
            const auto imgA = fonttex->GetImage(ImageDataType::GRAY, false);
            img::WriteImage(imgA, Basepath / u"A.png");
            auto imgShow = fontCreator->clgraysdfs(U'啊', 16);
            img::WriteImage(imgShow, Basepath / u"Show.png");
            imgShow.FlipVertical(); // pre-flip
            fonttex->SetData(TextureInnerFormat::R8, imgShow, true, false);
            fonttex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::Clamp);
            fontViewer->BindTexture(fonttex);
        }
    }
    catch (BaseException& be)
    {
        basLog().error(u"Font Construct failure:\n{}\n", be.message);
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
        const auto ow = cam.Width, oh = cam.Height;
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
        glContext->SetSRGBFBO(true);
        Resize(ow, oh, false);
        {
            const auto sw = w * oh / h;
            const auto widthscale = sw * 1.0f / ow;
            progPost->Draw().SetUniform("widthscale", widthscale).Draw(ppVAO);
        }
        //fontViewer->Draw();
    }
}

void BasicTest::Resize(const uint32_t w, const uint32_t h, const bool changeWindow)
{
    RefreshContext();
    glContext->SetViewPort(0, 0, w, h);
    cam.Resize(w, h);
    prog3D->SetProject(cam.GetProjection());
    if (changeWindow)
        WindowWidth = w, WindowHeight = h;
}

void BasicTest::ResizeFBO(const uint32_t w, const uint32_t h, const bool isFloatDepth)
{
    RefreshContext();
    fboTex.reset(w, h, TextureInnerFormat::RG11B10);
    fboTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    MiddleFrame->AttachColorTexture(fboTex, 0);
    oglRBO mainRBO(w, h, isFloatDepth ? oglu::RBOFormat::Depth32Stencil8 : oglu::RBOFormat::Depth24Stencil8);
    MiddleFrame->AttachDepthStencilBuffer(mainRBO);
    basLog().info(u"FBO resize to [{}x{}], status:{}\n", w, h, MiddleFrame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
    progPost->State().SetTexture(fboTex, "tex");
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
    std::thread([onFinish, onError, this](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for Model");
        try
        {
            Wrapper<Model> mod(name, GLWorker);
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

void BasicTest::LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(oglDrawProgram)> onFinish, std::function<void(const BaseException&)> onError /*= nullptr*/)
{
    using common::asyexe::StackSize;
    const auto loadPms = std::make_shared<std::promise<oglDrawProgram>>();
    auto fut = loadPms->get_future();
    GLWorker->InvokeShare([fname, shdName, loadPms](const common::asyexe::AsyncAgent& agent)
    {
        oglDrawProgram prog(shdName);
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
    std::thread([onFinish, onError](std::future<oglDrawProgram>&& fut)
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
    RefreshContext();
    drawable->PrepareMaterial(ThumbMan.weakRef());
    drawable->AssignMaterial();
    for(const auto& prog : Prog3Ds)
        drawable->PrepareGL(prog);
    drawables.push_back(drawable);
    basLog().success(u"Add an Drawable [{}][{}]:  {}\n", drawables.size() - 1, drawable->GetType(), drawable->Name);
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
    basLog().success(u"Add a Light [{}][{}]:  {}\n", lights.size() - 1, (int32_t)light->type, light->name);
    return true;
}

void BasicTest::ChangeShader(const oglDrawProgram& prog)
{
    RefreshContext();
    if (Prog3Ds.count(prog))
    {
        prog3D = prog;
        prog3D->SetProject(cam.GetProjection());
        prog3D->SetView(cam.GetView());
        prepareLight();
    }
    else
        basLog().warning(u"change to an unknown shader [{}], ignored.\n", prog ? prog->Name : u"null");
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

xziar::img::Image BasicTest::Scrrenshot()
{
    RefreshContext();
    const auto width = WindowWidth & 0xfffc, height = WindowHeight & 0xfffc;
    oglu::oglFBO ssFBO(std::in_place);
    oglu::oglTex2DS ssTex(width, height, TextureInnerFormat::SRGBA8);
    ssTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    ssFBO->AttachColorTexture(ssTex, 0);
    basLog().info(u"Screenshot FBO [{}x{}], status:{}\n", width, height, ssFBO->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
    ssFBO->BlitColorFrom({}, { 0, 0, (int32_t)width, (int32_t)height });
    return ssTex->GetImage(xziar::img::ImageDataType::RGBA);
}

static ejson::JObject SerializeGLProg(const oglu::oglDrawProgram& prog, SerializeUtil& context)
{
    auto jprog = context.NewObject();
    jprog.Add("Name", str::to_u8string(prog->Name, Charset::UTF16LE));
    const auto& src = prog->GetExtShaderSource();
    jprog.Add("source", context.PutResource(src.data(), src.size()));
    return jprog;
}

void BasicTest::Serialize(const fs::path & fpath) const
{
    SerializeUtil serializer(fpath);
    serializer.IsPretty = true;
    serializer.AddFilter([](SerializeUtil&, const xziar::respak::Serializable& object)
    {
        return SerializeUtil::IsAnyType<detail::_ModelMesh>(object) ? "/meshes" : "";
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
        for (const auto& prog : glProgs)
            jprogs.Push(SerializeGLProg(prog, serializer));
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
    Camera cam0 = *deserializer.Deserialize<Camera>(deserializer.Root.GetObject("camera"));
}

}