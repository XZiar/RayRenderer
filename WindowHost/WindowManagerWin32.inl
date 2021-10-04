#include "WindowManager.h"
#include "Win32MsgName.hpp"
#include "WindowHost.h"

//#include "SystemCommon/SystemCommonRely.h"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"


#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>

constexpr uint32_t MessageCreate    = WM_USER + 1;
constexpr uint32_t MessageTask      = WM_USER + 2;
constexpr uint32_t MessageUpdTitle  = WM_USER + 3;
constexpr uint32_t MessageClose     = WM_USER + 4;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using event::CommonKeys;

struct Win32Data
{
    HWND Handle = nullptr;
    HDC DCHandle = nullptr;
    bool NeedBackground = true;
};


class WindowManagerWin32 : public WindowManager
{
private:
    //static intptr_t __stdcall WindowProc(uintptr_t, uint32_t, uintptr_t, intptr_t);
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK FirstProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_CREATE)
        {
            const auto manager = reinterpret_cast<WindowManagerWin32*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
            SetClassLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(manager));
        }
        else if (msg == WM_DESTROY)
        {
            SetClassLongPtrW(hwnd, GCLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)); // replace with normal wndproc
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void* InstanceHandle = nullptr;
    uint32_t ThreadId = 0;

    bool SupportNewThread() const noexcept final { return true; }
    common::span<const std::string_view> GetFeature() const noexcept final
    {
        constexpr std::string_view Features[] =
        {
            "OpenGL"sv, "OpenGLES"sv, "DirectX"sv, "Vulkan"sv
        };
        return Features;
    }
    size_t AllocateOSData(void* ptr) const noexcept final
    {
        if (ptr)
            new(ptr) Win32Data;
        return sizeof(Win32Data);
    }
    void DeallocateOSData(void* ptr) const noexcept final
    {
        reinterpret_cast<Win32Data*>(ptr)->~Win32Data();
    }
    const void* GetWindowData(const WindowHost_* host, std::string_view name) const noexcept final
    {
        const auto& data = host->GetOSData<Win32Data>();
        if (name == "HWND" || name == "window") return &data.Handle;
        if (name == "HDC" || name == "display") return &data.DCHandle;
        return nullptr;
    }
public:
    WindowManagerWin32() { }
    ~WindowManagerWin32() final { }

    static void SetDPIMode()
    {
        const auto dpimodes =
        {
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE,
            DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED,
            DPI_AWARENESS_CONTEXT_UNAWARE
        };
        for (const auto mode : dpimodes)
            if (SetThreadDpiAwarenessContext(mode) != NULL)
                break;
    }

    void Initialize() final
    {
        const auto instance = GetModuleHandleW(nullptr);
        InstanceHandle = instance;
        SetDPIMode();

        WNDCLASSEXW wc;
        // clear out the window class for use
        ZeroMemory(&wc, sizeof(WNDCLASSEXW));

        // fill in the struct with the needed information
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = reinterpret_cast<WNDPROC>(FirstProc); // 
        wc.cbClsExtra = sizeof(void*);
        wc.cbWndExtra = sizeof(void*);
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = L"XZIAR_GUI_WINDOWHOST";

        // register the window class
        RegisterClassExW(&wc);

        const auto tmp = CreateWindowExW(
            0, //WS_EX_ACCEPTFILES, // extended style
            L"XZIAR_GUI_WINDOWHOST", // window class 
            nullptr, // window title
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, // style
            0, 0, // position x, y
            0, 0, // width, height
            NULL, NULL, // parent window, menu
            reinterpret_cast<HINSTANCE>(InstanceHandle), this); // instance, param
        DestroyWindow(tmp);
    }

    void Prepare() noexcept final
    {
        ThreadId = GetCurrentThreadId();
    }
    void MessageLoop() final
    {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)//, PM_REMOVE))
        {
            TranslateMessage(&msg);
            if (msg.hwnd == nullptr)
            {
                switch (msg.message)
                {
                case MessageCreate:
                    CreateNewWindow_(*reinterpret_cast<CreatePayload*>(msg.wParam));
                    continue;
                case MessageTask:
                    HandleTask();
                    continue;
                case MessageUpdTitle:
                {
                    const auto host = reinterpret_cast<WindowHost_*>(msg.wParam);
                    while (host->Flags.Add(detail::WindowFlag::TitleLocked))
                        COMMON_PAUSE();
                    SetWindowTextW(host->template GetOSData<Win32Data>().Handle,
                        reinterpret_cast<const wchar_t*>(host->Title.c_str())); // expects return after finish
                    host->Flags.Extract(detail::WindowFlag::TitleLocked | detail::WindowFlag::TitleChanged);
                } continue;
                case MessageClose:
                    DestroyWindow(reinterpret_cast<WindowHost_*>(msg.wParam)->template GetOSData<Win32Data>().Handle);
                    continue;
                default:
                    Logger.verbose(FMT_STRING(u"Thread unknown MSG[{:x}]\n"), msg.message);
                }
            }
            DispatchMessageW(&msg);
        }
    }
    void Terminate() noexcept final
    {
        ThreadId = 0;
    }

    void NotifyTask() noexcept final
    {
        PostThreadMessageW(ThreadId, MessageTask, NULL, NULL);
        if (const auto err = GetLastError(); err != NO_ERROR)
            Logger.error(u"Error when post thread message: {}\n", err);
    }

    bool CheckCapsLock() const noexcept final
    {
        const auto state = GetKeyState(VK_CAPITAL);
        return state & 0x001;
    }

    void CreateNewWindow_(CreatePayload& payload)
    {
        const auto host = payload.Host;
        const auto hwnd = CreateWindowExW(
            0, //WS_EX_ACCEPTFILES, // extended style
            L"XZIAR_GUI_WINDOWHOST", // window class 
            reinterpret_cast<const wchar_t*>(host->Title.c_str()), // window title
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, // style
            0, 0, // position x, y
            host->Width, host->Height, // width, height
            NULL, NULL, // parent window, menu
            reinterpret_cast<HINSTANCE>(InstanceHandle), &payload); // instance, param
        payload.Promise.set_value(); // can release now
        DragAcceptFiles(hwnd, TRUE);
        RegisterHost(hwnd, host);
        ShowWindow(hwnd, SW_SHOW);
    }
    bool SendGeneralMessage(uintptr_t data, uint32_t msg) const
    {
        if (0 == PostThreadMessageW(ThreadId, msg, data, NULL))
        {
            Logger.error(u"Error when post thread message: {}\n", GetLastError());
            return false;
        }
        return true;
    }
    void CreateNewWindow(CreatePayload& payload) final
    {
        if (SendGeneralMessage(reinterpret_cast<uintptr_t>(&payload), MessageCreate))
            payload.Promise.get_future().get();
    }
    void PrepareForWindow(WindowHost_*) const final
    {
        SetDPIMode();
    }
    void UpdateTitle(WindowHost_* host) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host), MessageUpdTitle);
    }
    void CloseWindow(WindowHost_* host) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host), MessageClose);
    }
    void ReleaseWindow(WindowHost_* host) final
    {
        UnregisterHost(host);
        const auto& data = host->template GetOSData<Win32Data>();
        ReleaseDC(data.Handle, data.DCHandle);
    }
};


static constexpr auto KeyCodeLookup = BuildStaticLookup(WPARAM, event::CombinedKey,
    { VK_F1,        CommonKeys::F1 },
    { VK_F2,        CommonKeys::F2 },
    { VK_F3,        CommonKeys::F3 },
    { VK_F4,        CommonKeys::F4 },
    { VK_F5,        CommonKeys::F5 },
    { VK_F6,        CommonKeys::F6 },
    { VK_F7,        CommonKeys::F7 },
    { VK_F8,        CommonKeys::F8 },
    { VK_F9,        CommonKeys::F9 },
    { VK_F10,       CommonKeys::F10 },
    { VK_F11,       CommonKeys::F11 },
    { VK_F12,       CommonKeys::F12 },
    { VK_LEFT,      CommonKeys::Left },
    { VK_UP,        CommonKeys::Up },
    { VK_RIGHT,     CommonKeys::Right },
    { VK_DOWN,      CommonKeys::Down },
    { VK_HOME,      CommonKeys::Home },
    { VK_END,       CommonKeys::End },
    { VK_PRIOR,     CommonKeys::PageUp },
    { VK_NEXT,      CommonKeys::PageDown },
    { VK_INSERT,    CommonKeys::Insert },
    { VK_CONTROL,   CommonKeys::Ctrl },
    { VK_SHIFT,     CommonKeys::Shift },
    { VK_MENU,      CommonKeys::Alt },
    { VK_ESCAPE,    CommonKeys::Esc },
    { VK_BACK,      CommonKeys::Backspace },
    { VK_DELETE,    CommonKeys::Delete },
    { VK_SPACE,     CommonKeys::Space },
    { VK_TAB,       CommonKeys::Tab },
    { VK_RETURN,    CommonKeys::Enter },
    { VK_CAPITAL,   CommonKeys::CapsLock },
    { VK_ADD,       '+' },
    { VK_OEM_PLUS,  '+' },
    { VK_SUBTRACT,  '-' },
    { VK_OEM_MINUS, '-' },
    { VK_MULTIPLY,  '*' },
    { VK_DIVIDE,    '/' },
    { VK_OEM_COMMA, ',' },
    { VK_DECIMAL,   '.' },
    { VK_OEM_PERIOD,'.' },
    { VK_OEM_1,     ';' },
    { VK_OEM_2,     '/' },
    { VK_OEM_3,     '`' },
    { VK_OEM_4,     '[' },
    { VK_OEM_5,     '\\' },
    { VK_OEM_6,     ']' },
    { VK_OEM_7,     '\'' }
    );
static constexpr event::CombinedKey ProcessKey(WPARAM wParam) noexcept
{
    if (wParam >= 'A' && wParam <= 'Z')
        return static_cast<uint8_t>(wParam);
    if (wParam >= '0' && wParam <= '9')
        return static_cast<uint8_t>(wParam);
    if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
        return static_cast<uint8_t>(wParam - VK_NUMPAD0 + '0');
    return KeyCodeLookup(wParam).value_or(CommonKeys::UNDEFINE);
}

static event::MouseButton TranslateButtonState(WPARAM wParam)
{
    event::MouseButton btn = event::MouseButton::None;
    if (wParam & MK_LBUTTON)
        btn |= event::MouseButton::Left;
    if (wParam & MK_MBUTTON)
        btn |= event::MouseButton::Middle;
    if (wParam & MK_RBUTTON)
        btn |= event::MouseButton::Right;
    return btn;
}

LRESULT CALLBACK WindowManagerWin32::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const auto TheManager = reinterpret_cast<WindowManagerWin32*>(GetClassLongPtrW(hwnd, 0));
    Expects(TheManager);
    const auto handle = reinterpret_cast<uintptr_t>(hwnd);
    if (msg == WM_CREATE)
    {
        const auto& payload = *reinterpret_cast<CreatePayload*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        const auto host = payload.Host;
        TheManager->Logger.success(FMT_STRING(u"Create window HWND[{:x}] with host [{:p}]\n"), handle, (void*)host);
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(host));
        auto& data = host->template GetOSData<Win32Data>();
        data.Handle = hwnd;
        data.DCHandle = GetDC(hwnd);
        if (payload.ExtraData)
        {
            const auto bg = (*payload.ExtraData)("background");
            if (bg)
                data.NeedBackground = *reinterpret_cast<const bool*>(bg);
        }
        host->Initialize();
    }
    else if (msg == WM_NCCREATE)
    {
        EnableNonClientDpiScaling(hwnd);
    }
    else
    {
        const auto host = reinterpret_cast<WindowHost_*>(GetWindowLongPtrW(hwnd, 0));
        if (host != nullptr)
        {
            switch (msg)
            {
            case WM_CLOSE:
                if (host->OnClose())
                    break;
                return 0;
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
            case WM_DPICHANGED:
            {
                const auto newdpi = HIWORD(wParam);

                RECT* const prcNewWindow = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(hwnd,
                    NULL,
                    prcNewWindow->left,
                    prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            } return 0;

            case WM_ERASEBKGND:
                if (host->template GetOSData<Win32Data>().NeedBackground) // let defwndproc to handle it
                    break;
                else // return 1(handled) to ignore
                    return 1;
            case WM_PAINT:
            {
                host->Invalidate();
                RECT rect;
                if (GetUpdateRect(hwnd, &rect, FALSE))
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);
                    EndPaint(hwnd, &ps);
                }
            } return 0; // clear redraw flag

            case WM_MOUSELEAVE:
            {
                host->OnMouseLeave();
            } return 0;
            case WM_MOUSEMOVE:
            {
                const auto x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
                event::Position pos(x, y);
                if (host->MouseHasLeft)
                {
                    const auto pressed = TranslateButtonState(wParam);
                    host->RefreshMouseButton(pressed);
                    host->OnMouseEnter(pos);

                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    tme.dwHoverTime = 100;
                    TrackMouseEvent(&tme);
                }
                else
                    host->OnMouseMove(pos);
            } break;
            case WM_MOUSEHWHEEL:
            case WM_MOUSEWHEEL:
            {
                // const auto keys = GET_KEYSTATE_WPARAM(wParam);
                const auto dz = GET_WHEEL_DELTA_WPARAM(wParam) / 120.f;
                const auto x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
                POINT point{ x,y };
                ScreenToClient(hwnd, &point);
                event::Position pos(point.x, point.y);
                float dh = 0, dv = 0;
                if (msg == WM_MOUSEHWHEEL) 
                    dh = -dz;
                else 
                    dv = dz;
                host->OnMouseScroll(pos, dh, dv);
            } break;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            {
                const auto pressed = TranslateButtonState(wParam);
                /*const bool isPress = msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN;
                TheManager->Logger.verbose(u"Btn state:[{}]\n", common::enum_cast(pressed));*/
                host->OnMouseButtonChange(pressed);
                if (msg == WM_LBUTTONDOWN)
                    SetCapture(hwnd);
                else if (msg == WM_LBUTTONUP)
                    ReleaseCapture();
            } break;
            
            case WM_KEYDOWN:
                host->OnKeyDown(ProcessKey(wParam));
                return 0;
            case WM_SYSKEYDOWN:
                host->OnKeyDown(ProcessKey(wParam));
                break;
            case WM_KEYUP:
                host->OnKeyUp(ProcessKey(wParam));
                return 0;
            case WM_SYSKEYUP:
                host->OnKeyUp(ProcessKey(wParam));
                break;

            case WM_DROPFILES:
            {
                HDROP hdrop = (HDROP)wParam;
                event::Position pos = host->LastPos;
                POINT point{};
                if (DragQueryPoint(hdrop, &point) == TRUE)
                {
                    ScreenToClient(hwnd, &point);
                    pos = { point.x, point.y };
                }
                common::StringPool<char16_t> fileNamePool;
                std::vector<common::StringPiece<char16_t>> fileNamePieces;
                const auto count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
                fileNamePieces.reserve(count);
                std::u16string tmp;
                for (uint32_t i = 0; i < count; ++i)
                {
                    const auto len = DragQueryFileW(hdrop, i, nullptr, 0);
                    tmp.resize(len + 1); // reserve for null-end
                    DragQueryFileW(hdrop, i, reinterpret_cast<wchar_t*>(tmp.data()), static_cast<uint32_t>(tmp.size()));
                    tmp.resize(len);
                    fileNamePieces.emplace_back(fileNamePool.AllocateString(tmp));
                }
                DragFinish(hdrop);
                host->OnDropFile(pos, std::move(fileNamePool), std::move(fileNamePieces));
            } return 0;

            case WM_GETICON:
            case WM_SETICON:
            case WM_GETTEXT:
            case WM_SETTEXT:
            case 0xae: // WM_NCUAHDRAWCAPTION
            case WM_SETCURSOR:
                break;

            case WM_MOVE:
            case WM_MOVING:

            case WM_WINDOWPOSCHANGED:
            case WM_WINDOWPOSCHANGING:

            case WM_GETMINMAXINFO:
                break;

            case WM_CHAR:
            case WM_IME_NOTIFY:
            case WM_IME_REQUEST:
            case WM_IME_SETCONTEXT:
                break;

            case WM_NCHITTEST:
            case WM_NCCALCSIZE:
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


std::unique_ptr<WindowManager> CreateManagerImpl()
{
    return std::make_unique<WindowManagerWin32>();
}


}