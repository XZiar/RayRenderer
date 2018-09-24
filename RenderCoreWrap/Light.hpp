#pragma once
#include "RenderCoreWrapRely.h"


using namespace System;

#define TryGetFromWeak(weak, func, def) if (const auto obj_ = weak->lock()) { return func; } else { return def; }
#define TryDoWithWeak(weak, func) if (const auto obj_ = weak->lock()) { func; }

namespace RayRender
{


public enum class LightType : int32_t
{
    Parallel = (int32_t)rayr::LightType::Parallel, Point = (int32_t)rayr::LightType::Point, Spot = (int32_t)rayr::LightType::Spot
};

public ref class Light : public BaseViewModel
{
internal:
    std::shared_ptr<rayr::Light> *light;
    initonly LightType type;
    Light(const Wrapper<rayr::Light>& obj) : light(new std::shared_ptr<rayr::Light>(obj)), type(LightType(obj->type)) { }
public:
    ~Light() { this->!Light(); }
    !Light() { delete light; }
    property String^ Name
    {
        String^ get() { return ToStr((*light)->name); }
        void set(String^ value) { (*light)->name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    property Vector3 Position
    {
        Vector3 get() { return ToVector3((*light)->position); }
        void set(Vector3 value) { StoreVector3(value, (*light)->position); OnPropertyChanged("Position"); }
    }
    property Vector3 Direction
    {
        Vector3 get() { return ToVector3((*light)->direction); }
        void set(Vector3 value) { StoreVector3(value, (*light)->direction); OnPropertyChanged("Direction"); }
    }
    property bool IsOn
    {
        bool get() { return (*light)->isOn; }
        void set(bool value) { (*light)->isOn = value; OnPropertyChanged("IsOn"); }
    }
    property float AttenuationC
    {
        float get() { return (*light)->attenuation.x; }
        void set(float value) { (*light)->attenuation.x = value; OnPropertyChanged("AttenuationC"); }
    }
    property float Attenuation1
    {
        float get() { return (*light)->attenuation.y; }
        void set(float value) { (*light)->attenuation.y = value; OnPropertyChanged("Attenuation1"); }
    }
    property float Attenuation2
    {
        float get() { return (*light)->attenuation.z; }
        void set(float value) { (*light)->attenuation.z = value; OnPropertyChanged("Attenuation2"); }
    }
    property float Luminance
    {
        float get() { return (*light)->attenuation.w; }
        void set(float value) { (*light)->attenuation.w = value; OnPropertyChanged("Luminance"); }
    }
    property float CutoffInner
    {
        float get() { return (*light)->cutoffInner; }
        void set(float value) { (*light)->cutoffInner = value; OnPropertyChanged("CutoffInner"); }
    }
    property float CutoffOuter
    {
        float get() { return (*light)->cutoffOuter; }
        void set(float value) { (*light)->cutoffOuter = value; OnPropertyChanged("CutoffOuter"); }
    }
    property System::Windows::Media::Color Color
    {
        System::Windows::Media::Color get() { return ToColor((*light)->color); }
        void set(System::Windows::Media::Color value)
        {
            FromColor(value, (*light)->color);
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
        (*light)->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        (*light)->Rotate(dx, dy, dz);
        OnPropertyChanged("Direction");
    }
};


}
