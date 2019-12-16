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
    Width(width), Height(height)
{
    Manager = detail::WindowManager::Get();
#if COMMON_OS_WIN
    Title = std::u16string(title);
#else
    Title = common::strchset::to_u8string(title, common::str::Charset::UTF16LE);
#endif
}

WindowHost_::~WindowHost_()
{
    Stop();
}

bool WindowHost_::OnStart(std::any cookie) noexcept
{
    InitializePms = Manager->CreateNewWindow(this);
    DrawTimer.Start();
    return true;
}

void WindowHost_::OnStop() noexcept
{
    OnClose(*this);
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
    OnDisplay(*this);
    DrawTimer.Start();
    return LoopState::Continue;
}

WindowHost WindowHost_::GetSelf()
{
    return shared_from_this();
}


void WindowHost_::InvokeUI(std::function<void(WindowHost_&)> task)
{
    InvokeList.AppendNode(new InvokeNode(std::move(task)));
}

void WindowHost_::Open()
{
    if (!InitializePms)
    {
        Start();
        //InitializePms->Wait();
    }
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

