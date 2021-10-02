#pragma once

#include "WindowHostRely.h"
#include "WindowEvent.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
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
    WDHOSTAPI ~WindowRunner();
    constexpr explicit operator bool() const noexcept { return Manager; }
    WDHOSTAPI bool RunInplace(common::BasicPromise<void>* pms = nullptr) const;
    WDHOSTAPI bool RunNewThread() const;
    WDHOSTAPI bool SupportNewThread() const noexcept;
    WDHOSTAPI bool CheckFeature(std::string_view feat) const noexcept;
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
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(WindowHost_&)> Task;
        InvokeNode(std::function<void(WindowHost_&)>&& task) : Task(std::move(task)) { }
    };
    detail::WindowManager& Manager;
    uint64_t OSData[4] = { 0 };
    std::u16string Title;
    std::atomic_flag IsOpened = ATOMIC_FLAG_INIT;
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    int32_t Width, Height;
    event::Position LastPos, LeftBtnPos;
    event::MouseButton PressedButton = event::MouseButton::None;
    event::ModifierKeys Modifiers = event::ModifierKeys::None;
    bool NeedCheckDrag = true, IsMouseDragging = false, MouseHasLeft = true;

    template<typename T>
    [[nodiscard]] T& GetOSData() noexcept
    {
        static_assert(sizeof(T) <= sizeof(OSData) && alignof(T) <= alignof(uint64_t));
        return *reinterpret_cast<T*>(OSData);
    }
    template<typename T>
    [[nodiscard]] const T& GetOSData() const noexcept
    {
        static_assert(sizeof(T) <= sizeof(OSData) && alignof(T) <= alignof(uint64_t));
        return *reinterpret_cast<const T*>(OSData);
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

    // below delegates are called inside UI thread (Window Loop)
    common::Delegate<WindowHost_&> Openning;
    common::Delegate<WindowHost_&> Displaying;
    common::Delegate<WindowHost_&> Closed;

    // below delegates are called inside Manager thread (Main Loop)
    common::Delegate<WindowHost_&, bool&> Closing;
    common::Delegate<WindowHost_&, int32_t, int32_t> Resizing;
    common::Delegate<WindowHost_&, const event::MouseEvent&> MouseEnter;
    common::Delegate<WindowHost_&, const event::MouseEvent&> MouseLeave;
    common::Delegate<WindowHost_&, const event::MouseButtonEvent&> MouseButtonDown;
    common::Delegate<WindowHost_&, const event::MouseButtonEvent&> MouseButtonUp;
    common::Delegate<WindowHost_&, const event::MouseMoveEvent&> MouseMove;
    common::Delegate<WindowHost_&, const event::MouseDragEvent&> MouseDrag;
    common::Delegate<WindowHost_&, const event::MouseScrollEvent&> MouseScroll;
    common::Delegate<WindowHost_&, const event::KeyEvent&> KeyDown;
    common::Delegate<WindowHost_&, const event::KeyEvent&> KeyUp;
    common::Delegate<WindowHost_&, const event::DropFileEvent&> DropFile;

    // below delegates are called from any thread
    void Show();
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
