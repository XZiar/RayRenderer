#pragma once

#include "WindowHostRely.h"

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include <map>
#include <thread>



namespace xziar::gui
{
class WindowRunner;
class WindowHost_;

namespace detail
{
struct ManagerBlock;

class WindowManager
{
    friend ManagerBlock;
    friend WindowRunner;
private:
    struct InvokeNode : public common::NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(void)> Task;
        InvokeNode(std::function<void(void)>&& task) : Task(std::move(task)) { }
    };

    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    std::thread MainThread;
    std::atomic_bool RunningFlag;

    void StartNewThread();
    void StopNewThread();
    void StartInplace(common::BasicPromise<void>* pms = nullptr);
    virtual bool SupportNewThread() const noexcept = 0;
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

    virtual void Initialize();
    virtual void DeInitialize() noexcept;
    virtual void Prepare() noexcept;
    virtual void MessageLoop() = 0;
    virtual void Terminate() noexcept;
    virtual void NotifyTask() noexcept = 0;
public:
    common::mlog::MiniLogger<false> Logger;

    virtual ~WindowManager();
    virtual bool CheckCapsLock() const noexcept = 0;
    virtual void CreateNewWindow(WindowHost_* host) = 0;
    virtual void PrepareForWindow(WindowHost_*) const {}
    virtual void CloseWindow(WindowHost_* host) = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    void AddInvoke(std::function<void(void)>&& task);

    static WindowManager& Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
