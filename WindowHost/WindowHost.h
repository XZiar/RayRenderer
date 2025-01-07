#pragma once

#include "WindowHostRely.h"
#include "WindowEvent.h"
#include "ImageUtil/ImageCore.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include "common/AtomicUtil.hpp"
#include "common/TimeUtil.hpp"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::gui
{
class WindowHost_;
using WindowHost = std::shared_ptr<WindowHost_>;

namespace detail
{
class WindowManager;
class WindowManagerWin32;
class WindowManagerXCB;
class WindowManagerWayland;
class WindowManagerCocoa;

}


struct CreateInfo
{
    std::u16string Title;
    uint32_t Width = 1280, Height = 720;
    uint16_t TargetFPS = 0;
};

enum class ClipBoardTypes : uint8_t
{
    Empty = 0, Text = 0x1, Image = 0x2, File = 0x4, Raw = 0x80
};
MAKE_ENUM_BITFIELD(ClipBoardTypes)

struct FilePickerInfo
{
    std::u16string Title;
    std::vector<std::pair<std::u16string, std::vector<std::u16string>>> ExtensionFilters;
    bool IsOpen = true;
    bool IsFolder = false;
    bool AllowMultiSelect = true;
};
struct IFilePicker
{
    virtual ~IFilePicker() = 0;
    virtual common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept = 0;
};

class WindowBackend : public IFilePicker
{
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> Impl;
    virtual void OnPrepare() noexcept;
    virtual void OnMessageLoop() = 0;
    virtual void OnTerminate() noexcept;
    virtual bool RequestStop() noexcept = 0;
protected:
    WindowBackend(bool supportNewThread) noexcept;
    WDHOSTAPI void EnsureInitState(const bool ask) const;
    virtual void OnInitialize(const void* info);
    virtual void OnDeInitialize() noexcept;
public:
    virtual ~WindowBackend();
    WDHOSTAPI bool IsInited() const noexcept;
    void Init()
    {
        EnsureInitState(false);
        OnInitialize(nullptr);
    }
    WDHOSTAPI bool Run(bool isNewThread, common::BasicPromise<void>* pms = nullptr);
    WDHOSTAPI bool Stop();
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual bool CheckFeature(std::string_view feat) const noexcept = 0;
    
    [[nodiscard]] WDHOSTAPI static common::span<WindowBackend* const> GetBackends() noexcept;
    template<typename T>
    [[nodiscard]] static T* GetBackend() noexcept
    {
        static_assert(std::is_base_of_v<WindowBackend, T>);
        for (const auto& backend : GetBackends())
        {
            if (const auto ld = dynamic_cast<const T*>(backend); ld)
                return ld;
        }
        return nullptr;
    }
    [[nodiscard]] static const WindowBackend* GetBackend(std::string_view name) noexcept
    {
        for (const auto& backend : GetBackends())
        {
            if (backend->Name() == name)
                return backend;
        }
        return nullptr;
    }
    [[nodiscard]] static const WindowBackend* GetBackend(common::span<const std::string_view> features) noexcept
    {
        for (const auto& backend : GetBackends())
        {
            bool match = true;
            for (const auto& feat : features)
            {
                if (!backend->CheckFeature(feat))
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return backend;
        }
        return nullptr;
    }
    virtual WindowHost Create(const CreateInfo& info = {}) = 0;
    common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept override;
};


template<typename... Args>
class WindowEventDelegate;
class CallbackToken
{
    template<typename...> friend class WindowEventDelegate;
    void* CallbackPtr = nullptr;
    uint32_t ID = 0;
};
template<typename... Args>
class WindowEventDelegate
{
    friend WindowHost_;
    void* Target;
    bool (*Handler)(void*, void*, CallbackToken*);
    constexpr WindowEventDelegate(void* target, bool (*handler)(void*, void*, CallbackToken*)) noexcept :
        Target(target), Handler(handler) { }
public:
    template<typename T>
    CallbackToken operator+=(T&& callback)
    {
        std::function<void(WindowHost_&, Args...)> cb;
        if constexpr (std::is_invocable_v<T, WindowHost_&, Args...>)
            cb = std::forward<T>(callback);
        else if constexpr (std::is_invocable_v<T, Args...>)
            cb = [real = std::forward<T>(callback)](WindowHost_&, Args... args) { real(args...); };
        else if constexpr (std::is_invocable_v<T>)
            cb = [real = std::forward<T>(callback)](WindowHost_&, Args...) { real(); };
        else
            static_assert(!common::AlwaysTrue<T>, "unsupported callback type");
        CallbackToken token;
        Handler(Target, &cb, &token);
        return token;
    }
    bool operator-=(CallbackToken token)
    {
        return Handler(Target, nullptr, &token);
    }
};


class WDHOSTAPI WindowHost_ : 
    private common::loop::LoopBase,
    public std::enable_shared_from_this<WindowHost_>
{
    friend detail::WindowManager;
    friend detail::WindowManagerWin32;
    friend detail::WindowManagerXCB;
    friend detail::WindowManagerWayland;
    friend detail::WindowManagerCocoa;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> Impl;
    detail::WindowManager& Manager;
    std::u16string Title;
    int32_t Width, Height;
    event::Position LastPos, LeftBtnPos;
    event::MouseButton PressedButton = event::MouseButton::None;
    event::ModifierKeys Modifiers = event::ModifierKeys::None;
    bool NeedCheckDrag = true, IsMouseDragging = false, MouseHasLeft = true;

    [[nodiscard]] const std::any* GetWindowData_(std::string_view name) const noexcept;
    void SetWindowData_(std::string_view name, std::any&& data) const noexcept;
    bool OnStart(const common::ThreadObject&, std::any& cookie) noexcept final;
    LoopAction OnLoop() final;
    void OnStop() noexcept final;
    bool OnError(std::exception_ptr) noexcept final;
    void Initialize();
    void RefreshMouseButton(event::MouseButton pressed) noexcept;
protected:
    WindowHost_(detail::WindowManager& manager, const CreateInfo& info) noexcept;
    detail::WindowManager& GetManager() noexcept { return Manager; }
    using LoopBase::Wakeup;
    bool HandleInvoke() noexcept;

    // below callbacks are called inside UI thread (Window Loop)
    virtual void OnOpen() noexcept;
    virtual LoopAction OnLoopPass();
    virtual void OnDisplay(bool forceRedraw) noexcept;

    // below callbacks are called inside Manager thread (Main Loop)
    [[nodiscard]] virtual bool OnClose() noexcept;
    virtual void OnResize(int32_t width, int32_t height) noexcept;
    virtual void OnDPIChange(float x, float y) noexcept;
    virtual void OnMouseEnter(event::Position pos) noexcept;
    virtual void OnMouseLeave() noexcept;
    virtual void OnMouseButton(event::MouseButton changedBtn, bool isPress) noexcept;
    virtual void OnMouseButtonChange(event::MouseButton btn) noexcept;
    virtual void OnMouseMove(event::Position pos) noexcept;
    virtual void OnMouseDrag(event::Position pos) noexcept;
    virtual void OnMouseScroll(event::Position pos, float dh, float dv) noexcept;
    virtual void OnKeyDown(event::CombinedKey key) noexcept;
    virtual void OnKeyUp(event::CombinedKey key) noexcept;
    virtual void OnDropFile(event::Position pos, FileList&& list) noexcept;
public:
    ~WindowHost_() override;

    event::ModifierKeys GetModifiers() const noexcept { return Modifiers; }
    event::Position     GetLastPosition() const noexcept { return LastPos; }
    template<typename T>
    const T* GetWindowData(std::string_view name) const noexcept 
    { 
        return std::any_cast<T>(GetWindowData_(name));
    }
    template<typename T>
    void SetWindowData(std::string_view name, const T& data) const noexcept
    {
        SetWindowData_(name, std::any{ data });
    }
    void RemoveWindowData(std::string_view name) const noexcept;

    // below delegates are called inside UI thread (Window Loop)
    WindowEventDelegate<> Openning() const noexcept;
    WindowEventDelegate<> Displaying() const noexcept;
    WindowEventDelegate<> Closed() const noexcept;

    // below delegates are called inside Manager thread (Main Loop)
    WindowEventDelegate<bool&> Closing() const noexcept;
    WindowEventDelegate<int32_t, int32_t> Resizing() const noexcept;
    WindowEventDelegate<> Minimizing() const noexcept;
    WindowEventDelegate<float, float> DPIChanging() const noexcept;
    WindowEventDelegate<const event::MouseEvent&> MouseEnter() const noexcept;
    WindowEventDelegate<const event::MouseEvent&> MouseLeave() const noexcept;
    WindowEventDelegate<const event::MouseButtonEvent&> MouseButtonDown() const noexcept;
    WindowEventDelegate<const event::MouseButtonEvent&> MouseButtonUp() const noexcept;
    WindowEventDelegate<const event::MouseMoveEvent&> MouseMove() const noexcept;
    WindowEventDelegate<const event::MouseDragEvent&> MouseDrag() const noexcept;
    WindowEventDelegate<const event::MouseScrollEvent&> MouseScroll() const noexcept;
    WindowEventDelegate<const event::KeyEvent&> KeyDown() const noexcept;
    WindowEventDelegate<const event::KeyEvent&> KeyUp() const noexcept;
    WindowEventDelegate<const event::DropFileEvent&> DropFile() const noexcept;

    // below delegates are called from any thread

    void Show(const std::function<std::any(std::string_view)>& provider = {});
    void Close();
    WindowHost GetSelf();
    virtual void GetClipboard(const std::function<bool(ClipBoardTypes, const std::any&)>& handler, ClipBoardTypes type);
    void SetTitle(const std::u16string_view title);
    void SetIcon(xziar::img::ImageView img);
    void SetBackground(std::optional<xziar::img::ImageView> img);
    void Invoke(std::function<void(void)> task);
    void InvokeUI(std::function<void(WindowHost_&)> task);
    virtual void Invalidate(bool forceRedraw = false);
    void SetTargetFPS(uint16_t fps) noexcept;
};


class Win32Backend : public WindowBackend
{
protected:
    using WindowBackend::WindowBackend;
public:
    struct Win32CreateInfo : public CreateInfo
    {

    };
    class Win32WdHost : public WindowHost_
    {
    protected:
        using WindowHost_::WindowHost_;
    public:
        [[nodiscard]] virtual void* GetHDC() const noexcept = 0;
        [[nodiscard]] virtual void* GetHWND() const noexcept = 0;
    };
    using WindowBackend::Create;
    [[nodiscard]] virtual std::shared_ptr<Win32WdHost> Create(const Win32CreateInfo& info = {}) = 0;
    common::PromiseResult<FileList> OpenFilePicker(const FilePickerInfo& info) noexcept final;
};
using Win32WdHost = std::shared_ptr<Win32Backend::Win32WdHost>;

class XCBBackend : public WindowBackend
{
protected:
    using WindowBackend::WindowBackend;
public:
    struct XCBCreateInfo : public CreateInfo
    {
    };
    struct XCBInitInfo
    {
        std::string DisplayName;
        bool UsePureXCB = false;
    };
    class XCBWdHost : public WindowHost_
    {
    protected:
        using WindowHost_::WindowHost_;
    public:
        [[nodiscard]] virtual uint32_t GetWindow() const noexcept = 0;
    };
    void Init(const XCBInitInfo& info)
    {
        EnsureInitState(false);
        OnInitialize(&info);
    }
    using WindowBackend::Create;
    [[nodiscard]] virtual std::shared_ptr<XCBWdHost> Create(const XCBCreateInfo& info = {}) = 0;
    [[nodiscard]] virtual void* GetDisplay() const noexcept = 0;
    [[nodiscard]] virtual void* GetConnection() const noexcept = 0;
    [[nodiscard]] virtual int32_t GetDefaultScreen() const noexcept = 0;
};
using XCBWdHost = std::shared_ptr<XCBBackend::XCBWdHost>;

class WaylandBackend : public WindowBackend
{
protected:
    using WindowBackend::WindowBackend;
public:
    struct WaylandCreateInfo : public CreateInfo
    {
        bool PreferLibDecor = false;
    };
    struct WaylandInitInfo
    {
        std::string DisplayName;
    };
    class WaylandWdHost : public WindowHost_
    {
    protected:
        using WindowHost_::WindowHost_;
    public:
        [[nodiscard]] virtual void* GetSurface() const noexcept = 0;
    };
    void Init(const WaylandInitInfo& info)
    {
        EnsureInitState(false);
        OnInitialize(&info);
    }
    using WindowBackend::Create;
    [[nodiscard]] virtual std::shared_ptr<WaylandWdHost> Create(const WaylandCreateInfo& info) = 0;
    [[nodiscard]] virtual void* GetDisplay() const noexcept = 0;
    [[nodiscard]] virtual bool CanServerDecorate() const noexcept = 0;
    [[nodiscard]] virtual bool CanClientDecorate() const noexcept = 0;
};
using WaylandWdHost = std::shared_ptr<WaylandBackend::WaylandWdHost>;

class CocoaBackend : public WindowBackend
{
protected:
    using WindowBackend::WindowBackend;
public:
    struct CocoaCreateInfo : public CreateInfo
    {

    };
    class CocoaWdHost : public WindowHost_
    {
    protected:
        using WindowHost_::WindowHost_;
    public:
        [[nodiscard]] virtual void* GetWindow() const noexcept = 0;
    };
    using WindowBackend::Create;
    [[nodiscard]] virtual std::shared_ptr<CocoaWdHost> Create(const CocoaCreateInfo& info = {}) = 0;
};
using XCBWdHost = std::shared_ptr<XCBBackend::XCBWdHost>;


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
