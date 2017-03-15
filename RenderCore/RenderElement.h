#pragma once

#include "RenderCoreRely.h"
#include <tuple>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace rayr
{
using namespace common;
using namespace b3d;

class alignas(16) Material : public AlignBase<>
{
public:
	Vec4 ambient, diffuse, specular, emission;
	float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
};

class Drawable;

class DrawableHelper
{
private:
	friend class Drawable;
	static vector<wstring> typeMap;
public:
	uint32_t id;
	DrawableHelper(const wstring& name);
	void InitDrawable(Drawable *d) const;
	static wstring getType(const Drawable& d);
	static void releaseAll(const oglu::oglProgram& prog);
};

class alignas(16) Drawable : public AlignBase<>, public NonCopyable, public NonMovable
{
	friend class DrawableHelper;
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
public:
	Vec3 position = Vec3::zero(), rotation = Vec3::zero(), scale = Vec3::one();
	wstring name;
	virtual ~Drawable();
	/*prepare VAO with given Vertex Attribute Location
	 *-param: VertPos,VertNorm,TexPos
	 */
	virtual void prepareGL(const oglu::oglProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
	virtual void draw(oglu::oglProgram& prog) const;
protected:
	uint32_t drawableID;
	oglu::oglVAO defaultVAO;
	auto defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare());
	auto drawPosition(oglu::oglProgram& prog) const -> decltype(prog->draw());
	void setVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const;
	const oglu::oglVAO& getVAO(const oglu::oglProgram& prog) const;
};

}
