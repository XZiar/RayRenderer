#include "RenderElement.h"

namespace rayr
{



vector<wstring> DrawableHelper::typeMap;
DrawableHelper::DrawableHelper(const wstring& name)
{
	typeMap.push_back(name);
	id = static_cast<uint32_t>(typeMap.size() - 1);
#ifdef _DEBUG
	printf("@@@@regist Drawable [%ls] -> %2d\n", name.c_str(), id);
#endif
}

void DrawableHelper::InitDrawable(Drawable *d) const
{
	d->drawableID = id;
}

wstring DrawableHelper::getType(const Drawable& d)
{
	return typeMap[d.drawableID];
}

void Drawable::draw(oglu::oglProgram& prog)
{
	Mat4x4 matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, rotation.z)) *
		Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, rotation.y)) *
		Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, rotation.x)));
	matModel = Mat4x4::TranslateMat(position) * matModel;
	prog->draw(matModel).draw(vao);
}

auto Drawable::defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare())
{
	const GLint attrs[3] = { prog->Attr_Vert_Pos, prog->Attr_Vert_Norm, prog->Attr_Vert_Texc };
	return std::move(vao->prepare().set(vbo, attrs, 0));
}

}
