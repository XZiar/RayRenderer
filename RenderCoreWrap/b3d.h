#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;
using namespace System::ComponentModel;

namespace Basic3D
{
using namespace common;

public value struct Vec3F
{
    float x, y, z;
    Vec3F(float x_, float y_, float z_) : x(x_), y(y_), z(z_) { }
    virtual String^ ToString() override
    {
        return String::Format("{0:F3},{1:F3},{2:F3}", x, y, z);
    }
internal:
    Vec3F(const b3d::Vec3& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    Vec3F(const b3d::Vec4& vec) : x(vec.x), y(vec.y), z(vec.z) { }
    void Store(b3d::Vec3& vec) { vec.x = x; vec.y; vec.z = z; }
    void Store(b3d::Vec4& vec) { vec.x = x; vec.y; vec.z = z; }
};

public ref class Camera : BaseViewModel
{
internal:
    b3d::Camera *cam;
    bool isRef = false;
internal:
    Camera(b3d::Camera *obj) :cam(obj), isRef(true) { }
public:
    ~Camera() { this->!Camera(); }
    !Camera()
    {
        if (!isRef)
            delete cam;
    }

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
    property Vec3F Position
    {
        Vec3F get() { return Vec3F(cam->position); }
        void set(Vec3F value) { value.Store(cam->position); OnPropertyChanged("Position"); }
    }

    void Move(const float dx, const float dy, const float dz)
    {
        cam->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    //rotate along x-axis, radius
    void Pitch(const float radx)
    {
        cam->pitch(radx);
    }
    //rotate along y-axis, radius
    void Yaw(const float rady)
    {
        cam->yaw(rady);
    }
    //rotate along z-axis, radius
    void Roll(const float radz)
    {
        cam->roll(radz);
    }
};

public enum class LightType : int32_t { Parallel, Point, Spot };

public ref class Light : public BaseViewModel
{
internal:
    std::weak_ptr<b3d::Light> *light;
    initonly LightType type;
    Light(const Wrapper<b3d::Light> *obj) : light(new std::weak_ptr<b3d::Light>(*obj)), type(LightType((*obj)->type)) { }
public:
    ~Light() { this->!Light(); }
    !Light() { delete light; }
    property String^ Name
    {
        String^ get() { return ToStr(light->lock()->name); }
        void set(String^ value) { light->lock()->name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    property Vec3F Position
    {
        Vec3F get() { return Vec3F(light->lock()->position); }
        void set(Vec3F value) { value.Store(light->lock()->position); OnPropertyChanged("Position"); }
    }
    property Vec3F Direction
    {
        Vec3F get() { return Vec3F(light->lock()->direction); }
        void set(Vec3F value) { value.Store(light->lock()->direction); OnPropertyChanged("Direction"); }
    }
    property bool IsOn
    {
        bool get() { return light->lock()->isOn; }
        void set(bool value) { light->lock()->isOn = value; OnPropertyChanged("IsOn"); }
    }
    property float AttenuationC
    {
        float get() { return light->lock()->attenuation.x; }
        void set(float value) { light->lock()->attenuation.x = value; OnPropertyChanged("AttenuationC"); }
    }
    property float Attenuation1
    {
        float get() { return light->lock()->attenuation.y; }
        void set(float value) { light->lock()->attenuation.y = value; OnPropertyChanged("Attenuation1"); }
    }
    property float Attenuation2
    {
        float get() { return light->lock()->attenuation.z; }
        void set(float value) { light->lock()->attenuation.z = value; OnPropertyChanged("Attenuation2"); }
    }
    property float Luminance
    {
        float get() { return light->lock()->attenuation.w; }
        void set(float value) { light->lock()->attenuation.w = value; OnPropertyChanged("Luminance"); }
    }
    property System::Windows::Media::Color Color
    {
        System::Windows::Media::Color get() { return ToColor(light->lock()->color); }
        void set(System::Windows::Media::Color value)
        {
            FromColor(value, light->lock()->color);
            OnPropertyChanged("Color");
        }
    }
    CLI_READONLY_PROPERTY(LightType, Type, type);

    virtual String^ ToString() override
    {
        return "[" + type.ToString() + "]" + Name;
    }
    void Move(const float dx, const float dy, const float dz)
    {
        light->lock()->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        light->lock()->Rotate(dx, dy, dz);
        OnPropertyChanged("Direction");
    }
};

}