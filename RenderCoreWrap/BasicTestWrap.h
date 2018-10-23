#pragma once

#include "RenderCoreWrapRely.h"
#include "Light.hpp"
#include "OpenGLTypes.h"
#include "Material.h"
#include "ControllableWrap.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace RayRender
{

public ref class Drawable : public BaseViewModel
{
internal:
    std::weak_ptr<rayr::Drawable> *drawable;
    initonly String^ type;
    initonly List<PBRMaterial^>^ materials;
    Drawable(const Wrapper<rayr::Drawable>& obj);
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable() { delete drawable; }
    property String^ Name
    {
        String^ get() { return ToStr(drawable->lock()->Name); }
        void set(String^ value) { drawable->lock()->Name = ToU16Str(value); RaisePropertyChanged("Name"); }
    }
    property Vector3 Position
    {
        Vector3 get() { return ToVector3(drawable->lock()->Position); }
        void set(Vector3 value) { StoreVector3(value, drawable->lock()->Position); RaisePropertyChanged("Position"); }
    }
    property Vector3 Rotation
    {
        Vector3 get() { return ToVector3(drawable->lock()->Rotation); }
        void set(Vector3 value) { StoreVector3(value, drawable->lock()->Rotation); RaisePropertyChanged("Rotation"); }
    }
    property bool ShouldRender
    {
        bool get() { return drawable->lock()->ShouldRender; }
        void set(bool value) { drawable->lock()->ShouldRender = value; RaisePropertyChanged("ShouldRender"); }
    }
    CLI_READONLY_PROPERTY(String^, Type, type);
    CLI_READONLY_PROPERTY(List<PBRMaterial^>^, Materials, materials);

    virtual String^ ToString() override
    {
        return "[" + type + "]" + Name;
    }
    void Move(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Move(dx, dy, dz);
        RaisePropertyChanged("Position");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Rotate(dx, dy, dz);
        RaisePropertyChanged("Rotation");
    }
};


generic<typename T>
public delegate void ObjectChangedEventHandler(Object^ sender, T obj);

template<typename Type, typename CLIType, typename ContainerType = vector<Type>>
public ref class HolderBase
{
protected:
    rayr::BasicTest& Core;
    const ContainerType& Src;
public:
    initonly List<CLIType^>^ Container;
protected:
    initonly PropertyChangedEventHandler^ PChangedHandler;
    ObjectChangedEventHandler<CLIType^>^ changed;
    void virtual OnPChangded(Object ^sender, PropertyChangedEventArgs ^e)
    {
        Changed(this, safe_cast<CLIType^>(sender));
    }
    void RemoveHandler(CLIType^ obj)
    {
        obj->PropertyChanged -= PChangedHandler;
    }
    CLIType^ CreateObject(const Type& src)
    {
        auto obj = gcnew CLIType(src);
        obj->PropertyChanged += PChangedHandler;
        return obj;
    }
    HolderBase(rayr::BasicTest * const core, const ContainerType& src)
        : Core(*core), Src(src), Container(gcnew List<CLIType^>()), PChangedHandler(gcnew PropertyChangedEventHandler(this, &HolderBase::OnPChangded)) 
    {
        Refresh();
    }
public:
    event ObjectChangedEventHandler<CLIType^>^ Changed
    {
        void add(ObjectChangedEventHandler<CLIType^>^ handler)
        {
            changed += handler;
        }
        void remove(ObjectChangedEventHandler<CLIType^>^ handler)
        {
            changed -= handler;
        }
        void raise(Object^ sender, CLIType^ obj)
        {
            auto handler = changed;
            if (handler == nullptr)
                return;
            handler->Invoke(sender, obj);
        }
    }
    property CLIType^ default[int32_t]
    {
        CLIType^ get(int32_t i)
        {
            if (i < 0 || i >= Container->Count)
                return nullptr;
            return Container[i];
        }
    };
    uint16_t GetIndex(CLIType^ obj)
    {
        auto idx = Container->IndexOf(obj);
        return idx < 0 ? UINT16_MAX : (uint16_t)idx;
    }
    property int32_t Size
    {
        int32_t get() { return Container->Count; }
    }
    void Refresh()
    {
        for each(CLIType^ obj in Container)
        {
            RemoveHandler(obj);
        }
        Container->Clear();
        for (const auto& obj : Src)
        {
            Container->Add(CreateObject(obj));
        }
    }
};

public ref class LightHolder : public HolderBase<Wrapper<rayr::Light>, Light>
{
internal:
    LightHolder(rayr::BasicTest * const core, const vector<Wrapper<rayr::Light>>& lights)
        : HolderBase<Wrapper<rayr::Light>, Light>(core, lights)
    { }
protected:
    void virtual OnPChangded(Object ^sender, PropertyChangedEventArgs ^e) override
    {
        if (e->PropertyName != "Name")
            Core.ReportChanged(rayr::ChangableUBO::Light);
        HolderBase<Wrapper<rayr::Light>, Light>::OnPChangded(sender, e);
    }
public:
    property List<Light^>^ Lights
    {
        List<Light^>^ get() { return Container; }
    }
    void Add(LightType type);
    void Clear();
};

public ref class DrawableHolder : public HolderBase<Wrapper<rayr::Drawable>, Drawable>
{
internal:
    DrawableHolder(rayr::BasicTest *core, const vector<Wrapper<rayr::Drawable>>& drawables)
        : HolderBase<Wrapper<rayr::Drawable>, Drawable>(core, drawables)
    { }
public:
    property List<Drawable^>^ Drawables
    {
        List<Drawable^>^ get() { return Container; }
    }
    Task<bool>^ AddModelAsync(String^ fname);
internal:
    bool AddModel(CLIWrapper<Wrapper<rayr::Model>>^ theModel);
};

public ref class ShaderHolder : public HolderBase<oglu::oglDrawProgram, OpenGLUtil::GLProgram, std::set<oglu::oglDrawProgram>>
{
internal:
    ShaderHolder(rayr::BasicTest * const core, const std::set<oglu::oglDrawProgram>& progs)
        : HolderBase<oglu::oglDrawProgram, OpenGLUtil::GLProgram, std::set<oglu::oglDrawProgram>>(core, progs)
    { }
    bool AddShader(CLIWrapper<oglu::oglDrawProgram>^ theShader);
public:
    property List<OpenGLUtil::GLProgram^>^ Shaders
    {
        List<OpenGLUtil::GLProgram^>^ get() { return Container; }
    }
    OpenGLUtil::GLProgram^ GetCurrent()
    { 
        for each(OpenGLUtil::GLProgram^ shader in Container)
        {
            if (shader->prog->lock() == Core.Cur3DProg())
                return shader;
        }
        return nullptr;
    }
    Task<bool>^ AddShaderAsync(String^ fname, String^ shaderName);
    void UseShader(OpenGLUtil::GLProgram^ shader);
};


public ref class CameraHolder : BaseViewModel
{
internal:
    rayr::Camera *cam;
    bool isRef = false;
internal:
    CameraHolder(rayr::Camera *obj) :cam(obj), isRef(true) { }
public:
    ~CameraHolder() { this->!CameraHolder(); }
    !CameraHolder()
    {
        if (!isRef)
            delete cam;
    }

    property Vector3 Position
    {
        Vector3 get() { return ToVector3(cam->Position); }
        void set(Vector3 value) { StoreVector3(value, cam->Position); RaisePropertyChanged("Position"); }
    }
    property Vector3 Direction
    {
        Vector3 get() { return ToVector3(cam->Rotation); }
        void set(Vector3 value) { StoreVector3(value, cam->Rotation); RaisePropertyChanged("Direction"); }
    }

    void Move(const float dx, const float dy, const float dz)
    {
        cam->Move(dx, dy, dz);
        RaisePropertyChanged("Position");
    }
    //rotate along x-axis, radius
    void Pitch(const float radx)
    {
        cam->Pitch(radx);
        RaisePropertyChanged("Direction");
    }
    //rotate along y-axis, radius
    void Yaw(const float rady)
    {
        cam->Yaw(rady);
        RaisePropertyChanged("Direction");
    }
    //rotate along z-axis, radius
    void Roll(const float radz)
    {
        cam->Roll(radz);
        RaisePropertyChanged("Direction");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        cam->Rotate(dx, dy, dz);
        RaisePropertyChanged("Direction");
    }
};


public ref class BasicTest : public BaseViewModel
{
private:
    rayr::BasicTest* core;
    Common::Controllable^ AddShader(CLIWrapper<Wrapper<rayr::GLShader>>^ theShader);
internal:
public:
    BasicTest();
    ~BasicTest() { this->!BasicTest(); }
    !BasicTest();

    property bool Mode
    {
        bool get() { return core->mode; }
        void set(bool value) { core->mode = value; }
    }

    property uint32_t Width
    {
        uint32_t get() { return core->WindowWidth; }
    }
    property uint32_t Height
    {
        uint32_t get() { return core->WindowHeight; }
    }

    initonly CameraHolder^ Camera;
    initonly LightHolder^ Lights;
    initonly DrawableHolder^ Drawables;
    initonly ShaderHolder^ Shaders;
    initonly Common::Controllable^ PostProc;
    initonly Common::Controllable^ FontView;
    initonly List<Common::Controllable^>^ GLShaders;

    property OpenGLUtil::FaceCullingType FaceCulling
    {
        OpenGLUtil::FaceCullingType get() { return (OpenGLUtil::FaceCullingType)core->GetContext()->GetFaceCulling(); }
        void set(OpenGLUtil::FaceCullingType value) { core->GetContext()->SetFaceCulling((oglu::FaceCullingType)value); RaisePropertyChanged("FaceCulling"); }
    }
    property OpenGLUtil::DepthTestType DepthTesting
    {
        OpenGLUtil::DepthTestType get() { return (OpenGLUtil::DepthTestType)core->GetContext()->GetDepthTest(); }
        void set(OpenGLUtil::DepthTestType value) { core->GetContext()->SetDepthTest((oglu::DepthTestType)value); RaisePropertyChanged("DepthTesting"); }
    }

    void Draw();
    void Resize(const uint32_t w, const uint32_t h);
    void ResizeOffScreen(const uint32_t w, const uint32_t h, const bool isFloatDepth);

    void ReLoadCL(String^ fname);
    Task<bool>^ ReloadCLAsync(String^ fname);
    Task<Common::Controllable^>^ AddShaderAsync(String^ fname, String^ shaderName);

    Action<String^>^ Screenshot();
    void Serialize(String^ fname);
    void DeSerialize(String^ fname);

};



}

