#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Light.hpp"
#include "Material.hpp"
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
    oglProgram prog2D, prog3D;
    oglTexture picTex, mskTex, tmpTex;
    oglBuffer picBuf, screenBox;
    oglVAO picVAO;
    oglUBO lightUBO;
    uint32_t WindowWidth, WindowHeight;
    uint8_t lightLim, materialLim;
    Wrapper<FontViewer> fontViewer;
    Wrapper<FontCreator> fontCreator;
    vector<Wrapper<Drawable>> drawables;
    vector<Wrapper<Light>> lights;
    set<oglProgram> Prog3Ds;
    set<oglProgram> glProgs;
    std::atomic_uint32_t IsUBOChanged = 0;
    fs::path basepath;
    void init2d(const u16string pname);
    void init3d(const u16string pname);
    void initTex();
    void initUBO();
    void fontTest(const char32_t word = 0x554A);
    void prepareLight();
public:
    bool mode = true;
    Camera cam;
    BasicTest(const u16string sname2d = u"", const u16string sname3d = u"");
    void Draw();
    void Resize(const int w, const int h);
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
    const vector<Wrapper<Light>>& Lights() const { return lights; }
    const vector<Wrapper<Drawable>>& Objects() const { return drawables; }
    const set<oglProgram>& Shaders() const { return glProgs; }
    const oglContext& GetContext() const { return glContext; }
};

}