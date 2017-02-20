#pragma once

#include "RenderCoreRely.h"
#include <vector>

namespace rayr
{
using std::string;
using miniBLAS::vector;
using namespace common;
using namespace b3d;

class alignas(16) Material : public AlignBase<>
{
public:
	Vec4 ambient, diffuse, specular, emission;
	float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
	string name;
};


class Drawable;
class DrawableHelper
{
private:
	static vector<string> typeMap;
public:
	uint32_t id;
	DrawableHelper(string name);
	void InitDrawable(Drawable *d);
	static string getType(const Drawable& d);
};

class alignas(16) Drawable : public AlignBase<>, public NonCopyable
{
	friend class DrawableHelper;
public:
	Vec4 position = Vec4::zero(), rotation = Vec4::zero(), scale = Vec4::one();
	oglu::oglVAO vao;
	string name;
	virtual ~Drawable() { };
	/*prepare VAO with given Vertex Attribute Location
	 *-param: VertPos,VertNorm,TexPos
	 */
	virtual void prepareGL(const GLuint(&AttrLoc)[3]) = 0;
protected:
	uint32_t drawableID;
};

}
