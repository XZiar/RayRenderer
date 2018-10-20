#include "RenderCoreWrapRely.h"
#include "SceneManagerWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"


namespace Dizz
{


std::shared_ptr<rayr::Drawable> Drawable::GetSelf()
{
    return std::static_pointer_cast<rayr::Drawable>(GetControl()); // type promised
}

Drawable::Drawable(const Wrapper<rayr::Drawable>& drawable) : Controllable(drawable)
{
}


std::shared_ptr<rayr::Light> Light::GetSelf()
{
    return std::static_pointer_cast<rayr::Light>(GetControl()); // type promised
}

Light::Light(const Wrapper<rayr::Light>& light) : Controllable(light)
{
}


std::shared_ptr<rayr::Camera> Camera::GetSelf()
{
    return std::static_pointer_cast<rayr::Camera>(GetControl()); // type promised
}

Camera::Camera(const Wrapper<rayr::Camera>& camera) : Controllable(camera)
{
}


Scene::Scene(const rayr::RenderCore * core) : Core(core)
{
    const auto& scene = Core->GetScene();
    TheScene = new std::weak_ptr<rayr::Scene>(scene);
    MainCamera = gcnew Camera(scene->GetCamera());
    Drawables = gcnew ObservableCollection<Drawable^>();
    Lights = gcnew ObservableCollection<Light^>();
    RefreshScene();
}

void Scene::RefreshScene()
{
    const auto scene = TheScene->lock();
    Drawables->Clear();
    for (const auto& drw : scene->GetDrawables())
    {
        Drawables->Add(gcnew Drawable(drw));
    }
    Lights->Clear();
    for (const auto& lgt : scene->GetLights())
    {
        Lights->Add(gcnew Light(lgt));
    }
}

Scene::!Scene()
{
    if (const auto ptr = ExchangeNullptr(TheScene); ptr)
        delete ptr;
}


}