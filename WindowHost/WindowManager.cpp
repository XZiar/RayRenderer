#include "WindowManager.h"
#include "WindowHost.h"
#include "SystemCommon/ThreadEx.h"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"

#if COMMON_OS_WIN
#   include "Win32MsgName.hpp"
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
MAKE_ENABLER_IMPL(WindowManager)


std::shared_ptr<WindowManager> WindowManager::CreateManager()
{
    std::promise<void> pms;
    const auto manager = MAKE_ENABLER_SHARED(WindowManager, (reinterpret_cast<uintptr_t>(&pms)));
    pms.get_future().get();
    return manager;
}

WindowManager::WindowManager(uintptr_t pms) :
    LoopBase(LoopBase::GetThreadedExecutor), 
    Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{
    Start(reinterpret_cast<std::promise<void>*>(pms));
}

WindowManager::~WindowManager()
{
    Stop();
}

static thread_local WindowManager* TheManager;

bool WindowManager::OnStart(std::any cookie) noexcept
{
    auto& pms = *std::any_cast<std::promise<void>*>(cookie);
    common::SetThreadName(u"WindowManager");
    TheManager = this;

#if COMMON_OS_WIN

    ThreadId = GetCurrentThreadId();
    const auto instance = GetModuleHandleW(nullptr);
    InstanceHandle = instance;
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = reinterpret_cast<WNDPROC>(WindowProc);
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"XZIAR_GUI_WINDOWHOST";

    // register the window class
    RegisterClassEx(&wc);

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

    // control window 
    xcb_window_t window = xcb_generate_id(conn);
    const uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const uint32_t eventmask = XCB_EVENT_MASK_NO_EVENT;
    const uint32_t valuelist[] = { screen->white_pixel, eventmask };

    xcb_create_window(
        conn,
        XCB_COPY_FROM_PARENT,
        window,
        screen->root,
        0, 0,
        1, 1,
        0,
        XCB_WINDOW_CLASS_INPUT_ONLY,
        screen->root_visual,
        valuemask,
        valuelist
    );

    Connection = conn;
    Display = display;
    Screen = screen;
    ControlWindow = window;
#endif

    pms.set_value();
    return true;
}

void WindowManager::OnStop() noexcept
{
    TheManager = nullptr;
#if COMMON_OS_WIN
#else
    const auto conn = reinterpret_cast<xcb_connection_t*>(Connection);
    xcb_destroy_window(conn, ControlWindow);
    xcb_disconnect(conn);
#endif
}

#if COMMON_OS_WIN
intptr_t __stdcall WindowManager::WindowProc(uintptr_t handle, uint32_t msg, uintptr_t wParam, intptr_t lParam)
{
    const auto hwnd = reinterpret_cast<HWND>(handle);
    if (msg == WM_CREATE)
    {
        const auto host = reinterpret_cast<WindowHost_*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        TheManager->Logger.success(FMT_STRING(u"Create window HWND[{:x}] with host [{:p}]\n"), handle, (void*)host);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        host->Handle = hwnd;
        host->DCHandle = GetDC(hwnd);
        host->Initialize();
    }
    else
    {
        const auto host = reinterpret_cast<WindowHost_*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (host != nullptr)
        {
            switch (msg)
            {
            case WM_SIZE:
            {
                if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
                {
                    const auto width = LOWORD(lParam), height = HIWORD(lParam);
                    host->OnResize(width, height);
                }
            } return 0;
            case WM_SIZING:
            {
                const auto& rect  = *reinterpret_cast<LPRECT>(lParam);
                const auto width  = rect.right - rect.left;
                const auto height = rect.bottom - rect.top;
                host->OnResize(width, height);
            } break;
            case WM_DROPFILES:
            {
                HDROP hdrop = (HDROP)wParam;
                std::u16string pathBuf(2048, u'\0');
                DragQueryFileW(hdrop, 0, reinterpret_cast<wchar_t*>(pathBuf.data()), 2000);
                DragFinish(hdrop);
                host->OnDropFile(pathBuf.c_str());
            } return 0;
            case WM_DESTROY:
                host->Stop();
                break;
            default:
                TheManager->Logger.verbose(FMT_STRING(u"HWND[{:x}], Host[{:p}], MSG[{:8x}]({})\n"),
                    handle, (void*)host, msg, detail::GetMessageName(msg));
                break;
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
#endif

LoopBase::LoopState WindowManager::OnLoop()
{
#if COMMON_OS_WIN
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)//, PM_REMOVE))
    {
        TranslateMessage(&msg);
        if (msg.hwnd == nullptr)
        {
            Logger.verbose(FMT_STRING(u"Thread, MSG[{:x}]\n"), msg.message);
            switch (msg.message)
            {
            case MessageCreate: 
                CreateNewWindow_(reinterpret_cast<WindowHost_*>(msg.wParam));
                continue;
            }
        }
        DispatchMessageW(&msg);
    }
#else
    const auto conn = reinterpret_cast<xcb_connection_t*>(Connection);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn)))
    {
        switch (event->response_type & 0x7f) 
        {
        case XCB_CONFIGURE_NOTIFY:
        {
            const auto& msg = *reinterpret_cast<xcb_configure_notify_event_t*>(event);
            WindowHost_* host = nullptr;
            uint16_t width  = msg.width;
            uint16_t height = msg.height;
            if ((width > 0 && width != host->Width) || (height > 0 && height != host->Height))
            {
                host->OnResize(width, height);
            }
        } break;
        case XCB_CLIENT_MESSAGE:
        {
            const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(event);
            if(msg.window == ControlWindow)
            {
                CreateNewWindow_(nullptr);
            }
        } break;
        }
        free (event);
    }
#endif
    return LoopBase::LoopState::Continue;
}

void WindowManager::CreateNewWindow_(WindowHost_* host)
{
#if COMMON_OS_WIN
    const auto hwnd = CreateWindowExW(
        0, //WS_EX_ACCEPTFILES, // extended style
        L"XZIAR_GUI_WINDOWHOST", // window class 
        reinterpret_cast<const wchar_t*>(host->Title.c_str()), // window title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, // style
        0, 0, // position x, y
        host->Width, host->Height, // width, height
        NULL, NULL, // parent window, menu
        reinterpret_cast<HINSTANCE>(InstanceHandle), host); // instance, param
    WindowList.emplace_back(reinterpret_cast<uintptr_t>(hwnd), host);
    ShowWindow(hwnd, SW_SHOW);
#else
    const auto conn = reinterpret_cast<xcb_connection_t*>(Connection);
    xcb_screen_t& screen = *reinterpret_cast<xcb_screen_t*>(Screen);
    // Create XID's for window 
    xcb_window_t window = xcb_generate_id(conn);

    /* Create window */
    const uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    const uint32_t valuelist[] = { screen.white_pixel, eventmask };

    xcb_create_window(
        conn,
        XCB_COPY_FROM_PARENT,
        window,
        screen.root,
        0, 0,
        host->Width, host->Height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen.root_visual,
        valuemask,
        valuelist
    );

    host->Handle = window;
    host->DCHandle = Display;
    WindowList.emplace_back(static_cast<uintptr_t>(host->Handle), host);

    // set title
    xcb_change_property(
        conn, 
        XCB_PROP_MODE_REPLACE, 
        window, 
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 
        gsl::narrow_cast<uint32_t>(host->Title.size()), 
        host->Title.c_str()
    );

    // window must be mapped before glXMakeContextCurrent
    xcb_map_window(conn, window);
    xcb_flush(conn);
#endif
}
void WindowManager::CreateNewWindow(WindowHost_* host)
{
#if COMMON_OS_WIN
    PostThreadMessageW(ThreadId, MessageCreate, (uintptr_t)(host), NULL);
    if (const auto err = GetLastError(); err != NO_ERROR)
        Logger.error(u"Error when post thread message: {}\n", err);
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
    static auto manager = CreateManager();
    return manager;
}

}