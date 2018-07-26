#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
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
    delete Lights;
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

void BasicTest::ResizeOffScreen(const uint32_t w, const uint32_t h)
{
    core->ResizeFBO(w, h);
}

void BasicTest::ReLoadCL(String^ fname)
{
    core->ReloadFontLoader(ToU16Str(fname));
}

Task<bool>^ BasicTest::ReloadCLAsync(String^ fname)
{
    return doAsync2<bool>(&rayr::BasicTest::ReloadFontLoaderAsync, core, ToU16Str(fname));
}

bool BasicTest::Screenshot(String^ fname)
{
    return false;
}

bool BasicTest::Screenshot(CLIWrapper<xziar::img::Image> theImg, Func<String^>^ fnameCallback)
{
    return false;
}

Task<bool>^ BasicTest::ScreenshotAsync(Func<String^>^ fnameCallback)
{
    /*gcnew Func<CLIWrapper<xziar::img::Image>, bool>::
    return doAsync3<bool>()*/
    return nullptr;
}

#pragma managed(push, off)
static Wrapper<b3d::Light> CreateLight(b3d::LightType type)
{
    Wrapper<b3d::Light> light;
    switch (type)
    {
    case b3d::LightType::Parallel:
        light = Wrapper<b3d::ParallelLight>(std::in_place);
        light->color = b3d::Vec4(1.0, 0.3, 0.3, 1.0);
        break;
    case b3d::LightType::Point:
        light = Wrapper<b3d::PointLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        break;
    case b3d::LightType::Spot:
        light = Wrapper<b3d::SpotLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 0.3, 1.0, 1.0);
        break;
    }
    light->direction = b3d::Vec4(0, 0, 1, 0);
    return light;
}
#pragma managed(pop)

void LightHolder::Add(Basic3D::LightType type)
{
    auto light = CreateLight((b3d::LightType)type);
    if (!light)
        return;
    bool ret = Core->AddLight(light);
    if (ret)
    {
        Lights->Add(CreateObject(light));
    }
}

void LightHolder::Clear()
{
    Core->DelAllLight();
    Lights->Clear();
}


bool DrawableHolder::AddModel(CLIWrapper<Wrapper<rayr::Model>> theModel)
{
    auto model = theModel.Extract();
    bool ret = Core->AddObject(model);
    if (ret)
    {
        Drawables->Add(CreateObject(model));
    }
    return ret;
}
Task<bool>^ DrawableHolder::AddModelAsync(String^ fname)
{
    return doAsync3<bool>(gcnew Func<CLIWrapper<Wrapper<rayr::Model>>, bool>(this, &DrawableHolder::AddModel), 
        &rayr::BasicTest::LoadModelAsync, Core, ToU16Str(fname));
}


bool ShaderHolder::AddShader(CLIWrapper<oglu::oglProgram> theShader)
{
    auto shader = theShader.Extract();
    if (Core->AddShader(shader))
    {
        Shaders->Add(CreateObject(shader));
        return true;
    }
    return false;
}

Task<bool>^ ShaderHolder::AddShaderAsync(String^ fname, String^ shaderName)
{
    return doAsync3<bool>(gcnew Func<CLIWrapper<oglu::oglProgram>, bool>(this, &ShaderHolder::AddShader),
        &rayr::BasicTest::LoadShaderAsync, Core, ToU16Str(fname), ToU16Str(shaderName));
}

void ShaderHolder::UseShader(OpenGLUtil::GLProgram^ shader)
{
    if (shader == nullptr) return;
    Core->ChangeShader(shader->prog->lock());
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