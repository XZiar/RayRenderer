#pragma once

#ifndef _M_CEE
#error("CLIAsync should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "CLIException.hpp"
#include <functional>

#include <vcclr.h>

using namespace System;
using namespace System::Threading::Tasks;


namespace Common
{

template<class RetType>
using StdRetFuncType = std::function<RetType(void)>;

//Wrapper for a std::function which returns RetType and has no argument
template<class RetType>
private ref class FuncWrapper
{
private:
    CLIWrapper<StdRetFuncType<RetType>>^ WrapperedFunc;
    RetType DoIt()
    {
        try
        {
            return WrapperedFunc->Extract()();
        }
        catch (common::BaseException& be)
        {
            throw gcnew CPPException(be);
        }
    }
public:
    initonly Func<RetType>^ ManagedFunc;
    FuncWrapper(const StdRetFuncType<RetType>& func) : WrapperedFunc(gcnew CLIWrapper<StdRetFuncType<RetType>>(func)),
        ManagedFunc(gcnew Func<RetType>(this, &FuncWrapper::DoIt)) { }
    ~FuncWrapper() { this->!FuncWrapper(); }
    !FuncWrapper()
    {
        delete WrapperedFunc;
    }
};

template<class RetType, class MiddleType>
private ref class TransFuncWrapper
{
private:
    TaskCompletionSource<RetType>^ OuterTask;
    Task<CLIWrapper<MiddleType>^>^ InnerManagedTask;
    Func<CLIWrapper<MiddleType>^, RetType>^ TransFunc;
    void DoManaged()
    {
        try
        {
            if (InnerManagedTask->IsFaulted)
                OuterTask->SetException(InnerManagedTask->Exception);
            else if (InnerManagedTask->IsCanceled)
                OuterTask->SetCanceled();
            else if (InnerManagedTask->IsCompleted)
            {
                auto middle = InnerManagedTask->Result;
                auto ret = TransFunc(middle);
                OuterTask->SetResult(ret);
            }
        }
        catch (common::BaseException& be)
        {
            OuterTask->SetException(gcnew CPPException(be));
        }
        catch (Exception^ e)
        {
            OuterTask->SetException(e);
        }
    }
public:
    initonly Action^ DoFunc;
    TransFuncWrapper(Task<CLIWrapper<MiddleType>^>^ innerTsk, TaskCompletionSource<RetType>^ tsk, Func<CLIWrapper<MiddleType>^, RetType>^ trans)
        : OuterTask(tsk), InnerManagedTask(innerTsk), TransFunc(trans), DoFunc(gcnew Action(this, &TransFuncWrapper::DoManaged))
    { }
};

template<class RetType>
private ref class ResultFuncWrapper
{
private:
    TaskCompletionSource<RetType>^ OuterTask;
    Task<Func<RetType>^>^ InnerManagedTask;
    Task<CLIWrapper<StdRetFuncType<RetType>>^>^ InnerUnmanagedTask;
    void DoManaged()
    {
        try
        {
            if (InnerManagedTask->IsFaulted)
                OuterTask->SetException(InnerManagedTask->Exception);
            else if (InnerManagedTask->IsCanceled)
                OuterTask->SetCanceled();
            else if (InnerManagedTask->IsCompleted)
            {
                auto callback = InnerManagedTask->Result;
                OuterTask->SetResult(callback());
            }
        }
        catch (common::BaseException& be)
        {
            OuterTask->SetException(gcnew CPPException(be));
        }
        finally
        {
            delete InnerManagedTask;
        }
    }
    void DoUnmanaged()
    {
        try
        {
            if (InnerUnmanagedTask->IsFaulted)
                OuterTask->SetException(InnerManagedTask->Exception);
            else if (InnerUnmanagedTask->IsCanceled)
                OuterTask->SetCanceled();
            else if (InnerUnmanagedTask->IsCompleted)
                OuterTask->SetResult(InnerUnmanagedTask->Result->Extract()());
        }
        catch (common::BaseException& be)
        {
            OuterTask->SetException(gcnew CPPException(be));
        }
    }
public:
    initonly Action^ DoFunc;
    ResultFuncWrapper(Task<Func<RetType>^>^ innerTsk, TaskCompletionSource<RetType>^ tsk) 
        : OuterTask(tsk), InnerManagedTask(innerTsk), DoFunc(gcnew Action(this, &ResultFuncWrapper::DoManaged))
    { }
    ResultFuncWrapper(Task<CLIWrapper<StdRetFuncType<RetType>>^>^ innerTsk, TaskCompletionSource<RetType>^ tsk)
        : OuterTask(tsk), InnerUnmanagedTask(innerTsk), DoFunc(gcnew Action(this, &ResultFuncWrapper::DoUnmanaged))
    { }
};

template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<Func<RetType>^>^>& tsk, const StdRetFuncType<RetType>& func)
{
    const auto wrapper = gcnew FuncWrapper<RetType>(func);
    tsk->SetResult(wrapper->ManagedFunc);
}
template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<CLIWrapper<RetType>^>^>& tsk, RetType&& ret)
{
    tsk->SetResult(gcnew CLIWrapper<RetType>(std::move(ret)));
}
template<class RetType>
inline void __cdecl SetTaskResult(const gcroot<TaskCompletionSource<CLIWrapper<RetType>^>^>& tsk, const RetType& ret)
{
    tsk->SetResult(gcnew CLIWrapper<RetType>(ret));
}
template<class TaskType>
inline void __cdecl SetTaskException(const TaskType& tsk, const common::BaseException& be)
{
    tsk->SetException(gcnew CPPException(be));
}


#pragma managed(push, off)

template<class RetType>
using RealFunc = std::function<void(StdRetFuncType<RetType>)>;

//set a Func as result
template<class RetType, class AsyncFunc, class AsyncObj, class... Args>
inline void doInside(gcroot<TaskCompletionSource<Func<RetType>^>^> tsk, AsyncFunc&& asyfunc, AsyncObj& asyobj, Args&&... args)
{
    std::invoke(asyfunc, asyobj, args..., [tsk](const StdRetFuncType<RetType>& cb)
    {
        SetTaskResult<StdRetFuncType<RetType>>(tsk, cb);
    }, [tsk](const common::BaseException& be)
    {
        SetTaskException(tsk, be);
    });
}

//set a std::function as result
template<class RetType, class AsyncFunc, class AsyncObj, class... Args>
inline void doInside(gcroot<TaskCompletionSource<CLIWrapper<StdRetFuncType<RetType>>^>^> tsk, AsyncFunc&& asyfunc, AsyncObj& asyobj, Args&&... args)
{
    std::invoke(asyfunc, asyobj, args..., [tsk](const StdRetFuncType<RetType>& cb)
    {
        SetTaskResult<StdRetFuncType<RetType>>(tsk, cb);
    }, [tsk](const common::BaseException& be)
    {
        SetTaskException(tsk, be);
    });
}

//set a CLIWrapper as result
template<class RetType, class AsyncFunc, class AsyncObj, class... Args>
inline void doInside3(gcroot<TaskCompletionSource<CLIWrapper<RetType>^>^> tsk, AsyncFunc&& asyfunc, AsyncObj& asyobj, Args&&... args)
{
    std::invoke(asyfunc, asyobj, args..., [tsk](auto&& obj)
    {
        SetTaskResult<RetType>(tsk, std::move(obj));
    }, [tsk](const common::BaseException& be)
    {
        SetTaskException(tsk, be);
    });
}

template<typename RetType, class FinishFunc, class FinishArgs, typename MiddleType, std::size_t... Indexes>
inline RetType MixInvoke(FinishFunc&& finfunc, FinishArgs&& finarg, MiddleType& ptr, std::index_sequence<Indexes...>)
{
    return std::invoke(finfunc, std::get<Indexes>(finarg)..., ptr);
}
//wrap a FinishFunc as std::function as result
template<class RetType, class FinishFunc, class FinishArgs, class AsyncFunc, class AsyncObj, class... Args>
inline void doInside4(gcroot<TaskCompletionSource<CLIWrapper<StdRetFuncType<RetType>>^>^> tsk, FinishFunc&& finfunc, FinishArgs&& finarg, AsyncFunc&& asyfunc, AsyncObj *asyobj, Args&&... args)
{
    std::invoke(asyfunc, *asyobj, args..., [=](auto&& obj)
    {
        using MiddleType = typename std::remove_reference_t<decltype(obj)>;
        SetTaskResult<StdRetFuncType<RetType>>(tsk, [=, mid = std::make_shared<MiddleType>(std::move(obj))]()
        {
            return MixInvoke<RetType>(finfunc, finarg, *mid, std::make_index_sequence<std::tuple_size_v<std::decay_t<FinishArgs>>>{});
        });
    }, [tsk](const common::BaseException& be)
    {
        SetTaskException(tsk, be);
    });
}

#pragma managed(pop)

//Async func that returns a callback which returns RetType
template<class RetType, class MemFunc, class AsyncObj, class... Args>
inline Task<Func<RetType>^>^ doAsync(MemFunc&& memfunc, AsyncObj& obj, Args&&... args)
{
    gcroot<TaskCompletionSource<Func<RetType>^>^> tsk = gcnew TaskCompletionSource<Func<RetType>^>();
    doInside(tsk, memfunc, obj, args...);
    return tsk->Task;
}

//Async func that returns a callback which returns RetType, it will be called in caller's thread
template<class RetType, class MemFunc, class AsyncObj, class... Args>
inline Task<RetType>^ doAsync2(MemFunc&& memfunc, AsyncObj& obj, Args&&... args)
{
    gcroot<TaskCompletionSource<CLIWrapper<StdRetFuncType<RetType>>^>^> innerTask = gcnew TaskCompletionSource<CLIWrapper<StdRetFuncType<RetType>>^>();
    TaskCompletionSource<RetType>^ tsk = gcnew TaskCompletionSource<RetType>();
    ResultFuncWrapper<RetType>^ wrapper = gcnew ResultFuncWrapper<RetType>(innerTask->Task, tsk);
    doInside(innerTask, memfunc, obj, args...);
    innerTask->Task->GetAwaiter().OnCompleted(wrapper->DoFunc);
    return tsk->Task;
}

//Async func with a managed callback to convert MiddleType to RetType, it will be called in caller's thread
template<class RetType, class MiddleType, class MemFunc, class AsyncObj, class... Args>
inline Task<RetType>^ doAsync3(Func<CLIWrapper<MiddleType>^, RetType>^ next, MemFunc&& memfunc, AsyncObj& obj, Args&&... args)
{
    gcroot<TaskCompletionSource<CLIWrapper<MiddleType>^>^> innerTask = gcnew TaskCompletionSource<CLIWrapper<MiddleType>^>();
    TaskCompletionSource<RetType>^ tsk = gcnew TaskCompletionSource<RetType>();
    TransFuncWrapper<RetType, MiddleType>^ wrapper = gcnew TransFuncWrapper<RetType, MiddleType>(innerTask->Task, tsk, next);
    doInside3(innerTask, memfunc, obj, args...);
    innerTask->Task->GetAwaiter().OnCompleted(wrapper->DoFunc);
    return tsk->Task;
}


}