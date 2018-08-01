#pragma once
#include "RenderCoreWrapRely.h"
#include "b3d.hpp"

using namespace System;

namespace RayRender
{
using Basic3D::Vec3F;


public enum class LightType : int32_t
{
    Parallel = (int32_t)rayr::LightType::Parallel, Point = (int32_t)rayr::LightType::Point, Spot = (int32_t)rayr::LightType::Spot
};

public ref class Light : public BaseViewModel
{
internal:
    std::weak_ptr<rayr::Light> *light;
    initonly LightType type;
    Light(const Wrapper<rayr::Light>& obj) : light(new std::weak_ptr<rayr::Light>(obj)), type(LightType(obj->type)) { }
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
    property float CutoffInner
    {
        float get() { return light->lock()->cutoffInner; }
        void set(float value) { light->lock()->cutoffInner = value; OnPropertyChanged("CutoffInner"); }
    }
    property float CutoffOuter
    {
        float get() { return light->lock()->cutoffOuter; }
        void set(float value) { light->lock()->cutoffOuter = value; OnPropertyChanged("CutoffOuter"); }
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
