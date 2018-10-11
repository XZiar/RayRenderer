#pragma once
#include "RenderCoreRely.h"
#include "RenderElement.h"
#include "Light.h"


namespace rayr
{

class Scene
{
private:
    vector<Wrapper<Drawable>> Drawables;
    vector<Wrapper<Light>> Lights;
public:
    const vector<Wrapper<Drawable>>& GetDrawables() { return Drawables; }
    const vector<Wrapper<Light>>& GetLights() { return Lights; }
};

}

