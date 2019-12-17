#pragma once

#include "WindowHostRely.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "common/TimeUtil.hpp"
#include "common/PromiseTask.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace xziar::gui
{
class WindowHost_;
using WindowHost = std::shared_ptr<WindowHost_>;

namespace detail
{
class WindowManager;
}


class WDHOSTAPI WindowHost_ : 
    private common::loop::LoopBase,
    public std::enable_shared_from_this<WindowHost_>
{
    friend class detail::WindowManager;
private:
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(WindowHost_&)> Task;
        InvokeNode(std::function<void(WindowHost_&)>&& task) : Task(std::move(task)) { }
    };

    MAKE_ENABLER();
#if COMMON_OS_WIN
    void* Handle = nullptr;
    void* DCHandle = nullptr;
    std::u16string Title;
#else
    uint32_t Handle = 0;
    void* DCHandle = nullptr;
    std::string Title;
#endif
    std::shared_ptr<detail::WindowManager> Manager;
    std::atomic_flag IsOpened = ATOMIC_FLAG_INIT;
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    common::SimpleTimer DrawTimer;
    int32_t Width, Height;
    int32_t sx = 0, sy = 0, lx = 0, ly = 0;//record x/y, last x/y
    uint8_t IsMovingMouse = 0;
    bool HasMoved = false;

    WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title);
    ~WindowHost_() override;
    bool OnStart(std::any cookie) noexcept override final;
    void OnStop() noexcept override final;
    LoopState OnLoop() override final;
    void Initialize();
protected:
    virtual void OnOpen() noexcept;
    virtual void OnClose() noexcept;
    virtual void OnDisplay() noexcept;

    virtual void OnResize(int32_t width, int32_t height) noexcept;
    virtual void OnMouseMove(int32_t dx, int32_t dy, int32_t flags) noexcept;
    virtual void OnMouseWheel(int32_t posx, int32_t posy, int32_t d, int32_t flags) noexcept;
    virtual void OnKeyDown(int32_t posx, int32_t posy, int32_t key) noexcept;
    virtual void OnKeyUp(int32_t posx, int32_t posy, int32_t key) noexcept;
    virtual void OnDropFile(std::u16string_view filePath) noexcept;
public:
    bool Deshake = true;

    common::Delegate<WindowHost_&> Openning;
    common::Delegate<WindowHost_&> Closing;
    common::Delegate<WindowHost_&> Displaying;
    common::Delegate<WindowHost_&, int32_t, int32_t> Resizing;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> MouseMove;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t, int32_t> MouseWheel;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> KeyDown;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> KeyUp;
    common::Delegate<WindowHost_&, std::u16string_view> DropFile;

    void Show();
    WindowHost GetSelf();
    void SetTimerCallback(std::function<bool(WindowHost_&, uint32_t)> funTime, const uint16_t ms);
    void SetTitle(const std::u16string_view title);
    void Invoke(std::function<void(WindowHost_&)> task);
    void InvokeUI(std::function<void(WindowHost_&)> task);

    void Invalidate();
    void Close();

    static WindowHost Create(const int32_t width = 1280, const int32_t height = 720, const std::u16string_view title = {});
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
