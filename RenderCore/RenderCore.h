#pragma once

#include "RenderCoreRely.h"


namespace dizz
{
class ThumbnailManager;
class TextureLoader;
class PostProcessor;
class RenderPipeLine;
class DefaultRenderPass;
class RenderPass;
class Scene;
class Model;


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI RenderCore final : public common::NonCopyable
{
private:
    oglu::oglContext GLContext;
    oclu::oclContext CLContext, CLSharedContext;
    oclu::oclCmdQue CLQue;
    std::shared_ptr<oglu::texutil::TexUtilWorker> TexWorker;
    std::shared_ptr<oglu::texutil::TexMipmap> MipMapper;
    std::shared_ptr<TextureLoader> TexLoader;
    std::shared_ptr<ThumbnailManager> ThumbMan;
    std::shared_ptr<PostProcessor> PostProc;
    std::shared_ptr<oglu::oglWorker> GLWorker;
    std::set<std::shared_ptr<RenderPass>> RenderPasses;
    std::shared_ptr<Scene> TheScene;
    std::set<std::shared_ptr<RenderPipeLine>> PipeLines;
    std::shared_ptr<RenderPipeLine> RenderTask;
    uint16_t WindowWidth, WindowHeight;
    void RefreshContext() const;
    void InitShaders();
public:
    RenderCore(const oglu::GLContextInfo& info);
    ~RenderCore();
    void TestSceneInit();
    void Draw();
    void Resize(const uint32_t w, const uint32_t h);
    std::pair<uint16_t, uint16_t> GetWindowSize() const noexcept { return { WindowWidth,WindowHeight }; }

    const std::shared_ptr<Scene>& GetScene() const noexcept { return TheScene; }
    const std::shared_ptr<RenderPipeLine>& GetCurPipeLine() const noexcept { return RenderTask; }
    const std::shared_ptr<PostProcessor>& GetPostProc() const noexcept { return PostProc; }
    const std::shared_ptr<TextureLoader>& GetTexLoader() const noexcept { return TexLoader; }
    const std::shared_ptr<ThumbnailManager>& GetThumbMan() const noexcept { return ThumbMan; }
    const std::set<std::shared_ptr<RenderPass>>& GetRenderPasses() const noexcept { return RenderPasses; }
    const std::set<std::shared_ptr<RenderPipeLine>>& GetPipeLines() const noexcept { return PipeLines; }
    std::vector<std::shared_ptr<common::Controllable>> GetControllables() const noexcept;

    void LoadModelAsync(const u16string& fname, std::function<void(std::shared_ptr<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr) const;
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(std::shared_ptr<DefaultRenderPass>)> onFinish, std::function<void(const BaseException&)> onError = nullptr) const;
    common::PromiseResult<std::shared_ptr<Model>> LoadModelAsync2(const u16string& fname) const;
    common::PromiseResult<std::shared_ptr<DefaultRenderPass>> LoadShaderAsync2(const u16string& fname, const u16string& shdName) const;
    bool AddShader(const std::shared_ptr<DefaultRenderPass>& shader);
    bool DelShader(const std::shared_ptr<DefaultRenderPass>& shader);
    void ChangePipeLine(const std::shared_ptr<RenderPipeLine>& pipeline);

    void Serialize(const common::fs::path& fpath) const;
    void DeSerialize(const common::fs::path& fpath);
    xziar::img::Image Screenshot();
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}

