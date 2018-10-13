#pragma once

#include "RenderCoreRely.h"
#include "SceneManager.h"
#include "RenderPass.h"
#include "ThumbnailManager.h"
#include "PostProcessor.h"
#include "GLShader.h"

namespace oglu::texutil
{
class TexMipmap;
}

namespace rayr
{
using namespace oglu;
using namespace oclu;
using xziar::img::Image;



class RAYCOREAPI RenderCore
{
private:
    oglContext GLContext;
    oclContext CLContext, CLSharedContext;
    oclCmdQue CLQue;
    std::shared_ptr<texutil::TexUtilWorker> TexWorker;
    std::shared_ptr<texutil::TexMipmap> MipMapper;
    std::shared_ptr<detail::TextureLoader> TexLoader;
    Wrapper<detail::ThumbnailManager> ThumbMan;
    Wrapper<PostProcessor> PostProc;
    Wrapper<oglWorker> GLWorker;
    set<Wrapper<GLShader>> Shaders;
    Wrapper<Scene> TheScene;
    set<Wrapper<RenderPipeLine>> PipeLines;
    Wrapper<RenderPipeLine> RenderTask;
    uint32_t WindowWidth, WindowHeight;
    void RefreshContext() const;
    void InitShaders();
public:
    RenderCore();
    ~RenderCore();
    void TestSceneInit();
    void Draw();
    void Resize(const uint32_t w, const uint32_t h);

    const Wrapper<Scene>& GetScene() const { return TheScene; }

    void LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(Wrapper<GLShader>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void AddShader(const Wrapper<GLShader>& shader);
    void ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline);
};


}

