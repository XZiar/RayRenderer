#pragma once
#include "RenderCoreWrapRely.h"


namespace Common
{


using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System::Runtime::InteropServices;

public ref struct ControlItem
{
    enum struct PropAccess : uint8_t { Empty = 0x0, Read = 0x1, Write = 0x2, ReadWrite = Read | Write };
    enum struct PropType : uint8_t 
    {   RawValue = (uint8_t)common::Controllable::ArgType::RawValue, Color = (uint8_t)common::Controllable::ArgType::Color,
        LongText = (uint8_t)common::Controllable::ArgType::LongText
    };
    initonly String^ Id;
    initonly String^ Name;
    initonly String^ Category;
    initonly String^ Description;
    initonly Object^ Cookie;
    initonly PropAccess Access;
    initonly PropType Type;
    initonly System::Type^ ValType;
internal:
    ControlItem(const common::Controllable::ControlItem& item);
};


public ref class Controllable : public DynamicObject, public INotifyPropertyChanged
{
private:
    const std::weak_ptr<common::Controllable>* Control;
    initonly String^ controlType;
protected:
    ViewModelStub ViewModel;
    std::shared_ptr<common::Controllable> GetControl() { return Control->lock(); }
internal:
    Controllable(const std::shared_ptr<common::Controllable>& control);
public:
    ~Controllable() { this->!Controllable(); }
    !Controllable();
    virtual event PropertyChangedEventHandler^ PropertyChanged
    {
        void add(PropertyChangedEventHandler^ handler)
        {
            ViewModel.PropertyChanged += handler;
        }
        void remove(PropertyChangedEventHandler^ handler)
        {
            ViewModel.PropertyChanged -= handler;
        }
        void raise(Object^ sender, PropertyChangedEventArgs^ args)
        {
            ViewModel.PropertyChanged(sender, args);
        }
    }
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
    initonly List<ControlItem^>^ Items;
    CLI_READONLY_PROPERTY(String^, ControlType, controlType)
};

}