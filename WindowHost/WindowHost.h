#pragma once

#include "WindowHostRely.h"
#include "WindowEvent.h"
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
class WindowManagerCocoa;

enum class WindowFlag : uint8_t
{
    None = 0x0, Running = 0x1, TitleChanged = 0x2, TitleLocked = 0x4
};
MAKE_ENUM_BITFIELD(WindowFlag)
}


class WindowRunner
{
    friend detail::WindowManager;
    friend WindowHost_;
    detail::WindowManager* Manager;
    WindowRunner(detail::WindowManager* manager) : Manager(manager) {}
public:
    COMMON_NO_COPY(WindowRunner)
    COMMON_DEF_MOVE(WindowRunner)
    constexpr explicit operator bool() const noexcept { return Manager; }
    WDHOSTAPI bool RunInplace(common::BasicPromise<void>* pms = nullptr) const;
    WDHOSTAPI bool RunNewThread() const;
    WDHOSTAPI bool SupportNewThread() const noexcept;
    WDHOSTAPI bool CheckFeature(std::string_view feat) const noexcept;
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
    friend detail::WindowManagerCocoa;
private:
    struct PrivateData;
    detail::WindowManager& Manager;
    PrivateData* Data;
    std::u16string Title;
    int32_t Width, Height;
    event::Position LastPos, LeftBtnPos;
    common::AtomicBitfield<detail::WindowFlag> Flags;
    event::MouseButton PressedButton = event::MouseButton::None;
    event::ModifierKeys Modifiers = event::ModifierKeys::None;
    bool NeedCheckDrag = true, IsMouseDragging = false, MouseHasLeft = true;

    [[nodiscard]] uintptr_t GetPrivateExtData() const noexcept;
    template<typename T>
    [[nodiscard]] T& GetOSData() const noexcept
    {
        static_assert(alignof(T) <= alignof(uint64_t));
        return *reinterpret_cast<T*>(GetPrivateExtData());
    }
    [[nodiscard]] const void* GetWindowData_(std::string_view name) const noexcept;
    bool OnStart(std::any cookie) noexcept override final;
    LoopAction OnLoop() override final;
    void OnStop() noexcept override final;
    void Initialize();
    void RefreshMouseButton(event::MouseButton pressed) noexcept;
protected:
    WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title);
    bool HandleInvoke() noexcept;

    // below callbacks are called inside UI thread (Window Loop)
    virtual void OnOpen() noexcept;
    virtual LoopAction OnLoopPass() = 0;
    virtual void OnDisplay() noexcept;

    // below callbacks are called inside Manager thread (Main Loop)
    [[nodiscard]] virtual bool OnClose() noexcept;
    virtual void OnResize(int32_t width, int32_t height) noexcept;
    virtual void OnMouseEnter(event::Position pos) noexcept;
    virtual void OnMouseLeave() noexcept;
    virtual void OnMouseButton(event::MouseButton changedBtn, bool isPress) noexcept;
    virtual void OnMouseButtonChange(event::MouseButton btn) noexcept;
    virtual void OnMouseMove(event::Position pos) noexcept;
    virtual void OnMouseDrag(event::Position pos) noexcept;
    virtual void OnMouseScroll(event::Position pos, float dh, float dv) noexcept;
    virtual void OnKeyDown(event::CombinedKey key) noexcept;
    virtual void OnKeyUp(event::CombinedKey key) noexcept;
    virtual void OnDropFile(event::Position pos, common::StringPool<char16_t>&& namepool, 
        std::vector<common::StringPiece<char16_t>>&& names) noexcept;
public:
    ~WindowHost_() override;

    event::ModifierKeys GetModifiers() const noexcept { return Modifiers; }
    event::Position     GetLastPosition() const noexcept { return LastPos; }
    template<typename T>
    const T* GetWindowData(std::string_view name) const noexcept 
    { 
        return reinterpret_cast<const T*>(GetWindowData_(name));
    }
    void SetWindowData(std::string_view name, common::span<const std::byte> data, size_t align) const noexcept;
    template<typename T>
    void SetWindowData(std::string_view name, const T& data) const noexcept
    {
        SetWindowData(name, { reinterpret_cast<const std::byte*>(&data), sizeof(T) }, alignof(T));
    }

    // below delegates are called inside UI thread (Window Loop)
    WindowEventDelegate<> Openning() const noexcept;
    WindowEventDelegate<> Displaying() const noexcept;
    WindowEventDelegate<> Closed() const noexcept;

    // below delegates are called inside Manager thread (Main Loop)
    WindowEventDelegate<bool&> Closing() const noexcept;
    WindowEventDelegate<int32_t, int32_t> Resizing() const noexcept;
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
    void Show(const std::function<const void* (std::string_view)>& provider = {});
    void Close();
    WindowHost GetSelf();
    void SetTitle(const std::u16string_view title);
    void Invoke(std::function<void(void)> task);
    void InvokeUI(std::function<void(WindowHost_&)> task);
    virtual void Invalidate();

    static WindowRunner Init();
    static WindowHost CreatePassive(const int32_t width = 1280, const int32_t height = 720, const std::u16string_view title = {});
    static WindowHost CreateActive(const int32_t width = 1280, const int32_t height = 720, const std::u16string_view title = {});
};


class WindowHostPassive : public WindowHost_
{
    friend class WindowHost_;
private:
    MAKE_ENABLER();
    std::atomic_flag IsUptodate = ATOMIC_FLAG_INIT;

    ::common::loop::LoopBase::LoopAction OnLoopPass() override final;
protected:
    WindowHostPassive(const int32_t width, const int32_t height, const std::u16string_view title);
public:
    ~WindowHostPassive() override;
    void Invalidate() override;
};

class WindowHostActive : public WindowHost_
{
    friend class WindowHost_;
private:
    MAKE_ENABLER();
    common::SimpleTimer DrawTimer;
    float TargetFPS = 60.0f;

    ::common::loop::LoopBase::LoopAction OnLoopPass() override final;
    void OnOpen() noexcept override;
protected:
    WindowHostActive(const int32_t width, const int32_t height, const std::u16string_view title);
public:
    ~WindowHostActive() override;

    void SetTargetFPS(float fps) noexcept;
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
