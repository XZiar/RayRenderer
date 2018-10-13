#pragma once
#include "RenderCoreRely.h"
#include "GLShader.h"
#include "RenderElement.h"

namespace rayr
{

class Scene;

class RenderPassContext
{
    friend class RenderPipeLine;
private:
    std::shared_ptr<Scene> TheScene;
    map<string, oglu::oglTex2D, std::less<>> Textures;
    map<string, oglu::oglFBO, std::less<>> FrameBufs;
public:
    RenderPassContext(const std::shared_ptr<Scene>& scene);
    const std::shared_ptr<Scene>& GetScene() const;
    oglu::oglFBO GetFrameBuffer(const string_view& name) const;
    void SetFrameBuffer(const string_view& name, const oglu::oglFBO& fbo);
    oglu::oglFBO GetOrCreateFrameBuffer(const string_view& name, const std::function<oglu::oglFBO(void)>& creator);
    oglu::oglTex2D GetTexture(const string_view& name) const;
    void SetTexture(const string_view& name, const oglu::oglTex2D& tex);
};

class RenderPass : public virtual Controllable
{
private:
    vector<std::weak_ptr<Drawable>> WaitAddDrawables;
    vector<std::weak_ptr<Drawable>> WaitDelDrawables;
protected:
    const oglu::oglContext GLContext;
    std::set<std::weak_ptr<Drawable>, std::owner_less<>> Drawables;
    virtual bool OnRegistDrawable(const std::shared_ptr<Drawable>& drawable) { return false; }
    // perform non-render preparation
    virtual void OnPrepare(RenderPassContext& context) {}
    // do actual rendering
    virtual void OnDraw(RenderPassContext& context) {}
public:
    u16string Name;
    RenderPass();
    virtual ~RenderPass() {}
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPass"sv;
    }
    void RegistDrawable(const Wrapper<Drawable>& drawable);
    void UnregistDrawable(const Wrapper<Drawable>& drawable);
    void Prepare(RenderPassContext& context);
    void Draw(RenderPassContext& context);
};

class DefaultRenderPass : public RenderPass, public GLShader
{
protected:
    virtual bool OnRegistDrawable(const std::shared_ptr<Drawable>& drawable) override;
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;
public:
    DefaultRenderPass(const u16string& name, const string& source, const oglu::ShaderConfig& config = {});
    ~DefaultRenderPass() {}
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"DefaultRenderPass"sv;
    }
};

class RenderPipeLine : public Controllable
{
protected:
    const oglu::oglContext GLContext;
public:
    vector<std::shared_ptr<RenderPass>> Passes;
    RenderPipeLine();
    virtual ~RenderPipeLine() {}
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPipeLine"sv;
    }
    virtual void Prepare() {}
    void Render(const std::shared_ptr<Scene>& scene);
};

}

