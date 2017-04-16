#include "RenderCoreWrapRely.h"
#include "RenderCoreWrap.h"
#include <msclr/marshal_cppstd.h>

#include <vcclr.h>


namespace RayRender
{

using namespace common;

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
	try
	{
		core = new rayr::BasicTest();
		cam_ = gcnew Basic3D::Camera(&core->cam);
		light = gcnew LightHolder(core->light());
	}
	catch (BaseException& be)
	{
		Console::WriteLine(L"msg:{0}\nstack:\n", gcnew String(be.message.c_str()));
		String^ stkmsg = L"";
		List<String^>^ msgs = gcnew List<String ^>();
		for (const auto& stk : be.exceptionstack())
			stkmsg = String::Format(L"{0}at:\t{1}--{2}[{3}]\n", stkmsg, gcnew String(stk.file.c_str()), gcnew String(stk.func.c_str()), stk.line);
		Console::WriteLine(stkmsg);
		String^ msg = L"";
		auto bew = be.clone();
		do
		{
			auto stk = bew->stacktrace();
			msg = String::Format(L"{0}at:\t{1}--{2}[{3}]\n{4}\n", msg, gcnew String(stk.file.c_str()), gcnew String(stk.func.c_str()), stk.line,
				gcnew String(bew->message.c_str()));
		} while (bew = bew->nestedException());
		throw gcnew System::ApplicationException(msg);
	}
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
#pragma unmanaged
void TryThrow2()
{
	COMMON_THROW(common::FileException, common::FileException::Reason::NotExist, L"sss", L"here's inner try");
}
void TryThrow(int type)
{
	switch (type)
	{
	case 1:
		COMMON_THROW(common::BaseException, L"Here's try 1");
		//throw common::BaseException(L"Here's try 1");
	case 2:
		COMMON_THROW(common::FileException, common::FileException::Reason::NotExist, L"sss", L"here's try 2");
		//throw common::FileException(common::FileException::Reason::NotExist, L"sss", L"here's try 2");
	case 3:
		try
		{
			TryThrow2();
		}
		catch (...)
		{
			COMMON_THROW(common::BaseException, L"Here's try 3");
		}
	}
}
#pragma managed
void BasicTest::TryException(int type)
{
	if (type > 13)
	{
		TryThrow(type - 3);
	}
	else
	{
		try
		{
			TryThrow(3);
		}
		catch (const common::BaseException& ex)
		{
			Console::WriteLine(L"msg:{0}\nstack:\n", gcnew String(ex.message.c_str()));
			String^ msg = L"";
			List<String^>^ msgs = gcnew List<String ^>();
			for (const auto& stk : ex.exceptionstack())
				msg = String::Format(L"{0}\t{1}--{2}[{3}]\n", msg, gcnew String(stk.file.c_str()), gcnew String(stk.func.c_str()), stk.line);
			Console::WriteLine(msg);
			throw gcnew System::NotImplementedException(msg);
		}
		catch (...)
		{
			auto curex = std::current_exception();
			Console::WriteLine(L"other exception");
			throw gcnew System::NotImplementedException(L"other exception");
		}
	}
}

}