#include "WindowHost.h"
#include "WindowManager.h"
#include "StringCharset/Convert.h"
#include <thread>


namespace xziar::gui
{
using common::loop::LoopBase;
MAKE_ENABLER_IMPL(WindowHost_)


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
    OnOpen();
    DrawTimer.Start();
    return true;
}

void WindowHost_::OnStop() noexcept
{
    Closed(*this);
    Manager->ReleaseWindow(this);
}

LoopBase::LoopState WindowHost_::OnLoop()
{
    DrawTimer.Stop();
    while (DrawTimer.ElapseMs() < 16 && !InvokeList.IsEmpty())
    {
        auto task = InvokeList.Begin();
        task->Task(*this);
        InvokeList.PopNode(task);
        DrawTimer.Stop();
    }
    if (DrawTimer.ElapseMs() < 16)
        std::this_thread::sleep_for(std::chrono::milliseconds(16 - DrawTimer.ElapseMs()));
    OnDisplay();
    DrawTimer.Start();
    return LoopState::Continue;
}

void WindowHost_::Initialize()
{
    Start();
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

void WindowHost_::Close()
{
    Manager->CloseWindow(this);
}

WindowHost WindowHost_::Create(const int32_t width, const int32_t height, const std::u16string_view title)
{
    return MAKE_ENABLER_SHARED(WindowHost_, (width, height, title));
}


}

