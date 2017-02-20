#include "RenderElement.h"

namespace rayr
{



vector<string> DrawableHelper::typeMap;
DrawableHelper::DrawableHelper(string name)
{
	typeMap.push_back(name);
	id = typeMap.size() - 1;
#ifdef _DEBUG
	printf("@@@@regist Drawable %s with id %d\n", name.c_str(), id);
#endif
}

void DrawableHelper::InitDrawable(Drawable *d)
{
	d->drawableID = id;
}

string DrawableHelper::getType(const Drawable& d)
{
	return typeMap[d.drawableID];
}

}
