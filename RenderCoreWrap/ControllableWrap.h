#include "RenderCoreWrapRely.h"


namespace RayRender
{


using namespace System;
using namespace System::Dynamic;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

public ref struct ControlItem
{
    enum struct PropAccess : uint8_t { Read = 0x1, Write = 0x2, ReadWrite = Read | Write };
    enum struct PropType : uint8_t { RawValue = (uint8_t)common::Controllable::ArgType::RawValue, Color = (uint8_t)common::Controllable::ArgType::Color };
    initonly String^ Id;
    initonly String^ Name;
    initonly String^ Category;
    initonly String^ Description;
    initonly Object^ Cookie;
    initonly PropAccess Access;
    initonly PropType Type;
internal:
    ControlItem(const common::Controllable::ControlItem& item);
};
public ref class Controllable : public DynamicObject
{
private:
    ViewModelStub ViewModel;
    const std::weak_ptr<common::Controllable>* Control;
    initonly String^ controlType;
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
    virtual bool TryGetMember(GetMemberBinder^ binder, [Out] Object^% arg) override;
    virtual bool TrySetMember(SetMemberBinder^ binder, Object^ arg) override;
    void RefreshControl();

    initonly Dictionary<String^, String^>^ Categories;
    initonly List<ControlItem^>^ Items;
    CLI_READONLY_PROPERTY(String^, ControlType, controlType)
};

}