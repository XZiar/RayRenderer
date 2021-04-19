#pragma once

#include "WindowHostRely.h"
#include "WindowEvent.h"
#include "MiniLogger/MiniLogger.h"
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
}


class WDHOSTAPI WindowHost_ : 
    private common::loop::LoopBase,
    public std::enable_shared_from_this<WindowHost_>
{
    friend class detail::WindowManager;
    friend class detail::WindowManagerWin32;
    friend class detail::WindowManagerXCB;
private:
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(WindowHost_&)> Task;
        InvokeNode(std::function<void(WindowHost_&)>&& task) : Task(std::move(task)) { }
    };

#if COMMON_OS_WIN
    void* Handle = nullptr;
    void* DCHandle = nullptr;
#else
    uint32_t Handle = 0;
    void* DCHandle = nullptr;
#endif
    std::u16string Title;
    std::shared_ptr<detail::WindowManager> Manager;
    std::atomic_flag IsOpened = ATOMIC_FLAG_INIT;
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    int32_t Width, Height;
    event::Position LastPos, LeftBtnPos;
    event::MouseButton PressedButton = event::MouseButton::None;
    event::ModifierKeys Modifiers = event::ModifierKeys::None;
    bool IsMouseDragging = false, MouseHasLeft = true;

    bool OnStart(std::any cookie) noexcept override final;
    LoopAction OnLoop() override final;
    void OnStop() noexcept override final;
    void Initialize();
    void RefreshMouseButton(event::MouseButton pressed) noexcept;
protected:
    WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title);
    bool HandleInvoke() noexcept;
    virtual LoopAction OnLoopPass() = 0;

    virtual void OnOpen() noexcept;
    virtual void OnClose() noexcept;
    virtual void OnDisplay() noexcept;

    virtual void OnResize(int32_t width, int32_t height) noexcept;
    virtual void OnMouseEnter(event::Position pos) noexcept;
    virtual void OnMouseLeave() noexcept;
    virtual void OnMouseButton(event::MouseButton changedBtn, bool isPress) noexcept;
    virtual void OnMouseButtonChange(event::MouseButton btn) noexcept;
    virtual void OnMouseMove(event::Position pos) noexcept;
    virtual void OnMouseWheel(event::Position pos, float dz) noexcept;
    virtual void OnKeyDown(event::CombinedKey key) noexcept;
    virtual void OnKeyUp(event::CombinedKey key) noexcept;
    virtual void OnDropFile(std::u16string_view filePath) noexcept;
public:
    ~WindowHost_() override;

    event::ModifierKeys GetModifiers() const noexcept { return Modifiers; }
    event::Position     GetLastPosition() const noexcept { return LastPos; }

    common::Delegate<WindowHost_&> Openning;
    common::Delegate<WindowHost_&, bool&> Closing;
    common::Delegate<WindowHost_&> Closed;
    common::Delegate<WindowHost_&> Displaying;
    common::Delegate<WindowHost_&, int32_t, int32_t> Resizing;
    common::Delegate<WindowHost_&, const event::MouseEvent&> MouseEnter;
    common::Delegate<WindowHost_&, const event::MouseEvent&> MouseLeave;
    common::Delegate<WindowHost_&, const event::MouseButtonEvent&> MouseButtonDown;
    common::Delegate<WindowHost_&, const event::MouseButtonEvent&> MouseButtonUp;
    common::Delegate<WindowHost_&, const event::MouseMoveEvent&> MouseMove;
    common::Delegate<WindowHost_&, const event::MouseDragEvent&> MouseDrag;
    common::Delegate<WindowHost_&, const event::MouseWheelEvent&> MouseWheel;
    common::Delegate<WindowHost_&, const event::KeyEvent&> KeyDown;
    common::Delegate<WindowHost_&, const event::KeyEvent&> KeyUp;
    common::Delegate<WindowHost_&, std::u16string_view> DropFile;

    void Show();
    WindowHost GetSelf();
    //void SetTimerCallback(std::function<bool(WindowHost_&, uint32_t)> funTime, const uint16_t ms);
    void SetTitle(const std::u16string_view title);
    void Invoke(std::function<void(void)> task);
    void InvokeUI(std::function<void(WindowHost_&)> task);
    virtual void Invalidate();

    void Close();

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
