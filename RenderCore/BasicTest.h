#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"
#include "Model.h"

namespace rayr
{
using namespace common;
using namespace b3d;
using namespace oglu;
using miniBLAS::AlignBase;


class RAYCOREAPI alignas(32) BasicTest : public NonMovable, public NonCopyable, public AlignBase<>
{
private:
	oglProgram prog2D, prog3D;
	oglTexture picTex, mskTex, tmpTex;
	oglBuffer picBuf, screenBox, testTri;
	oglEBO triIdx;
	oglVAO picVAO, testVAO;
	vector<TransformOP> transf;
	vector<Wrapper<Drawable, false>> drawables;
	void init2d(const wstring pname);
	void init3d(const wstring pname);
	void initTex();
public:
	bool mode = true;
	Camera cam;
	Vec4 rvec, tvec;
	BasicTest(const wstring sname2d = L"", const wstring sname3d = L"");
	void draw();
	void resize(const int w, const int h);
	void moveobj(const float x, const float y, const float z);
	void rotateobj(const float x, const float y, const float z);
	void rfsData();
	uint16_t objectCount() const;
	void showObject(uint16_t objIdx) const;
};

}