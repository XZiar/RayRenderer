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

RenderPassContext::RenderPassContext()
{
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

std::shared_ptr<Scene> rayr::RenderPass::GetScene() const
{
    return TheScene.lock();
}

void RenderPass::RegisterDrawable(std::weak_ptr<Drawable> drawable)
{
    Drawables.insert(std::move(drawable));
}


DefaultRenderPass::DefaultRenderPass(std::shared_ptr<GLShader> shader) : Shader(shader)
{
    
}

void DefaultRenderPass::OnPrepare(RenderPassContext& context)
{
}

void DefaultRenderPass::OnDraw(RenderPassContext& context)
{
    const auto cam = GetScene()->GetCamera();
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
    Shader->Program->SetProject(cam->GetProjection());
    Shader->Program->SetView(cam->GetView());
    Shader->Program->SetVec("vecCamPos", cam->Position);
    {
        auto drawcall = Shader->Program->Draw();
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

void RenderPipeLine::Render(const oglu::oglContext& glContext)
{
    RenderPassContext context;
    for (auto& pass : Passes)
    {
        pass.OnPrepare(context);
    }
    for (auto& pass : Passes)
    {
        pass.OnDraw(context);
    }
}

}
