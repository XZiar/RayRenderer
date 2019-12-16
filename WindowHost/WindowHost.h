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
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    common::PromiseResult<void> InitializePms;
    common::SimpleTimer DrawTimer;
    int32_t Width, Height;
    int32_t sx = 0, sy = 0, lx = 0, ly = 0;//record x/y, last x/y
    uint8_t IsMovingMouse = 0;
    bool HasMoved = false;
    bool HasInitialized = false;

    WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title);
    ~WindowHost_() override;
    bool OnStart(std::any cookie) noexcept override;
    void OnStop() noexcept override;
    LoopState OnLoop() override;
public:
    bool Deshake = true;

    common::Delegate<WindowHost_&> OnOpen;
    common::Delegate<WindowHost_&> OnClose;
    common::Delegate<WindowHost_&> OnDisplay;
    common::Delegate<WindowHost_&, int32_t, int32_t> OnReshape;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> OnMouseMove;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> OnMouseWheel;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> OnKeyDown;
    common::Delegate<WindowHost_&, int32_t, int32_t, int32_t> OnKeyUp;
    common::Delegate<WindowHost_&, std::u16string_view> OnDropFile;

    WindowHost GetSelf();
    void SetTimerCallback(std::function<bool(WindowHost_&, uint32_t)> funTime, const uint16_t ms);
    void SetTitle(const std::u16string_view title);
    void Invoke(std::function<void(WindowHost_&)> task);
    void InvokeUI(std::function<void(WindowHost_&)> task);

    void Invalidate();
    void Open();
    void Close();

    static WindowHost Create(const int32_t width = 1280, const int32_t height = 720, const std::u16string_view title = {});
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
