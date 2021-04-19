#pragma once
#include "RenderCoreRely.h"
#include "GLShader.h"
#include "RenderElement.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
namespace rayr
{

class Scene;

class RenderPassContext
{
    friend class RenderPipeLine;
private:
    std::shared_ptr<Scene> TheScene;
    std::map<string, oglu::oglTex2D, std::less<>> Textures;
    std::map<string, oglu::oglFBO, std::less<>> FrameBufs;
    std::pair<uint16_t, uint16_t> ScreenSize;
public:
    RenderPassContext(const std::shared_ptr<Scene>& scene, const uint16_t ScreenWidth, const uint16_t ScreenHeight);
    const std::shared_ptr<Scene>& GetScene() const;
    std::pair<uint16_t, uint16_t> GetScreenSize() const { return ScreenSize; };
    oglu::oglFBO GetFrameBuffer(const string_view& name) const;
    void SetFrameBuffer(const string_view& name, const oglu::oglFBO& fbo);
    oglu::oglFBO GetOrCreateFrameBuffer(const string_view& name, const std::function<oglu::oglFBO(void)>& creator);
    oglu::oglTex2D GetTexture(const string_view& name) const;
    void SetTexture(const string_view& name, const oglu::oglTex2D& tex);
};

class RAYCOREAPI RenderPass : public virtual common::Controllable, public virtual xziar::respak::Serializable
{
private:
    std::vector<std::weak_ptr<Drawable>> WaitAddDrawables;
    std::vector<std::weak_ptr<Drawable>> WaitDelDrawables;
protected:
    const oglu::oglContext GLContext;
    std::set<std::weak_ptr<Drawable>, std::owner_less<>> Drawables;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPass"sv;
    }
    virtual bool OnRegistDrawable(const std::shared_ptr<Drawable>&) { return false; }
    // perform non-render preparation
    virtual void OnPrepare(RenderPassContext&) {}
    // do actual rendering
    virtual void OnDraw(RenderPassContext&) {}
public:
    virtual u16string GetName() const { return u""; }
    virtual void SetName(const u16string&) { }
    RenderPass();
    virtual ~RenderPass() {}
    void RegistDrawable(const std::shared_ptr<Drawable>& drawable);
    void UnregistDrawable(const std::shared_ptr<Drawable>& drawable);
    void CleanDrawable();
    void Prepare(RenderPassContext& context);
    void Draw(RenderPassContext& context);

    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};

class RAYCOREAPI DefaultRenderPass : public RenderPass, public GLShader
{
protected:
    virtual bool OnRegistDrawable(const std::shared_ptr<Drawable>& drawable) override;
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"DefaultRenderPass"sv;
    }
    virtual u16string GetName() const override { return Program->Name; }
    virtual void SetName(const u16string& name) override { Program->Name = name; }
public:
    DefaultRenderPass(const u16string& name, const string& source, const oglu::ShaderConfig& config = {});
    ~DefaultRenderPass() {}
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#DefaultRenderPass")
};

class RAYCOREAPI RenderPipeLine : public common::Controllable
{
protected:
    const oglu::oglContext GLContext;
    u16string Name;
    void RegistControllable();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPipeLine"sv;
    }
public:
    std::vector<std::shared_ptr<RenderPass>> Passes;
    RenderPipeLine();
    virtual ~RenderPipeLine() {}
    virtual void Prepare() {}
    void Render(RenderPassContext context);
};

}
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
