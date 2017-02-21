#include "Basic3DObject.h"

namespace rayr
{



std::vector<uint16_t> Sphere::CreateSphere(std::vector<Point>& pts, const float radius, const uint16_t rings /*= 80*/, const uint16_t sectors /*= 80*/)
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
	std::vector<uint16_t> indexs;
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
	static DrawableHelper helper("Sphere");
	helper.InitDrawable(this);
	using std::vector;
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

}
