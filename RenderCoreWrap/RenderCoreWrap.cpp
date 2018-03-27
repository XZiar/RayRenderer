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
        core = new rayr::BasicTest();
        Camera = gcnew Basic3D::Camera(&core->cam);
        Lights = gcnew LightHolder(core, core->Lights());
        Drawables = gcnew DrawableHolder(core, core->Objects());
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

void LightHolder::Add(Basic3D::LightType type)
{
    Core->AddLight((b3d::LightType)type);
    Refresh();
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
}


}