#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"
#include "Light.h"

namespace rayr
{
using namespace common;
using namespace b3d;
using namespace oglu;
using namespace oclu;
using miniBLAS::AlignBase;


class RAYCOREAPI alignas(32) BasicTest : public NonMovable, public NonCopyable, public AlignBase<>
{
private:
	oclContext clContext;
	oglProgram prog2D, prog3D;
	oglTexture picTex, mskTex, tmpTex;
	oglBuffer picBuf, screenBox;
	oglVAO picVAO;
	oglUBO lightUBO, materialUBO;
	vector<TransformOP> transf;
	vector<Wrapper<Drawable>> drawables;
	vector<Wrapper<Light>> lights;
	void init2d(const wstring pname);
	void init3d(const wstring pname);
	void initTex();
	void initUBO();
	void prepareUBO();
	Wrapper<Model> _addModel(const wstring& fname);
public:
	bool mode = true;
	Camera cam;
	BasicTest(const wstring sname2d = L"", const wstring sname3d = L"");
	void draw();
	void resize(const int w, const int h);
	bool addModel(const wstring& fname);
	void addModelAsync(const wstring& fname, std::function<void(std::function<bool(void)>)> onFinish);
	void addLight(const b3d::LightType type);
	void moveobj(const uint16_t id, const float x, const float y, const float z);
	void rotateobj(const uint16_t id, const float x, const float y, const float z);
	void movelgt(const uint16_t id, const float x, const float y, const float z);
	void rotatelgt(const uint16_t id, const float x, const float y, const float z);
	uint16_t lightCount() const;
	const vector<Wrapper<Light>>& light() const { return lights; }
	uint16_t objectCount() const;
	const vector<Wrapper<Drawable>>& object() const { return drawables; }
	void showObject(uint16_t objIdx) const;
};

}