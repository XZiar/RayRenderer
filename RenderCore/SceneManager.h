#pragma once
#include "RenderCoreRely.h"
#include "RenderCoreUtil.hpp"
#include "RenderElement.h"
#include "Light.h"
#include "Camera.h"
#include "Model.h"

namespace rayr
{

enum class SceneChange : uint32_t { Light = 0x1, Object = 0x2 };
MAKE_ENUM_BITFIELD(SceneChange)


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class RAYCOREAPI Scene : public xziar::respak::Serializable
{
    // TODO: make AddLight/AddObject able to be called by other thread?
    //       currently only changes are reported atomicly
private:
    vector<Wrapper<Drawable>> Drawables;
    vector<Wrapper<Drawable>> WaitDrawables;
    vector<Wrapper<Light>> Lights;
    Wrapper<Camera> MainCam;
    oglu::oglUBO LightUBO;
    uint32_t LightOnCount;
    AtomicBitfiled<SceneChange> SceneChanges = SceneChange::Light;
public:
    Scene();
    RESPAK_DECL_SIMP_DESERIALIZE("rayr#Scene")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;

    const vector<Wrapper<Drawable>>& GetDrawables() const { return Drawables; }
    const vector<Wrapper<Light>>& GetLights() const { return Lights; }
    const Wrapper<Camera>& GetCamera() const { return MainCam; }
    const oglu::oglUBO& GetLightUBO() const { return LightUBO; }
    uint32_t GetLightUBOCount() const { return LightOnCount; }

    void PrepareLight();
    void PrepareDrawable();

    bool AddObject(const Wrapper<Drawable>& drawable);
    bool AddLight(const Wrapper<Light>& light);
    void ReportChanged(const SceneChange target);
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}

