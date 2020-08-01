#include "RenderCorePch.h"
#include "resource.h"
#include "RenderCore.h"
#include "SceneManager.h"
#include "RenderPass.h"
#include "GLShader.h"
#include "FontTest.h"
#include "ThumbnailManager.h"
#include "TextureLoader.h"
#include "PostProcessor.h"
#include "Basic3DObject.h"
#include "OpenGLUtil/oglWorker.h"
#include "TextureUtil/TexUtilWorker.h"
#include "TextureUtil/TexMipmap.h"
#include <thread>
#include <future>


namespace rayr
{
using std::vector;
using common::container::FindInSet;
using common::container::FindInMap;
using common::container::ValSet;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;
using namespace oclu;
using namespace oglu;


RESPAK_IMPL_COMP_DESERIALIZE(FontTester, oclu::oclContext)
{
    return std::any();
}


static int32_t JudgeVendor(const Vendors vendor)
{
    switch (vendor)
    {
    case Vendors::NVIDIA: return 1;
    case Vendors::AMD:    return 2;
    case Vendors::Intel:  return 3;
    default:              return 4;
    }
}

static std::pair<oclContext, oclContext> CreateOCLContext(const Vendors vendor, const oglContext glContext)
{
    const auto gpuPlats = common::linq::FromIterable(oclUtil::GetPlatforms())
        .Where([](const auto& plat) 
            { 
                return plat->Version >= 12 && common::linq::FromContainer(plat->GetDevices())
                    .ContainsIf([](const auto& dev) { return dev->Type == DeviceType::GPU; }); 
            })
        .ToVector();
    const auto venderClPlat = common::linq::FromIterable(gpuPlats)
        .Select([&](const auto& plat) { return std::pair{ plat->PlatVendor == vendor ? 0 : JudgeVendor(plat->PlatVendor), plat }; })
        .OrderBy<common::container::PairLess>().Select([](const auto& p) { return p.second; })
        .TryGetFirst().value_or(oclPlatform{});
    if (venderClPlat)
    {
        const auto glPlat = common::linq::FromIterable(gpuPlats)
            .Where([&](const auto& plat) { return GLInterop::CheckIsGLShared(*plat, glContext); })
            .TryGetFirst().value_or(oclPlatform{});
        oclContext defCtx, sharedCtx;
        if (glPlat)
        {
            sharedCtx = GLInterop::CreateGLSharedContext(*glPlat, glContext);
            dizzLog().success(u"Created Shared OCLContext in platform {}!\n", glPlat->Name);
            sharedCtx->OnMessage += [](const u16string& txt)
            {
                dizzLog().verbose(u"From shared CLContext:\t{}\n", txt);
            };
        }
        if (glPlat == venderClPlat)
            defCtx = sharedCtx;
        else
        {
            defCtx = venderClPlat->CreateContext();
            defCtx->OnMessage += [](const u16string& txt)
            {
                dizzLog().verbose(u"From CLContext:\t{}\n", txt);
            };
        }
        dizzLog().success(u"Created OCLContext in platform {}!\n", venderClPlat->Name);
        return { defCtx, sharedCtx };
    }
    else
    {
        dizzLog().warning(u"No avaliable OpenCL Platform found\n");
        return {};
    }
}


RenderCore::RenderCore()
{
    oglu::oglUtil::InitLatestVersion();
    const auto oriCtx = oglu::oglContext_::Refresh();
    //oriCtx->SetRetain(true);
    GLContext = oglu::oglContext_::NewContext(oriCtx);
    GLContext->UseContext();

    //for reverse-z
    GLContext->SetDepthClip(true);
    GLContext->SetDepthTest(DepthTestType::GreaterEqual);
    GLContext->SetSRGBFBO(true);
    //GLContext->SetFaceCulling(FaceCullingType::CullCW);
    {
        std::tie(CLContext, CLSharedContext) = CreateOCLContext(Vendors::NVIDIA, GLContext);
        if (CLSharedContext)
        {
            CLQue = oclCmdQue_::Create(CLSharedContext, CLSharedContext->GetGPUDevice());
            if (!CLQue)
                COMMON_THROWEX(BaseException, u"clQueue initialized failed!");
        }
    }
    GLWorker = std::make_shared<oglu::oglWorker>(u"Core");
    GLWorker->Start();
    TexWorker = std::make_shared<texutil::TexUtilWorker>(oglContext_::NewContext(GLContext, true), CLSharedContext);
    MipMapper = std::make_shared<texutil::TexMipmap>(TexWorker);
    TexLoader = std::make_shared<TextureLoader>(MipMapper);
    //MipMapper->Test2();
    ThumbMan = std::make_shared<ThumbnailManager>(TexWorker, GLWorker);
    PostProc = std::make_shared<PostProcessor>();
    TheScene = std::make_unique<Scene>();

    InitShaders();
    PostProc->SetMidFrame(1280, 720, true);
    
    {
        auto basicPipeLine = std::make_shared<RenderPipeLine>();
        const auto pbrPass = *RenderPasses.begin();
        basicPipeLine->Passes.push_back(pbrPass);
        basicPipeLine->Passes.push_back(PostProc);
        RenderTask = basicPipeLine;
        PipeLines.insert(basicPipeLine);
    }
    if (CLContext)
    {
        auto fontPipeLine = std::make_shared<RenderPipeLine>();
        auto fontTest = std::make_shared<FontTester>(CLContext);
        RenderPasses.insert(fontTest);
        fontPipeLine->Passes.push_back(fontTest);
        PipeLines.insert(fontPipeLine);
    }
}

RenderCore::~RenderCore()
{
}

void RenderCore::RefreshContext() const
{
    //oglContext_::Refresh();
    GLContext->UseContext(true);
}

void RenderCore::InitShaders()
{
    ShaderConfig config;
    config.Routines["getNorm"] = "bothNormal";
    config.Routines["getAlbedo"] = "bothAlbedo";
    auto prog3D = std::make_shared<DefaultRenderPass>(u"3D-pbr", LoadShaderFromDLL(IDR_SHADER_3DPBR), config);
    AddShader(prog3D);
    prog3D->Program->State()
        .SetSubroutine("lighter", "albedoOnly")
        .SetSubroutine("getNorm", "bothNormal")
        .SetSubroutine("getAlbedo", "bothAlbedo");
}

void RenderCore::TestSceneInit()
{
    const auto pbrPass = *RenderPasses.begin();
    auto pyramid = std::make_shared<Pyramid>(1.0f);
    pyramid->Name = u"Pyramid";
    pyramid->Position = { 0,0,0 };
    TheScene->AddObject(pyramid);
    auto ball = std::make_shared<Sphere>(0.75f);
    ball->Name = u"Ball";
    ball->Position = { 1,0,0 };
    TheScene->AddObject(ball);
    auto box = std::make_shared<Box>(0.5f, 1.0f, 2.0f);
    box->Name = u"Box";
    box->Position = { 0,1,0 };
    TheScene->AddObject(box);
    auto ground = std::make_shared<Plane>(500.0f, 50.0f);
    ground->Name = u"Ground";
    ground->Position = { 0,-2,0 };
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
    {
        RenderPassContext context(TheScene, WindowWidth, WindowHeight);
        RenderTask->Render(std::move(context));
    }
}

void RenderCore::Resize(const uint32_t w, const uint32_t h)
{
    RefreshContext();
    WindowWidth = static_cast<uint16_t>(std::clamp<uint32_t>(w, 8, UINT16_MAX)), WindowHeight = static_cast<uint16_t>(std::clamp<uint32_t>(h, 8, UINT16_MAX));
    oglDefaultFrameBuffer_::Get()->SetWindowSize(WindowWidth, WindowHeight);
}

vector<std::shared_ptr<common::Controllable>> RenderCore::GetControllables() const noexcept
{
    vector<std::shared_ptr<common::Controllable>> controls;
    controls.emplace_back(TexLoader);
    controls.emplace_back(PostProc);
    return controls;
}

void RenderCore::LoadModelAsync(const u16string & fname, std::function<void(std::shared_ptr<Model>)> onFinish, std::function<void(const BaseException&)> onError) const
{
    std::thread([onFinish, onError, this](const u16string name)
    {
        common::SetThreadName(u"AsyncLoader for Model");
        try
        {
            auto mod = std::make_shared<Model>(name, TexLoader, GLWorker);
            mod->Name = u"model";
            onFinish(mod);
        }
        catch (const BaseException& be)
        {
            dizzLog().error(u"failed to load model by file {}\n", name);
            if (onError)
                onError(be);
            else
                onFinish(std::shared_ptr<Model>());
        }
    }, fname).detach();
}

common::PromiseResult<std::shared_ptr<Model>> RenderCore::LoadModelAsync2(const u16string& fname) const
{
    auto fut = std::async(std::launch::async, [&](const u16string name)
        {
            common::SetThreadName(u"AsyncLoader for Model");
            try
            {
                auto mod = std::make_shared<Model>(name, TexLoader, GLWorker);
                mod->Name = u"model";
                return mod;
            }
            catch (const BaseException& be)
            {
                dizzLog().error(u"failed to load model by file {}\n", name);
                throw be;
            }
        }, fname);
    return common::PromiseResultSTD<std::shared_ptr<Model>>::Get(std::move(fut));
}

void RenderCore::LoadShaderAsync(const u16string & fname, const u16string & shdName, std::function<void(std::shared_ptr<DefaultRenderPass>)> onFinish, std::function<void(const BaseException&)> onError) const
{
    auto pms = GLWorker->InvokeShare([fname, shdName](const common::asyexe::AsyncAgent&)
    {
        auto shader = std::make_shared<DefaultRenderPass>(shdName, common::file::ReadAllText(fname));
        shader->Program->State()
            .SetSubroutine("lighter", "tex0")
            .SetSubroutine("getNorm", "bothNormal")
            .SetSubroutine("getAlbedo", "bothAlbedo");
        return shader;
    }, u"load shader " + shdName, common::asyexe::StackSize::Big);
    std::thread([onFinish, onError](common::PromiseResult<std::shared_ptr<DefaultRenderPass>>&& pms)
    {
        common::SetThreadName(u"AsyncLoader for Shader");
        try
        {
            auto shader = pms->Get();
            onFinish(shader);
        }
        catch (const BaseException& be)
        {
            onError(be);
        }
    }, std::move(pms)).detach();
}

common::PromiseResult<std::shared_ptr<DefaultRenderPass>> RenderCore::LoadShaderAsync2(const u16string& fname, const u16string& shdName) const
{
    return GLWorker->InvokeShare([fname, shdName](const common::asyexe::AsyncAgent&)
        {
            auto shader = std::make_shared<DefaultRenderPass>(shdName, common::file::ReadAllText(fname));
            shader->Program->State()
                .SetSubroutine("lighter", "tex0")
                .SetSubroutine("getNorm", "bothNormal")
                .SetSubroutine("getAlbedo", "bothAlbedo");
            return shader;
        }, u"load shader " + shdName, common::asyexe::StackSize::Big);
}

bool RenderCore::AddShader(const std::shared_ptr<DefaultRenderPass>& shader)
{
    if (!shader) return false;
    return RenderPasses.insert(shader).second;
}

bool RenderCore::DelShader(const std::shared_ptr<DefaultRenderPass>& shader)
{
    return RenderPasses.erase(shader) > 0;
}

void RenderCore::ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline)
{
    if (!FindInSet(PipeLines, pipeline))
        PipeLines.insert(pipeline);
    RenderTask = pipeline;
}


void RenderCore::Serialize(const fs::path & fpath) const
{
    RefreshContext();
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
        vector<std::shared_ptr<RenderPass>> tmpShaders;
        const auto jpasses = deserializer.Root.GetArray("passes");
        for (const auto ele : jpasses)
        {
            const xziar::ejson::JObjectRef<true> jpass(ele);
            const auto k = deserializer.DeserializeShare<RenderPass>(jpass);
            tmpShaders.push_back(k);
        }
    }
    for (const auto& shd : RenderPasses)
    {
        for (const auto& drw : ValSet(TheScene->GetDrawables()))
            shd->RegistDrawable(drw);
        shd->CleanDrawable();
    }
}

xziar::img::Image RenderCore::Screenshot()
{
    RefreshContext();
    const auto width = WindowWidth & 0xfffc, height = WindowHeight & 0xfffc;
    auto ssFBO = oglu::oglFrameBuffer2D_::Create();
    auto ssTex = oglu::oglTex2DStatic_::Create(width, height, xziar::img::TextureFormat::SRGBA8);
    ssTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    ssFBO->AttachColorTexture(ssTex);
    dizzLog().info(u"Screenshot FBO [{}x{}], status:{}\n", width, height, ssFBO->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
    ssFBO->BlitColorFrom(oglu::oglDefaultFrameBuffer_::Get(), { 0, 0, (int32_t)width, (int32_t)height });
    return ssTex->GetImage(xziar::img::ImageDataType::RGBA);
}


}