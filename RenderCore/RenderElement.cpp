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

void Drawable::draw(const oglu::oglProgram& prog)
{
	//transf[0].vec = d->rotation;
	//transf[1].vec = d->position;
	//prog->draw(transf.begin(), transf.end()).draw(d->vao);
}

}
