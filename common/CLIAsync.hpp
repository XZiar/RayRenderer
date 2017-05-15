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
		return (*func)();
	}
public:
	FuncWrapper() {}
	~FuncWrapper()
	{
		this->!FuncWrapper();
	}
	!FuncWrapper()
	{
		if (func != nullptr)
			delete func;
	}
	void setFunc(const std::function<RetType(void)>* const func_)
	{
		func = new std::function<RetType(void)>(*func_);
	}
	Func<RetType>^ getFunc()
	{
		return gcnew Func<RetType>(this, &FuncWrapper::doit);
	}
};


template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<Func<RetType>^>^>& tsk, const std::function<RetType(void)>& func)
{
	FuncWrapper<RetType>^ wrapper = gcnew FuncWrapper<RetType>();
	wrapper->setFunc(&func);
	tsk->SetResult(wrapper->getFunc());
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
	}, [tsk](BaseException be)
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

}