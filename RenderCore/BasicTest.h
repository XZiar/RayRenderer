#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Light.h"
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


class RAYCOREAPI alignas(32) BasicTest : public NonCopyable, public AlignBase<32>
{
private:
	oclContext clContext;
	oglProgram prog2D, prog3D;
	oglTexture picTex, mskTex, tmpTex;
	oglBuffer picBuf, screenBox;
	oglVAO picVAO;
	oglUBO lightUBO, materialUBO;
	uint8_t lightLim, materialLim;
	Wrapper<FontViewer> fontViewer;
	Wrapper<FontCreator> fontCreator;
	vector<Wrapper<Drawable>> drawables;
	vector<Wrapper<Light>> lights;
	fs::path basepath;
	void init2d(const u16string pname);
	void init3d(const u16string pname);
	void initTex();
	void initUBO();
	void fontTest(const char32_t word = 0x554A);
	void prepareLight();
	Wrapper<Model> _addModel(const u16string& fname);
public:
	bool mode = true;
	Camera cam;
	BasicTest(const u16string sname2d = u"", const u16string sname3d = u"");
	void Draw();
	void Resize(const int w, const int h);
	void ReloadFontLoader(const u16string& fname);
	void ReloadFontLoaderAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError = nullptr);
	bool AddModel(const u16string& fname);
	void AddModelAsync(const u16string& fname, CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError = nullptr);
	void AddLight(const b3d::LightType type);
	void DelAllLight();
	void Moveobj(const uint16_t id, const float x, const float y, const float z);
	void Rotateobj(const uint16_t id, const float x, const float y, const float z);
	void Movelgt(const uint16_t id, const float x, const float y, const float z);
	void Rotatelgt(const uint16_t id, const float x, const float y, const float z);
	const vector<Wrapper<Light>>& Lights() const { return lights; }
	const vector<Wrapper<Drawable>>& Objects() const { return drawables; }
	void showObject(uint16_t objIdx) const;
	void tryAsync(CallbackInvoke<bool> onFinish, std::function<void(BaseException&)> onError = nullptr) const;
};

}