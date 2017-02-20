#pragma once

#include "RenderElement.h"

namespace rayr
{

class alignas(16) Sphere : public Drawable
{
protected:
	float radius, radius_sqr;
	oglu::oglBuffer vbo, ebo;
	static std::vector<uint16_t> CreateSphere(std::vector<Point>& pts, const float radius, const uint16_t rings = 31, const uint16_t sectors = 31);
public:
	Sphere(const float r);
	virtual void prepareGL(const GLuint(&AttrLoc)[3]) override;
};

}
