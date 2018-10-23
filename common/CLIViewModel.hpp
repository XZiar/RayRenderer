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

public ref class ViewModelSyncRoot
{
private:
    static SynchronizationContext^ SyncContext = SynchronizationContext::Current;
internal:
    static bool CheckMainThread()
    {
        return SynchronizationContext::Current == SyncContext;
    }
    static void SyncCall(System::Threading::SendOrPostCallback^ callback, ... array<Object^>^ args)
    {
        SyncContext->Post(callback, args);
    }
public:
    static void Init() { }
};

public ref class BaseViewModel : public INotifyPropertyChanged
{
private:
    void RaisePropertyChangedFunc(Object^ state)
    {
        if (auto cookie = dynamic_cast<array<Object^>^>(state); cookie)
            PropertyChanged(cookie[0], dynamic_cast<PropertyChangedEventArgs^>(cookie[1]));
        else
            PropertyChanged(this, dynamic_cast<PropertyChangedEventArgs^>(state));
    }
protected:
    void RaisePropertyChanged(System::String^ propertyName)
    {
        auto arg = gcnew PropertyChangedEventArgs(propertyName);
        if (ViewModelSyncRoot::CheckMainThread())
            PropertyChanged(this, arg);
        else
            ViewModelSyncRoot::SyncCall(gcnew System::Threading::SendOrPostCallback(this, &BaseViewModel::RaisePropertyChangedFunc), arg);
    }
    void RaisePropertyChanged(System::Object^ object, System::String^ propertyName)
    {
        auto arg = gcnew PropertyChangedEventArgs(propertyName);
        if (ViewModelSyncRoot::CheckMainThread())
            PropertyChanged(object, arg);
        else
            ViewModelSyncRoot::SyncCall(gcnew System::Threading::SendOrPostCallback(this, &BaseViewModel::RaisePropertyChangedFunc), object, arg);
    }
public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;
};



}