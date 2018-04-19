#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"
#include "OpenGLTypes.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace RayRender
{
using Basic3D::Vec3F;

inline String^ GetTexHolderName(const rayr::PBRMaterial::TexHolder& holder)
{
    const auto name = rayr::PBRMaterial::GetName(holder);
    return name.empty() ? "(None)" : ToStr(name);
}

public ref class PBRMaterial : public BaseViewModel
{
private:
    std::weak_ptr<rayr::Drawable>& Drawable;
    rayr::PBRMaterial& Material;
    void RefreshMaterial()
    {
        const auto d = Drawable.lock();
        d->AssignMaterial();
    }
internal:
    PBRMaterial(std::weak_ptr<rayr::Drawable>* drawable, rayr::PBRMaterial& material) : Drawable(*drawable), Material(material) {}
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
    property String^ AlbedoMap
    {
        String^ get() { return GetTexHolderName(Material.DiffuseMap); }
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
    property String^ NormalMap
    {
        String^ get() { return GetTexHolderName(Material.NormalMap); }
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

public ref class Drawable : public BaseViewModel
{
internal:
    std::weak_ptr<rayr::Drawable> *drawable;
    initonly String^ type;
    initonly List<PBRMaterial^>^ materials;
    Drawable(const Wrapper<rayr::Drawable>& obj);
public:
    ~Drawable() { this->!Drawable(); }
    !Drawable() { delete drawable; }
    property String^ Name
    {
        String^ get() { return ToStr(drawable->lock()->Name); }
        void set(String^ value) { drawable->lock()->Name = ToU16Str(value); OnPropertyChanged("Name"); }
    }
    property Vec3F Position
    {
        Vec3F get() { return Vec3F(drawable->lock()->position); }
        void set(Vec3F value) { value.Store(drawable->lock()->position); OnPropertyChanged("Position"); }
    }
    property Vec3F Rotation
    {
        Vec3F get() { return Vec3F(drawable->lock()->rotation); }
        void set(Vec3F value) { value.Store(drawable->lock()->rotation); OnPropertyChanged("Rotation"); }
    }
    property bool ShouldRender
    {
        bool get() { return drawable->lock()->ShouldRender; }
        void set(bool value) { drawable->lock()->ShouldRender = value; OnPropertyChanged("ShouldRender"); }
    }
    CLI_READONLY_PROPERTY(String^, Type, type);
    CLI_READONLY_PROPERTY(List<PBRMaterial^>^, Materials, materials);

    virtual String^ ToString() override
    {
        return "[" + type + "]" + Name;
    }
    void Move(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Move(dx, dy, dz);
        OnPropertyChanged("Position");
    }
    void Rotate(const float dx, const float dy, const float dz)
    {
        drawable->lock()->Rotate(dx, dy, dz);
        OnPropertyChanged("Rotation");
    }
};


generic<typename T>
public delegate void ObjectChangedEventHandler(Object^ sender, T obj);

template<typename Type, typename CLIType, typename ContainerType = vector<Type>>
public ref class HolderBase
{
protected:
    rayr::BasicTest * const Core;
    const ContainerType& Src;
public:
    initonly List<CLIType^>^ Container;
protected:
    initonly PropertyChangedEventHandler^ PChangedHandler;
    ObjectChangedEventHandler<CLIType^>^ changed;
    void virtual OnPChangded(Object ^sender, PropertyChangedEventArgs ^e)
    {
        Changed(this, safe_cast<CLIType^>(sender));
    }
    void RemoveHandler(CLIType^ obj)
    {
        obj->PropertyChanged -= PChangedHandler;
    }
    CLIType^ CreateObject(const Type& src)
    {
        auto obj = gcnew CLIType(src);
        obj->PropertyChanged += PChangedHandler;
        return obj;
    }
    HolderBase(rayr::BasicTest * const core, const ContainerType& src)
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
protected:
    void virtual OnPChangded(Object ^sender, PropertyChangedEventArgs ^e) override
    {
        if (e->PropertyName != "Name")
            Core->ReportChanged(rayr::ChangableUBO::Light);
        HolderBase<Wrapper<b3d::Light>, Basic3D::Light>::OnPChangded(sender, e);
    }
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

public ref class ShaderHolder : public HolderBase<oglu::oglProgram, OpenGLUtil::GLProgram, std::set<oglu::oglProgram>>
{
internal:
    ShaderHolder(rayr::BasicTest * const core, const std::set<oglu::oglProgram>& progs)
        : HolderBase<oglu::oglProgram, OpenGLUtil::GLProgram, std::set<oglu::oglProgram>>(core, progs)
    { }
    bool AddShader(CLIWrapper<oglu::oglProgram> theShader);
public:
    property List<OpenGLUtil::GLProgram^>^ Shaders
    {
        List<OpenGLUtil::GLProgram^>^ get() { return Container; }
    }
    OpenGLUtil::GLProgram^ GetCurrent()
    { 
        for each(OpenGLUtil::GLProgram^ shader in Container)
        {
            if (shader->prog->lock() == Core->Cur3DProg())
                return shader;
        }
        return nullptr;
    }
    Task<bool>^ AddShaderAsync(String^ fname, String^ shaderName);
    void UseShader(OpenGLUtil::GLProgram^ shader);
};

public ref class BasicTest : public BaseViewModel
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
    initonly ShaderHolder^ Shaders;

    property OpenGLUtil::FaceCullingType FaceCulling
    {
        OpenGLUtil::FaceCullingType get() { return (OpenGLUtil::FaceCullingType)core->GetContext()->GetFaceCulling(); }
        void set(OpenGLUtil::FaceCullingType value) { core->GetContext()->SetFaceCulling((oglu::FaceCullingType)value); OnPropertyChanged("FaceCulling"); }
    }
    property OpenGLUtil::DepthTestType DepthTesting
    {
        OpenGLUtil::DepthTestType get() { return (OpenGLUtil::DepthTestType)core->GetContext()->GetDepthTest(); }
        void set(OpenGLUtil::DepthTestType value) { core->GetContext()->SetDepthTest((oglu::DepthTestType)value); OnPropertyChanged("DepthTesting"); }
    }

    void Draw();
    void Resize(const int w, const int h);

    void ReLoadCL(String^ fname);
    Task<bool>^ ReloadCLAsync(String^ fname);
};



}

