#pragma once

#ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RAYCOREAPI _declspec(dllimport)
#endif



#include <cstdint>
#include <cstdio>
#include <string>
#include "../3DBasic/3dElement.hpp"
#include "../OpenGLUtil/oglUtil.h"


namespace rayr
{
using namespace common;
using namespace b3d;
using namespace oglu;
using miniBLAS::AlignBase;
using std::string;


class RAYCOREAPI alignas(16) BasicTest : public NonMovable, public NonCopyable, public AlignBase<>
{
private:
	oglProgram prog2D, prog3D;
	oglTexture picTex, mskTex;
	oglBuffer picBuf, screenBox, testTri;
	oglVAO picVAO, testVAO;
	//Vec4 rvec, tvec;
	vector_<TransformOP> transf;
	void init2d(const string pname);
	void init3d(const string pname);
	void initTex();
public:
	bool mode = true;
	Camera cam;
	BasicTest(const string sname2d = "", const string sname3d = "");
	void draw();
	void resize(const int w, const int h);
	void rfsData(const Vec4& rvec_, const Vec4& tvec_);
};

}