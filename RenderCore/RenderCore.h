#pragma once

#include "RenderCoreRely.h"
#include "SceneManager.h"
#include "RenderPass.h"

namespace rayr
{

class PBRBasicPass : public DefaultRenderPass
{
private:
    std::shared_ptr<GLShader> PBRShader;
protected:
public:
    PBRBasicPass();
    ~PBRBasicPass() {}
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;
};

class RenderCore
{
private:
    Scene TheScene;
public:
};


}

