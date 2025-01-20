#include "WindowManager.h"
#include "Win32MsgName.hpp"
#include "WindowHost.h"

#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/ImageBMP.h"
#include "SystemCommon/ErrorCodeHelper.h"
#include "SystemCommon/PromiseTaskSTD.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/SystemCommonRely.h"
#include "common/ContainerEx.hpp"
#include "common/StaticLookup.hpp"
#include "common/StringPool.hpp"
#include "common/TimeUtil.hpp"
#include "common/Linq2.hpp"

#include <mutex>

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include <windowsx.h>
#include <D2D1.h>
#include <D2D1helper.h>
#include <shellapi.h>
#define STRICT_TYPED_ITEMIDS
#include <shobjidl.h>
#include <wrl/client.h>
#pragma comment(lib, "d2d1.lib")


static inline constexpr uint32_t MessageCreate    = WM_USER + 1;
static inline constexpr uint32_t MessageTask      = WM_USER + 2;
static inline constexpr uint32_t MessageClose     = WM_USER + 3;
static inline constexpr uint32_t MessageStop      = WM_USER + 4;
static inline constexpr uint32_t MessageDpi       = WM_USER + 5;
static inline constexpr uint32_t MessageUpdTitle  = WM_USER + 10;
static inline constexpr uint32_t MessageUpdIcon   = WM_USER + 11;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using common::HResultHolder;
using common::BaseException;
using common::SimpleTimer;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::ImgDType;
using event::CommonKeys;


namespace win32
{

struct PixFormatTarget
{
    ImgDType MidType;
    D2D1_PIXEL_FORMAT Format;
};
#define PixItem(dtype, mid, format, alpha) { xziar::img::ImageDataType::dtype.Value, { xziar::img::ImageDataType::mid, { DXGI_FORMAT_##format, D2D1_ALPHA_MODE_##alpha } } }
static constexpr auto PixFormatLookup = BuildStaticLookup(uint32_t, PixFormatTarget,
    PixItem(RGBA,   RGBA,   B8G8R8A8_UNORM, PREMULTIPLIED),
    PixItem(BGRA,   BGRA,   B8G8R8A8_UNORM, PREMULTIPLIED),
    PixItem(RGB,    RGBA,   B8G8R8X8_UNORM, IGNORE),
    PixItem(BGR,    BGRA,   B8G8R8X8_UNORM, IGNORE),
    PixItem(RGBAf,  RGBAf,  R32G32B32A32_FLOAT, PREMULTIPLIED),
    PixItem(RGBf,   RGBf,   R32G32B32_FLOAT, IGNORE),
    PixItem(BGRAf,  RGBAf,  R32G32B32A32_FLOAT, PREMULTIPLIED),
    PixItem(BGRf,   RGBf,   R32G32B32_FLOAT, IGNORE),
    PixItem(RGBAh,  RGBAh,  R16G16B16A16_FLOAT, PREMULTIPLIED),
    PixItem(BGRAh,  RGBAh,  R16G16B16A16_FLOAT, PREMULTIPLIED),
    PixItem(RGBh,   RGBAh,  R16G16B16A16_FLOAT, IGNORE),
    PixItem(BGRh,   RGBAh,  R16G16B16A16_FLOAT, IGNORE),
    PixItem(GRAY,   GRAY,   A8_UNORM, PREMULTIPLIED),
    PixItem(GA,     GRAY,   A8_UNORM, PREMULTIPLIED)
    );
#undef PixItem

struct HBMPHolder
{
    HBITMAP Bitmap = nullptr;
    constexpr HBMPHolder() noexcept = default;
    constexpr HBMPHolder(HBITMAP bmp) noexcept : Bitmap(bmp) {};
    COMMON_NO_COPY(HBMPHolder)
    HBMPHolder(HBMPHolder&& rhs) noexcept : Bitmap(rhs.Bitmap)
    {
        rhs.Bitmap = nullptr;
    }
    HBMPHolder& operator= (HBMPHolder&& rhs) noexcept
    {
        if (&rhs != this)
        {
            std::swap(Bitmap, rhs.Bitmap);
            if (rhs.Bitmap) DeleteObject(rhs.Bitmap);
            rhs.Bitmap = nullptr;
        }
        return *this;
    }
    ~HBMPHolder()
    {
        if (Bitmap) DeleteObject(Bitmap);
    }
    explicit constexpr operator bool() const noexcept { return Bitmap; }
    constexpr operator HGDIOBJ () const noexcept { return Bitmap; }
    constexpr operator HBITMAP& () noexcept { return Bitmap; }
    constexpr HBITMAP Extract() noexcept
    {
        auto ret = Bitmap;
        Bitmap = nullptr;
        return ret;
    }
    static HBMPHolder Create(ImageView img, HDC dc, bool useDDB)
    {
        BITMAPINFO bmpinfo{};
        bmpinfo.bmiHeader.biSize = sizeof(bmpinfo.bmiHeader);
        bmpinfo.bmiHeader.biPlanes = 1;
        bmpinfo.bmiHeader.biWidth = gsl::narrow_cast<LONG>(img.GetWidth());
        bmpinfo.bmiHeader.biHeight = -1 * gsl::narrow_cast<LONG>(img.GetHeight());
        {
            const auto dtype = img.GetDataType();
            if (!dtype.Is(xziar::img::ImgDType::DataTypes::Uint8))
                return {};
            std::optional<xziar::img::ImgDType> convert;
            if (!dtype.IsBGROrder())
                convert = dtype.HasAlpha() ? xziar::img::ImageDataType::BGRA : xziar::img::ImageDataType::BGR;
            if ((img.GetWidth() * 3) % 4) // needs 4 channel
                convert = xziar::img::ImageDataType::BGRA;
            if (convert && convert != dtype)
                img = img.ConvertTo(*convert);
        }
        const auto dtype = img.GetDataType();
        Ensures(dtype == xziar::img::ImageDataType::BGRA || dtype == xziar::img::ImageDataType::BGR);
        bmpinfo.bmiHeader.biBitCount = static_cast<WORD>(dtype.ChannelCount() * 8);
        bmpinfo.bmiHeader.biCompression = BI_RGB; // 32bpp ensures DWORD alignment
        HBITMAP bmp = nullptr;
        if (useDDB)
            bmp = CreateDIBitmap(dc, &bmpinfo.bmiHeader, CBM_INIT, img.GetRawPtr(), &bmpinfo, DIB_RGB_COLORS);
        else
        {
            void* ptr = nullptr;
            bmp = CreateDIBSection(dc, &bmpinfo, DIB_RGB_COLORS, &ptr, nullptr, 0);
            if (bmp)
                memcpy_s(ptr, img.GetSize(), img.GetRawPtr(), img.GetSize());
        }
        return { bmp };
    }
};

struct IconHolder : public OpaqueResource
{
    explicit IconHolder(HICON sicon, HICON bicon) noexcept : OpaqueResource(&TheDisposer, reinterpret_cast<uintptr_t>(sicon), reinterpret_cast<uintptr_t>(bicon)) {}
    HICON SmallIcon() const noexcept { return reinterpret_cast<HICON>(static_cast<uintptr_t>(Cookie[0])); }
    HICON BigIcon() const noexcept { return reinterpret_cast<HICON>(static_cast<uintptr_t>(Cookie[1])); }
    static void TheDisposer(const OpaqueResource& res) noexcept
    {
        const auto& holder = static_cast<const IconHolder&>(res);
        const auto sicon = holder.SmallIcon(), bicon = holder.BigIcon();
        if (sicon) DeleteObject(sicon);
        if (bicon) DeleteObject(bicon);
    }
};

}
using namespace win32;

class WindowManagerWin32 final : public Win32Backend, public WindowManager
{
public:
    static constexpr std::string_view BackendName = "Win32"sv;
private:
    class WdHost;
    class GDIRenderer final : private CacheRect<int32_t>, public BasicRenderer
    {
        friend WindowManagerWin32;
        WdHost& Window;
        HDC MemDC = nullptr;
        HBRUSH BGBrush = nullptr;
        HRGN Region = nullptr;
        std::tuple<HBITMAP, uint32_t, uint32_t> AttachedImage = { nullptr, 0u, 0u };
        LockField ResourceLock;
        [[nodiscard]] forceinline auto& Log() noexcept { return Window.Manager.Logger; }
        [[nodiscard]] forceinline auto UseImg() noexcept { return detail::LockField::ConsumerLock<0>{ResourceLock}; }
        void ReplaceImage(HBITMAP bmp, uint32_t w, uint32_t h, bool invalidate) noexcept
        {
            detail::LockField::ProducerLock<0> lock{ ResourceLock };
            if (auto& bmp_ = std::get<0>(AttachedImage); bmp_)
                DeleteObject(bmp_);
            AttachedImage = { bmp, w, h };
            if (lock && invalidate)
                Window.Invalidate();
        }
    public:
        GDIRenderer(WdHost* wd) : Window(*wd) {}
        ~GDIRenderer() final 
        {
            ReplaceImage(nullptr, 0, 0, false);
            DeleteObject(Region);
            DeleteObject(BGBrush);
            DeleteDC(MemDC);
        }
        void Initialize() noexcept
        {
            MemDC = CreateCompatibleDC(Window.DCHandle);
            BGBrush = GetSysColorBrush(COLOR_WINDOW);
            {
                SetStretchBltMode(MemDC, STRETCH_HALFTONE);
                SetBrushOrgEx(MemDC, 0, 0, nullptr);
            }
        }
        void Render(bool forceRedraw) noexcept;
        void SetImage(std::optional<xziar::img::ImageView> img) final
        {
            if (img)
            {
                if (auto bmp = HBMPHolder::Create(*img, Window.DCHandle, true); bmp)
                {
                    ReplaceImage(bmp.Extract(), img->GetWidth(), img->GetHeight(), true);
                    Window.Invalidate();
                }
            }
            else
            {
                ReplaceImage(nullptr, 0, 0, true);
                Window.Invalidate();
            }
        }
    };
    class D2DRenderer final : private CacheRect<int32_t>, public BasicRenderer
    {
        friend WindowManagerWin32;
        WdHost& Window;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> Target;
        Microsoft::WRL::ComPtr<ID2D1Brush> BlackBrush;
        std::tuple<Microsoft::WRL::ComPtr<ID2D1Bitmap>, bool> AttachedImage = { {}, false };
        uint32_t BgColor = 0;
        uint32_t MaxBmpSize = 0;
        LockField ResourceLock;
        [[nodiscard]] forceinline auto& Log() noexcept { return Window.Manager.Logger; }
        [[nodiscard]] forceinline auto UseImg() noexcept { return detail::LockField::ConsumerLock<0>{ResourceLock}; }
        void ReplaceImage(const Microsoft::WRL::ComPtr<ID2D1Bitmap>& bmp, bool isGray, bool invalidate) noexcept
        {
            detail::LockField::ProducerLock<0> lock{ ResourceLock };
            AttachedImage = { bmp, isGray };
            if (lock && invalidate)
                Window.Invalidate();
        }
    public:
        D2DRenderer(WdHost* wd) : Window(*wd), BgColor(GetSysColor(COLOR_WINDOW)) { }
        ~D2DRenderer() final
        {
            ReplaceImage({}, false, false);
            Target->Release();
        }
        void Initialize() noexcept
        {
            auto& manager = Window.GetManager();
            const HResultHolder hr = manager.D2DFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE }),
                D2D1::HwndRenderTargetProperties(Window.Handle, D2D1::SizeU(Window.Width, Window.Height), D2D1_PRESENT_OPTIONS_NONE),
                Target.GetAddressOf());
            if (!hr)
                manager.Logger.Warning(u"Cannot create rernder target: {}\n", hr);
            {
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
                Target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::Black), brush.GetAddressOf());
                brush.As(&BlackBrush);
            }
            MaxBmpSize = Target->GetMaximumBitmapSize();
        }
        void Render(bool forceRedraw) noexcept;
        void SetImage(std::optional<xziar::img::ImageView> img) final
        {
            if (img)
            {
                auto pfmt = PixFormatLookup(img->GetDataType().Value);
                if (!pfmt)
                {
                    img = img->ConvertTo(xziar::img::ImageDataType::RGBA);
                    pfmt = PixFormatLookup(img->GetDataType().Value);
                }
                if (!pfmt) return;
                if (const auto w = img->GetWidth(), h = img->GetHeight(); MaxBmpSize && (w > MaxBmpSize || h > MaxBmpSize))
                {
                    uint32_t sw = 0, sh = 0;
                    (w > h ? sw : sh) = MaxBmpSize;
                    img = img->ResizeTo(sw, sh, true, true);
                }
                if (pfmt->MidType != img->GetDataType())
                    img = img->ConvertTo(pfmt->MidType);
                Microsoft::WRL::ComPtr<ID2D1Bitmap> bmp;
                const HResultHolder hr = Target->CreateBitmap(D2D1::SizeU(img->GetWidth(), img->GetHeight()), img->GetRawPtr(), gsl::narrow_cast<uint32_t>(img->RowSize()),
                    D2D1::BitmapProperties(pfmt->Format), bmp.GetAddressOf());
                if (hr)
                    ReplaceImage(bmp, pfmt->MidType.ChannelCount() < 3, true);
            }
            else
                ReplaceImage({}, false, true);
        }
    };
    class WdHost final : public Win32Backend::Win32WdHost
    {
        friend GDIRenderer;
    public:
        [[nodiscard]] forceinline WindowManagerWin32& GetManager() const noexcept { return static_cast<WindowManagerWin32&>(Manager); }
        
        HWND Handle = nullptr;
        HDC DCHandle = nullptr;
        std::variant<std::monostate, GDIRenderer, D2DRenderer> Renderer;
        WdHost(WindowManagerWin32& manager, const Win32CreateInfo& info) noexcept :
            Win32WdHost(manager, info) 
        {
            if (info.UseDefaultRenderer)
            {
                if (info.RendererType == Win32CreateInfo::RendererTypes::D2D && manager.D2DFactory)
                    Renderer.emplace<D2DRenderer>(this);
                else
                    Renderer.emplace<GDIRenderer>(this);
            }
        }
        ~WdHost() final { }
        uintptr_t FormatAs() const noexcept
        {
            return reinterpret_cast<uintptr_t>(Handle);
        }
        WindowBackend& GetBackend() const noexcept final { return GetManager(); }
        void* GetHDC() const noexcept final { return DCHandle; }
        void* GetHWND() const noexcept final { return Handle; }
        void OnDPIChange(float x, float y) noexcept final
        {
            ReSizingLock lock(*this);
            if (Renderer.index() == 2)
            {
                std::get<2>(Renderer).Target->SetDpi(x, y);
                Invalidate();
            }
            WindowHost_::OnDPIChange(x, y);
        }
        void OnDisplay(bool forceRedraw) noexcept final
        {
            switch (Renderer.index())
            {
            case 1: std::get<1>(Renderer).Render(forceRedraw); break;
            case 2: std::get<2>(Renderer).Render(forceRedraw); break;
            default: break;
            }
            WindowHost_::OnDisplay(forceRedraw);
        }
        BasicRenderer* GetRenderer() noexcept final 
        {
            switch (Renderer.index())
            {
            case 1: return &std::get<1>(Renderer);
            case 2: return &std::get<2>(Renderer);
            default: return nullptr;
            }
        }
        void GetClipboard(const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type) final
        {
            GetManager().GetClipboard(*this, handler, type);
        }
    };

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
    static FileList ProcessDropFile(HDROP hdrop) noexcept
    {
        FileList list;
        const auto count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
        std::u16string tmp;
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto len = DragQueryFileW(hdrop, i, nullptr, 0);
            tmp.resize(len + 1); // reserve for null-end
            DragQueryFileW(hdrop, i, reinterpret_cast<wchar_t*>(tmp.data()), static_cast<uint32_t>(tmp.size()));
            tmp.resize(len);
            list.AppendFile(tmp);
        }
        return list;
    }

    std::string_view Name() const noexcept { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept
    {
        constexpr std::string_view Features[] =
        {
            "OpenGL"sv, "OpenGLES"sv, "DirectX"sv, "Vulkan"sv, "NewThread"sv,
        };
        return std::find(std::begin(Features), std::end(Features), feat) != std::end(Features);
    }

    std::mutex ClipboardLock;
    Microsoft::WRL::ComPtr<ID2D1Factory> D2DFactory;
    void* InstanceHandle = nullptr;
    HDC MemDC;
    uint32_t BuildNumber;
    uint32_t ThreadId = 0;
    static inline const auto DPIModes =
    {
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE,
        DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED,
        DPI_AWARENESS_CONTEXT_UNAWARE
    };
public:
    WindowManagerWin32() : Win32Backend(true), MemDC(CreateCompatibleDC(nullptr)), BuildNumber(common::GetWinBuildNumber())
    {
        Logger.Info(u"Win32 WindowManager with build number [{}].\n", BuildNumber);
        if (BuildNumber >= 15063) // since win10 1703(rs2)
        {
            for (const auto mode : DPIModes)
                if (SetProcessDpiAwarenessContext(mode) != NULL)
                    break;
        }
        D2D1_FACTORY_OPTIONS options
        {
#if CM_DEBUG
            D2D1_DEBUG_LEVEL_INFORMATION
#else
            D2D1_DEBUG_LEVEL_ERROR
#endif
        };
        if (HResultHolder hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options, D2DFactory.GetAddressOf()); !hr)
        {
            Logger.Warning(u"Win32 WindowManager cannot create D2D factory: {}\n", hr);
        }
    }
    ~WindowManagerWin32() final { }

    static void SetDPIMode()
    {
        if (common::GetWinBuildNumber() >= 14393) // since win10 1703(rs2)
        {
            for (const auto mode : DPIModes)
                if (SetThreadDpiAwarenessContext(mode) != NULL)
                    break;
        }
    }

    void OnInitialize(const void* info) final
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

        Win32Backend::OnInitialize(info);
    }

    void OnPrepare() noexcept final
    {
        ThreadId = GetCurrentThreadId();
    }
    void OnMessageLoop() final
    {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)//, PM_REMOVE))
        {
            TranslateMessage(&msg);
            bool finish = false;
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
                case MessageClose:
                    DestroyWindow(static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(msg.wParam))->Handle);
                    continue;
                case MessageDpi:
                {
                    const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(msg.wParam));
                    float dpi = 96;
                    if (BuildNumber >= 14393)
                        dpi = gsl::narrow_cast<float>(GetDpiForWindow(host->Handle));
                    host->OnDPIChange(dpi, dpi);
                } continue;
                case MessageStop:
                    finish = true;
                    break;
                case MessageUpdTitle:
                {
                    const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(msg.wParam));
                    TitleLock lock(host);
                    SetWindowTextW(host->Handle, reinterpret_cast<const wchar_t*>(host->Title.c_str())); // expects return after finish
                } continue;
                case MessageUpdIcon:
                {
                    const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(msg.wParam));
                    const auto ptrIcon = GetWindowResource(host, WdAttrIndex::Icon);
                    if (ptrIcon)
                    {
                        IconLock lock(host);
                        const auto& holder = static_cast<IconHolder&>(*ptrIcon);
                        DefWindowProcW(host->Handle, WM_SETICON, ICON_SMALL, (LPARAM)holder.SmallIcon());
                        DefWindowProcW(host->Handle, WM_SETICON, ICON_BIG, (LPARAM)holder.BigIcon());
                    }
                } continue;
                default:
                    Logger.Verbose(FmtString(u"Thread unknown MSG[{:x}]\n"), msg.message);
                }
            }
            if (finish)
                break;
            DispatchMessageW(&msg);
        }
    }
    void OnTerminate() noexcept final
    {
        ThreadId = 0;
    }
    bool RequestStop() noexcept final
    {
        return SendGeneralMessage(0, MessageStop);
    }


    void NotifyTask() noexcept final
    {
        PostThreadMessageW(ThreadId, MessageTask, NULL, NULL);
        if (const auto err = GetLastError(); err != NO_ERROR)
            Logger.Error(u"Error when post thread message: {}\n", err);
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
            Logger.Error(u"Error when post thread message: {}\n", GetLastError());
            return false;
        }
        return true;
    }
    void GetClipboard(WdHost& wd, const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type);

    void CreateNewWindow(CreatePayload& payload) final
    {
        if (SendGeneralMessage(reinterpret_cast<uintptr_t>(&payload), MessageCreate))
            payload.Promise.get_future().get();
    }
    void BeforeWindowOpen(WindowHost_*) const final
    {
        SetDPIMode();
    }
    void AfterWindowOpen(WindowHost_* host_) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host_), MessageDpi);
    }
    void UpdateTitle(WindowHost_* host) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host), MessageUpdTitle);
    }
    void UpdateIcon(WindowHost_* host) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host), MessageUpdIcon);
    }
    void CloseWindow(WindowHost_* host) const final
    {
        SendGeneralMessage(reinterpret_cast<uintptr_t>(host), MessageClose);
    }
    void ReleaseWindow(WindowHost_* host_) final
    {
        UnregisterHost(host_);
        const auto host = static_cast<WdHost*>(host_);
        host->Renderer.emplace<std::monostate>();
        ReleaseDC(host->Handle, host->DCHandle);
    }

    OpaqueResource PrepareIcon(WindowHost_& host, xziar::img::ImageView img) const noexcept final
    {
        if (!img.GetDataType().Is(xziar::img::ImgDType::DataTypes::Uint8))
            return {};
        const auto ow = img.GetWidth(), oh = img.GetHeight();
        if (!ow || !oh)
            return {};

        const auto osize = std::min(ow, oh);
        uint16_t tsize = 0;
        if (osize > 256u)
            tsize = 256; // resize to 256x256
        else if (osize < 32u)
            tsize = 32;
        else if (osize % 16 != 0)
            tsize = static_cast<uint16_t>(osize / 16 * 16);
        if (tsize)
            img = img.ResizeTo(tsize, tsize, true, true);
        else
            tsize = static_cast<uint16_t>(osize);
        Ensures(tsize <= 256u && tsize >= 32u && tsize % 16 == 0u);
        const auto simg = img.ResizeTo(16, 16, true, true); // small icon
        Ensures(simg.GetDataType() == img.GetDataType());

        std::vector<uint8_t> maskdata((16 * 16 + tsize * tsize) / 8u, 0);
        if (simg.GetDataType().HasAlpha())
        {
            const auto alphaCh = static_cast<uint8_t>(simg.GetDataType().ChannelCount() - 1);
            const auto salpha = simg.ExtractChannel(alphaCh), balpha = img.ExtractChannel(alphaCh);
            uint8_t* mask = maskdata.data();
            const auto toMask = [&](const Image& ch, uint32_t size) 
            {
                auto data = ch.GetRawPtr<uint64_t>();
                for (uint32_t i = 0; i < size * size; i += 16)
                {
                    uint8_t out = 0;
                    const auto pix8 = *data++;
                    out |= static_cast<uint8_t>((pix8 >> ( 8 - 1)) & 0x01u);
                    out |= static_cast<uint8_t>((pix8 >> (16 - 2)) & 0x02u);
                    out |= static_cast<uint8_t>((pix8 >> (24 - 3)) & 0x04u);
                    out |= static_cast<uint8_t>((pix8 >> (32 - 4)) & 0x08u);
                    out |= static_cast<uint8_t>((pix8 >> (40 - 5)) & 0x10u);
                    out |= static_cast<uint8_t>((pix8 >> (48 - 6)) & 0x20u);
                    out |= static_cast<uint8_t>((pix8 >> (56 - 7)) & 0x40u);
                    out |= static_cast<uint8_t>((pix8 >> (64 - 8)) & 0x80u);
                    *mask++ = out;
                }
            };
            toMask(salpha, 16);
            toMask(balpha, tsize);
        }
        const auto smask = CreateBitmap(16, 16, 1, 1, maskdata.data());
        const auto bmask = CreateBitmap(tsize, tsize, 1, 1, maskdata.data() + (16 * 16 / 8));

        bool useDDB = true;
        auto dc = static_cast<WdHost&>(host).DCHandle;
        if (!dc)
        {
            useDDB = false;
            dc = MemDC;
        }
        const auto toIcon = [&](ImageView img, HBITMAP mask) 
        {
            auto bmp = HBMPHolder::Create(img, dc, useDDB);
            ICONINFO iconInfo
            {
                .fIcon = TRUE,
                .xHotspot = 0u,
                .yHotspot = 0u,
                .hbmMask = mask,
                .hbmColor = bmp,
            };
            return CreateIconIndirect(&iconInfo);
        };
        const auto sicon = toIcon(simg, smask);
        const auto bicon = toIcon( img, bmask);

        return static_cast<OpaqueResource>(IconHolder(sicon, bicon));
    }

    WindowHost Create(const CreateInfo& info_) final
    {
        Win32CreateInfo info;
        static_cast<CreateInfo&>(info) = info_;
        return Create(info);
    }
    std::shared_ptr<Win32WdHost> Create(const Win32CreateInfo& info) final
    {
        return std::make_shared<WdHost>(*this, info);
    }

    common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept final;


    static inline const auto Dummy = RegisterBackend<WindowManagerWin32>();
};



void WindowManagerWin32::GDIRenderer::Render(bool forceRedraw) noexcept
{
    const auto [sizeChanged, needInit] = Update(Window);
    if (sizeChanged)
    {
        const auto bmp = CreateCompatibleBitmap(Window.DCHandle, Width, Height);
        const auto oldBmp = SelectObject(MemDC, bmp);
        if (oldBmp) DeleteObject(oldBmp);
        if (Region) DeleteObject(Region);
        Region = CreateRectRgn(0, 0, Width, Height);
    }
    bool needDraw = false;
    {
        auto imgLock = UseImg();
        const bool imgChanged = imgLock;
        const auto& [bmp, w, h] = AttachedImage;
        needDraw = forceRedraw || imgChanged || (sizeChanged && bmp);
        if (forceRedraw || imgChanged || sizeChanged)
        {
            SimpleTimer timer;
            timer.Start();
            FillRgn(MemDC, Region, BGBrush);
            if (bmp)
            {
                const auto [tw, th] = ResizeWithin(w, h);
                const auto bmpDC = CreateCompatibleDC(Window.DCHandle);
                const auto oldbmp = SelectObject(bmpDC, bmp);
                [[maybe_unused]] const auto ret = StretchBlt(MemDC, 0, 0, tw, th, bmpDC, 0, 0, static_cast<int>(w), static_cast<int>(h), SRCCOPY);
                GdiFlush();
                SelectObject(bmpDC, oldbmp);
                timer.Stop();
                Log().Verbose(FmtString(u"Window[{:x}] rebuild backbuffer in [{} ms].\n"), Window, timer.ElapseMs<true>());
            }
        }
        else
            Ensures(!needDraw);
    }
    if (needDraw)
        BitBlt(Window.DCHandle, 0, 0, Width, Height, MemDC, 0, 0, SRCCOPY);
}

void WindowManagerWin32::D2DRenderer::Render(bool forceRedraw) noexcept
{
    const auto [sizeChanged, needInit] = Update(Window);
    if (sizeChanged)
    {
        Target->Resize(D2D1::SizeU(Width, Height));
    }
    {
        auto imgLock = UseImg();
        const bool imgChanged = imgLock;
        const auto& [bmp, isGray] = AttachedImage;
        if (forceRedraw || imgChanged || sizeChanged)
        {
            Target->BeginDraw();
            Target->Clear(D2D1::ColorF(BgColor));
            if (bmp)
            {
                const auto rtsize = Target->GetSize();
                const auto isize = bmp->GetSize();
                const auto scaleW = rtsize.width / isize.width, scaleH = rtsize.height / isize.height;
                const auto scale = std::min(scaleW, scaleH);
                const auto drawRect = D2D1::RectF(0.f, 0.f, isize.width * scale, isize.height * scale);
                if (isGray)
                    Target->FillOpacityMask(bmp.Get(), BlackBrush.Get(), D2D1_OPACITY_MASK_CONTENT_GRAPHICS, &drawRect);
                else
                    Target->DrawBitmap(bmp.Get(), drawRect);
            }
            const HResultHolder ret = Target->EndDraw();
            IF_UNLIKELY(!ret)
            {
                Log().Warning(FmtString(u"Failed to finish draw : {}\n"), ret);
                if (ret == D2DERR_RECREATE_TARGET)
                {
                    Initialize();
                    Window.Invalidate(true);
                }
            }
        }
    }
}


void WindowManagerWin32::GetClipboard(WdHost& wd, const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type)
{
    std::unique_lock<std::mutex> cpLock(ClipboardLock);
    if (!OpenClipboard(wd.Handle)) return;
    bool finished = false;
    constexpr bool StopAtFault = true;
    wchar_t tmp[256] = { L'\0' };
    for (uint32_t format = EnumClipboardFormats(0); format && !finished; format = EnumClipboardFormats(format))
    {
        const size_t strlen = GetClipboardFormatNameW(format, tmp, 255);
        Logger.Verbose(u"At clipboard format[{}]({}).\n", format, std::wstring_view{ tmp, strlen });
        switch (format)
        {
        case CF_UNICODETEXT:
        case CF_TEXT:
            if (HAS_FIELD(type, ClipBoardTypes::Text))
            {
                const auto data = GetClipboardData(format);
                const auto size = GlobalSize(data);
                const auto ptr = GlobalLock(data);
                if (ptr && size)
                {
                    if (format == CF_UNICODETEXT)
                        finished = InvokeClipboard(handler, ClipBoardTypes::Text, std::u16string_view{ reinterpret_cast<const char16_t*>(ptr), size / sizeof(char16_t) }, StopAtFault);
                    else
                        finished = InvokeClipboard(handler, ClipBoardTypes::Text, std::   string_view{ reinterpret_cast<const char    *>(ptr), size / sizeof(char    ) }, StopAtFault);
                }
                GlobalUnlock(data);
            } break;
        case CF_BITMAP:
        case CF_DSPBITMAP:
            if (HAS_FIELD(type, ClipBoardTypes::Image))
            {
                const auto hbitmap = GetClipboardData(format);
                if (const auto img = xziar::img::ConvertFromHBITMAP(hbitmap); img.GetSize())
                    finished = InvokeClipboard(handler, ClipBoardTypes::Image, ImageView(img), StopAtFault);
            } break;
        case CF_DIB:
        case CF_DIBV5:
            if (HAS_FIELD(type, ClipBoardTypes::Image))
            {
                const auto data = GetClipboardData(format);
                const size_t size = GlobalSize(data);
                const auto ptr = reinterpret_cast<const std::byte*>(data);
                if (!ptr || size <= sizeof(BITMAPINFOHEADER))
                    break;

                const auto& bmpHeader = reinterpret_cast<const BITMAPINFOHEADER*>(ptr);
                const auto pixOffset = size - bmpHeader->biSizeImage;
                if (pixOffset < (format == CF_DIBV5 ? sizeof(BITMAPV5HEADER) : sizeof(BITMAPINFOHEADER)))
                    break;
                
                std::optional<ImageView> img;
                if (const auto zexbmp = xziar::img::GetImageSupport<xziar::img::zex::BmpSupport>(); zexbmp)
                {
                    WrapException([&]()
                    {
                        common::io::MemoryInputStream stream(common::span<const std::byte>{ptr, size});
                        const auto reader_ = zexbmp->GetReader(stream, u"BMP");
                        auto& reader = *static_cast<xziar::img::zex::BmpReader*>(reader_.get());
                        if (reader.ValidateNoHeader(gsl::narrow_cast<uint32_t>(pixOffset)))
                        {
                            if (const auto img_ = reader.Read(xziar::img::ImageDataType::BGRA); img_.GetSize())
                                img = img_;
                        }
                    }, u"read DIB directly");
                }
                if (!img)
                {
                    const auto fsize = size + sizeof(BITMAPFILEHEADER);
                    common::AlignedBuffer buffer(fsize);
                    BITMAPFILEHEADER header
                    {
                        .bfType = 0x4d42, // LE
                        .bfSize = gsl::narrow_cast<uint32_t>(fsize),
                        .bfReserved1 = 0,
                        .bfReserved2 = 0,
                        .bfOffBits = gsl::narrow_cast<uint32_t>(pixOffset + sizeof(BITMAPFILEHEADER)),
                    };
                    memcpy(buffer.GetRawPtr(), &header, sizeof(header));
                    memcpy(buffer.GetRawPtr() + sizeof(header), ptr, size);
                    if (ImageView img_ = TryReadImage(buffer.AsSpan(), u"BMP", xziar::img::ImageDataType::RGBA, u"read DIB with header"); img_.GetSize() > 0)
                        img = img_;
                }
                if (img)
                    finished = InvokeClipboard(handler, ClipBoardTypes::Image, *img, StopAtFault);
            } break;
        case CF_HDROP:
            if (HAS_FIELD(type, ClipBoardTypes::File))
            {
                const auto data = GetClipboardData(format);
                auto list = ProcessDropFile((HDROP)data);
                finished = InvokeClipboard(handler, ClipBoardTypes::File, std::move(list), StopAtFault);
            } break;
        default:
            if (HAS_FIELD(type, ClipBoardTypes::Raw))
            {
                const auto data = GetClipboardData(format);
                finished = InvokeClipboard(handler, ClipBoardTypes::Raw, std::pair<const void*, uint32_t>{data, format}, StopAtFault);
            } break;
        }
    }
    CloseClipboard();
}


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
    { VK_VOLUME_UP,         CommonKeys::VolumeUp },
    { VK_VOLUME_DOWN,       CommonKeys::VolumeDown },
    { VK_VOLUME_MUTE,       CommonKeys::VolumeMute },
    { VK_MEDIA_NEXT_TRACK,  CommonKeys::PlayNext },
    { VK_MEDIA_PREV_TRACK,  CommonKeys::PlayPrev },
    { VK_MEDIA_PLAY_PAUSE,  CommonKeys::PlayPause },
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
        const auto host = static_cast<WdHost*>(payload.Host);
        TheManager->Logger.Success(FmtString(u"Create window HWND[{:x}] with host [{:p}]\n"), handle, (void*)host);
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(host));
        host->Handle = hwnd;
        host->DCHandle = GetDC(hwnd);
        std::visit([](auto& renderer)
        {
            if constexpr (!std::is_same_v<std::decay_t<decltype(renderer)>, std::monostate>)
                renderer.Initialize();
        }, host->Renderer);
        host->Initialize();
    }
    else if (msg == WM_NCCREATE)
    {
        EnableNonClientDpiScaling(hwnd);
    }
    else
    {
        const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(GetWindowLongPtrW(hwnd, 0)));
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
                else if (wParam == SIZE_MINIMIZED)
                {
                    host->OnResize(0, 0);
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
                host->OnDPIChange(newdpi, newdpi);

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
                // return 1(handled) to ignore
                return 1;
            case WM_PAINT:
            {
                RECT rect;
                if (GetUpdateRect(hwnd, &rect, FALSE))
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);
                    EndPaint(hwnd, &ps);
                }
                host->Invalidate();
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
                TheManager->Logger.Verbose(u"Btn state:[{}]\n", common::enum_cast(pressed));*/
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
                event::Position pos = host->LastPos;
                const auto hdrop = (HDROP)wParam;
                POINT point{};
                if (DragQueryPoint(hdrop, &point) == TRUE)
                {
                    ScreenToClient(hwnd, &point);
                    pos = { point.x, point.y };
                }
                auto filelist = ProcessDropFile(hdrop);
                DragFinish(hdrop);
                host->OnDropFile(pos, std::move(filelist));
            }
            return 0;

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
                TheManager->Logger.Verbose(FmtString(u"HWND[{:x}], Host[{:p}], MSG[{:8x}]({})\n"),
                    handle, (void*)host, msg, detail::GetMessageName(msg));
                break;
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

common::PromiseResult<FileList> WindowManagerWin32::OpenFilePicker(const FilePickerInfo& info) noexcept
{
    common::StringPool<char16_t> pool;
    std::vector<std::pair<common::StringPiece<char16_t>, common::StringPiece<char16_t>>> extFilters;
    if (!info.ExtensionFilters.empty())
    {
        for (const auto& p : info.ExtensionFilters)
        {
            if (p.second.empty()) continue;
            constexpr std::u16string_view Null{ u"\0", 1 };
            const auto desc = pool.AllocateConcatString(p.first, Null);
            auto ext = pool.Allocate();
            uint32_t i = 0;
            for (const auto name : p.second)
                ext.Add(i++ ? u";"sv : u""sv, u"*."sv, name);
            ext.Add(Null);
            //const auto ext = pool.AllocateConcatString(u"*."sv, p.second, Null);
            extFilters.emplace_back(desc, ext);
        }
    }
    std::promise<FileList> pms_;
    auto res = common::PromiseResultSTD<FileList>::Get(pms_.get_future());

    std::thread thr([=, title = info.Title, pool = std::move(pool), exts = std::move(extFilters), pms = std::move(pms_)](bool isOpen, bool isFolder, bool isMultiSelect) mutable
    {
        Microsoft::WRL::ComPtr<IFileDialog> fd;

        if (HResultHolder hr = CoCreateInstance(isOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fd)); !hr)
        {
            pms.set_value({});
            return;
        }
        fd->SetTitle(reinterpret_cast<LPCWSTR>(title.data()));

        FILEOPENDIALOGOPTIONS opts = {};
        fd->GetOptions(&opts);
        opts |= FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM | FOS_FORCESHOWHIDDEN;
        if (isFolder) opts |= FOS_PICKFOLDERS;
        if (isOpen)
        {
            opts |= FOS_FILEMUSTEXIST;
            if (isMultiSelect) opts |= FOS_ALLOWMULTISELECT;
        }
        else 
            opts |= FOS_NOREADONLYRETURN | FOS_CREATEPROMPT;
        fd->SetOptions(opts);

        if (!exts.empty())
        {
            std::vector<COMDLG_FILTERSPEC> filters;
            for (const auto& [desc, ext] : exts)
                filters.push_back({ reinterpret_cast<const wchar_t*>(pool.GetStringView(desc).data()), reinterpret_cast<const wchar_t*>(pool.GetStringView(ext).data()) });
            fd->SetFileTypes(gsl::narrow_cast<UINT>(filters.size()), filters.data());
            fd->SetFileTypeIndex(1);
        }

        if (HResultHolder hr = fd->Show(nullptr); !hr)
        {
            pms.set_value({});
            return;
        }

        FileList filelist;
        const auto addPath = [&](const auto& item)
        {
            if (item)
            {
                PWSTR fpath = nullptr;
                item->GetDisplayName(SIGDN_FILESYSPATH, &fpath);
                if (fpath)
                    filelist.AppendFile(reinterpret_cast<const char16_t*>(fpath));
                CoTaskMemFree(fpath);
            }
        };
        if (isOpen && isMultiSelect)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> fod;
            if (HResultHolder hr = fd->QueryInterface(fod.GetAddressOf()); !hr)
            {
                pms.set_value({});
                return;
            }
            Microsoft::WRL::ComPtr<IShellItemArray> results;
            fod->GetResults(&results);
            if (!results)
            {
                pms.set_value({});
                return;
            }
            DWORD resCount = 0;
            results->GetCount(&resCount);
            if (!resCount)
            {
                pms.set_value({});
                return;
            }
            for (uint32_t i = 0; i < resCount; ++i)
            {
                Microsoft::WRL::ComPtr<IShellItem> item;
                results->GetItemAt(i, &item);
                addPath(item);
            }
        }
        else
        {
            Microsoft::WRL::ComPtr<IShellItem> item;
            fd->GetResult(&item);
            addPath(item);
        }

        pms.set_value(std::move(filelist));
    }, info.IsOpen, info.IsFolder, info.AllowMultiSelect);
    thr.detach();
    return res;
}


}