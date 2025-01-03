#pragma once

#ifndef _M_CEE
#error("CLIAsync should only be used with /clr")
#endif

#include "CLICommonRely.hpp"
#include "CLIException.hpp"
#include "SystemCommon/PromiseTask.h"
#include <functional>

#include <vcclr.h>

using namespace System;
using namespace System::Threading::Tasks;


namespace Common
{


template<class RetType>
inline void __cdecl SetTcsResult(const gcroot<TaskCompletionSource<RetType>^>& tcs, const gcroot<RetType>& ret)
{
    tcs->SetResult(ret);
}
template<class RetType>
inline void __cdecl SetTcsException(const gcroot<TaskCompletionSource<RetType>^>& tcs, const common::BaseException& be)
{
    tcs->SetException(CPPException::FromException(be));
}
template<class RetType>
inline void __cdecl SetTcsException(const gcroot<TaskCompletionSource<RetType>^>& tcs, std::exception_ptr e)
{
    tcs->SetException(CPPException::FromException(e));
}


#pragma managed(push, off)

template<auto Convertor, typename NativeT, typename ManagedT, typename Arg>
inline void ReturnTaskNative(gcroot<TaskCompletionSource<ManagedT>^> tcs, common::PromiseResult<NativeT>&& pms, gcroot<Arg> cookie)
{
    pms->OnComplete([=](const common::PromiseResult<NativeT>& pms_)
        {
            auto obj2 = Convertor(pms_->Get(), cookie);
            SetTcsResult(tcs, obj2);
        });
}

template<auto Convertor, typename NativeT, typename ManagedT>
inline void ReturnTaskNative(gcroot<TaskCompletionSource<ManagedT>^> tcs, common::PromiseResult<NativeT>&& pms)
{
    pms->OnComplete([=](const common::PromiseResult<NativeT>& pms_)
        {
            auto obj2 = Convertor(pms_->Get());
            SetTcsResult(tcs, obj2);
        });
}

template<auto Caller, auto Convertor, typename NativeT, typename ManagedT, typename C, typename... Args>
inline void NewDoAsyncNative(C& self, gcroot<TaskCompletionSource<ManagedT>^> tcs, Args... args)
{
    //common::PromiseResult<NativeT> pms = self.*Caller(std::forward<Args>(args)...);
    common::PromiseResult<NativeT> pms = std::invoke(Caller, self, std::forward<Args>(args)...);
    ReturnTaskNative<Convertor, NativeT, ManagedT>(tcs, std::move(pms));
}
#pragma managed(pop)

template<auto Caller, auto Convertor, typename C, typename... Args>
inline auto NewDoAsync(C& self, Args... args)
{
    static_assert(std::is_invocable_v<decltype(Caller), C, Args...>, "invalid caller");
    using TaskPtr = std::invoke_result_t<decltype(Caller), C, Args...>;
    using NativeT = typename common::PromiseChecker<TaskPtr>::TaskRet;
    static_assert(std::is_invocable_v<decltype(Convertor), NativeT&&>, "convertor should accept a reference of ResultType");
    using ManagedT2 = std::invoke_result_t<decltype(Convertor), NativeT&&>;
    using ManagedT = decltype(std::declval<ManagedT2&>().operator->());

    gcroot<TaskCompletionSource<ManagedT>^> tcs = gcnew TaskCompletionSource<ManagedT>();
    NewDoAsyncNative<Caller, Convertor, NativeT, ManagedT>(self, tcs, std::forward<Args>(args)...);
    return tcs->Task;
}

template<auto Convertor, typename Pms>
inline auto ReturnTask(Pms&& pms)
{
    using NativeT = typename common::PromiseChecker<Pms>::TaskRet;
    static_assert(std::is_invocable_v<decltype(Convertor), NativeT&&>, "convertor should accept a reference of ResultType");
    using ManagedT2 = std::invoke_result_t<decltype(Convertor), NativeT&&>;
    using ManagedT = decltype(std::declval<ManagedT2&>().operator->());

    gcroot<TaskCompletionSource<ManagedT>^> tcs = gcnew TaskCompletionSource<ManagedT>();
    ReturnTaskNative<Convertor, NativeT, ManagedT>(tcs, std::move(pms));
    return tcs->Task;
}

template<auto Convertor, typename Pms, typename Arg>
inline auto ReturnTask(Pms&& pms, Arg cookie)
{
    using NativeT = typename common::PromiseChecker<Pms>::TaskRet;
    static_assert(std::is_invocable_v<decltype(Convertor), NativeT&&, gcroot<Arg>>, "convertor should accept a reference of ResultType");
    using ManagedT2 = std::invoke_result_t<decltype(Convertor), NativeT&&, gcroot<Arg>>;
    using ManagedT = decltype(std::declval<ManagedT2&>().operator->());

    gcroot<TaskCompletionSource<ManagedT>^> tcs = gcnew TaskCompletionSource<ManagedT>();
    ReturnTaskNative<Convertor, NativeT, ManagedT>(tcs, std::move(pms), gcroot<Arg>(cookie));
    return tcs->Task;
}


}