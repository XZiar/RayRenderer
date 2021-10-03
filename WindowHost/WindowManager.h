#pragma once

#include "WindowHostRely.h"
#include "WindowHost.h"

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include <map>
#include <thread>
#include <future>


namespace xziar::gui
{
class WindowRunner;
class WindowHost_;

namespace detail
{

class WindowManager
{
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
    virtual common::span<const std::string_view> GetFeature() const noexcept;
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
    struct CreatePayload
    {
        WindowHost_* Host;
        const std::function<const void* (std::string_view)>* Provider;
        std::promise<void> Promise;
    };
    common::mlog::MiniLogger<false> Logger;

    virtual ~WindowManager();
    virtual bool CheckCapsLock() const noexcept = 0;
    virtual void CreateNewWindow(CreatePayload& payload) = 0;
    virtual void PrepareForWindow(WindowHost_*) const {}
    virtual void CloseWindow(WindowHost_* host) = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    virtual const void* GetWindowData(const WindowHost_* host, std::string_view name) const noexcept;
    void AddInvoke(std::function<void(void)>&& task);

    static WindowManager& Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
