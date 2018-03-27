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
    void Move(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Move(dx, dy, dz);
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Rotate(dx, dy, dz);
    }
};


generic<typename T>
public delegate void ObjectChangedEventHandler(Object^ sender, T obj);

template<typename Type, typename CLIType>
public ref class HolderBase
{
protected:
    rayr::BasicTest * const Core;
    const vector<Type>& Src;
    initonly List<CLIType^>^ Container;
    initonly PropertyChangedEventHandler^ PChangedHandler;
    ObjectChangedEventHandler<CLIType^>^ changed;
    void OnPChangded(Object ^sender, PropertyChangedEventArgs ^e)
    {
        Changed(this, safe_cast<CLIType^>(sender));
    }
    void RemoveHandler(CLIType^ obj)
    {
        obj->PropertyChanged -= PChangedHandler;
    }
    CLIType^ CreateObject(const Type& src)
    {
        auto obj = gcnew CLIType(&src);
        obj->PropertyChanged += PChangedHandler;
        return obj;
    }
    HolderBase(rayr::BasicTest * const core, const vector<Type>& src)
        : Core(core), Src(src), Container(gcnew List<CLIType^>()), PChangedHandler(gcnew PropertyChangedEventHandler(this, &HolderBase::OnPChangded)) 
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

public ref class LightHolder : public HolderBase<Wrapper<b3d::Light>, Basic3D::Light>
{
internal:
    LightHolder(rayr::BasicTest * const core, const vector<Wrapper<b3d::Light>>& lights) 
        : HolderBase<Wrapper<b3d::Light>, Basic3D::Light>(core, lights)
    { }
public:
    property List<Basic3D::Light^>^ Lights
    {
        List<Basic3D::Light^>^ get() { return Container; }
    }
    void Add(Basic3D::LightType type);
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
    bool AddModel(CLIWrapper<Wrapper<rayr::Model>> theModel);
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

    void ReLoadCL(String^ fname);

    Task<bool>^ ReloadCLAsync(String^ fname);
    Task<bool>^ TryAsync();

};



}

