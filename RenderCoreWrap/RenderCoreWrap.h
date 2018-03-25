#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace RayRender
{

ref class BasicTest;

public ref class Drawable
{
private:
    Wrapper<rayr::Drawable> *drawable;
    bool isRef = false;
internal:
    Drawable(const Wrapper<rayr::Drawable> *obj) : drawable(new Wrapper<rayr::Drawable>(*obj)), isRef(true) { }
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable()
    {
        if (!isRef)
            delete drawable;
    }
    property String^ Name
    {
        String^ get() { return ToStr((*drawable)->name); }
    }
    property String^ Type
    {
        String^ get() { return ToStr((*drawable)->getType()); }
    }
};

public ref class LightHolder
{
private:
    const vector<Wrapper<b3d::Light>>& Src;
    List<Basic3D::Light^>^ Lights;
    rayr::BasicTest * const Core;
internal:
    LightHolder(rayr::BasicTest *core, const vector<Wrapper<b3d::Light>>& lights) : Core(core), Src(lights) {}
public:
    property Basic3D::Light^ default[int32_t]
    {
        Basic3D::Light^ get(int32_t i)
        {
            if (i < 0 || (uint32_t)i >= Src.size())
                return nullptr;
            return gcnew Basic3D::Light(&Src[i]);
        }
    };
    property int32_t Size
    {
        int32_t get() { return (int32_t)Src.size(); }
    }
    void Add(Basic3D::LightType type);
    void Clear();
};

public ref class DrawableHolder
{
private:
    const vector<Wrapper<rayr::Drawable>>& Src;
    rayr::BasicTest * const Core;
internal:
    DrawableHolder(rayr::BasicTest *core, const vector<Wrapper<rayr::Drawable>>& drawables) : Core(core), Src(drawables) {}
public:
    property Drawable^ default[int32_t]
    {
        Drawable^ get(int32_t i)
        {
            if (i < 0 || (uint32_t)i >= Src.size())
                return nullptr; //throw gcnew System::IndexOutOfRangeException();
            return gcnew Drawable(&Src[i]);
        }
    };
    property int32_t Size
    {
        int32_t get() { return (int32_t)Src.size(); }
    }
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

    Task<bool>^ AddModelAsync(String^ fname);
    Task<bool>^ ReloadCLAsync(String^ fname);
    Task<bool>^ TryAsync();

};

}
