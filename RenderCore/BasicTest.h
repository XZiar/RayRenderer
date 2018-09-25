#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Light.h"
#include "Camera.h"
#include "Material.h"
#include "FontHelper/FontHelper.h"
#include "ThumbnailManager.h"
#include "PostProcessor.h"
#include "GLShader.h"

namespace rayr
{
using namespace common;
using namespace b3d;
using namespace oglu;
using namespace oclu;
namespace img = xziar::img;
using xziar::img::Image;
using xziar::img::ImageDataType;

enum class ChangableUBO : uint32_t { Light = 0x1, Material = 0x2 };
MAKE_ENUM_BITFIELD(ChangableUBO)

class RAYCOREAPI BasicTest final : public NonCopyable, public AlignBase<32>
{
private:
    oglContext glContext;
    oclContext ClContext, ClSharedContext;
    oclCmdQue ClQue;
    oglDrawProgram prog2D, prog3D, progPost;
    Wrapper<GLShader> Prog2D, Prog3D, Prog3DPBR, ProgPost;
    oglTex2DS picTex;
    oglTex2DV chkTex;
    oglTex2DS fboTex;
    oglPBO picBuf;
    oglVBO screenBox;
    oglVAO picVAO, ppVAO;
    oglUBO lightUBO;
    oglFBO MiddleFrame;
    vector<byte> LightBuf;
    Wrapper<detail::ThumbnailManager> ThumbMan;
    Wrapper<PostProcessor> PostProc;
    Wrapper<oglu::oglWorker> GLWorker;
    Wrapper<FontViewer> fontViewer;
    Wrapper<FontCreator> fontCreator;
    vector<Wrapper<Drawable>> drawables;
    vector<Wrapper<Light>> lights;
    set<oglDrawProgram> Prog3Ds;
    set<Wrapper<GLShader>> glProgs;
    set<oglDrawProgram> glProgs2;
    fs::path Basepath;
    std::atomic_uint32_t IsUBOChanged{ 0 };
    uint32_t WindowWidth, WindowHeight;
    void init2d(const fs::path& shaderPath);
    void init3d(const fs::path& shaderPath);
    void initTex();
    void initUBO();
    void fontTest(const char32_t word = 0x554A);
    void prepareLight();
    void RefreshContext() const;
public:
    bool mode = true;
    Camera cam;
    BasicTest(const fs::path& shaderPath = u"");
    void Draw();
    void Resize(const uint32_t w, const uint32_t h, const bool changeWindow = true);
    void ResizeFBO(const uint32_t w, const uint32_t h, const bool isFloatDepth);
    void ReloadFontLoader(const u16string& fname);
    void ReloadFontLoaderAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(oglDrawProgram)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    bool AddObject(const Wrapper<Drawable>& drawable);
    bool AddLight(const Wrapper<Light>& light);
    void DelAllLight();
    bool AddShader(const oglDrawProgram& prog);
    void ChangeShader(const oglDrawProgram& prog);
    void ReportChanged(const ChangableUBO target);
    xziar::img::Image Screenshot();

    const vector<Wrapper<Light>>& Lights() const { return lights; }
    const vector<Wrapper<Drawable>>& Objects() const { return drawables; }
    const set<Wrapper<GLShader>>& GLShaders() const { return glProgs; }
    const set<oglDrawProgram>& Shaders() const { return glProgs2; }
    const oglDrawProgram& Cur3DProg() const { return prog3D; }
    const oglContext& GetContext() const { return glContext; }
    const detail::ThumbnailManager& GetThumbMan() const { return *ThumbMan; }
    const Wrapper<PostProcessor>& GetPostProc() const { return PostProc; };
    const Wrapper<FontViewer>& GetFontViewer() const { return fontViewer; };

    void Serialize(const fs::path& fpath) const;
    void DeSerialize(const fs::path& fpath);
};

}