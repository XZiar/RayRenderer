#pragma once

#include "WindowHostRely.h"

#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "common/PromiseTask.hpp"
#include <map>



namespace xziar::gui
{
class WindowHost_;

namespace detail
{

class WindowManager : private common::loop::LoopBase
{
private:
    static std::shared_ptr<WindowManager> CreateManager();
    MAKE_ENABLER();
#if COMMON_OS_WIN
    static intptr_t __stdcall WindowProc(uintptr_t, uint32_t, uintptr_t, intptr_t);
    void* InstanceHandle = nullptr;
    uint32_t ThreadId = 0;
#else
    void* Connection = nullptr;
    void* Display = nullptr;
    void* Screen = nullptr;
    uint32_t ControlWindow = 0;
#endif
    std::vector<std::pair<uintptr_t, WindowHost_*>> WindowList;

    WindowManager(uintptr_t pms);
    bool OnStart(std::any cookie) noexcept override;
    void OnStop() noexcept override;
    LoopState OnLoop() override;
    void CreateNewWindow_(WindowHost_* host);
public:
    common::mlog::MiniLogger<false> Logger;

    ~WindowManager() override;
    void CreateNewWindow(WindowHost_* host);
    void CloseWindow(WindowHost_* host);
    void ReleaseWindow(WindowHost_* host);

    static std::shared_ptr<WindowManager> Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
