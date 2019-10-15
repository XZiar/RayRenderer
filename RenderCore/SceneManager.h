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
    std::map<boost::uuids::uuid, std::shared_ptr<Drawable>> Drawables;
    std::set<std::shared_ptr<Drawable>> WaitDrawables;
    std::vector<std::shared_ptr<Light>> Lights;
    std::shared_ptr<Camera> MainCam;
    oglu::oglUBO LightUBO;
    uint32_t LightOnCount;
    AtomicBitfiled<SceneChange> SceneChanges = SceneChange::Light;
public:
    Scene();
    RESPAK_DECL_SIMP_DESERIALIZE("rayr#Scene")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;

    const std::map<boost::uuids::uuid, std::shared_ptr<Drawable>>& GetDrawables() const { return Drawables; }
    const std::vector<std::shared_ptr<Light>>& GetLights() const { return Lights; }
    const std::shared_ptr<Camera>& GetCamera() const { return MainCam; }
    const oglu::oglUBO& GetLightUBO() const { return LightUBO; }
    uint32_t GetLightUBOCount() const { return LightOnCount; }

    void PrepareLight();
    void PrepareDrawable();

    bool AddObject(const std::shared_ptr<Drawable>& drawable);
    bool AddLight(const std::shared_ptr<Light>& light);
    bool DelObject(const boost::uuids::uuid& uid);
    bool DelObject(const std::shared_ptr<Drawable>& drawable) { return DelObject(drawable->GetUid()); }
    bool DelLight(const std::shared_ptr<Light>& light);
    void ReportChanged(const SceneChange target);
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}

