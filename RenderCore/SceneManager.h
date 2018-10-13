#pragma once
#include "RenderCoreRely.h"
#include "RenderElement.h"
#include "Light.h"
#include "Camera.h"
#include "Model.h"

namespace rayr
{

enum class SceneChange : uint32_t { Light = 0x1, Object = 0x2 };
MAKE_ENUM_BITFIELD(SceneChange)


class RAYCOREAPI Scene
{
    // TODO: make AddLight/AddObject able to be called by other thread?
    //       currently only changes are reported atomicly
private:
    vector<Wrapper<Drawable>> Drawables;
    vector<Wrapper<Drawable>> WaitDrawables;
    vector<Wrapper<Light>> Lights;
    Wrapper<Camera> MainCam;
    vector<byte> LightBuf;
    oglu::oglUBO LightUBO;
    uint32_t LightOnCount;
    std::atomic_uint32_t SceneChanges{ (uint32_t)SceneChange::Light };
public:
    Scene();

    const vector<Wrapper<Drawable>>& GetDrawables() const { return Drawables; }
    const vector<Wrapper<Light>>& GetLights() const { return Lights; }
    const Wrapper<Camera>& GetCamera() const { return MainCam; }
    const oglu::oglUBO& GetLightUBO() const { return LightUBO; }
    uint32_t GetLightUBOCount() const { return LightOnCount; }

    bool AddObject(const Wrapper<Drawable>& drawable);
    bool AddLight(const Wrapper<Light>& light);
    void PrepareLight();
    void PrepareDrawable();
    void ReportChanged(const SceneChange target);
};

}

