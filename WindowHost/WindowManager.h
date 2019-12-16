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
#if COMMON_OS_WIN
    void* EventHandle = nullptr;
    uint32_t ThreadId = 0;
#else
    void* Connection = nullptr;
    void* Display = nullptr;
    void* Screen = nullptr;
    uint32_t VisualID = 0;
    uint32_t ColorMap = 0;
    int32_t ConnFD = 0;
#endif
    std::vector<std::pair<uintptr_t, WindowHost_*>> WindowList;
    bool OnStart(std::any cookie) noexcept override;
    void OnStop() noexcept override;
    LoopState OnLoop() override;
    void CreateNewWindow_(WindowHost_* host);
public:
    common::mlog::MiniLogger<false> Logger;

    WindowManager();
    ~WindowManager() override;
    common::PromiseResult<void> CreateNewWindow(WindowHost_* host);
    void CloseWindow(WindowHost_* host);
    void ReleaseWindow(WindowHost_* host);

    static std::shared_ptr<WindowManager> Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
