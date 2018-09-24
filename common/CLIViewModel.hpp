#pragma once

#ifndef _M_CEE
#error("CLIViewModel should only be used with /clr")
#endif

#include "atomic"
#include "CLICommonRely.hpp"

namespace common
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
        PropertyChanged(this, safe_cast<PropertyChangedEventArgs^>(state));
    }
protected:
    void OnPropertyChanged(System::String^ propertyName)
    {
        auto arg = gcnew PropertyChangedEventArgs(propertyName);
        if (SynchronizationContext::Current == SyncContext)
            PropertyChanged(this, arg);
        else
            SyncContext->Post(gcnew System::Threading::SendOrPostCallback(this, &BaseViewModel::OnPropertyChangedFunc), arg);
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
    void OnPropertyChanged(System::String^ propertyName)
    {
        BaseViewModel::OnPropertyChanged(propertyName);
    }
};


}