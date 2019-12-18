#include "WindowManager.h"
#include "Win32MsgName.hpp"
#include "WindowHost.h"

#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"


#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
constexpr uint32_t MessageCreate = WM_USER + 1;


namespace xziar::gui::detail
{

static thread_local WindowManager* TheManager;

class WindowManagerWin32 : public WindowManager
{
private:
    //static intptr_t __stdcall WindowProc(uintptr_t, uint32_t, uintptr_t, intptr_t);
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    void* InstanceHandle = nullptr;
    uint32_t ThreadId = 0;

public:
    WindowManagerWin32(uintptr_t pms) : WindowManager(pms) { }
    ~WindowManagerWin32() override { }

    void Initialize() override
    {
        TheManager = this;
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
    }
    void Terminate() noexcept override
    {
        TheManager = nullptr;
    }

    void MessageLoop() override
    {
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
    }
    void CreateNewWindow_(WindowHost_* host)
    {
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
    }
    void CreateNewWindow(WindowHost_* host) override
    {
        PostThreadMessageW(ThreadId, MessageCreate, (uintptr_t)(host), NULL);
        if (const auto err = GetLastError(); err != NO_ERROR)
            Logger.error(u"Error when post thread message: {}\n", err);
    }
    void CloseWindow(WindowHost_* host) override
    {
        PostMessageW(reinterpret_cast<HWND>(host->Handle), WM_CLOSE, 0, 0);
    }
    void ReleaseWindow(WindowHost_* host) override
    {
        ReleaseDC(reinterpret_cast<HWND>(host->Handle), reinterpret_cast<HDC>(host->DCHandle));
    }
};



LRESULT CALLBACK WindowManagerWin32::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const auto handle = reinterpret_cast<uintptr_t>(hwnd);
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
            case WM_DESTROY:
                host->Stop();
                return 0;

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
                const auto& rect = *reinterpret_cast<LPRECT>(lParam);
                const auto width = rect.right - rect.left;
                const auto height = rect.bottom - rect.top;
                host->OnResize(width, height);
            } break;

            case WM_ERASEBKGND:
                return 1; // just ignore
            case WM_PAINT:
            {
                RECT rect;
                if (GetUpdateRect(hwnd, &rect, FALSE))
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);
                    EndPaint(hwnd, &ps);
                }
            } return 0; // clear redraw flag
            case WM_MOUSEMOVE:
            {
                const auto x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
                host->OnMouseMove(x, y, 0);
            } break;
            case WM_MOUSEWHEEL:
            {
                const auto keys = GET_KEYSTATE_WPARAM(wParam);
                const auto dz = GET_WHEEL_DELTA_WPARAM(wParam);
                const auto x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
                POINT point{ x,y };
                ScreenToClient(hwnd, &point);
                host->OnMouseWheel(point.x, point.y, dz / 120.f, keys);
            } break;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                break;

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
                break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                host->OnKeyDown(0, 0, 0);
            } return 0;
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                host->OnKeyUp(0, 0, 0);
            } return 0;


            case WM_DROPFILES:
            {
                HDROP hdrop = (HDROP)wParam;
                std::u16string pathBuf(2048, u'\0');
                DragQueryFileW(hdrop, 0, reinterpret_cast<wchar_t*>(pathBuf.data()), 2000);
                DragFinish(hdrop);
                host->OnDropFile(pathBuf.c_str());
            } return 0;

            case WM_SETCURSOR:
                break;

            case WM_MOVE:
            case WM_MOVING:

            case WM_WINDOWPOSCHANGED:
            case WM_WINDOWPOSCHANGING:

            case WM_GETMINMAXINFO:
                break;

            case WM_IME_NOTIFY:
            case WM_IME_REQUEST:
            case WM_IME_SETCONTEXT:
                break;

            case WM_NCHITTEST:
            case WM_NCACTIVATE:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCPAINT:
            case WM_NCMOUSEMOVE:
            case WM_NCMOUSELEAVE:
                // bypass logging
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


std::shared_ptr<WindowManager> CreateManagerImpl(uintptr_t pms)
{
    return std::make_shared<WindowManagerWin32>(pms);
}


}