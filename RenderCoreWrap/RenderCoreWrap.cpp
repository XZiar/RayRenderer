#include "RenderCoreWrap.h"
#include <msclr/marshal_cppstd.h>

#include <vcclr.h>


namespace RayRender
{

private ref class BoolFuncWrapper
{
private:
	std::function<bool(void)> *func;
	bool doit()
	{
		return (*func)();
	}
public:
	BoolFuncWrapper() {}
	void setFunc(const std::function<bool(void)> * const func_)
	{
		func = new std::function<bool(void)>(*func_);
	}
	Func<bool>^ getFunc()
	{
		return gcnew Func<bool>(this, &BoolFuncWrapper::doit);
	}
};

void __cdecl SetToTask(const gcroot<TaskCompletionSource<Func<bool>^>^>& tsk, const std::function<bool(void)>& func)
{
	BoolFuncWrapper^ wrapper = gcnew BoolFuncWrapper();
	wrapper->setFunc(&func);
	tsk->SetResult(wrapper->getFunc());
}

#pragma unmanaged

bool __cdecl OPResultWrapper(rayr::BasicTest *core, std::function<common::OPResult<>(void)> cb)
{
	if (cb())
	{
		const auto curObj = core->objectCount() - 1;
		core->rotateobj(curObj, -90, 0, 0);
		core->moveobj(curObj, -1, 0, 0);
		return true;
	}
	return false;
}

void coreAddModel(rayr::BasicTest *core, const std::wstring fname, gcroot<TaskCompletionSource<Func<bool>^>^> tsk)
{
	core->addModelAsync(fname, [core, tsk](std::function<common::OPResult<>(void)> cb)
	{
		SetToTask(tsk, std::bind(&OPResultWrapper, core, cb));
	});
}

#pragma managed


BasicTest::BasicTest()
{
	core = new rayr::BasicTest();
	cam_ = gcnew Basic3D::Camera(&core->cam);
	light = gcnew LightHolder(core->light());
}

BasicTest::!BasicTest()
{
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

Task<Func<bool>^>^ BasicTest::AddModelAsync(String^ name)
{
	gcroot<TaskCompletionSource<Func<bool>^>^> tsk = gcnew TaskCompletionSource<Func<bool>^>();
	coreAddModel(core, msclr::interop::marshal_as<std::wstring>(name), tsk);
	return tsk->Task;
}

void BasicTest::AddLight(Basic3D::LightType type)
{
	core->addLight((b3d::LightType)type);
}

}