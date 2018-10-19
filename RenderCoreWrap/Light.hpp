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
    Light(const Wrapper<rayr::Light>& obj) : light(new std::shared_ptr<rayr::Light>(obj)), type(LightType(obj->Type)) { }
public:
    ~Light() { this->!Light(); }
    !Light() { delete light; }
    property String^ Name
    {
        String^ get() { return ToStr((*light)->Name); }
        void set(String^ value) { (*light)->Name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    property Vector3 Position
    {
        Vector3 get() { return ToVector3((*light)->Position); }
        void set(Vector3 value) { StoreVector3(value, (*light)->Position); OnPropertyChanged("Position"); }
    }
    property Vector3 Direction
    {
        Vector3 get() { return ToVector3((*light)->Direction); }
        void set(Vector3 value) { StoreVector3(value, (*light)->Direction); OnPropertyChanged("Direction"); }
    }
    property bool IsOn
    {
        bool get() { return (*light)->IsOn; }
        void set(bool value) { (*light)->IsOn = value; OnPropertyChanged("IsOn"); }
    }
    property float AttenuationC
    {
        float get() { return (*light)->Attenuation.x; }
        void set(float value) { (*light)->Attenuation.x = value; OnPropertyChanged("AttenuationC"); }
    }
    property float Attenuation1
    {
        float get() { return (*light)->Attenuation.y; }
        void set(float value) { (*light)->Attenuation.y = value; OnPropertyChanged("Attenuation1"); }
    }
    property float Attenuation2
    {
        float get() { return (*light)->Attenuation.z; }
        void set(float value) { (*light)->Attenuation.z = value; OnPropertyChanged("Attenuation2"); }
    }
    property float Luminance
    {
        float get() { return (*light)->Attenuation.w; }
        void set(float value) { (*light)->Attenuation.w = value; OnPropertyChanged("Luminance"); }
    }
    property float CutoffInner
    {
        float get() { return (*light)->CutoffInner; }
        void set(float value) { (*light)->CutoffInner = value; OnPropertyChanged("CutoffInner"); }
    }
    property float CutoffOuter
    {
        float get() { return (*light)->CutoffOuter; }
        void set(float value) { (*light)->CutoffOuter = value; OnPropertyChanged("CutoffOuter"); }
    }
    property System::Windows::Media::Color Color
    {
        System::Windows::Media::Color get() { return ToColor((*light)->Color); }
        void set(System::Windows::Media::Color value)
        {
            FromColor(value, (*light)->Color);
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
