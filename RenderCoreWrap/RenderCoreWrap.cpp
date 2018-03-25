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

void BasicTest::Moveobj(const uint16_t id, const float x, const float y, const float z)
{
    core->Moveobj(id, x, y, z);
}

void BasicTest::Rotateobj(const uint16_t id, const float x, const float y, const float z)
{
    core->Rotateobj(id, x, y, z);
}

void BasicTest::ReLoadCL(String^ fname)
{
    core->ReloadFontLoader(ToU16Str(fname));
}

Task<bool>^ BasicTest::AddModelAsync(String^ fname)
{
    return doAsync2<bool>(&rayr::BasicTest::AddModelAsync, core, ToU16Str(fname));
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
}

void LightHolder::Clear()
{
    Core->DelAllLight();
}

}