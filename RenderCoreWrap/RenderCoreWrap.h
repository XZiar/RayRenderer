#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"
#include <atomic>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace RayRender
{
public delegate void ObjectChangedEventHandler(Object^ o, Object^ obj);

ref class BasicTest;

public ref class Drawable : public BaseViewModel
{
internal:
    std::weak_ptr<rayr::Drawable> *drawable;
internal:
    Drawable(const Wrapper<rayr::Drawable> *obj) : drawable(new std::weak_ptr<rayr::Drawable>(*obj)), Type(ToStr((*obj)->getType())) { }
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable() { delete drawable; }
    property String^ Name
    {
        String^ get() { return ToStr(drawable->lock()->name); }
        void set(String^ value) { drawable->lock()->name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    initonly String^ Type;
    virtual String^ ToString() override
    {
        return "[" + Type + "]" + Name;
    }
};


public ref class LightHolder
{
private:
    const vector<Wrapper<b3d::Light>>& Src;
    rayr::BasicTest * const Core;
    initonly List<Basic3D::Light^>^ Lights;
    initonly PropertyChangedEventHandler^ PChangedHandler;
    void OnPChangded(Object ^sender, PropertyChangedEventArgs ^e)
    {
        Changed(this, sender);
    }
internal:
    LightHolder(rayr::BasicTest *core, const vector<Wrapper<b3d::Light>>& lights) 
        : Core(core), Src(lights), Lights(gcnew List<Basic3D::Light^>()), PChangedHandler(gcnew PropertyChangedEventHandler(this, &LightHolder::OnPChangded))
    {
        Refresh();
    }
public:
    event ObjectChangedEventHandler^ Changed;
    property Basic3D::Light^ default[int32_t]
    {
        Basic3D::Light^ get(int32_t i)
        {
            if (i < 0 || i >= Lights->Count)
                return nullptr;
            return Lights[i];
        }
    };
    uint16_t GetIndex(Basic3D::Light^ light)
    {
        auto idx = Lights->IndexOf(light);
        return idx < 0 ? UINT16_MAX : (uint16_t)idx;
    }
    property int32_t Size
    {
        int32_t get() { return Lights->Count; }
    }
    void Refresh()
    {
        Lights->Clear();
        for (const auto& obj : Src)
        {
            auto lgt = gcnew Basic3D::Light(&obj);
            lgt->PropertyChanged += PChangedHandler;
            Lights->Add(lgt);
        }
    }
    void Add(Basic3D::LightType type);
    void Clear();
    void OnPropertyChanged(System::Object ^sender, System::ComponentModel::PropertyChangedEventArgs ^e);
};

public ref class DrawableHolder
{
private:
    const vector<Wrapper<rayr::Drawable>>& Src;
    rayr::BasicTest * const Core;
    initonly List<Drawable^>^ Drawables;
    initonly PropertyChangedEventHandler^ PChangedHandler;
    void OnPChangded(Object ^sender, PropertyChangedEventArgs ^e)
    {
        Changed(this, sender);
    }
internal:
    DrawableHolder(rayr::BasicTest *core, const vector<Wrapper<rayr::Drawable>>& drawables)
        : Core(core), Src(drawables), Drawables(gcnew List<Drawable^>()), PChangedHandler(gcnew PropertyChangedEventHandler(this, &DrawableHolder::OnPChangded))
    {
        Refresh();
    }
public:
    event ObjectChangedEventHandler^ Changed;
    property Drawable^ default[int32_t]
    {
        Drawable^ get(int32_t i)
        {
            if (i < 0 || i >= Drawables->Count)
                return nullptr;
            return Drawables[i];
        }
    };
    uint16_t GetIndex(Drawable^ drawable)
    {
        auto idx = Drawables->IndexOf(drawable);
        return idx < 0 ? UINT16_MAX : (uint16_t)idx;
    }
    property int32_t Size
    {
        int32_t get() { return Drawables->Count; }
    }
    void Refresh()
    {
        Drawables->Clear();
        for (const auto& obj : Src)
        {
            auto drawable = gcnew Drawable(&obj);
            drawable->PropertyChanged += PChangedHandler;
            Drawables->Add(drawable);
        }
    }
    Task<bool>^ AddModelAsync(String^ fname);
};

public ref class BasicTest
{
private:
    rayr::BasicTest *core;
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

    initonly Basic3D::Camera^ Camera;
    initonly LightHolder^ Lights;
    initonly DrawableHolder^ Drawables;

    void Draw();
    void Resize(const int w, const int h);
    void Moveobj(const uint16_t id, const float x, const float y, const float z);
    void Rotateobj(const uint16_t id, const float x, const float y, const float z);

    void ReLoadCL(String^ fname);

    Task<bool>^ ReloadCLAsync(String^ fname);
    Task<bool>^ TryAsync();

};

}


void RayRender::LightHolder::OnPropertyChanged(System::Object ^sender, System::ComponentModel::PropertyChangedEventArgs ^e)
{
    throw gcnew System::NotImplementedException();
}
