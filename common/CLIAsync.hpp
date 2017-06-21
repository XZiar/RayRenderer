#pragma once

#ifndef _M_CEE
#error("CLIAsync should only be used with /clr")
#endif

#include "CLIException.hpp"
#include <functional>

#include <vcclr.h>

using namespace System;
using namespace System::Threading::Tasks;


namespace common
{

template<class RetType>
private ref class FuncWrapper
{
private:
	std::function<RetType(void)> *func = nullptr;
	RetType doit()
	{
		try
		{
			auto ret = (*func)();
			return ret;
		}
		catch (BaseException& be)
		{
			throw gcnew CPPException(be);
		}
		finally
		{
			delete func;
			func = nullptr;
		}
	}
public:
	initonly Func<RetType>^ cliFunc = gcnew Func<RetType>(this, &FuncWrapper::doit);
	FuncWrapper(const std::function<RetType(void)>* const func_) 
	{
		func = new std::function<RetType(void)>(*func_);
	}
	~FuncWrapper()
	{
		this->!FuncWrapper();
	}
	!FuncWrapper()
	{
		if (func != nullptr)
		{
			delete func;
			func = nullptr;
		}
	}
};


template<class RetType>
private ref class ResultFuncWrapper
{
private:
	TaskCompletionSource<RetType>^ outtertask;
	Task<Func<RetType>^>^ innerTask;
	void doit()
	{
		try
		{
			if (innerTask->IsFaulted)
				outtertask->SetException(innerTask->Exception);
			else if (innerTask->IsCanceled)
				outtertask->SetCanceled();
			else if (innerTask->IsCompleted)
			{
				auto callback = innerTask->Result;
				outtertask->SetResult(callback());
			}
		}
		catch (BaseException& be)
		{
			outtertask->SetException(gcnew CPPException(be));
		}
	}
public:
	initonly Action^ DoFunc = gcnew Action(this, &ResultFuncWrapper::doit);
	ResultFuncWrapper(Task<Func<RetType>^>^ innerTsk, TaskCompletionSource<RetType>^ tsk) : outtertask(tsk), innerTask(innerTsk)
	{ }
};

template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<Func<RetType>^>^>& tsk, const std::function<RetType(void)>& func)
{
	const FuncWrapper<RetType>^ wrapper = gcnew FuncWrapper<RetType>(&func);
	tsk->SetResult(wrapper->cliFunc);
}

template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<Func<RetType>^>^>& tsk, const BaseException& be)
{
	tsk->SetException(gcnew CPPException(be));
}


#pragma managed(push, off)

template<class RetType>
using RealFunc = std::function<void(std::function<RetType(void)>)>;

template<class RetType, class MemFunc, class Obj, class... ARGS>
inline void doInside(gcroot<TaskCompletionSource<Func<RetType>^>^> tsk, MemFunc&& memfunc, Obj *obj, ARGS&&... args)
{

	std::invoke(memfunc, *obj, args..., [tsk](std::function<RetType(void)> cb)
	{
		SetTaskResult(tsk, cb);
	}, [tsk](BaseException& be)
	{
		SetTaskResult(tsk, be);
	});
}

#pragma managed(pop)


template<class RetType, class MemFunc, class Obj, class... ARGS>
inline Task<Func<RetType>^>^ doAsync(MemFunc&& memfunc, Obj *obj, ARGS&&... args)
{
	gcroot<TaskCompletionSource<Func<RetType>^>^> tsk = gcnew TaskCompletionSource<Func<RetType>^>();
	doInside(tsk, memfunc, obj, args...);
	return tsk->Task;
}

template<class RetType, class MemFunc, class Obj, class... ARGS>
inline Task<RetType>^ doAsync2(MemFunc&& memfunc, Obj *obj, ARGS&&... args)
{
	gcroot<TaskCompletionSource<Func<RetType>^>^> innerTask = gcnew TaskCompletionSource<Func<RetType>^>();
	TaskCompletionSource<RetType>^ tsk = gcnew TaskCompletionSource<RetType>();
	ResultFuncWrapper<RetType>^ wrapper = gcnew ResultFuncWrapper<RetType>(innerTask->Task, tsk);
	doInside(innerTask, memfunc, obj, args...);
	innerTask->Task->GetAwaiter().OnCompleted(wrapper->DoFunc);
	return tsk->Task;
}


}