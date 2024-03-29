#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "ObservableContainer.hpp"
#include "ThumbManWrap.h"

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
public ref class IMovableExtensions abstract sealed 
{
public:
    [ExtensionAttribute]
    static void Move(IMovable ^obj, Vector3 vec) { obj->Move(vec.X, vec.Y, vec.Z); }
    [ExtensionAttribute]
    static void Rotate(IMovable ^obj, Vector3 vec) { obj->Rotate(vec.X, vec.Y, vec.Z); }
};


#define DECLARE_TEX_PROPERTY(Name, name, target) \
    property TexHolder^ Name \
    { \
        TexHolder^ get() { return name; } \
        void set(TexHolder^ value) \
        { \
            auto holder = value->ExtractHolder(); \
            GetSelf()->target = holder; name = TexHolder::CreateTexHolder(holder); \
            RaisePropertyChanged(#Name); \
        } \
    }
public ref class PBRMaterial : public Controllable
{
private:
    TexHolder^ diffuseMap;
internal:
    std::shared_ptr<dizz::PBRMaterial> GetSelf();
    PBRMaterial(const std::shared_ptr<dizz::PBRMaterial>& material);
public:
    virtual String^ ToString() override;
    DECLARE_TEX_PROPERTY(DiffuseMap, diffuseMap, DiffuseMap);
    //property TexHolder^ DiffuseMap { TexHolder^ get(); }
    property TexHolder^ NormalMap  { TexHolder^ get(); }
    property TexHolder^ MetalMap   { TexHolder^ get(); }
    property TexHolder^ RoughMap   { TexHolder^ get(); }
    property TexHolder^ AOMap      { TexHolder^ get(); }
};


public ref class Drawable : public Controllable, public IMovable
{
private:
    const std::shared_ptr<dizz::Drawable>* TempHandle;
    initonly String^ DrawableType;
    initonly ObjectPropertyChangedEventHandler<PBRMaterial^>^ MaterialPropertyChangedCallback;
    void OnMaterialChanged(Object^ sender, PBRMaterial^ material, PropertyChangedEventArgs^ e);
    initonly ObservableProxyReadonlyContainer<PBRMaterial^>^ materials;
internal:
    std::shared_ptr<dizz::Drawable> GetSelf();
    Drawable(const std::shared_ptr<dizz::Drawable>& drawable);
    Drawable(std::shared_ptr<dizz::Drawable>&& drawable);
    void ReleaseTempHandle();
    bool CreateMaterials();
public:
    initonly Guid^ Uid;
    ~Drawable() { this->!Drawable(); }
    !Drawable();
    virtual void Move(const float dx, const float dy, const float dz);
    virtual void Rotate(const float dx, const float dy, const float dz);
    virtual String^ ToString() override;
    CLI_READONLY_PROPERTY(ObservableProxyReadonlyContainer<PBRMaterial^>^, Materials, materials)
};


public enum class LightType : int32_t
{
    Parallel = (int32_t)dizz::LightType::Parallel, Point = (int32_t)dizz::LightType::Point, Spot = (int32_t)dizz::LightType::Spot
};
public ref class Light : public Controllable, public IMovable
{
private:
    const std::shared_ptr<dizz::Light>* TempHandle;
internal:
    std::shared_ptr<dizz::Light> GetSelf();
    Light(const std::shared_ptr<dizz::Light>& light);
    Light(std::shared_ptr<dizz::Light>&& light);
    void ReleaseTempHandle();
public:
    initonly LightType LgtType;
    virtual void Move(const float dx, const float dy, const float dz);
    virtual void Rotate(const float dx, const float dy, const float dz);
    virtual String^ ToString() override;
    static Light^ NewLight(LightType type);
};


public ref class Camera : public Controllable, public IMovable
{
internal:
    std::shared_ptr<dizz::Camera> GetSelf();
    Camera(const std::shared_ptr<dizz::Camera>& camera);
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
    static Func<Drawable^, bool>^ DrawablePrepareFunc;
private:
    List<Drawable^>^ WaitDrawables;
    void BeforeAddModel(Object^ sender, Drawable^ object, bool% shouldAdd);
    void BeforeDelModel(Object^ sender, Drawable^ object, bool% shouldDel);
    void BeforeAddLight(Object^ sender, Light^ object, bool% shouldAdd);
    void BeforeDelLight(Object^ sender, Light^ object, bool% shouldDel);
    void OnDrawablesChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
    //void OnLightsChanged(Object^ sender, NotifyCollectionChangedEventArgs^ e);
    void OnLightPropertyChanged(Object^ sender, Light^ object, PropertyChangedEventArgs^ e);
internal:
    const dizz::RenderCore * const Core;
    const std::weak_ptr<dizz::Scene> *TheScene;
    Scene(const dizz::RenderCore * core);
    void PrepareScene();
    void RefreshScene();
public:
    static Scene();
    ~Scene() { this->!Scene(); }
    !Scene();

    initonly Camera^ MainCamera;
    initonly ObservableProxyContainer<Drawable^>^ Drawables;
    initonly ObservableProxyContainer<Light^>^ Lights;
};



}

