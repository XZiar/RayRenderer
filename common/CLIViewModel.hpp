#pragma once

#ifndef _M_CEE
#error("CLIViewModel should only be used with /clr")
#endif

#include "CLICommonRely.hpp"

namespace common
{

using System::ComponentModel::INotifyPropertyChanged;
using System::ComponentModel::PropertyChangedEventHandler;
using System::ComponentModel::PropertyChangedEventArgs;
public ref class BaseViewModel : public INotifyPropertyChanged
{
protected:
    virtual void OnPropertyChanged(System::String^ propertyName)
    {
        PropertyChanged(this, gcnew PropertyChangedEventArgs(propertyName));
    }
public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;
};



}