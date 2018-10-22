#pragma once

#ifndef _M_CEE
#error("CLIViewModel should only be used with /clr")
#endif

#include "atomic"
#include "CLICommonRely.hpp"

namespace Common
{
using System::ComponentModel::INotifyPropertyChanged;
using System::ComponentModel::PropertyChangedEventHandler;
using System::ComponentModel::PropertyChangedEventArgs;
using System::Threading::SynchronizationContext;
public ref class BaseViewModel : public INotifyPropertyChanged
{
private:
    static SynchronizationContext^ SyncContext = SynchronizationContext::Current;
    PropertyChangedEventHandler ^ propertyChanged;
    void OnPropertyChangedFunc(Object^ state)
    {
        auto cookie = safe_cast<System::ValueTuple<Object^, PropertyChangedEventArgs^>>(state);
        PropertyChanged(cookie.Item1, cookie.Item2);
    }
protected:
    void OnPropertyChanged(System::String^ propertyName)
    {
        OnPropertyChanged(this, propertyName);
    }
    void OnPropertyChanged(System::Object^ object, System::String^ propertyName)
    {
        auto arg = gcnew PropertyChangedEventArgs(propertyName);
        if (SynchronizationContext::Current == SyncContext)
            PropertyChanged(object, arg);
        else
            SyncContext->Post(gcnew System::Threading::SendOrPostCallback(this, &BaseViewModel::OnPropertyChangedFunc), 
                System::ValueTuple<Object^, PropertyChangedEventArgs^>(object, arg));
    }
public:
    static void Init() { }
    virtual event PropertyChangedEventHandler^ PropertyChanged
    {
        void add(PropertyChangedEventHandler^ handler)
        {
            propertyChanged += handler;
        }
        void remove(PropertyChangedEventHandler^ handler)
        {
            propertyChanged -= handler;
        }
        void raise(Object^ sender, PropertyChangedEventArgs^ args)
        {
            auto handler = propertyChanged;
            if (handler == nullptr)
                return;
            handler->Invoke(sender, args);
        }
    }
};

public ref class ViewModelStub : public BaseViewModel
{
public:
    void OnPropertyChanged(Object^ object, System::String^ propertyName)
    {
        BaseViewModel::OnPropertyChanged(object, propertyName);
    }
};


}