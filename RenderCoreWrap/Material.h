#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.hpp"

using namespace System;
using namespace System::Collections::Generic;
using System::Windows::Media::Imaging::BitmapSource;


namespace RayRender
{


public ref class TexMap
{
private:
    rayr::PBRMaterial::TexHolder& Holder;
    BitmapSource^ thumbnail;
    String^ name;
    String^ description;
public:
    TexMap(rayr::PBRMaterial::TexHolder& holder);
    property String^ Name
    {
        String^ get() { return name; }
    }
    property String^ Description
    {
        String^ get() { return description; }
    }
    property BitmapSource^ Thumbnail
    {
        BitmapSource^ get() { return thumbnail; }
    }
};

public ref class PBRMaterial : public BaseViewModel
{
private:
    std::weak_ptr<rayr::Drawable>& Drawable;
    rayr::PBRMaterial& Material;
    TexMap ^albedoMap, ^normalMap;
    void RefreshMaterial();
internal:
    PBRMaterial(std::weak_ptr<rayr::Drawable>* drawable, rayr::PBRMaterial& material);
public:
    property String^ Name
    {
        String^ get() { return ToStr(Material.Name); }
        void set(String^ value) { Material.Name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    property System::Windows::Media::Color Albedo
    {
        System::Windows::Media::Color get() { return ToColor(Material.Albedo); }
        void set(System::Windows::Media::Color value)
        {
            FromColor(value, Material.Albedo);
            OnPropertyChanged("Albedo"); RefreshMaterial();
        }
    }
    property TexMap^ AlbedoMap
    {
        TexMap^ get() { return albedoMap; }
    }
    property bool IsMappedAlbedo
    {
        bool get() { return Material.UseDiffuseMap; }
        void set(bool value)
        {
            Material.UseDiffuseMap = value;
            OnPropertyChanged("IsMappedAlbedo"); RefreshMaterial();
        }
    }
    property TexMap^ NormalMap
    {
        TexMap^ get() { return normalMap; }
    }
    property bool IsMappedNormal
    {
        bool get() { return Material.UseNormalMap; }
        void set(bool value)
        {
            Material.UseNormalMap = value;
            OnPropertyChanged("IsMappedNormal"); RefreshMaterial();
        }
    }
    property float Metallic
    {
        float get() { return Material.Metalness; }
        void set(float value) { Material.Metalness = value; OnPropertyChanged("Metallic"); RefreshMaterial(); }
    }
    property float Roughness
    {
        float get() { return Material.Roughness; }
        void set(float value) { Material.Roughness = value; OnPropertyChanged("Roughness"); RefreshMaterial(); }
    }
    property float AO
    {
        float get() { return Material.AO; }
        void set(float value) { Material.AO = value; OnPropertyChanged("AO"); RefreshMaterial(); }
    }
    virtual String^ ToString() override
    {
        return Name;
    }
};

}
