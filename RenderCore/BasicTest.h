#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Light.h"
#include "Material.h"
#include "FontHelper/FontHelper.h"

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

class RAYCOREAPI alignas(32) BasicTest final : public NonCopyable, public AlignBase<32>
{
private:
    oglContext glContext;
    oclContext clContext;
    oglProgram prog2D, prog3D, progPost;
    oglTex2DS picTex;
    oglTex2DV chkTex;
    oglTex2DS fboTex;
    oglPBO picBuf;
    oglVBO screenBox;
    oglVAO picVAO, ppVAO;
    oglUBO lightUBO;
    oglFBO MiddleFrame;
    vector<byte> LightBuf;
    Wrapper<FontViewer> fontViewer;
    Wrapper<FontCreator> fontCreator;
    vector<Wrapper<Drawable>> drawables;
    vector<Wrapper<Light>> lights;
    set<oglProgram> Prog3Ds;
    set<oglProgram> glProgs;
    fs::path Basepath;
    std::atomic_uint32_t IsUBOChanged{ 0 };
    uint32_t WindowWidth, WindowHeight;
    void init2d(const fs::path& shaderPath);
    void init3d(const fs::path& shaderPath);
    void initTex();
    void initUBO();
    void fontTest(const char32_t word = 0x554A);
    void prepareLight();
public:
    bool mode = true;
    Camera cam;
    BasicTest(const fs::path& shaderPath = u"");
    void Draw();
    void Resize(const uint32_t w, const uint32_t h, const bool changeWindow = true);
    void ResizeFBO(const uint32_t w, const uint32_t h, const bool isFloatDepth);
    void ReloadFontLoader(const u16string& fname);
    void ReloadFontLoaderAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadShaderAsync(const u16string& fname, const u16string& shdName, std::function<void(oglProgram)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    void LoadModelAsync(const u16string& fname, std::function<void(Wrapper<Model>)> onFinish, std::function<void(const BaseException&)> onError = nullptr);
    bool AddObject(const Wrapper<Drawable>& drawable);
    bool AddLight(const Wrapper<Light>& light);
    void DelAllLight();
    bool AddShader(const oglProgram& prog);
    void ChangeShader(const oglProgram& prog);
    void ReportChanged(const ChangableUBO target);
    xziar::img::Image Scrrenshot();

    const vector<Wrapper<Light>>& Lights() const { return lights; }
    const vector<Wrapper<Drawable>>& Objects() const { return drawables; }
    const set<oglProgram>& Shaders() const { return glProgs; }
    const oglProgram& Cur3DProg() const { return prog3D; }
    const oglContext& GetContext() const { return glContext; }

    void Serialize(const fs::path& fpath) const;
    void DeSerialize(const fs::path& fpath) const;
};

}