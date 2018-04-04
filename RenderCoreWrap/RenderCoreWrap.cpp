#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include "common/CLIAsync.hpp"


namespace RayRender
{

using namespace common;


BasicTest::BasicTest()
{
    try
    {
        core = new rayr::BasicTest(u"", u"D:\\Programs Data\\VSProject\\RayRenderer\\RenderCore\\3d.glsl");
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

void BasicTest::Resize(const int w, const int h)
{
    core->Resize(w, h);
}

void BasicTest::ReLoadCL(String^ fname)
{
    core->ReloadFontLoader(ToU16Str(fname));
}

Task<bool>^ BasicTest::ReloadCLAsync(String^ fname)
{
    return doAsync2<bool>(&rayr::BasicTest::ReloadFontLoaderAsync, core, ToU16Str(fname));
}

Task<bool>^ BasicTest::TryAsync()
{
    return doAsync2<bool>(&rayr::BasicTest::tryAsync, core);
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