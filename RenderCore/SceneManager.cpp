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
    LightBuf.resize(size);
}

bool Scene::AddObject(const Wrapper<Drawable>& drawable)
{
    WaitDrawables.push_back(drawable);
    ReportChanged(SceneChange::Object);
    basLog().success(u"Add an Drawable [{}]:  {}\n", drawable->GetType(), drawable->Name);
    return true;
}

bool Scene::AddLight(const Wrapper<Light>& light)
{
    Lights.push_back(light);
    ReportChanged(SceneChange::Light);
    basLog().success(u"Add a Light [{}][{}]:  {}\n", Lights.size() - 1, (int32_t)light->type, light->name);
    return true;
}

void Scene::PrepareDrawable()
{
    constexpr auto LightMask = ~(uint32_t)SceneChange::Object;
    if (!HAS_FIELD((SceneChange)SceneChanges.fetch_and(LightMask), SceneChange::Object))
        return;
    for (const auto& drawable : WaitDrawables)
    {
        drawable->PrepareMaterial({});
        drawable->AssignMaterial();
        Drawables.push_back(drawable);
    }
    WaitDrawables.clear();
}

void Scene::ReportChanged(const SceneChange target)
{
    SceneChanges |= (uint32_t)target;
}

void Scene::PrepareLight()
{
    constexpr auto LightMask = ~(uint32_t)SceneChange::Light;
    if (!HAS_FIELD((SceneChange)SceneChanges.fetch_and(LightMask), SceneChange::Light))
        return;
    size_t pos = 0;
    uint32_t onCnt = 0;
    for (const auto& lgt : Lights)
    {
        if (!lgt->isOn)
            continue;
        onCnt++;
        pos += lgt->WriteData(&LightBuf[pos]);
        if (pos >= LightUBO->Size())
            break;
    }
    LightOnCount = onCnt;
    LightUBO->Write(LightBuf.data(), pos, BufferWriteMode::StreamDraw);
}



}
