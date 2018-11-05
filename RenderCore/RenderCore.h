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
    std::shared_ptr<TextureLoader> TexLoader;
    Wrapper<ThumbnailManager> ThumbMan;
    Wrapper<PostProcessor> PostProc;
    std::shared_ptr<oglWorker> GLWorker;
    set<Wrapper<RenderPass>> RenderPasses;
    Wrapper<Scene> TheScene;
    set<Wrapper<RenderPipeLine>> PipeLines;
    Wrapper<RenderPipeLine> RenderTask;
    uint16_t WindowWidth, WindowHeight;
    void RefreshContext() const;
    void InitShaders();
public:
    RenderCore();
    ~RenderCore();
    void TestSceneInit();
    void Draw();
    void Resize(const uint32_t w, const uint32_t h);
    std::pair<uint16_t, uint16_t> GetWindowSize() const noexcept { return { WindowWidth,WindowHeight }; }

    const Wrapper<Scene>& GetScene() const noexcept { return TheScene; }
    const Wrapper<RenderPipeLine>& GetCurPipeLine() const noexcept { return RenderTask; }
    const Wrapper<PostProcessor>& GetPostProc() const noexcept { return PostProc; }
    const Wrapper<ThumbnailManager>& GetThumbMan() const noexcept { return ThumbMan; }
    const set<Wrapper<RenderPass>>& GetRenderPasses() const noexcept { return RenderPasses; }
    const set<Wrapper<RenderPipeLine>>& GetPipeLines() const noexcept { return PipeLines; }

    void LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr) const;
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(Wrapper<DefaultRenderPass>)> onFinish, std::function<void(const BaseException&)> onError = nullptr) const;
    bool AddShader(const Wrapper<DefaultRenderPass>& shader);
    bool DelShader(const Wrapper<DefaultRenderPass>& shader);
    void ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline);

    void Serialize(const fs::path& fpath) const;
    void DeSerialize(const fs::path& fpath);
    xziar::img::Image Screenshot();
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}

