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
    IntPtr Control;
    static auto GetControlPtr(IntPtr control)
    {
        return reinterpret_cast<const std::weak_ptr<rayr::Controllable>*>(control.ToPointer());
    }
    std::shared_ptr<rayr::Controllable> GetControl() { return GetControlPtr(Control)->lock(); }
internal:
    Controllable(const Wrapper<rayr::Controllable>& control)
    {
        Control = IntPtr(new std::weak_ptr<rayr::Controllable>(control.weakRef()));
    }
public:
    ~Controllable() { this->!Controllable(); }
    !Controllable() 
    {
        const auto ptr = GetControlPtr(System::Threading::Interlocked::Exchange(Control, IntPtr::Zero));
        if (ptr)
            delete ptr;
    }
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
};

}