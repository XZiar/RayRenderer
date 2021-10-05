#pragma once

#include "RenderCoreWrapRely.h"


using namespace System;
using namespace System::Collections::Generic;
using System::Windows::Media::Imaging::BitmapSource;


namespace DizzRender
{


public ref class TexMap
{
private:
    dizz::TexHolder& Holder;
    BitmapSource^ thumbnail;
    String^ name;
    String^ description;
public:
    TexMap(dizz::TexHolder& holder, const std::shared_ptr<dizz::ThumbnailManager>& thumbman);
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
    std::weak_ptr<dizz::Drawable>& Drawable;
    dizz::PBRMaterial& Material;
    TexMap ^albedoMap, ^normalMap;
    void RefreshMaterial();
internal:
    PBRMaterial(std::weak_ptr<dizz::Drawable>* drawable, dizz::PBRMaterial& material, const std::shared_ptr<dizz::ThumbnailManager>& thumbman);
public:
    property String^ Name
    {
        String^ get() { return ToStr(Material.Name); }
        void set(String^ value) { Material.Name = ToU16Str(value); RaisePropertyChanged("Name"); }
    }
    property System::Windows::Media::Color Albedo
    {
        System::Windows::Media::Color get() { return ToColor(Material.Albedo); }
        void set(System::Windows::Media::Color value)
        {
            FromColor(value, Material.Albedo);
            RaisePropertyChanged("Albedo"); RefreshMaterial();
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
            RaisePropertyChanged("IsMappedAlbedo"); RefreshMaterial();
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
            RaisePropertyChanged("IsMappedNormal"); RefreshMaterial();
        }
    }
    property float Metallic
    {
        float get() { return Material.Metalness; }
        void set(float value) { Material.Metalness = value; RaisePropertyChanged("Metallic"); RefreshMaterial(); }
    }
    property float Roughness
    {
        float get() { return Material.Roughness; }
        void set(float value) { Material.Roughness = value; RaisePropertyChanged("Roughness"); RefreshMaterial(); }
    }
    property float AO
    {
        float get() { return Material.AO; }
        void set(float value) { Material.AO = value; RaisePropertyChanged("AO"); RefreshMaterial(); }
    }
    virtual String^ ToString() override
    {
        return Name;
    }
};

}
