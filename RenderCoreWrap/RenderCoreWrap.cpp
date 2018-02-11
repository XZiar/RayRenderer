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
		cam_ = gcnew Basic3D::Camera(&core->cam);
		Lights = gcnew LightHolder(core, core->light());
	}
	catch (BaseException& be)
	{
		Console::WriteLine(L"Initilize failed : {0}\n", marshal_as<String^>(be.message));
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
	core->draw();
}

void BasicTest::Resize(const int w, const int h)
{
	core->resize(w, h);
}

void BasicTest::Moveobj(const uint16_t id, const float x, const float y, const float z)
{
	core->moveobj(id, x, y, z);
}

void BasicTest::Rotateobj(const uint16_t id, const float x, const float y, const float z)
{
	core->rotateobj(id, x, y, z);
}

void BasicTest::ReLoadCL(String^ fname)
{
	core->reloadFontLoader(msclr::interop::marshal_as<std::wstring>(fname));
}

Task<bool>^ BasicTest::AddModelAsync(String^ fname)
{
	return doAsync2<bool>(&rayr::BasicTest::addModelAsync, core, msclr::interop::marshal_as<std::wstring>(fname));
}


Task<bool>^ BasicTest::ReloadCLAsync(String^ fname)
{
	return doAsync2<bool>(&rayr::BasicTest::reloadFontLoaderAsync, core, msclr::interop::marshal_as<std::wstring>(fname));
}

Task<bool>^ BasicTest::TryAsync()
{
	return doAsync2<bool>(&rayr::BasicTest::tryAsync, core);
}

void LightHolder::Add(Basic3D::LightType type)
{
	core->addLight((b3d::LightType)type);
}

void LightHolder::Clear()
{
	core->delAllLight();
}

}