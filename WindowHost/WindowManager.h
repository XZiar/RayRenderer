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
template<typename T = void>
struct WaitablePayload;
template<>
struct WaitablePayload<void>
{
    WindowHost_* Host;
    std::promise<void> Promise;
    WaitablePayload(WindowHost_* host) : Host(host)
    { }
};
template<typename T>
struct WaitablePayload : WaitablePayload<void>
{
    T ExtraData;
    WaitablePayload(WindowHost_* host, T&& data) : WaitablePayload<void>{ host }, ExtraData(std::forward<T>(data))
    { }
};
using CreatePayload = WaitablePayload<const std::function<const void* (std::string_view)>*>;
//struct CreatePayload : public WaitablePayload<void>
//{
//    const std::function<const void* (std::string_view)>* Provider;
//    CreatePayload(WindowHost_* host, const std::function<const void* (std::string_view)>& provider) :
//        WaitablePayload{ host }, Provider(provider ? &provider : nullptr)
//    { }
//};

class WindowManager
{
    friend WindowRunner;
private:
    struct InvokeNode : public common::NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(void)> Task;
        InvokeNode(std::function<void(void)>&& task) : Task(std::move(task)) { }
    };

    std::vector<std::pair<uintptr_t, WindowHost>> WindowList;
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
    std::thread MainThread;
    std::atomic_bool RunningFlag;

    void StartNewThread();
    void StopNewThread();
    void StartInplace(common::BasicPromise<void>* pms = nullptr);
    virtual bool SupportNewThread() const noexcept = 0;
    virtual common::span<const std::string_view> GetFeature() const noexcept;
protected:

    WindowManager();

    template<typename T>
    void RegisterHost(T handle, WindowHost_* host)
    {
        WindowList.emplace_back((uintptr_t)handle, host->shared_from_this());
    }
    template<typename T>
    WindowHost_* GetWindow(T handle) const noexcept
    {
        const auto window = (uintptr_t)handle;
        for (const auto& pair : WindowList)
        {
            if (pair.first == window)
            {
                return pair.second.get();
            }
        }
        return nullptr;
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
    mutable common::mlog::MiniLogger<false> Logger;

    virtual ~WindowManager();
    virtual bool CheckCapsLock() const noexcept = 0;
    virtual void CreateNewWindow(CreatePayload& payload) = 0;
    virtual void PrepareForWindow(WindowHost_*) const {}
    virtual void UpdateTitle(WindowHost_* host) const = 0;
    virtual void CloseWindow(WindowHost_* host) const = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    virtual const void* GetWindowData(const WindowHost_* host, std::string_view name) const noexcept;

    virtual size_t AllocateOSData(void* ptr) const noexcept = 0;
    virtual void DeallocateOSData(void* ptr) const noexcept = 0;
    void AddInvoke(std::function<void(void)>&& task);

    static WindowManager& Get();
    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
