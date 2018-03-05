#include "RenderCoreRely.h"
#include "RenderElement.h"
#include <mutex>

namespace rayr
{


class DrawableHelper
{
	friend class Drawable;
private:
	std::mutex mtx;
	boost::bimap<boost::bimaps::set_of<u16string>, boost::bimaps::set_of<size_t>> typeMap;
	size_t regist(const u16string& name)
	{
		std::lock_guard<std::mutex> locker(mtx);
		const size_t id = typeMap.size();
		auto ret = typeMap.insert({ name,id });
		if (ret.second)
		{
			basLog().debug(u"Regist Drawable [{}] -> {}\n", name, id);
			return id;
		}
		else
			return (*ret.first).right;
	}
    u16string getType(const size_t id) const
	{
		auto it = typeMap.right.find(id);
		if (it == typeMap.right.end())
			return u"";
		else
			return (*it).second;
	}
};


//Drawable::VAOMap Drawable::vaoMap;

rayr::DrawableHelper& Drawable::getHelper()
{
	static DrawableHelper helper;
	return helper;
}

Drawable::Drawable(const u16string& typeName)
{
	drawableID = (uint32_t)getHelper().regist(typeName);
}

Drawable::~Drawable()
{
	auto& key = vaoMap.get<1>();
	const auto its = key.equal_range(this);
	key.erase(its.first, its.second);
}

void Drawable::draw(oglu::oglProgram& prog) const
{
	drawPosition(prog).draw(getVAO(prog)).end();
}

u16string Drawable::getType() const
{
	return getHelper().getType(drawableID);
}

void Drawable::releaseAll(const oglu::oglProgram& prog)
{
	const auto its = vaoMap.equal_range(prog.weakRef());
	vaoMap.erase(its.first, its.second);
}

auto Drawable::defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare())
{
	const GLint attrs[3] = { prog->Attr_Vert_Pos, prog->Attr_Vert_Norm, prog->Attr_Vert_Texc };
	return std::move(vao->prepare().set(vbo, attrs, 0));
}

auto Drawable::drawPosition(oglu::oglProgram & prog) const -> decltype(prog -> draw())
{
	Mat3x3 matNormal = Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, rotation.z)) *
		Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, rotation.y)) *
		Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, rotation.x));
	return prog->draw(Mat4x4::TranslateMat(position) * Mat4x4(matNormal * Mat3x3::ScaleMat(scale)), matNormal);
}

void Drawable::setVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const
{
	vaoMap.insert({ prog.weakRef(),this,vao });
}

const oglu::oglVAO& Drawable::getVAO(const oglu::oglProgram& prog) const
{
	const auto& it = vaoMap.find(std::make_tuple(prog.weakRef(), this));
	if (it == vaoMap.end())
		return defaultVAO;
	else
		return it->vao;
}


}
