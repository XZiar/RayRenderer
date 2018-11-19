#pragma once

#include "RenderCoreWrapRely.h"
#include "ControllableWrap.h"
#include "common/CLIException.hpp"

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
private:
    ref class AsyncTaskBase abstract
    {
    protected:
        Common::NativeWrapper<std::shared_ptr<common::detail::PromiseResultCore>> PmsCore;
    public:
        ~AsyncTaskBase() { this->!AsyncTaskBase(); }
        !AsyncTaskBase();
        bool IsComplete();
        void virtual SetResult() abstract;
    };
    template<typename CPPType, typename CLRType>
    ref class AsyncTaskItem : public AsyncTaskBase
    {
    private:
        initonly Func<IntPtr, CLRType>^ Converter;
        initonly TaskCompletionSource<CLRType>^ TaskProxy;
    public:
        void virtual SetResult() override
        {
            WRAPPER_NATIVE_PTR(PmsCore, ptr);
            auto pms = std::static_pointer_cast<common::detail::PromiseResult_<CPPType>>(*ptr);
            try
            {
                Common::NativeWrapper<CPPType> temp;
                temp.Construct(pms->Wait());
                WRAPPER_NATIVE_PTR(temp, objptr);
                TaskProxy->SetResult(Converter->Invoke(IntPtr(objptr)));
                temp.Destruct();
            }
            catch (const common::BaseException& be)
            {
                TaskProxy->SetException(gcnew CPPException(be));
            }
            catch (Exception^ ex)
            {
                TaskProxy->SetException(ex);
            }
        }
        AsyncTaskItem(common::PromiseResult<CPPType>&& pms, Func<IntPtr, CLRType>^ converter, TaskCompletionSource<CLRType>^ tcs)
        {
            PmsCore.Construct(std::move(pms));
            Converter = converter;
            TaskProxy = tcs;
        }
    };
    static initonly LinkedList<AsyncTaskBase^>^ TaskList;
    static initonly Thread^ TaskThread;
    static initonly SendOrPostCallback^ AsyncCallback;
    static bool ShouldRun;
    static void Destroy(Object^ sender, EventArgs^ e);
    static void PerformTask();
    static void Put(AsyncTaskBase^ item);
public:
    static AsyncWaiter();
internal:
    template<typename CPPType, typename CLRType>
    static Task<CLRType>^ ReturnTask(common::PromiseResult<CPPType>&& pms, Func<IntPtr, CLRType>^ converter, bool fastCheck)
    {
        if (fastCheck && static_cast<uint8_t>(pms->GetState()) >= CompleteState)
        {
            try
            {
                auto temp = std::make_unique<CPPType>(pms->Wait());
                auto obj = converter->Invoke(IntPtr(temp.get()));
                return Task::FromResult(obj);
            }
            catch (const common::BaseException& be)
            {
                return Task::FromException<CLRType>(gcnew CPPException(be));
            }
            catch (Exception^ ex)
            {
                return Task::FromException<CLRType>(ex);
            }
        }
        else
        {
            auto tcs = gcnew TaskCompletionSource<CLRType>();
            Put(gcnew AsyncTaskItem<CPPType, CLRType>(std::move(pms), converter, tcs));
            return tcs->Task;
        }
    }
    template<typename CPPType, typename CLRType>
    forceinline static Task<CLRType>^ ReturnTask(common::PromiseResult<CPPType>&& pms, Func<IntPtr, CLRType>^ converter)
    {
        return ReturnTask(std::move(pms), converter, true);
    }
};


}
