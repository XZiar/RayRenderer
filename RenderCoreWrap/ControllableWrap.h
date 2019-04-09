#pragma once
#include "RenderCoreWrapRely.h"


namespace Common
{


using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System::Runtime::InteropServices;

ref class Controllable;

private delegate void ControlItemChangedEventHandler(PropertyChangedEventArgs^ arg);

public ref class ControlItem
{
private:
    initonly Controllable^ Control;
    initonly PropertyChangedEventArgs^ ChangedEvent;
    const common::Controllable::ControlItem* PtrItem;
    event ControlItemChangedEventHandler^ ValueChanged;
internal:
    ControlItem(Controllable^ control, String^ id, const common::Controllable::ControlItem& item);
    bool GetValue(const std::shared_ptr<common::Controllable>& control, [Out] Object^% arg);
    bool SetValue(const std::shared_ptr<common::Controllable>& control, Object^ arg);
public:
    enum struct PropAccess : uint8_t { Empty = 0x0, Read = 0x1, Write = 0x2, ReadWrite = Read | Write };
    enum struct PropType : uint8_t 
    {   RawValue = (uint8_t)common::Controllable::ArgType::RawValue, Color = (uint8_t)common::Controllable::ArgType::Color,
        LongText = (uint8_t)common::Controllable::ArgType::LongText, Enum  = (uint8_t)common::Controllable::ArgType::Enum,
    };
    initonly String^ Id;
    initonly String^ Name;
    initonly String^ Category;
    initonly String^ Description;
    initonly Object^ Cookie;
    initonly System::Type^ ValType;
    initonly PropAccess Access;
    initonly PropType Type;
};


public ref class Controllable : public DynamicObject, public INotifyPropertyChanged
{
private:
    const std::weak_ptr<common::Controllable>* Control;
    initonly String^ controlType;
    initonly Dictionary<String^, ControlItem^>^ ControlItems;
    void RaisePropertyChangedFunc(Object^ state)
    {
        PropertyChanged(this, static_cast<PropertyChangedEventArgs^>(static_cast<array<Object^>^>(state)[0]));
    }
protected:
    void RaisePropertyChanged(System::String^ propertyName)
    {
        auto arg = gcnew PropertyChangedEventArgs(propertyName);
        WhenPropertyChanged(arg);
    }
internal:
    Controllable(const std::shared_ptr<common::Controllable>& control);
    std::shared_ptr<common::Controllable> GetControl() { return Control->lock(); }
    void WhenPropertyChanged(PropertyChangedEventArgs^ arg)
    {
        if (ViewModelSyncRoot::CheckMainThread())
            PropertyChanged(this, arg);
        else
            ViewModelSyncRoot::SyncCall(gcnew System::Threading::SendOrPostCallback(this, &Controllable::RaisePropertyChangedFunc), arg);
    }
public:
    ~Controllable() { this->!Controllable(); }
    !Controllable();
    virtual event PropertyChangedEventHandler^ PropertyChanged;
    virtual IEnumerable<String^>^ GetDynamicMemberNames() override;
    bool DoGetMember(String^ id, [Out] Object^% arg);
    bool DoSetMember(String^ id, Object^ arg);
    virtual bool TryGetMember(GetMemberBinder^ binder, [Out] Object^% arg) override
    {
        return DoGetMember(binder->Name, arg);
    }
    virtual bool TrySetMember(SetMemberBinder^ binder, Object^ arg) override
    {
        return DoSetMember(binder->Name, arg);
    }
    void RefreshControl();

    initonly Dictionary<String^, String^>^ Categories;
    property Dictionary<String^, ControlItem^>::ValueCollection^ Items 
    {
        Dictionary<String^, ControlItem^>::ValueCollection^ get() { return ControlItems->Values; }
    }
    CLI_READONLY_PROPERTY(String^, ControlType, controlType)
};

}