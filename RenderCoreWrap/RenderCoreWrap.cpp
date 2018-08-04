#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include "ImageUtil.h"
#include "common/CLIAsync.hpp"


namespace RayRender
{

using namespace common;

Drawable::Drawable(const Wrapper<rayr::Drawable>& obj) : drawable(new std::weak_ptr<rayr::Drawable>(obj)), type(ToStr(obj->GetType()))
{
    materials = gcnew List<PBRMaterial^>();
    for (auto& mat : obj->MaterialHolder)
    {
        materials->Add(gcnew PBRMaterial(drawable, mat));
    }
}

#pragma unmanaged
auto FindPath()
{
    fs::path shdpath(UTF16ER(__FILE__));
    return shdpath.parent_path().parent_path() / u"RenderCore";
}
#pragma managed

BasicTest::BasicTest()
{
    try
    {
        core = new rayr::BasicTest(FindPath());
        Camera = gcnew Basic3D::Camera(&core->cam);
        Lights = gcnew LightHolder(core, core->Lights());
        Drawables = gcnew DrawableHolder(core, core->Objects());
        Shaders = gcnew ShaderHolder(core, core->Shaders());
    }
    catch (BaseException& be)
    {
        Console::WriteLine(L"Initilize failed : {0}\n", ToStr(be.message));
        throw gcnew CPPException(be);
    }
}

BasicTest::!BasicTest()
{
    delete Shaders;
    delete Drawables;
    delete Lights;
    delete Camera;
    delete core;
}

void BasicTest::Draw()
{
    core->Draw();
}

void BasicTest::Resize(const uint32_t w, const uint32_t h)
{
    core->Resize(w, h);
}

void BasicTest::ResizeOffScreen(const uint32_t w, const uint32_t h, const bool isFloatDepth)
{
    core->ResizeFBO(w, h, isFloatDepth);
}

void BasicTest::ReLoadCL(String^ fname)
{
    core->ReloadFontLoader(ToU16Str(fname));
}

Task<bool>^ BasicTest::ReloadCLAsync(String^ fname)
{
    return doAsync2<bool>(&rayr::BasicTest::ReloadFontLoaderAsync, *core, ToU16Str(fname));
}

Action<String^>^ BasicTest::Screenshot()
{
    auto saver = gcnew XZiar::Img::ImageSaver(core->Scrrenshot());
    return gcnew Action<String^>(saver, &XZiar::Img::ImageSaver::Save);
}

void BasicTest::Save(String^ fname)
{
    core->Serialize(ToU16Str(fname));
    core->DeSerialize(ToU16Str(fname));
}

#pragma managed(push, off)
static Wrapper<rayr::Light> CreateLight(rayr::LightType type)
{
    Wrapper<rayr::Light> light;
    switch (type)
    {
    case rayr::LightType::Parallel:
        light = Wrapper<rayr::ParallelLight>(std::in_place);
        light->color = b3d::Vec4(1.0, 0.3, 0.3, 1.0);
        break;
    case rayr::LightType::Point:
        light = Wrapper<rayr::PointLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        break;
    case rayr::LightType::Spot:
        light = Wrapper<rayr::SpotLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 0.3, 1.0, 1.0);
        break;
    }
    light->direction = b3d::Vec4(0, 0, 1, 0);
    return light;
}
#pragma managed(pop)

void LightHolder::Add(RayRender::LightType type)
{
    auto light = CreateLight((rayr::LightType)type);
    if (!light)
        return;
    bool ret = Core.AddLight(light);
    if (ret)
    {
        Lights->Add(CreateObject(light));
    }
}

void LightHolder::Clear()
{
    Core.DelAllLight();
    Lights->Clear();
}


bool DrawableHolder::AddModel(CLIWrapper<Wrapper<rayr::Model>>^ theModel)
{
    auto model = theModel->Extract();
    bool ret = Core.AddObject(model);
    if (ret)
    {
        Drawables->Add(CreateObject(model));
    }
    return ret;
}
Task<bool>^ DrawableHolder::AddModelAsync(String^ fname)
{
    return doAsync3<bool>(gcnew Func<CLIWrapper<Wrapper<rayr::Model>>^, bool>(this, &DrawableHolder::AddModel), 
        &rayr::BasicTest::LoadModelAsync, Core, ToU16Str(fname));
}


bool ShaderHolder::AddShader(CLIWrapper<oglu::oglProgram>^ theShader)
{
    auto shader = theShader->Extract();
    if (Core.AddShader(shader))
    {
        Shaders->Add(CreateObject(shader));
        return true;
    }
    return false;
}

Task<bool>^ ShaderHolder::AddShaderAsync(String^ fname, String^ shaderName)
{
    return doAsync3<bool>(gcnew Func<CLIWrapper<oglu::oglProgram>^, bool>(this, &ShaderHolder::AddShader),
        &rayr::BasicTest::LoadShaderAsync, Core, ToU16Str(fname), ToU16Str(shaderName));
}

void ShaderHolder::UseShader(OpenGLUtil::GLProgram^ shader)
{
    if (shader == nullptr) return;
    Core.ChangeShader(shader->prog->lock());
}


static void DummyTemplateExport()
{
    {
        DrawableHolder^ dummy;
        dummy->GetIndex(nullptr);
    }
    {
        LightHolder^ dummy;
        dummy->GetIndex(nullptr);
    }
    {
        ShaderHolder^ dummy;
        dummy->GetIndex(nullptr);
    }
}


}