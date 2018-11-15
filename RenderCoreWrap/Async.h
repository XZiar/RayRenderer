#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace System::Threading;
using namespace msclr::interop;

namespace Common
{

constexpr auto CompleteState = static_cast<uint8_t>(common::PromiseState::Executed);

ref class AsyncWaiter
{
public:
    ref class AsyncItem
    {
    internal:
        Common::NativeWrapper<std::shared_ptr<common::detail::PromiseResultCore>> PmsCore;
        initonly SynchronizationContext^ SyncContext;
        initonly Action^ AfterComplete;
        AsyncItem(std::shared_ptr<common::detail::PromiseResultCore>&& pmsCore, SynchronizationContext^ syncContext, Action^ afterComplete);
        ~AsyncItem() { this->!AsyncItem(); }
        !AsyncItem();
        bool IsComplete();
    };
private:
    static initonly LinkedList<AsyncItem^>^ TaskList;
    static initonly Thread^ TaskThread;
    static initonly SendOrPostCallback^ AsyncCallback;
    static void PerformTask();
public:
    static AsyncWaiter();
    static void Put(AsyncItem^ item);
};


template<typename CPPType, typename CLIType>
public ref class WaitObj
{
public:
    template<typename CPPType, typename CLIType>
    ref class WaitObjAwaiter : System::Runtime::CompilerServices::INotifyCompletion
    {
    private:
        common::PromiseResult<CPPType>* const Promise;
        initonly Func<CLIWrapper<CPPType>^, CLIType>^ Convertor;
    internal:
        WaitObjAwaiter(common::PromiseResult<CPPType>&& pms, Func<CLIWrapper<CPPType>^, CLIType>^ conv)
            : Promise(new common::PromiseResult<CPPType>(std::move(pms)))
        {
            Convertor = conv;
        }
    public:
        ~WaitObjAwaiter() { this->!WaitObjAwaiter(); }
        !WaitObjAwaiter()
        {
            delete Promise;
        }
        virtual void OnCompleted(Action^ continuation)
        {
            auto item = gcnew AsyncWaiter::AsyncItem(*Promise, SynchronizationContext::Current, continuation);
            AsyncWaiter::Put(item);
        }
        property bool IsCompleted { bool get() { return static_cast<uint8_t>((*Promise)->GetState()) >= CompleteState; } }
        CLIType GetResult()
        {
            try
            {
                return Convertor->Invoke(gcnew CLIWrapper<CPPType>((*Promise)->Wait()));
            }
            catch (const common::BaseException& be)
            {
                throw gcnew CPPException(be);
            }
        }
    };
internal:
    initonly WaitObjAwaiter<CPPType, CLIType>^ TheAwaiter;
    WaitObj(common::PromiseResult<CPPType>&& pms, Func<CLIWrapper<CPPType>^, CLIType>^ conv)
    {
        TheAwaiter = gcnew WaitObjAwaiter<CPPType, CLIType>(std::move(pms), conv);
    }
public:
    WaitObjAwaiter<CPPType, CLIType>^ GetAwaiter() { return TheAwaiter; }
};


}
