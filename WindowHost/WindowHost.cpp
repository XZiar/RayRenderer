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
    OnClose();
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
    Closing(*this);
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

void WindowHost_::OnMouseMove(int32_t dx, int32_t dy, int32_t flags) noexcept
{
    MouseMove(*this, dx, dy, flags);
}

void WindowHost_::OnMouseWheel(int32_t posx, int32_t posy, float dz, int32_t flags) noexcept
{
    MouseWheel(*this, posx, posy, dz, flags);
}

void WindowHost_::OnKeyDown(int32_t posx, int32_t posy, int32_t key) noexcept
{
    KeyDown(*this, posx, posy, key);
}

void WindowHost_::OnKeyUp(int32_t posx, int32_t posy, int32_t key) noexcept
{
    KeyUp(*this, posx, posy, key);
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

