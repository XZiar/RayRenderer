#include "Basic3DObject.h"

namespace rayr
{



vector<uint16_t> Sphere::CreateSphere(vector<Point>& pts, const float radius, const uint16_t rings /*= 80*/, const uint16_t sectors /*= 80*/)
{
	const float rstep = 1.0f / (rings - 1);
	const float sstep = 1.0f / (sectors - 1);
	pts.clear();
	pts.reserve(rings*sectors);
	uint16_t rcnt = rings, scnt = sectors;
	for (float r = 0; rcnt--; r += rstep)
	{
		scnt = sectors;
		for (float s = 0; scnt--; s += sstep)
		{
			const auto x = cos(2 * M_PI * s) * sin(M_PI * r);
			const auto y = sin(M_PI * r - M_PI / 2);
			const auto z = sin(2 * M_PI * s) * sin(M_PI * r);
			Normal norm(x, y, z);
			Coord2D texc(s, r);
			Point pt(norm * radius, norm, texc);
			pts.push_back(pt);
		}
	}
	vector<uint16_t> indexs;
	indexs.reserve(rings*sectors * 6);
	for (uint16_t r = 0; r < rings - 1; r++)
	{
		for (uint16_t s = 0; s < sectors - 1; s++)
		{
			const auto idx0 = r * sectors + s;
			indexs.push_back(idx0);
			indexs.push_back(idx0 + 1);
			indexs.push_back(idx0 + sectors);
			indexs.push_back(idx0 + 1);
			indexs.push_back(idx0 + sectors + 1);
			indexs.push_back(idx0 + sectors);
		}
	}
	return indexs;
}

Sphere::Sphere(const float r) : radius(r), radius_sqr(r*r)
{
	static DrawableHelper helper(L"Sphere");
	helper.InitDrawable(this);
	vector<Point> pts;
	auto indexs = CreateSphere(pts, radius);
	vbo.reset(oglu::BufferType::Array);
	vbo->write(pts.data(), pts.size() * sizeof(Point));
	ebo.reset(oglu::BufferType::Element);
	ebo->write(indexs.data(), indexs.size() * sizeof(uint16_t));
	vao.reset(oglu::VAODrawMode::Triangles);
	vao->setDrawSize(0, (uint16_t)indexs.size());
}

void Sphere::prepareGL(const GLuint(&attrLoc)[3])
{
	vao->prepare().set(vbo, attrLoc, 0)
		.setIndex(ebo, oglu::IndexSize::Short)//index draw
		.end();
}


/*
 *      v4-------v7
 *      /:       /|
 *     / :      / |
 *   v0--:----v3  |
 *    | v5-----|-v6
 *    | /      | /
 *    |/       |/
 *   v1-------v2
 *
 **/
const Point Box::basePts[] = 
{ 
	{ { +0.5f,+0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
	{ { +0.5f,-0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },//v2
	{ { +0.5f,-0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
	{ { +0.5f,+0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
	{ { +0.5f,-0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
	{ { +0.5f,+0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 0.0f } },//v7

	{ { -0.5f,+0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
	{ { -0.5f,-0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },//v5
	{ { -0.5f,-0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
	{ { -0.5f,+0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
	{ { -0.5f,-0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
	{ { -0.5f,+0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } },//v0

	{ { -0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
	{ { -0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 0.0f, 2.0f } },//v0
	{ { +0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
	{ { -0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
	{ { +0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
	{ { +0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 1.0f, 2.0f } },//v7

	{ { -0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
	{ { -0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 3.0f } },//v5
	{ { +0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
	{ { -0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
	{ { +0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
	{ { +0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 1.0f, 3.0f } },//v2

	{ { -0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
	{ { -0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 0.0f, 4.0f } },//v1
	{ { +0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
	{ { -0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
	{ { +0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
	{ { +0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 1.0f, 4.0f } },//v3

	{ { +0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
	{ { +0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 0.0f, 5.0f } },//v6
	{ { -0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
	{ { +0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
	{ { -0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
	{ { -0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 1.0f, 5.0f } },//v4
};

Box::Box(const float length, const float height, const float width)
{
	static DrawableHelper helper(L"Box");
	helper.InitDrawable(this);
	size = Vec3(length, height, width);
	vector<Point> pts;
	pts.assign(basePts, basePts + 36);
	for (auto& pt : pts)
		pt.pos *= size;
	vbo.reset(oglu::BufferType::Array);
	vbo->write(pts.data(), pts.size() * sizeof(Point));
	vao.reset(oglu::VAODrawMode::Triangles);
	vao->setDrawSize(0, 36);
}

void Box::prepareGL(const GLuint(&attrLoc)[3])
{
	vao->prepare().set(vbo, attrLoc[0], sizeof(Point), 3, 0)
		.set(vbo, attrLoc[1], sizeof(Point), 3, 16)
		.set(vbo, attrLoc[2], sizeof(Point), 3, 32)
		.end();
}


Plane::Plane(const float len, const float texRepeat)
{
	static DrawableHelper helper(L"Plane");
	helper.InitDrawable(this);
	vbo.reset(oglu::BufferType::Array);
	const Point pts[] =
	{
		{ { -len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, texRepeat } },//v4
		{ { -len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 0.0f } },//v0
		{ { +len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, 0.0f } },//v3
		{ { -len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, texRepeat } },//v4
		{ { +len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, 0.0f } },//v3
		{ { +len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, texRepeat } },//v7
	};
	vbo->write(pts, sizeof(pts));
	vao.reset(oglu::VAODrawMode::Triangles);
	vao->setDrawSize(0, 6);
}

void Plane::prepareGL(const GLuint(&attrLoc)[3])
{
	vao->prepare().set(vbo, attrLoc, 0).end();
}

}
