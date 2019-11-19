#include "RenderCorePch.h"
#include "RenderPass.h"
#include "SceneManager.h"


namespace rayr
{
using common::str::Charset;
using common::container::FindInSet;
using common::container::FindInMap;
using common::container::FindInMapOrDefault;
using common::container::FindInVec;
using common::container::ReplaceInVec;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


RenderPassContext::RenderPassContext(const std::shared_ptr<Scene>& scene, const uint16_t ScreenWidth, const uint16_t ScreenHeight) 
    : TheScene(scene), ScreenSize(ScreenWidth, ScreenHeight)
{
}

const std::shared_ptr<Scene>& RenderPassContext::GetScene() const
{
    return TheScene;
}

oglu::oglFBO RenderPassContext::GetFrameBuffer(const string_view & name) const
{
    return FindInMapOrDefault(FrameBufs, name);
}

void RenderPassContext::SetFrameBuffer(const string_view & name, const oglu::oglFBO & fbo)
{
    FrameBufs.insert_or_assign(string(name), fbo);
}

oglu::oglFBO RenderPassContext::GetOrCreateFrameBuffer(const string_view & name, const std::function<oglu::oglFBO(void)>& creator)
{
    const auto it = FindInMap(FrameBufs, name);
    if (it)
        return *it;
    const auto fbo = creator();
    FrameBufs.insert_or_assign(string(name), fbo);
    return fbo;
}

oglu::oglTex2D RenderPassContext::GetTexture(const string_view & name) const
{
    return FindInMapOrDefault(Textures, name);
}

void RenderPassContext::SetTexture(const string_view & name, const oglu::oglTex2D & tex)
{
    Textures.insert_or_assign(string(name), tex);
}


RenderPass::RenderPass() : GLContext(oglu::oglContext_::CurrentContext())
{

}

void RenderPass::RegistDrawable(const std::shared_ptr<Drawable>& drawable)
{
    WaitAddDrawables.push_back(drawable);
}

void RenderPass::UnregistDrawable(const std::shared_ptr<Drawable>& drawable)
{
    Drawables.erase(drawable);
    //WaitDelDrawables.push_back(drawable);
}

void RenderPass::CleanDrawable()
{
    Drawables = common::linq::FromIterable(Drawables)
        .Where([](const auto& d) { return !d.expired(); })
        .ToOrderSet<std::owner_less<>>();
}

void RenderPass::Prepare(RenderPassContext& context)
{
    for (const auto& d : WaitAddDrawables)
    {
        const auto drw = d.lock();
        if (drw && OnRegistDrawable(drw))
            Drawables.insert(d);
    }
    WaitAddDrawables.clear();
    OnPrepare(context);
}

void RenderPass::Draw(RenderPassContext & context)
{
    OnDraw(context);
}

void RenderPass::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    jself.Add("Name", common::strchset::to_u8string(GetName(), Charset::UTF16LE));
    auto jdrawables = context.NewArray();
    for (const auto& d : Drawables)
    {
        const auto drw = d.lock();
        if (drw)
            jdrawables.Push(boost::uuids::to_string(drw->GetUid()));
    }
    jself.Add("drawables", jdrawables);
}

void RenderPass::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object)
{
    SetName(common::strchset::to_u16string(object.Get<string>("Name"), Charset::UTF8));
}


DefaultRenderPass::DefaultRenderPass(const u16string& name, const string& source, const oglu::ShaderConfig& config)
    : GLShader(name, source, config)
{
    
}

bool DefaultRenderPass::OnRegistDrawable(const std::shared_ptr<Drawable>& drawable)
{
    drawable->PrepareGL(Program);
    return true;
}

void DefaultRenderPass::OnPrepare(RenderPassContext& context)
{
    const auto& scene = context.GetScene();
    Program->SetUniform("lightCount", scene->GetLightUBOCount());
    Program->State().SetUBO(scene->GetLightUBO(), "lightBlock");
}

void DefaultRenderPass::OnDraw(RenderPassContext& context)
{
    const auto cam = context.GetScene()->GetCamera();
    const auto fboTex = context.GetTexture("MainFBTex");
    const auto fbo = context.GetFrameBuffer("MainFB");
    const bool needNewCam = fboTex && fbo;
    const auto[scw, sch] = context.GetScreenSize();
    if (needNewCam)
    {
        const auto[w, h] = fboTex->GetSize();
        fbo->Use();
        GLContext->SetViewPort(0, 0, w, h);
    }
    else
    {
        oglu::oglFrameBuffer_::UseDefault();
    }
    // set camera data
    const auto projMat = cam->GetProjection(float(scw) / sch);
    const auto viewMat = cam->GetView();
    Program->SetMat("@ProjectMat", projMat);
    Program->SetMat("@ProjectMat", viewMat);
    Program->SetVec("@vecCamPos", cam->Position);
    {
        Drawable::Drawcall drawcall(Program, projMat, viewMat);
        for (const auto& d : Drawables)
        {
            const auto drw = d.lock();
            if (!drw || !drw->ShouldRender)
                continue;
            drw->Draw(drawcall);
            drawcall.Drawer.Restore(true);
        }
    }
    if (needNewCam)
    {
        GLContext->SetViewPort(0, 0, scw, sch);
    }

}

void DefaultRenderPass::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    RenderPass::Serialize(context, jself);
    GLShader::Serialize(context, jself);
}

void DefaultRenderPass::Deserialize(DeserializeUtil & context, const xziar::ejson::JObjectRef<true>& object)
{
    RenderPass::Deserialize(context, object);
    GLShader::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(DefaultRenderPass, u16string, string, oglu::ShaderConfig)
{
    return GLShader::DeserializeArg(context, object);
}


void RenderPipeLine::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"Shader名称")
        .RegistMember(&RenderPipeLine::Name);
}

RenderPipeLine::RenderPipeLine() : GLContext(oglu::oglContext_::CurrentContext())
{
}

void RenderPipeLine::Render(RenderPassContext context)
{
    for (auto& pass : Passes)
    {
        pass->Prepare(context);
    }
    oglu::oglFrameBuffer_::UseDefault();
    const auto [scw, sch] = context.GetScreenSize();
    GLContext->SetViewPort(0, 0, scw, sch);
    GLContext->SetSRGBFBO(true);
    for (auto& pass : Passes)
    {
        pass->Draw(context);
    }
}

}
