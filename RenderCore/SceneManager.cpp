#include "RenderCoreRely.h"
#include "SceneManager.h"

using namespace oglu;

namespace rayr
{

Scene::Scene()
{
    MainCam.reset(std::in_place);
    constexpr uint16_t size = 16 * 64;
    LightUBO.reset(size);
}

void Scene::PrepareDrawable()
{
    if (!SceneChanges.Extract(SceneChange::Object))
        return;
    for (const auto& drawable : WaitDrawables)
    {
        drawable->PrepareMaterial({});
        drawable->AssignMaterial();
    }
    WaitDrawables.clear();
}

void Scene::PrepareLight()
{
    if (!SceneChanges.Extract(SceneChange::Light))
        return;
    const auto ptr = LightUBO->GetMappedPtr();
    size_t pos = 0;
    uint32_t onCnt = 0;
    for (const auto& lgt : Lights)
    {
        if (!lgt->isOn)
            continue;
        onCnt++;
        pos += lgt->WriteData(ptr.AsType<std::byte>() + pos);
        if (pos >= LightUBO->Size())
            break;
    }
    LightOnCount = onCnt;
}

bool Scene::AddObject(const Wrapper<Drawable>& drawable)
{
    WaitDrawables.push_back(drawable);
    Drawables.push_back(drawable);
    SceneChanges.Add(SceneChange::Object);
    basLog().success(u"Add an Drawable [{}][{}]:  {}\n", Drawables.size() - 1, drawable->GetType(), drawable->Name);
    return true;
}

bool Scene::AddLight(const Wrapper<Light>& light)
{
    Lights.push_back(light);
    SceneChanges.Add(SceneChange::Light);
    basLog().success(u"Add a Light [{}][{}]:  {}\n", Lights.size() - 1, (int32_t)light->type, light->name);
    return true;
}

void Scene::ReportChanged(const SceneChange target)
{
    SceneChanges.Add(target);
}

ejson::JObject Scene::Serialize(SerializeUtil& context) const
{
    auto jself = context.NewObject();
    {
        auto jlights = context.NewArray();
        for (const auto& lgt : Lights)
            context.AddObject(jlights, *lgt);
        jself.Add("lights", jlights);
    }
    {
        auto jdrawables = context.NewArray();
        for (const auto& drw : Drawables)
            context.AddObject(jdrawables, *drw);
        jself.Add("drawables", jdrawables);
    }
    context.AddObject(jself, "camera", *MainCam);
    return jself;
}

void Scene::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    {
        Lights.clear();
        const auto jlights = object.GetArray("lights");
        for (const auto ele : jlights)
        {
            const ejson::JObjectRef<true> jlgt(ele);
            Lights.push_back(context.DeserializeShare<Light>(jlgt));
        }
        ReportChanged(SceneChange::Light);
    }
    {
        Drawables.clear();
        const auto jdrawables = object.GetArray("drawables");
        for (const auto ele : jdrawables)
        {
            const ejson::JObjectRef<true> jdrw(ele);
            const auto k = context.DeserializeShare<Drawable>(jdrw);
            Drawables.push_back(context.DeserializeShare<Drawable>(jdrw));
        }
        ReportChanged(SceneChange::Object);
    }
    MainCam = context.DeserializeShare<Camera>(object.GetObject("camera"));
}

RESPAK_DESERIALIZER(Scene)
{
    auto ret = new Scene();
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Scene)

}
