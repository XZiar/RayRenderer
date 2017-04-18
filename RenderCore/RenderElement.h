#pragma once

#include "RenderCoreRely.h"

namespace rayr
{
using namespace common;
using namespace b3d;

class alignas(16) Material : public AlignBase<Material>
{
public:
	Vec4 ambient, diffuse, specular, emission;
	float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
};


class DrawableHelper;

class alignas(16) Drawable : public AlignBase<Material>, public NonCopyable
{
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
	static VAOMap vaoMap;
	static DrawableHelper& getHelper();
public:
	Vec3 position = Vec3::zero(), rotation = Vec3::zero(), scale = Vec3::one();
	wstring name;
	static void releaseAll(const oglu::oglProgram& prog);
	virtual ~Drawable();
	/*prepare VAO with given Vertex Attribute Location
	 *-param: VertPos,VertNorm,TexPos
	 */
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
	virtual void draw(oglu::oglProgram& prog) const;
	wstring getType() const;
protected:
	uint32_t drawableID;
	oglu::oglVAO defaultVAO;
	Drawable(const wstring& typeName);
	auto defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare());
	auto drawPosition(oglu::oglProgram& prog) const -> decltype(prog->draw());
	void setVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const;
	const oglu::oglVAO& getVAO(const oglu::oglProgram& prog) const;
};

}
