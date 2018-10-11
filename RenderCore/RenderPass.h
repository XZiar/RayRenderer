#pragma once
#include "RenderCoreRely.h"
#include "GLShader.h"
#include "RenderElement.h"
#include "SceneManager.h"

namespace rayr
{

class RenderPass : public Controllable
{
private:
    u16string Name;
    std::weak_ptr<Scene> TheScene;
protected:
    std::set<std::weak_ptr<Drawable>> Drawables;
    std::shared_ptr<Scene> GetScene() const;
public:
    RenderPass();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"RenderPass"sv;
    }
    virtual ~RenderPass() {}
    virtual std::shared_ptr<Controllable> GetMainControl() const { return {}; }
    virtual std::shared_ptr<common::Controllable> GetMainControl() { return {}; };
    virtual void OnDraw() {}
    void RegisterDrawable(std::weak_ptr<Drawable> drawable);
};

class RenderPipeLine
{
private:
    vector<RenderPass> Passes;
public:

};

}

