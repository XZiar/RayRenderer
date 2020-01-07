#include "WindowHost.h"
#include "WindowManager.h"
#include "StringCharset/Convert.h"
#include <thread>


namespace xziar::gui
{
using common::loop::LoopBase;
MAKE_ENABLER_IMPL(WindowHostPassive)
MAKE_ENABLER_IMPL(WindowHostActive)


WindowHost_::WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title) : 
    LoopBase(LoopBase::GetThreadedExecutor), 
    Title(std::u16string(title)), Width(width), Height(height)
{
    Manager = detail::WindowManager::Get();
}

WindowHost_::~WindowHost_()
{
    Stop();
}

bool WindowHost_::OnStart(std::any cookie) noexcept
{
    Manager->PrepareForWindow(this);
    OnOpen();
    return true;
}

LoopBase::LoopState WindowHost_::OnLoop()
{
    return OnLoopPass() ? LoopBase::LoopState::Continue : LoopBase::LoopState::Sleep;
}

void WindowHost_::OnStop() noexcept
{
    Closed(*this);
    Manager->ReleaseWindow(this);
}

void WindowHost_::Initialize()
{
    Start();
}

bool WindowHost_::HandleInvoke() noexcept
{
    if (!InvokeList.IsEmpty())
    {
        auto task = InvokeList.Begin();
        task->Task(*this);
        InvokeList.PopNode(task);
        return true;
    }
    return false;
}

void WindowHost_::OnOpen() noexcept
{
    Openning(*this);
}

void WindowHost_::OnClose() noexcept
{
    bool shouldeClose = true;
    Closing(*this, shouldeClose);
    if (shouldeClose)
        Manager->CloseWindow(this);
}

void WindowHost_::OnDisplay() noexcept
{
    Displaying(*this);
}

void WindowHost_::OnResize(int32_t width, int32_t height) noexcept
{
    Resizing(*this, width, height);
    Width = width; Height = height;
}

void WindowHost_::RefreshMouseButton(event::MouseButton pressed) noexcept
{
    PressedButton = pressed;
}

void WindowHost_::OnMouseEnter(event::Position pos) noexcept
{
    event::MouseEvent evt(pos);
    LastPos = pos;
    MouseEnter(*this, evt);
    MouseHasLeft = false;
}

void WindowHost_::OnMouseLeave() noexcept
{
    event::MouseEvent evt(LastPos);
    MouseLeave(*this, evt);
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
        MouseButtonDown(*this, evt);
    else
        MouseButtonUp(*this, evt);
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
        MouseButtonUp(*this, evt);
    }
    if (pressed != event::MouseButton::None) // not pressed button changed
    {
        event::MouseButtonEvent evt(LastPos, btn, pressed);
        MouseButtonDown(*this, evt);
    }

    PressedButton = btn;
}

void WindowHost_::OnMouseMove(event::Position pos) noexcept
{
    event::MouseMoveEvent evt(LastPos, pos);
    MouseMove(*this, evt);
    if (HAS_FIELD(PressedButton, event::MouseButton::Left))
    {
        if (!IsMouseDragging)
        {
            const auto delta = pos - LeftBtnPos;
            if (std::abs(delta.X * 500.f) > Width || std::abs(delta.Y * 500.f) > Height)
                IsMouseDragging = true;
            /*else
                Manager->Logger.verbose(u"delta[{},{}] pending not drag\n", delta.X, delta.Y);*/
        }
        if (IsMouseDragging)
        {
            event::MouseDragEvent dragEvt(LeftBtnPos, LastPos, pos);
            MouseDrag(*this, dragEvt);
        }
    }
    LastPos = pos;
}

void WindowHost_::OnMouseWheel(event::Position pos, float dz) noexcept
{
    event::MouseWheelEvent evt(pos, dz);
    MouseWheel(*this, evt);
}

void WindowHost_::OnKeyDown(event::CombinedKey key) noexcept
{
    Modifiers |= key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    KeyDown(*this, evt);
}

void WindowHost_::OnKeyUp(event::CombinedKey key) noexcept
{
    Modifiers &= ~key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    KeyUp(*this, evt);
}

void WindowHost_::OnDropFile(std::u16string_view filePath) noexcept
{
    DropFile(*this, filePath);
}

void WindowHost_::Show()
{
    if (!IsOpened.test_and_set())
    {
        Manager->CreateNewWindow(this);
    }
}

WindowHost WindowHost_::GetSelf()
{
    return shared_from_this();
}


void WindowHost_::Invoke(std::function<void(void)> task)
{
    Manager->AddInvoke(std::move(task));
}

void WindowHost_::InvokeUI(std::function<void(WindowHost_&)> task)
{
    InvokeList.AppendNode(new InvokeNode(std::move(task)));
}

void WindowHost_::Invalidate()
{
}

void WindowHost_::Close()
{
    Manager->CloseWindow(this);
}

WindowHost WindowHost_::CreatePassive(const int32_t width, const int32_t height, const std::u16string_view title)
{
    return MAKE_ENABLER_SHARED(WindowHostPassive, (width, height, title));
}
WindowHost WindowHost_::CreateActive(const int32_t width, const int32_t height, const std::u16string_view title)
{
    return MAKE_ENABLER_SHARED(WindowHostActive, (width, height, title));
}


WindowHostPassive::WindowHostPassive(const int32_t width, const int32_t height, const std::u16string_view title)
    : WindowHost_(width, height, title)
{ }

WindowHostPassive::~WindowHostPassive()
{ }

bool WindowHostPassive::OnLoopPass()
{
    if (!HandleInvoke())
    {
        if (!IsUptodate.test_and_set())
            OnDisplay();
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

void WindowHostPassive::Invalidate()
{
    IsUptodate.clear();
}


WindowHostActive::WindowHostActive(const int32_t width, const int32_t height, const std::u16string_view title)
    : WindowHost_(width, height, title)
{ }

WindowHostActive::~WindowHostActive()
{ }

void WindowHostActive::SetTargetFPS(float fps) noexcept
{
    TargetFPS = fps;
}

bool WindowHostActive::OnLoopPass()
{
    const auto targetWaitTime = 1000.0f / TargetFPS;
    DrawTimer.Stop();
    if (DrawTimer.ElapseMs() < targetWaitTime)
    {
        if (HandleInvoke())
            return true;
        else
        {
            const auto waitTime = static_cast<int32_t>(targetWaitTime - DrawTimer.ElapseMs());
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        }
    }
    OnDisplay();
    DrawTimer.Start();
    return true;
}

void WindowHostActive::OnOpen() noexcept
{
    WindowHost_::OnOpen();
    DrawTimer.Start();
}


}

