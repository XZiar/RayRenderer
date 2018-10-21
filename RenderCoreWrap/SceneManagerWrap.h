#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

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
    const Wrapper<rayr::Drawable>* TempHandle;
internal:
    std::shared_ptr<rayr::Drawable> GetSelf();
    Drawable(const Wrapper<rayr::Drawable>& drawable);
    Drawable(Wrapper<rayr::Drawable>&& drawable);
    void ReleaseTempHandle();
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable();
    virtual void Move(const float dx, const float dy, const float dz)
    {
        GetSelf()->Move(dx, dy, dz);
        ViewModel.OnPropertyChanged("Position");
    }
    virtual void Rotate(const float dx, const float dy, const float dz)
    {
        GetSelf()->Rotate(dx, dy, dz);
        ViewModel.OnPropertyChanged("Rotation");
    }
};


public ref class Light : public Controllable, public IMovable
{
internal:
    std::shared_ptr<rayr::Light> GetSelf();
    Light(const Wrapper<rayr::Light>& light);
public:
    virtual void Move(const float dx, const float dy, const float dz)
    {
        GetSelf()->Move(dx, dy, dz);
        ViewModel.OnPropertyChanged("Position");
    }
    virtual void Rotate(const float dx, const float dy, const float dz)
    {
        GetSelf()->Rotate(dx, dy, dz);
        ViewModel.OnPropertyChanged("Direction");
    }
};


public ref class Camera : public Controllable, public IMovable
{
internal:
    std::shared_ptr<rayr::Camera> GetSelf();
    Camera(const Wrapper<rayr::Camera>& camera);
public:
    virtual void Move(const float dx, const float dy, const float dz)
    {
        GetSelf()->Move(dx, dy, dz);
        ViewModel.OnPropertyChanged("Position");
    }
    virtual void Rotate(const float dx, const float dy, const float dz)
    {
        GetSelf()->Rotate(dx, dy, dz);
        ViewModel.OnPropertyChanged("Direction");
    }
    //rotate along x-axis, radius
    void Pitch(const float radx)
    {
        GetSelf()->Pitch(radx);
        ViewModel.OnPropertyChanged("Direction");
    }
    //rotate along y-axis, radius
    void Yaw(const float rady)
    {
        GetSelf()->Yaw(rady);
        ViewModel.OnPropertyChanged("Direction");
    }
    //rotate along z-axis, radius
    void Roll(const float radz)
    {
        GetSelf()->Roll(radz);
        ViewModel.OnPropertyChanged("Direction");
    }
};


public ref class Scene : public BaseViewModel
{
internal:
    const rayr::RenderCore * const Core;
    const std::weak_ptr<rayr::Scene> *TheScene;
    Scene(const rayr::RenderCore * core);
    void RefreshScene();
    void OnAddModel(Object^ sender, NotifyCollectionChangedEventArgs^ e);
    initonly NotifyCollectionChangedEventHandler^ OnAddModelHandler;
    void OnAddLight(Object^ sender, NotifyCollectionChangedEventArgs^ e);
    initonly NotifyCollectionChangedEventHandler^ OnAddLightHandler;
public:
    ~Scene() { this->!Scene(); }
    !Scene();

    initonly Camera^ MainCamera;
    initonly ObservableCollection<Drawable^>^ Drawables;
    initonly ObservableCollection<Light^>^ Lights;
};



}

