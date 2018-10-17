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


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class RAYCOREAPI RenderCore final : public NonCopyable
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
    std::shared_ptr<oglWorker> GLWorker;
    set<Wrapper<RenderPass>> RenderPasses;
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
    const Wrapper<RenderPipeLine>& GetCurPipeLine() const { return RenderTask; }
    const Wrapper<PostProcessor>& GetPostProc() const { return PostProc; }
    const Wrapper<detail::ThumbnailManager>& GetThumbMan() const { return ThumbMan; }
    const set<Wrapper<RenderPass>>& GetRenderPasses() const { return RenderPasses; }
    const set<Wrapper<RenderPipeLine>>& GetPipeLines() const { return PipeLines; }

    void LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(Wrapper<DefaultRenderPass>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void AddShader(const Wrapper<DefaultRenderPass>& shader);
    void ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline);

    void Serialize(const fs::path& fpath) const;
    void DeSerialize(const fs::path& fpath);
    xziar::img::Image Screenshot();
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}

