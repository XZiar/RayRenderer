#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/ThreadEx.h"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#   include <shellapi.h>
constexpr uint32_t MessageCreate = WM_USER + 1;
#else
#   include <X11/Xlib.h>
#   include <X11/Xlib-xcb.h>
#   include <xcb/xcb.h>
#   undef Always
#endif

namespace xziar::gui::detail
{
using common::loop::LoopBase;


WindowManager::WindowManager() : 
    LoopBase(LoopBase::GetThreadedExecutor), 
    Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{
    Start();
}

WindowManager::~WindowManager()
{
    Stop();
}

bool WindowManager::OnStart(std::any cookie) noexcept
{
    common::SetThreadName(u"WindowManager");
    //EventHandle = CreateEventW(NULL, FALSE, );
    ThreadId = GetCurrentThreadId();
#if COMMON_OS_WIN
#else
    const auto display = XOpenDisplay(nullptr);
    /* open display */
    if (!display)
    {
        Logger.error(u"Failed to open display\n");
        return false;
    }

    // Get the XCB connection from the display
    const auto conn = XGetXCBConnection(display);
    if (!conn)
    {
        XCloseDisplay(display);
        Logger.error(u"Can't get xcb connection from display\n");
        return false;
    }

    // Acquire event queue ownership
    XSetEventQueueOwner(display, XCBOwnsEventQueue);

    // Find XCB screen
    auto screenIter = xcb_setup_roots_iterator(xcb_get_setup(conn));
    for (auto screenCnt = DefaultRootWindow(display); screenIter.rem && screenCnt--;)
        xcb_screen_next(&screenIter);
    const auto screen = screenIter.data;
    const auto visualId = screen->root_visual;

    // Create XID's for colormap
    xcb_colormap_t colormap = xcb_generate_id(conn);

    // Create colormap 
    xcb_create_colormap(
        conn,
        XCB_COLORMAP_ALLOC_NONE,
        colormap,
        screen->root,
        visualId
    );

    Connection = conn;
    Display = display;
    Screen = screen;
    VisualID = visualId;
    ColorMap = colormap;
    ConnFD = xcb_get_file_descriptor(conn);
#endif
    return true;
}

void WindowManager::OnStop() noexcept
{
}

LoopBase::LoopState WindowManager::OnLoop()
{
#if COMMON_OS_WIN
    //if (MsgWaitForMultipleObjects(1, &EventHandle, FALSE, 2000, QS_ALLEVENTS) == WAIT_OBJECT_0)
    {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0))//, PM_REMOVE))
        {
            Logger.verbose(FMT_STRING(u"HWND[{:p}], MSG[{:x}]\n"), (void*)msg.hwnd, msg.message);
            if (msg.hwnd == nullptr)
            {
                switch (msg.message)
                {
                case MessageCreate: 
                {
                    const auto host = reinterpret_cast<WindowHost_*>(msg.wParam);
                    CreateNewWindow_(host);
                    const auto pms = reinterpret_cast<std::promise<void>*>(msg.lParam);
                    pms->set_value();
                    delete pms;
                    host->OnOpen(*host);
                } break;
                }
                continue;
            }
            WindowHost_* host = nullptr;
            for (const auto& item : WindowList)
            {
                if (item.first == reinterpret_cast<uintptr_t>(msg.hwnd))
                {
                    host = item.second; break;
                }
            }
            if (!host)
                continue;
            switch (msg.message)
            {
            case WM_CREATE:
                host->OnOpen(*host);
                break;
            case WM_DROPFILES:
            {
                HDROP hdrop = (HDROP)msg.wParam;
                std::u16string pathBuf(2048, u'\0');
                DragQueryFileW(hdrop, 0, reinterpret_cast<wchar_t*>(pathBuf.data()), 2000);
                DragFinish(hdrop);
                host->OnDropFile(*host, pathBuf.c_str());
            } break;
            case WM_DESTROY:
                host->Stop();
                break;
            default:
                DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                break;
            }
        }
    }
#else
    constexpr timespec timeout = { 0, 20000000 }; // 20ms

    fd_set fileDescriptors;
    FD_ZERO(&fileDescriptors);
    FD_SET(ConnFD, &fileDescriptors);

    if (pselect(ConnFD + 1, &fileDescriptors, nullptr, nullptr, &timeout, nullptr) > 0)
    {
        while (true)
        {
            const auto msg = xcb_poll_for_event(reinterpret_cast<xcb_connection_t*>(Connection));
            if (msg == nullptr)
                break;
        }
    }
#endif
    return LoopBase::LoopState::Continue;
}

void WindowManager::CreateNewWindow_(WindowHost_* host)
{
#if COMMON_OS_WIN
    const auto handle = CreateWindowExW(
        WS_EX_ACCEPTFILES, // extended style
        L"Static", // window class 
        reinterpret_cast<const wchar_t*>(host->Title.c_str()), // window title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, // style
        0, 0, // position x, y
        host->Width, host->Height, // width, height
        NULL, NULL, // parent window, menu
        nullptr, NULL); // instance, param
    host->Handle = handle;
    host->DCHandle = GetDC(handle);
    WindowList.emplace_back(reinterpret_cast<uintptr_t>(host->Handle), host);
    ShowWindow(handle, SW_SHOW);
#else
    const auto conn = reinterpret_cast<xcb_connection_t*>(Connection);
    xcb_screen_t& screen = *reinterpret_cast<xcb_screen_t*>(Screen);
    // Create XID's for window 
    xcb_window_t window = xcb_generate_id(conn);

    /* Create window */
    uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
    uint32_t valuelist[] = { eventmask, ColorMap, 0 };
    uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

    xcb_create_window(
        conn,
        XCB_COPY_FROM_PARENT,
        window,
        screen.root,
        0, 0,
        host->Width, host->Height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        VisualID,
        valuemask,
        valuelist
    );

    host->Handle = window;
    host->DCHandle = Display;
    WindowList.emplace_back(static_cast<uintptr_t>(host->Handle), host);

    // NOTE: window must be mapped before glXMakeContextCurrent
    xcb_map_window(conn, window);
    xcb_flush(conn);
#endif
}
common::PromiseResult<void> WindowManager::CreateNewWindow(WindowHost_* host)
{
#if COMMON_OS_WIN
    auto ptr = new std::promise<void>();
    PostThreadMessageW(ThreadId, MessageCreate, (uintptr_t)(host), (intptr_t)ptr);
    return common::PromiseResultSTD<void>::Get(*ptr);
#else
#endif
}

void WindowManager::CloseWindow(WindowHost_* host)
{
#if COMMON_OS_WIN
    PostMessageW(reinterpret_cast<HWND>(host->Handle), WM_CLOSE, 0, 0);
#else
#endif
}

void WindowManager::ReleaseWindow(WindowHost_* host)
{
#if COMMON_OS_WIN
    ReleaseDC(reinterpret_cast<HWND>(host->Handle), reinterpret_cast<HDC>(host->DCHandle));
#else
#endif
}

std::shared_ptr<WindowManager> WindowManager::Get()
{
    static auto manager = std::make_shared<WindowManager>();
    return manager;
}

}