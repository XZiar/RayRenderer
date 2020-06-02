#pragma once

#include "WindowHostRely.h"

#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include <map>
#include <thread>



namespace xziar::gui
{
class WindowHost_;

namespace detail
{

class WindowManager
{
private:
    struct InvokeNode : public common::NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(void)> Task;
        InvokeNode(std::function<void(void)>&& task) : Task(std::move(task)) { }
    };
    static std::shared_ptr<WindowManager> CreateManager();

    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    std::thread MainThread;

    void Start();
    void Stop();
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

    virtual ~WindowManager();
    virtual void CreateNewWindow(WindowHost_* host) = 0;
    virtual void PrepareForWindow(WindowHost_*) const {}
    virtual void CloseWindow(WindowHost_* host) = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    void AddInvoke(std::function<void(void)>&& task);

    static std::shared_ptr<WindowManager> Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
