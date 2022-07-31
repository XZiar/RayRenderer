#include "RenderCorePch.h"
#include "SceneManager.h"


namespace dizz
{
using common::str::Encoding;
using common::container::ValSet;
using common::container::FindInMap;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;
using namespace oglu;


static constexpr uint32_t LightLimit = 16;

Scene::Scene()
{
    MainCam = std::make_shared<Camera>();
    LightUBO = oglu::oglUniformBuffer_::Create(LightLimit * Light::WriteSize);
    LightUBO->SetName(u"LightData");
}

void Scene::PrepareDrawable()
{
    if (!SceneChanges.Extract(SceneChange::Object))
        return;
    for (const auto& drawable : WaitDrawables)
    {
        drawable->PrepareMaterial();
        drawable->AssignMaterial();
    }
    WaitDrawables.clear();
}

void Scene::PrepareLight()
{
    if (!SceneChanges.Extract(SceneChange::Light))
        return;
    const auto ptr = LightUBO->GetPersistentPtr();
    size_t pos = 0;
    uint32_t onCnt = 0;
    for (const auto& lgt : Lights)
    {
        if (!lgt->IsOn)
            continue;
        const auto subSpace = ptr.subspan(pos);
        if ((size_t)subSpace.size() < Light::WriteSize)
            break;
        lgt->WriteData(subSpace);
        pos += Light::WriteSize;
        onCnt++;
    }
    LightOnCount = onCnt;
}

bool Scene::AddObject(const std::shared_ptr<Drawable>& drawable)
{
    const auto uid = drawable->GetUid();
    if (!Drawables.try_emplace(uid, drawable).second)
        return false;
    WaitDrawables.insert(drawable);
    SceneChanges.Add(SceneChange::Object);
    dizzLog().Success(u"Add an Drawable [{}][{}]:  {}\n", Drawables.size() - 1, drawable->GetType(), drawable->Name);
    return true;
}

bool Scene::AddLight(const std::shared_ptr<Light>& light)
{
    Lights.push_back(light);
    SceneChanges.Add(SceneChange::Light);
    dizzLog().Success(u"Add a Light [{}][{}]:  {}\n", Lights.size() - 1, (int32_t)light->Type, light->Name);
    return true;
}

bool Scene::DelObject(const boost::uuids::uuid& uid)
{
    auto it = Drawables.find(uid);
    if (it == Drawables.end())
        return false;
    WaitDrawables.erase(it->second);
    Drawables.erase(it);
    return true;
}

bool Scene::DelLight(const std::shared_ptr<Light>&)
{
    return false;
}

void Scene::ReportChanged(const SceneChange target)
{
    SceneChanges.Add(target);
}

void Scene::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    {
        auto jlights = context.NewArray();
        for (const auto& lgt : Lights)
            context.AddObject(jlights, *lgt);
        jself.Add("Lights", jlights);
    }
    {
        auto jdrawables = context.NewArray();
        for (const auto& drw : ValSet(Drawables))
            context.AddObject(jdrawables, *drw);
        jself.Add("Drawables", jdrawables);
    }
    context.AddObject(jself, "Camera", *MainCam);
}

void Scene::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    {
        Lights.clear();
        const auto jlights = object.GetArray("Lights");
        for (const auto ele : jlights)
        {
            const xziar::ejson::JObjectRef<true> jlgt(ele);
            Lights.push_back(context.DeserializeShare<Light>(jlgt));
        }
        ReportChanged(SceneChange::Light);
    }
    {
        Drawables.clear();
        const auto jdrawables = object.GetArray("Drawables");
        for (const auto ele : jdrawables)
        {
            const xziar::ejson::JObjectRef<true> jdrw(ele);
            dizzLog().Debug(u"Deserialize Drawable: [{}]({})\n", common::str::to_u16string(jdrw.Get<string>("Name"), Encoding::UTF8),
                common::str::to_u16string(jdrw.Get<string>("#Type"), Encoding::UTF8));
            const auto drw = context.DeserializeShare<Drawable>(jdrw);
            if (Drawables.try_emplace(drw->GetUid(), drw).second)
                WaitDrawables.insert(drw);
        }
        ReportChanged(SceneChange::Object);
    }
    MainCam = context.DeserializeShare<Camera>(object.GetObject("Camera"));
}

RESPAK_IMPL_SIMP_DESERIALIZE(Scene)

}
