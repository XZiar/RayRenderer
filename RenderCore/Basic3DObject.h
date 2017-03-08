#pragma once

#include "RenderElement.h"

namespace rayr
{

class alignas(16) Sphere : public Drawable
{
protected:
	float radius, radius_sqr;
	oglu::oglBuffer vbo;
	oglu::oglEBO ebo;
	uint32_t ptcount;
	static vector<uint16_t> CreateSphere(vector<Point>& pts, const float radius, const uint16_t rings = 31, const uint16_t sectors = 31);
public:
	Sphere(const float r);
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
};


class alignas(16) Box : public Drawable
{
protected:
	static const Point basePts[36];
	Vec3 size;
	oglu::oglBuffer vbo;
public:
	Box(const float len) : Box(len, len, len) { };
	Box(const float length, const float height, const float width);
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
};


class alignas(16) Plane : public Drawable
{
protected:
	oglu::oglBuffer vbo;
public:
	Plane(const float len = 500.0f, const float texRepeat = 1.0f);
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
};

}
