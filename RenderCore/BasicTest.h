#pragma once

#include "RenderCoreRely.h"
#include "Basic3DObject.h"

namespace rayr
{
using namespace common;
using namespace b3d;
using namespace oglu;
using miniBLAS::AlignBase;
using std::string;


class RAYCOREAPI alignas(32) BasicTest : public NonMovable, public NonCopyable, public AlignBase<>
{
private:
	oglProgram prog2D, prog3D;
	oglTexture picTex, mskTex;
	oglBuffer picBuf, screenBox, testTri, triIdx;
	oglVAO picVAO, testVAO;
	vector<TransformOP> transf;
	vector<Wrapper<Drawable, false>> drawables;
	void init2d(const string pname);
	void init3d(const string pname);
	void initTex();
public:
	bool mode = true;
	Camera cam;
	Vec4 rvec, tvec;
	BasicTest(const string sname2d = "", const string sname3d = "");
	void draw();
	void resize(const int w, const int h);
	void moveobj(const float x, const float y, const float z);
	void rotateobj(const float x, const float y, const float z);
	void rfsData();
};

}