#include "WindowHost.h"
#include "WindowManager.h"
#include "SystemCommon/StringConvert.h"
#include "common/Delegate.hpp"
#include "common/TrunckedContainer.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <thread>


namespace xziar::gui
{
using namespace std::string_view_literals;
using common::loop::LoopBase;
//MAKE_ENABLER_IMPL(WindowHostPassive)
//MAKE_ENABLER_IMPL(WindowHostActive)


#define WD_EVT_NAMES BOOST_PP_VARIADIC_TO_SEQ(Openning, Displaying, Closed, \
    Closing, DPIChanging, Resizing, Minimizing, \
    MouseEnter, MouseLeave, MouseButtonDown, MouseButtonUp,  \
    MouseMove, MouseDrag, MouseScroll, KeyDown, KeyUp, DropFile)
#define WD_EVT_EACH_(r, func, name) func(name)
#define WD_EVT_EACH(func) BOOST_PP_SEQ_FOR_EACH(WD_EVT_EACH_, func, WD_EVT_NAMES)

template<typename T> struct CastEventType;
template<typename... Args>
struct CastEventType<WindowEventDelegate<Args...>>
{
    using DlgType  = common::Delegate<WindowHost_&, Args...>;
    using FuncType = std::function<void(WindowHost_&, Args...)>;
};

enum class WindowFlag : uint32_t
{
    None = 0x0, Running = 0x1, ContentDirty = 0x2, TitleChanged = 0x4, 
    TitleLocked = 0x80000000u
};
MAKE_ENUM_BITFIELD(WindowFlag)

struct alignas(uint64_t) WindowHost_::Pimpl
{
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(WindowHost_&)> Task;
        InvokeNode(std::function<void(WindowHost_&)>&& task) : Task(std::move(task)) { }
    };
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    common::container::ResourceDict Data;
    common::SimpleTimer DrawTimer;
    common::AtomicBitfield<WindowFlag> Flags;
    common::RWSpinLock DataLock;
    uint16_t TargetFPS;
#define DEF_DELEGATE(name) typename CastEventType<decltype(std::declval<WindowHost_&>().name())>::DlgType name;
    WD_EVT_EACH(DEF_DELEGATE)
#undef DEF_DELEGATE

    Pimpl(uint16_t fps) noexcept : Flags(WindowFlag::None), TargetFPS(fps) {}
};


detail::WindowManager::TitleLock::TitleLock(WindowHost_* host) : Host(host)
{
    while (Host->Impl->Flags.Add(WindowFlag::TitleLocked))
        COMMON_PAUSE();
}
detail::WindowManager::TitleLock::~TitleLock()
{
    Host->Impl->Flags.Extract(WindowFlag::TitleLocked | WindowFlag::TitleChanged);
}


WindowHost_::WindowHost_(detail::WindowManager& manager, const CreateInfo& info) noexcept :
    LoopBase(LoopBase::GetThreadedExecutor), Impl(std::make_unique<Pimpl>(info.TargetFPS)), Manager(manager),
    Title(info.Title), Width(info.Width), Height(info.Height)
{ }
WindowHost_::~WindowHost_()
{
    Stop();
}

#define DEF_ACCESSOR(name) static bool Handle##name(void* dlg, void* cb, CallbackToken* tk) \
{                                                                                           \
    using Caster = CastEventType<decltype(std::declval<WindowHost_&>().name())>;            \
    const auto token = reinterpret_cast<common::CallbackToken*>(tk);                        \
    const auto target = reinterpret_cast<typename Caster::DlgType*>(dlg);                   \
    if (cb)                                                                                 \
    {                                                                                       \
        *token = *target += std::move(*reinterpret_cast<typename Caster::FuncType*>(cb));   \
        return true;                                                                        \
    }                                                                                       \
    else                                                                                    \
        return *target -= *token;                                                           \
}                                                                                           \
decltype(std::declval<WindowHost_&>().name()) WindowHost_::name() const noexcept            \
{                                                                                           \
    return { &Impl->name, &Handle##name };                                                  \
}
WD_EVT_EACH(DEF_ACCESSOR)
#undef DEF_ACCESSOR


const std::any* WindowHost_::GetWindowData_(std::string_view name) const noexcept
{
    const auto scope = Impl->DataLock.ReadScope();
    return Impl->Data.QueryItem(name);
}
void WindowHost_::SetWindowData_(std::string_view name, std::any&& data) const noexcept
{
    const auto scope = Impl->DataLock.WriteScope();
    Impl->Data.Add(name, std::move(data));
}

bool WindowHost_::OnStart(std::any cookie) noexcept
{
    Manager.BeforeWindowOpen(this);
    OnOpen();
    Manager.AfterWindowOpen(this);
    return true;
}

LoopBase::LoopAction WindowHost_::OnLoop()
{
    return OnLoopPass();
}

void WindowHost_::OnStop() noexcept
{
    Impl->Closed(*this);
    Manager.ReleaseWindow(this);
}

void WindowHost_::Initialize()
{
    Start();
}

bool WindowHost_::HandleInvoke() noexcept
{
    if (!Impl->InvokeList.IsEmpty())
    {
        auto task = Impl->InvokeList.Begin();
        task->Task(*this);
        Impl->InvokeList.PopNode(task);
        return true;
    }
    return false;
}

void WindowHost_::OnOpen() noexcept
{
    Impl->Openning(*this);
    Impl->DrawTimer.Start();
}

LoopBase::LoopAction WindowHost_::OnLoopPass()
{
    //passive
    if (Impl->TargetFPS == 0)
    {
        if (!HandleInvoke())
        {
            if (Impl->Flags.Extract(WindowFlag::ContentDirty))
                OnDisplay();
            else
                return ::common::loop::LoopBase::LoopAction::Sleep();
        }
    }
    else
    {
        //active
        const auto targetWaitTime = 1000.0f / Impl->TargetFPS;
        [[maybe_unused]] const auto curtime = Impl->DrawTimer.Stop();
        /*const auto elapse = DrawTimer.ElapseNs();
        const auto fromtime = curtime - elapse;*/
        const auto deltaTime = targetWaitTime - Impl->DrawTimer.ElapseMs();
        // printf("from [%zu], cur [%zu], delta [%f]\n", fromtime, curtime, deltaTime);
        if (deltaTime > targetWaitTime * 0.1f) // > 10% difference
        {
            if (HandleInvoke())
            {
                return ::common::loop::LoopBase::LoopAction::Continue();
            }
            else
            {
                const auto waitTime = static_cast<int32_t>(deltaTime) / (deltaTime > targetWaitTime * 0.2f ? 2 : 1);
                return ::common::loop::LoopBase::LoopAction::SleepFor(waitTime);
            }
        }
        Impl->DrawTimer.Start(); // Reset timer before draw, so that elapse time will include drawing itself
        OnDisplay();
    }
    return ::common::loop::LoopBase::LoopAction::Continue();
}

void WindowHost_::OnDisplay() noexcept
{
    Impl->Displaying(*this);
}

bool WindowHost_::OnClose() noexcept
{
    bool shouldClose = true;
    Impl->Closing(*this, shouldClose);
    return shouldClose;
}

void WindowHost_::OnResize(int32_t width, int32_t height) noexcept
{
    if (width == 0 || height == 0)
        Impl->Minimizing(*this);
    else
    {
        Impl->Resizing(*this, width, height);
        Width = width; Height = height;
    }
}

void WindowHost_::OnDPIChange(float x, float y) noexcept
{
    Impl->DPIChanging(*this, x, y);
}

void WindowHost_::RefreshMouseButton(event::MouseButton pressed) noexcept
{
    PressedButton = pressed;
}

void WindowHost_::OnMouseEnter(event::Position pos) noexcept
{
    event::MouseEvent evt(pos);
    LastPos = pos;
    Impl->MouseEnter(*this, evt);
    MouseHasLeft = false;
}

void WindowHost_::OnMouseLeave() noexcept
{
    event::MouseEvent evt(LastPos);
    Impl->MouseLeave(*this, evt);
    MouseHasLeft = true;
}

void WindowHost_::OnMouseButton(event::MouseButton changedBtn, bool isPress) noexcept
{
    if (isPress)
    {
        PressedButton |= changedBtn;
        if (changedBtn == event::MouseButton::Left)
            LeftBtnPos = LastPos;
    }
    else
    {
        PressedButton &= ~changedBtn;
        if (changedBtn == event::MouseButton::Left)
            IsMouseDragging = false;
    }

    event::MouseButtonEvent evt(LastPos, PressedButton, changedBtn);
    
    if (isPress)
        Impl->MouseButtonDown(*this, evt);
    else
        Impl->MouseButtonUp(*this, evt);
}

void WindowHost_::OnMouseButtonChange(event::MouseButton btn) noexcept
{
    if (btn == PressedButton)
        return;
    const auto changed = btn ^ PressedButton;
    const auto released = changed & PressedButton;
    const auto pressed  = changed & btn;

    if (HAS_FIELD(pressed, event::MouseButton::Left))
        LeftBtnPos = LastPos;
    else if (HAS_FIELD(released, event::MouseButton::Left))
        IsMouseDragging = false;
    
    if (released != event::MouseButton::None) // already pressed button changed
    {
        event::MouseButtonEvent evt(LastPos, btn, released);
        Impl->MouseButtonUp(*this, evt);
    }
    if (pressed != event::MouseButton::None) // not pressed button changed
    {
        event::MouseButtonEvent evt(LastPos, btn, pressed);
        Impl->MouseButtonDown(*this, evt);
    }

    PressedButton = btn;
}

void WindowHost_::OnMouseMove(event::Position pos) noexcept
{
    event::MouseMoveEvent moveEvt(LastPos, pos);
    Impl->MouseMove(*this, moveEvt);
    if (NeedCheckDrag && HAS_FIELD(PressedButton, event::MouseButton::Left))
    {
        if (!IsMouseDragging)
        {
            const auto delta = pos - LeftBtnPos;
            if (std::abs(delta.X * 500.f) > Width || std::abs(delta.Y * 500.f) > Height)
                IsMouseDragging = true;
            /*else
                Manager->Logger.Verbose(u"delta[{},{}] pending not drag\n", delta.X, delta.Y);*/
        }
        if (IsMouseDragging)
        {
            event::MouseDragEvent dragEvt(LeftBtnPos, LastPos, pos);
            Impl->MouseDrag(*this, dragEvt);
        }
    }
    LastPos = pos;
}

void WindowHost_::OnMouseDrag(event::Position pos) noexcept
{
    event::MouseMoveEvent moveEvt(LastPos, pos);
    Impl->MouseMove(*this, moveEvt);
    event::MouseDragEvent dragEvt(LeftBtnPos, LastPos, pos);
    Impl->MouseDrag(*this, dragEvt);
    LastPos = pos;
}

void WindowHost_::OnMouseScroll(event::Position pos, float dh, float dv) noexcept
{
    event::MouseScrollEvent evt(pos, dh, dv);
    Impl->MouseScroll(*this, evt);
}

void WindowHost_::OnKeyDown(event::CombinedKey key) noexcept
{
    if (key.Key == event::CommonKeys::CapsLock)
    {
        Manager.Logger.Verbose(u"CapsLock {}\n"sv, Manager.CheckCapsLock() ? u"pressed"sv : u"released"sv);
    }
    Modifiers |= key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    Impl->KeyDown(*this, evt);
}

void WindowHost_::OnKeyUp(event::CombinedKey key) noexcept
{
    Modifiers &= ~key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    Impl->KeyUp(*this, evt);
}

void WindowHost_::OnDropFile(event::Position pos, common::StringPool<char16_t>&& namepool, 
        std::vector<common::StringPiece<char16_t>>&& names) noexcept
{
    event::DropFileEvent evt(pos, std::move(namepool), std::move(names));
    Impl->DropFile(*this, evt);
}

void WindowHost_::Show(const std::function<std::any(std::string_view)>& provider)
{
    if (!Impl->Flags.Add(WindowFlag::Running))
    {
        detail::CreatePayload payload{ this, provider ? &provider : nullptr };
        Manager.CreateNewWindow(payload);
    }
}

WindowHost WindowHost_::GetSelf()
{
    return shared_from_this();
}

void WindowHost_::SetTitle(const std::u16string_view title)
{
    while (Impl->Flags.Add(WindowFlag::TitleLocked))
        COMMON_PAUSE();
    Title.assign(title.begin(), title.end());
    if (!Impl->Flags.Add(WindowFlag::TitleChanged))
        Manager.UpdateTitle(this);
    Impl->Flags.Extract(WindowFlag::TitleLocked);
}

void WindowHost_::Invoke(std::function<void(void)> task)
{
    Manager.AddInvoke(std::move(task));
}

void WindowHost_::InvokeUI(std::function<void(WindowHost_&)> task)
{
    Impl->InvokeList.AppendNode(new Pimpl::InvokeNode(std::move(task)));
    Wakeup();
}

void WindowHost_::Invalidate()
{
    if(!Impl->Flags.Add(WindowFlag::ContentDirty))
        Wakeup();
}

void WindowHost_::SetTargetFPS(uint16_t fps) noexcept
{
    Impl->TargetFPS = fps;
    Wakeup();
}

void WindowHost_::Close()
{
    Manager.CloseWindow(this);
}


}

