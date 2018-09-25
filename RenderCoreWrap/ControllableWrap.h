#include "RenderCoreWrapRely.h"


namespace RayRender
{


using namespace System;
using namespace System::Dynamic;
using namespace System::Runtime::InteropServices;

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
    virtual System::Collections::Generic::IEnumerable<String^>^ GetDynamicMemberNames() override;
    virtual bool TryGetMember(GetMemberBinder^ binder, [Out] Object^% arg) override;
    virtual bool TrySetMember(SetMemberBinder^ binder, Object^ arg) override;

    CLI_READONLY_PROPERTY(String^, ControlType, controlType)
};

}