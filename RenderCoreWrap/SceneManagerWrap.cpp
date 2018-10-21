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
Drawable::Drawable(Wrapper<rayr::Drawable>&& drawable) : Controllable(drawable), TempHandle(new Wrapper<rayr::Drawable>(drawable))
{
}

void Drawable::ReleaseTempHandle()
{
    if (const auto ptr = ExchangeNullptr(TempHandle); ptr)
        delete ptr;
}

Drawable::!Drawable()
{
    ReleaseTempHandle();
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
    OnAddModelHandler = gcnew NotifyCollectionChangedEventHandler(this, &Scene::OnAddModel);
    OnAddLightHandler = gcnew NotifyCollectionChangedEventHandler(this, &Scene::OnAddLight);
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

    Drawables->CollectionChanged -= OnAddModelHandler;
    Drawables->Clear();
    for (const auto& drw : scene->GetDrawables())
    {
        Drawables->Add(gcnew Drawable(drw));
    }
    Drawables->CollectionChanged += OnAddModelHandler;

    Lights->CollectionChanged -= OnAddLightHandler;
    Lights->Clear();
    for (const auto& lgt : scene->GetLights())
    {
        Lights->Add(gcnew Light(lgt));
    }
    Lights->CollectionChanged += OnAddLightHandler;
}

void Scene::OnAddModel(Object ^ sender, NotifyCollectionChangedEventArgs ^ e)
{
    const auto scene = TheScene->lock();
    switch (e->Action)
    {
    case NotifyCollectionChangedAction::Add:
        for each (Object^ item in e->NewItems)
        {
            const auto drw = safe_cast<Drawable^>(item);
            scene->AddObject(drw->GetSelf());
            drw->ReleaseTempHandle();
        }
        break;
    default: break;
    }
}

void Scene::OnAddLight(Object ^ sender, NotifyCollectionChangedEventArgs ^ e)
{
    const auto scene = TheScene->lock();
    switch (e->Action)
    {
    case NotifyCollectionChangedAction::Add:
        for each (Object^ drw in e->NewItems)
        {
            scene->AddLight(safe_cast<Light^>(drw)->GetSelf());
        }
        break;
    default: break;
    }
}

Scene::!Scene()
{
    if (const auto ptr = ExchangeNullptr(TheScene); ptr)
        delete ptr;
}


}