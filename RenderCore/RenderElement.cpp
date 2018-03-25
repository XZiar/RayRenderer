#include "RenderCoreRely.h"
#include "RenderElement.h"

namespace rayr
{


class DrawableHelper
{
	friend class Drawable;
private:
    common::RWSpinLock Locker;
	boost::bimap<boost::bimaps::set_of<u16string>, boost::bimaps::set_of<size_t>> typeMap;
	size_t regist(const u16string& name)
	{
        Locker.LockWrite();
		const size_t id = typeMap.size();
		auto ret = typeMap.insert({ name,id });
		if (ret.second)
		{
			basLog().debug(u"Regist Drawable [{}] -> {}\n", name, id);
            Locker.UnlockWrite();
            return id;
		}
        Locker.UnlockWrite();
        return (*ret.first).right;
	}
    u16string getType(const size_t id)
	{
        Locker.LockRead();
        auto it = typeMap.right.find(id);
        const auto name = (it == typeMap.right.end()) ? u"" : (*it).second;
        Locker.UnlockRead();
        return name;
	}
};
static DrawableHelper DRAWABLE_HELPER;


struct VAOPack
{
    const oglu::oglProgram::weak_type prog;
    const Drawable *drawable;
    oglu::oglVAO vao;
};

using VAOKey = boost::multi_index::composite_key<VAOPack,
    boost::multi_index::member<VAOPack, const oglu::oglProgram::weak_type, &VAOPack::prog>,
    boost::multi_index::member<VAOPack, const Drawable*, &VAOPack::drawable>
>;
using VAOKeyComp = boost::multi_index::composite_key_compare<
    WeakPtrComparerator<const oglu::oglProgram::weak_type>, std::less<const Drawable*>
>;
using VAOMap = boost::multi_index_container<VAOPack, boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<VAOKey, VAOKeyComp>,
    boost::multi_index::ordered_non_unique<boost::multi_index::member<VAOPack, const Drawable*, &VAOPack::drawable>>
    >>;
static VAOMap VAO_MAP;

Drawable::Drawable(const u16string& typeName)
{
	drawableID = (uint32_t)DRAWABLE_HELPER.regist(typeName);
}

Drawable::~Drawable()
{
	auto& key = VAO_MAP.get<1>();
	const auto its = key.equal_range(this);
	key.erase(its.first, its.second);
}

void Drawable::draw(oglu::oglProgram& prog) const
{
	drawPosition(prog).draw(getVAO(prog)).end();
}

u16string Drawable::getType() const
{
	return DRAWABLE_HELPER.getType(drawableID);
}

void Drawable::releaseAll(const oglu::oglProgram& prog)
{
	const auto its = VAO_MAP.equal_range(prog.weakRef());
    VAO_MAP.erase(its.first, its.second);
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
    VAO_MAP.insert({ prog.weakRef(),this,vao });
}

const oglu::oglVAO& Drawable::getVAO(const oglu::oglProgram& prog) const
{
	const auto& it = VAO_MAP.find(std::make_tuple(prog.weakRef(), this));
	if (it == VAO_MAP.end())
		return defaultVAO;
	else
		return it->vao;
}


}
