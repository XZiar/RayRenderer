#pragma once
#include "RenderCoreRely.h"
#include "RenderElement.h"
#include "Light.h"
#include "Camera.h"

namespace rayr
{

class Scene
{
private:
    vector<Wrapper<Drawable>> Drawables;
    vector<Wrapper<Light>> Lights;
    Wrapper<Camera> MainCam;
public:
    const vector<Wrapper<Drawable>>& GetDrawables() const { return Drawables; }
    const vector<Wrapper<Light>>& GetLights() const { return Lights; }
    const Wrapper<Camera>& GetCamera() const { return MainCam; }
};

}

