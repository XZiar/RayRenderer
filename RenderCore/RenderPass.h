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
    map<string, oglu::oglTex2D, std::less<>> Textures;
    map<string, oglu::oglFBO, std::less<>> FrameBufs;
public:
    RenderPassContext();
    oglu::oglFBO GetFrameBuffer(const string_view& name) const;
    void SetFrameBuffer(const string_view& name, const oglu::oglFBO& fbo);
    oglu::oglFBO GetOrCreateFrameBuffer(const string_view& name, const std::function<oglu::oglFBO(void)>& creator);
    oglu::oglTex2D GetTexture(const string_view& name) const;
    void SetTexture(const string_view& name, const oglu::oglTex2D& tex);
};

class RenderPass : public Controllable
{
private:
    std::weak_ptr<Scene> TheScene;
protected:
    const oglu::oglContext GLContext;
    std::set<std::weak_ptr<Drawable>, std::owner_less<>> Drawables;
    std::shared_ptr<Scene> GetScene() const;
public:
    u16string Name;
    RenderPass();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPass"sv;
    }
    virtual ~RenderPass() {}
    // perform non-render preparation
    virtual void OnPrepare(RenderPassContext& context) {}
    // do actual rendering
    virtual void OnDraw(RenderPassContext& context) {}
    virtual std::shared_ptr<Controllable> GetMainControl() const { return {}; }
    void RegisterDrawable(std::weak_ptr<Drawable> drawable);
};

class DefaultRenderPass : public RenderPass
{
protected:
    std::shared_ptr<GLShader> Shader;
public:
    DefaultRenderPass(std::shared_ptr<GLShader> shader = {});
    ~DefaultRenderPass() {}
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;

    virtual std::shared_ptr<Controllable> GetMainControl() const override 
    { 
        return std::static_pointer_cast<Controllable>(Shader);
    }
};

class RenderPipeLine : public Controllable
{
protected:
    const oglu::oglContext GLContext;
    vector<RenderPass> Passes;
public:
    RenderPipeLine();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPipeLine"sv;
    }
    virtual ~RenderPipeLine() {}
    virtual void Prepare() {}
    void Render(const oglu::oglContext& glContext);
};

}

