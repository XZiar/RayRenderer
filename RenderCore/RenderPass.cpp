#include "RenderCoreRely.h"
#include "RenderPass.h"
#include "SceneManager.h"


namespace rayr
{
using common::container::FindInSet;
using common::container::FindInMap;
using common::container::FindInMapOrDefault;
using common::container::FindInVec;
using common::container::ReplaceInVec;

RenderPassContext::RenderPassContext(const std::shared_ptr<Scene>& scene) : TheScene(scene)
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


RenderPass::RenderPass() : GLContext(oglu::oglContext::CurrentContext())
{

}

void RenderPass::RegistDrawable(const Wrapper<Drawable>& drawable)
{
    WaitAddDrawables.push_back(drawable);
}

void RenderPass::UnregistDrawable(const Wrapper<Drawable>& drawable)
{
    Drawables.erase(drawable);
    //WaitDelDrawables.push_back(drawable);
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
    uint16_t oldWidth = 0, oldHeight = 0;
    if (needNewCam)
    {
        const auto[w, h] = fboTex->GetSize();
        fbo->Use();
        GLContext->SetViewPort(0, 0, w, h);
        oldWidth = cam->Width, oldHeight = cam->Height;
        cam->Resize(w, h);
    }
    else
    {
        oglu::oglFBO::UseDefault();
    }
    Program->SetProject(cam->GetProjection());
    Program->SetView(cam->GetView());
    Program->SetVec("vecCamPos", cam->Position);
    {
        auto drawcall = Program->Draw();
        for (const auto& d : Drawables)
        {
            const auto drw = d.lock();
            if (!drw || !drw->ShouldRender)
                continue;
            drw->Draw(drawcall);
            drawcall.Restore(true);
        }
    }
    if (needNewCam)
    {
        cam->Resize(oldWidth, oldHeight);
        GLContext->SetViewPort(0, 0, oldWidth, oldHeight);
    }

}


RenderPipeLine::RenderPipeLine() : GLContext(oglu::oglContext::CurrentContext())
{
}

void RenderPipeLine::Render(const std::shared_ptr<Scene>& scene)
{
    RenderPassContext context(scene);
    for (auto& pass : Passes)
    {
        pass->Prepare(context);
    }
    for (auto& pass : Passes)
    {
        pass->Draw(context);
    }
}

}
