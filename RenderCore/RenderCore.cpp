#include "RenderCoreRely.h"
#include "resource.h"
#include "RenderCore.h"
#include "TextureLoader.h"
#include "FontTest.h"
#include "OpenGLUtil/oglWorker.h"
#include "TextureUtil/TexUtilWorker.h"
#include "TextureUtil/TexMipmap.h"
#include "Basic3DObject.h"
#include <thread>

using common::container::FindInSet;
using common::container::FindInMap;

namespace rayr
{


RESPAK_IMPL_COMP_DESERIALIZE(FontTester, oclu::oclContext)
{
    return std::any();
}


struct Init
{
    Init()
    {
        dizzLog().verbose(u"RenderCore Static Init\n");
        oglUtil::Init();
        oclUtil::Init();
    }
};

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
    const auto venderClPlat = Linq::FromIterable(oclUtil::getPlatforms())
        .Where([](const auto& plat) { return Linq::FromIterable(plat->GetDevices())
            .ContainsIf([](const auto& dev) { return dev->Type == DeviceType::GPU; }); })
        .Select([&](const auto& plat) { return std::pair{ plat->PlatVendor == vendor ? 0 : JudgeVendor(plat->PlatVendor), plat }; })
        .SortBy<common::container::PairLess>().Select([](const auto& p) { return p.second; })
        .TryGetFirst().value_or(oclPlatform{});
    if (!venderClPlat)
        COMMON_THROWEX(BaseException, u"No avaliable OpenCL Platform found");
    const auto glPlat = Linq::FromIterable(oclUtil::getPlatforms())
        .Where([](const auto& plat) { return Linq::FromIterable(plat->GetDevices())
            .ContainsIf([](const auto& dev) { return dev->Type == DeviceType::GPU; }); })
        .Where([&](const auto& plat) { return plat->IsGLShared(glContext); })
        .TryGetFirst().value_or(oclPlatform{});
    oclContext defCtx, sharedCtx;
    if (glPlat)
    {
        sharedCtx = glPlat->CreateContext(glContext);
        dizzLog().success(u"Created Shared OCLContext in platform {}!\n", glPlat->Name);
        sharedCtx->onMessage = [](const u16string& errtxt) { dizzLog().error(u"Error from shared CLContext:\t{}\n", errtxt); };
    }
    defCtx = glPlat == venderClPlat ? sharedCtx : venderClPlat->CreateContext();
    dizzLog().success(u"Created OCLContext in platform {}!\n", venderClPlat->Name);
    defCtx->onMessage = [](const u16string& errtxt) { dizzLog().error(u"Error from CLContext:\t{}\n", errtxt); };
    return { defCtx, sharedCtx };
}


RenderCore::RenderCore()
{
    static Init init;
    const auto oriCtx = oglu::oglContext::CurrentContext();
    oriCtx->SetRetain(true);
    GLContext = oglu::oglContext::NewContext(oriCtx);
    GLContext->UseContext();
    //for reverse-z
    GLContext->SetDepthClip(true);
    GLContext->SetDepthTest(DepthTestType::GreaterEqual);
    GLContext->SetSRGBFBO(true);
    //GLContext->SetFaceCulling(FaceCullingType::CullCW);
    {
        const auto ctxs = CreateOCLContext(Vendor::NVIDIA, GLContext);
        CLContext = ctxs.first; CLSharedContext = ctxs.second;
        CLQue.reset(CLSharedContext, CLSharedContext->GetGPUDevice());
        if (!CLQue)
            COMMON_THROWEX(BaseException, u"clQueue initialized failed!");
    }
    TexWorker = std::make_shared<texutil::TexUtilWorker>(oglContext::NewContext(GLContext, true), CLSharedContext);
    MipMapper = std::make_shared<texutil::TexMipmap>(TexWorker);
    TexLoader = std::make_shared<detail::TextureLoader>(MipMapper);
    //MipMapper->Test2();
    ThumbMan.reset(TexWorker);
    PostProc.reset(CLSharedContext, CLQue);
    GLWorker = std::make_shared<oglu::oglWorker>(u"Core");
    GLWorker->Start();
    TheScene.reset();

    InitShaders();
    PostProc->SetMidFrame(1280, 720, true);
    
    {
        Wrapper<RenderPipeLine> basicPipeLine(std::in_place);
        const auto pbrPass = *RenderPasses.begin();
        basicPipeLine->Passes.push_back(pbrPass);
        basicPipeLine->Passes.push_back(PostProc);
        RenderTask = basicPipeLine;
        PipeLines.insert(basicPipeLine);
    }
    {
        Wrapper<RenderPipeLine> fontPipeLine(std::in_place);
        Wrapper<FontTester> fontTest(CLContext);
        RenderPasses.insert(fontTest);
        fontPipeLine->Passes.push_back(fontTest);
        PipeLines.insert(fontPipeLine);
    }
}

void RenderCore::RefreshContext() const
{
    oglContext::Refresh();
    GLContext->UseContext();
}

void RenderCore::InitShaders()
{
    ShaderConfig config;
    config.Routines["getNorm"] = "bothNormal";
    config.Routines["getAlbedo"] = "bothAlbedo";
    Wrapper<DefaultRenderPass> prog3D(u"3D-pbr", getShaderFromDLL(IDR_SHADER_3DPBR), config);
    AddShader(prog3D);
    prog3D->Program->State()
        .SetSubroutine("lighter", "albedoOnly")
        .SetSubroutine("getNorm", "bothNormal")
        .SetSubroutine("getAlbedo", "bothAlbedo");
}

RenderCore::~RenderCore()
{
}

void RenderCore::TestSceneInit()
{
    const auto pbrPass = *RenderPasses.begin();
    Wrapper<Pyramid> pyramid(1.0f);
    pyramid->Name = u"Pyramid";
    pyramid->position = { 0,0,0 };
    TheScene->AddObject(pyramid);
    Wrapper<Sphere> ball(0.75f);
    ball->Name = u"Ball";
    ball->position = { 1,0,0 };
    TheScene->AddObject(ball);
    Wrapper<Box> box(0.5f, 1.0f, 2.0f);
    box->Name = u"Box";
    box->position = { 0,1,0 };
    TheScene->AddObject(box);
    Wrapper<Plane> ground(500.0f, 50.0f);
    ground->Name = u"Ground";
    ground->position = { 0,-2,0 };
    TheScene->AddObject(ground);
    for (const auto& pass : RenderPasses)
    {
        pass->RegistDrawable(pyramid);
        pass->RegistDrawable(ball);
        pass->RegistDrawable(box);
        pass->RegistDrawable(ground);
    }
}

void RenderCore::Draw()
{
    RefreshContext();
    {
        TheScene->PrepareLight();
        TheScene->PrepareDrawable();
    }
    if (RenderTask)
        RenderTask->Render(TheScene);
}

void RenderCore::Resize(const uint32_t w, const uint32_t h)
{
    RefreshContext();
    GLContext->SetViewPort(0, 0, w, h);
    TheScene->GetCamera()->Resize(w, h);
    WindowWidth = w, WindowHeight = h;
}

void RenderCore::LoadModelAsync(const u16string & fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError)
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

void RenderCore::LoadShaderAsync(const u16string & fname, const u16string & shdName, std::function<void(Wrapper<DefaultRenderPass>)> onFinish, std::function<void(const BaseException&)> onError)
{
    auto pms = GLWorker->InvokeShare([fname, shdName](const common::asyexe::AsyncAgent&)
    {
        auto shader = Wrapper<DefaultRenderPass>(shdName, common::file::ReadAllText(fname));
        shader->Program->State()
            .SetSubroutine("lighter", "tex0")
            .SetSubroutine("getNorm", "bothNormal")
            .SetSubroutine("getAlbedo", "bothAlbedo");
        return shader;
    }, u"load shader " + shdName, common::asyexe::StackSize::Big);
    std::thread([this, onFinish, onError](common::PromiseResult<Wrapper<DefaultRenderPass>>&& pms)
    {
        common::SetThreadName(u"AsyncLoader for Shader");
        try
        {
            auto shader = pms->Wait();
            onFinish(shader);
        }
        catch (const BaseException& be)
        {
            onError(be);
        }
    }, std::move(pms)).detach();
}

void RenderCore::AddShader(const Wrapper<DefaultRenderPass>& shader)
{
    RenderPasses.insert(shader);
}

void RenderCore::ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline)
{
    if (!FindInSet(PipeLines, pipeline))
        PipeLines.insert(pipeline);
    RenderTask = pipeline;
}


void RenderCore::Serialize(const fs::path & fpath) const
{
    SerializeUtil serializer(fpath);
    serializer.IsPretty = true;
    serializer.AddFilter([](const string_view& serType)
    {
        if (serType == detail::_ModelMesh::SERIALIZE_TYPE)
            return "/meshes";
        else if (serType == detail::_FakeTex::SERIALIZE_TYPE)
            return "/textures";
        return "";
    });
    {
        auto jprogs = serializer.NewArray();
        for (const auto& pass : RenderPasses)
        {
            serializer.AddObject(jprogs, *pass);
        }
        serializer.AddObject("passes", jprogs);
    }
    serializer.AddObject(serializer.Root, "scene", *TheScene);
    serializer.Finish();
}

void RenderCore::DeSerialize(const fs::path & fpath)
{
    RefreshContext();
    DeserializeUtil deserializer(fpath);
    deserializer.SetCookie("oglWorker", GLWorker);
    deserializer.SetCookie("texLoader", TexLoader);
    TheScene = deserializer.DeserializeShare<Scene>(deserializer.Root.GetObject("scene"));
    {
        vector<Wrapper<RenderPass>> tmpShaders;
        const auto jpasses = deserializer.Root.GetArray("passes");
        for (const auto ele : jpasses)
        {
            const ejson::JObjectRef<true> jpass(ele);
            const auto k = deserializer.DeserializeShare<RenderPass>(jpass);
            tmpShaders.push_back(k);
        }
    }
    for (const auto& shd : RenderPasses)
    {
        for (const auto& drw : TheScene->GetDrawables())
            shd->RegistDrawable(drw);
        shd->CleanDrawable();
    }
}


}