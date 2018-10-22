#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "ObservableContainer.hpp"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;
using namespace System::Collections::Specialized;
using namespace System::Runtime::CompilerServices;
using namespace System::Threading::Tasks;
using namespace msclr::interop;

namespace Dizz
{

public interface class IMovable
{
public:
    void Move(const float dx, const float dy, const float dz);
    void Rotate(const float dx, const float dy, const float dz);
};
[ExtensionAttribute]
public ref class IMovableExtensions abstract sealed {
public:
    [ExtensionAttribute]
    static void Move(IMovable ^obj, Vector3 vec) { obj->Move(vec.X, vec.Y, vec.Z); }
    [ExtensionAttribute]
    static void Rotate(IMovable ^obj, Vector3 vec) { obj->Rotate(vec.X, vec.Y, vec.Z); }
};


public ref class Drawable : public Controllable, public IMovable
{
private:
    const Wrapper<rayr::Drawable>* TempHandle;
internal:
    std::shared_ptr<rayr::Drawable> GetSelf();
    Drawable(const Wrapper<rayr::Drawable>& drawable);
    Drawable(Wrapper<rayr::Drawable>&& drawable);
    void ReleaseTempHandle();
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable();
    virtual void Move(const float dx, const float dy, const float dz);
    virtual void Rotate(const float dx, const float dy, const float dz);
};


public enum class LightType : int32_t
{
    Parallel = (int32_t)rayr::LightType::Parallel, Point = (int32_t)rayr::LightType::Point, Spot = (int32_t)rayr::LightType::Spot
};
public ref class Light : public Controllable, public IMovable
{
private:
    const Wrapper<rayr::Light>* TempHandle;
internal:
    std::shared_ptr<rayr::Light> GetSelf();
    Light(const Wrapper<rayr::Light>& light);
    Light(Wrapper<rayr::Light>&& light);
    void ReleaseTempHandle();
public:
    virtual void Move(const float dx, const float dy, const float dz);
    virtual void Rotate(const float dx, const float dy, const float dz);
    static Light^ NewLight(LightType type);
};


public ref class Camera : public Controllable, public IMovable
{
internal:
    std::shared_ptr<rayr::Camera> GetSelf();
    Camera(const Wrapper<rayr::Camera>& camera);
public:
    virtual void Move(const float dx, const float dy, const float dz);
    virtual void Rotate(const float dx, const float dy, const float dz);
    //rotate along x-axis, radius
    void Pitch(const float radx);
    //rotate along y-axis, radius
    void Yaw(const float rady);
    //rotate along z-axis, radius
    void Roll(const float radz);
};


public ref class Scene : public BaseViewModel
{
private:
    void OnAddModel(Object^ sender, Drawable^ object, bool% shouldAdd);
    void OnAddLight(Object^ sender, Light^ object, bool% shouldAdd);
    /*void OnDrawablesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
    void OnLightsChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);*/
    void OnLightPropertyChanged(Object^ sender, Light^ object, PropertyChangedEventArgs^ e);
internal:
    const rayr::RenderCore * const Core;
    const std::weak_ptr<rayr::Scene> *TheScene;
    Scene(const rayr::RenderCore * core);
    void RefreshScene();
public:
    ~Scene() { this->!Scene(); }
    !Scene();

    initonly Camera^ MainCamera;
    initonly ObservableProxyContainer<Drawable^>^ Drawables;
    initonly ObservableProxyContainer<Light^>^ Lights;
};



}

