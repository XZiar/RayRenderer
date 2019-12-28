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
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(void)> Task;
        InvokeNode(std::function<void(void)>&& task) : Task(std::move(task)) { }
    };
    static std::shared_ptr<WindowManager> CreateManager();

    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;

    bool OnStart(std::any cookie) noexcept override;
    void OnStop() noexcept override;
    LoopState OnLoop() override;
protected:
    std::vector<std::pair<uintptr_t, WindowHost_*>> WindowList;

    WindowManager();

    template<typename T>
    void RegisterHost(T handle, WindowHost_* host)
    {
        WindowList.emplace_back((uintptr_t)handle, host);
    }
    bool UnregisterHost(WindowHost_* host);
    void HandleTask();

    virtual void Initialize() = 0;
    virtual void Terminate() noexcept = 0;
    virtual void NotifyTask() noexcept = 0;
    virtual void MessageLoop() = 0;
public:
    common::mlog::MiniLogger<false> Logger;

    ~WindowManager() override;
    virtual void CreateNewWindow(WindowHost_* host) = 0;
    virtual void CloseWindow(WindowHost_* host) = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    void AddInvoke(std::function<void(void)>&& task);

    static std::shared_ptr<WindowManager> Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
