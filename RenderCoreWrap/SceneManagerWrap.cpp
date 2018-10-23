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

void Drawable::Move(const float dx, const float dy, const float dz)
{
    GetSelf()->Move(dx, dy, dz);
    RaisePropertyChanged("Position");
}

void Drawable::Rotate(const float dx, const float dy, const float dz)
{
    GetSelf()->Rotate(dx, dy, dz);
    RaisePropertyChanged("Rotation");
}

std::shared_ptr<rayr::Light> Light::GetSelf()
{
    return std::static_pointer_cast<rayr::Light>(GetControl()); // type promised
}

Light::Light(const Wrapper<rayr::Light>& light) : Controllable(light)
{
}
Light::Light(Wrapper<rayr::Light>&& light) : Controllable(light), TempHandle(new Wrapper<rayr::Light>(light))
{
}

void Light::ReleaseTempHandle()
{
    if (const auto ptr = ExchangeNullptr(TempHandle); ptr)
        delete ptr;
}

void Light::Move(const float dx, const float dy, const float dz)
{
    GetSelf()->Move(dx, dy, dz);
    RaisePropertyChanged("Position");
}

void Light::Rotate(const float dx, const float dy, const float dz)
{
    GetSelf()->Rotate(dx, dy, dz);
    RaisePropertyChanged("Direction");
}

#pragma managed(push, off)
static Wrapper<rayr::Light> CreateLight(rayr::LightType type)
{
    Wrapper<rayr::Light> light;
    switch (type)
    {
    case rayr::LightType::Parallel:
        light = Wrapper<rayr::ParallelLight>(std::in_place);
        light->Color = b3d::Vec4(1.0, 0.3, 0.3, 1.0);
        break;
    case rayr::LightType::Point:
        light = Wrapper<rayr::PointLight>(std::in_place);
        light->Color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        break;
    case rayr::LightType::Spot:
        light = Wrapper<rayr::SpotLight>(std::in_place);
        light->Color = b3d::Vec4(0.3, 0.3, 1.0, 1.0);
        break;
    }
    light->Direction = b3d::Vec4(0, 0, 1, 0);
    return light;
}
#pragma managed(pop)
Light^ Light::NewLight(LightType type)
{
    return gcnew Light(CreateLight((rayr::LightType)type));
}


std::shared_ptr<rayr::Camera> Camera::GetSelf()
{
    return std::static_pointer_cast<rayr::Camera>(GetControl()); // type promised
}

Camera::Camera(const Wrapper<rayr::Camera>& camera) : Controllable(camera)
{
}

void Camera::Move(const float dx, const float dy, const float dz)
{
    GetSelf()->Move(dx, dy, dz);
    RaisePropertyChanged("Position");
}

void Camera::Rotate(const float dx, const float dy, const float dz)
{
    GetSelf()->Rotate(dx, dy, dz);
    RaisePropertyChanged("Direction");
}

//rotate along x-axis, radius
void Camera::Pitch(const float radx)
{
    GetSelf()->Pitch(radx);
    RaisePropertyChanged("Direction");
}

//rotate along y-axis, radius
void Camera::Yaw(const float rady)
{
    GetSelf()->Yaw(rady);
    RaisePropertyChanged("Direction");
}

//rotate along z-axis, radius
void Camera::Roll(const float radz)
{
    GetSelf()->Roll(radz);
    RaisePropertyChanged("Direction");
}


Scene::Scene(const rayr::RenderCore * core) : Core(core)
{
    const auto& scene = Core->GetScene();
    TheScene = new std::weak_ptr<rayr::Scene>(scene);
    MainCamera = gcnew Camera(scene->GetCamera());
    Drawables = gcnew ObservableProxyContainer<Drawable^>();
    Drawables->BeforeAddObject += gcnew AddObjectEventHandler<Drawable^>(this, &Scene::OnAddModel);
    //Drawables->CollectionChanged += gcnew NotifyCollectionChangedEventHandler(this, &Scene::OnDrawablesChanged);
    Lights = gcnew ObservableProxyContainer<Light^>();
    Lights->BeforeAddObject += gcnew AddObjectEventHandler<Light^>(this, &Scene::OnAddLight);
    Lights->ObjectPropertyChanged += gcnew ObjectPropertyChangedEventHandler<Light^>(this, &Scene::OnLightPropertyChanged);
    //Lights->CollectionChanged += gcnew NotifyCollectionChangedEventHandler(this, &Scene::OnLightsChanged);
    RefreshScene();
}

void Scene::RefreshScene()
{
    const auto scene = TheScene->lock();

    for (const auto& drw : common::container::ValSet(scene->GetDrawables()))
    {
        Drawables->InnerAdd(gcnew Drawable(drw));
    }

    for (const auto& lgt : scene->GetLights())
    {
        Lights->InnerAdd(gcnew Light(lgt));
    }
}

void Scene::OnAddModel(Object^ sender, Drawable^ object, bool% shouldAdd)
{
    if (TheScene->lock()->AddObject(object->GetSelf()))
    {
        object->ReleaseTempHandle();
        shouldAdd = true;
    }
}

void Scene::OnAddLight(Object^ sender, Light^ object, bool% shouldAdd)
{
    if (TheScene->lock()->AddLight(object->GetSelf()))
    {
        object->ReleaseTempHandle();
        shouldAdd = true;
    }
}
//
//void Scene::OnDrawablesChanged(Object ^ sender, NotifyCollectionChangedEventArgs ^ e)
//{
//    switch (e->Action)
//    {
//    case NotifyCollectionChangedAction::Add:
//        break;
//    default: break;
//    }
//}
//
//void Scene::OnLightsChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e)
//{
//    switch (e->Action)
//    {
//    case NotifyCollectionChangedAction::Add:
//        break;
//    default: break;
//    }
//}

void Scene::OnLightPropertyChanged(Object^ sender, Light^ object, PropertyChangedEventArgs^ e)
{
    TheScene->lock()->ReportChanged(rayr::SceneChange::Light);
}

Scene::!Scene()
{
    if (const auto ptr = ExchangeNullptr(TheScene); ptr)
        delete ptr;
}


}