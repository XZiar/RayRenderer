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

    bool OnStart(std::any cookie) noexcept override;
    void OnStop() noexcept override;
    LoopState OnLoop() override;
protected:
    std::vector<std::pair<uintptr_t, WindowHost_*>> WindowList;

    WindowManager();
    virtual void Initialize() = 0;
    virtual void Terminate() noexcept = 0;
    virtual void MessageLoop() = 0;
public:
    common::mlog::MiniLogger<false> Logger;

    ~WindowManager() override;
    virtual void CreateNewWindow(WindowHost_* host) = 0;
    virtual void CloseWindow(WindowHost_* host) = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;

    static std::shared_ptr<WindowManager> Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
