#include "RenderCoreRely.h"
#include "SceneManager.h"

using namespace oglu;
using common::container::ValSet;
using common::container::FindInMap;

namespace rayr
{

static constexpr uint32_t LightLimit = 16;

Scene::Scene()
{
    MainCam.reset(std::in_place);
    LightUBO.reset(LightLimit * Light::WriteSize);
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
        onCnt++;
        lgt->WriteData(ptr.AsType<std::byte>() + pos);
        pos += Light::WriteSize;
        if (pos >= LightUBO->Size())
            break;
    }
    LightOnCount = onCnt;
}

bool Scene::AddObject(const Wrapper<Drawable>& drawable)
{
    const auto uid = drawable->GetUid();
    if (!Drawables.try_emplace(uid, drawable).second)
        return false;
    WaitDrawables.insert(drawable);
    SceneChanges.Add(SceneChange::Object);
    dizzLog().success(u"Add an Drawable [{}][{}]:  {}\n", Drawables.size() - 1, drawable->GetType(), drawable->Name);
    return true;
}

bool Scene::AddLight(const Wrapper<Light>& light)
{
    Lights.push_back(light);
    SceneChanges.Add(SceneChange::Light);
    dizzLog().success(u"Add a Light [{}][{}]:  {}\n", Lights.size() - 1, (int32_t)light->Type, light->Name);
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

bool Scene::DelLight(const Wrapper<Light>& light)
{
    return false;
}

void Scene::ReportChanged(const SceneChange target)
{
    SceneChanges.Add(target);
}

void Scene::Serialize(SerializeUtil & context, ejson::JObject& jself) const
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

void Scene::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    {
        Lights.clear();
        const auto jlights = object.GetArray("Lights");
        for (const auto ele : jlights)
        {
            const ejson::JObjectRef<true> jlgt(ele);
            Lights.push_back(context.DeserializeShare<Light>(jlgt));
        }
        ReportChanged(SceneChange::Light);
    }
    {
        Drawables.clear();
        const auto jdrawables = object.GetArray("Drawables");
        for (const auto ele : jdrawables)
        {
            const ejson::JObjectRef<true> jdrw(ele);
            dizzLog().debug(u"Deserialize Drawable: [{}]({})\n", str::to_u16string(jdrw.Get<string>("Name"), str::Charset::UTF8),
                str::to_u16string(jdrw.Get<string>("#Type"), str::Charset::UTF8));
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
