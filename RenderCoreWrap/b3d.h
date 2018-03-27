#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;
using namespace System::ComponentModel;

namespace Basic3D
{

using namespace common;

public ref class Camera
{
internal:
    b3d::Camera *cam;
    bool isRef = false;
internal:
    Camera(b3d::Camera *obj) :cam(obj), isRef(true) { }
public:
    property int Width
    {
        int get() { return cam->width; }
        void set(int value) { cam->width = value; }
    }
    property int Height
    {
        int get() { return cam->height; }
        void set(int value) { cam->height = value; }
    }
    ~Camera() { this->!Camera(); }

    !Camera()
    {
        if (!isRef)
            delete cam;
    }

    void Move(const float dx, const float dy, const float dz)
    {
        cam->Move(dx, dy, dz);
    }
    //rotate along x-axis
    void Pitch(const float angx)
    {
        cam->pitch(angx);
    }
    //rotate along y-axis
    void Yaw(const float angy)
    {
        cam->yaw(angy);
    }
    //rotate along z-axis
    void Roll(const float angz)
    {
        cam->roll(angz);
    }
};

public enum class LightType : int32_t { Parallel, Point, Spot };

public ref class Light : public BaseViewModel
{
internal:
    std::weak_ptr<b3d::Light> *light;
internal:
    Light(const Wrapper<b3d::Light> *obj) : light(new std::weak_ptr<b3d::Light>(*obj)), Type(LightType((*obj)->type)) { }
public:
    ~Light() { this->!Light(); }
    !Light() { delete light; }
    property String^ Name
    {
        String^ get() { return ToStr(light->lock()->name); }
        void set(String^ value) { light->lock()->name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    initonly LightType Type;
    virtual String^ ToString() override
    {
        return "[" + Type.ToString() + "]" + Name;
    }
    void Move(const float dx, const float dy, const float dz)
    {
        light->lock()->Move(dx, dy, dz);
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        light->lock()->Rotate(dx, dy, dz);
    }
};

}