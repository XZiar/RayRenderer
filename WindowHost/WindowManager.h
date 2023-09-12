#pragma once

#include "WindowHostRely.h"
#include "WindowHost.h"

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/LoopBase.h"
#include "SystemCommon/PromiseTask.h"
#include "SystemCommon/SpinLock.h"
#include <map>
#include <thread>
#include <future>


namespace xziar::gui
{
class WindowRunner;
class WindowHost_;

namespace detail
{
common::mlog::MiniLogger<false>& wdLog();

void RegisterBackend(std::unique_ptr<WindowBackend> backend) noexcept;
template<typename T>
inline bool RegisterBackend() noexcept
{
    static_assert(std::is_base_of_v<WindowBackend, T>);
    std::string_view name = T::BackendName;
    try
    {
        RegisterBackend(std::make_unique<T>());
    }
    catch (const common::BaseException& be)
    {
        wdLog().Warning(u"Failed to create backend [{}]: {}\n{}\n", name, be.Message(), be.GetDetailMessage());
    }
    catch (const std::exception& ex)
    {
        wdLog().Warning(u"Failed to create backend [{}]: {}\n", name, ex.what());
    }
    catch (...)
    {
        wdLog().Warning(u"Failed to create backend [{}]\n", name);
    }
    return true;
}

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
using CreatePayload = WaitablePayload<const std::function<std::any(std::string_view)>*>;

template<typename T, typename U>
inline void TryGetT(const T*& dst, const std::any& dat) noexcept
{
    if (!dst)
    {
        const auto ptr = std::any_cast<U>(&dat);
        if (ptr)
            dst = reinterpret_cast<const T*>(ptr);
    }
}
template<typename T, typename... Ts>
inline const T* TryGetFinally(const std::any& dat) noexcept
{
    const T* ret = nullptr;
    if (dat.has_value())
    {
        TryGetT<T, T>(ret, dat);
        (..., void(TryGetT<T, Ts>(ret, dat)));
    }
    return ret;
}
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
    common::container::IntrusiveDoubleLinkList<InvokeNode, common::spinlock::WRSpinLock> InvokeList;
protected:
    WindowManager();

    class TitleLock
    {
        WindowHost_* Host;
    public:
        TitleLock(WindowHost_* host);
        TitleLock(const TitleLock&) noexcept = delete;
        TitleLock(TitleLock&&) noexcept = delete;
        TitleLock& operator=(const TitleLock&) noexcept = delete;
        TitleLock& operator=(TitleLock&&) noexcept = delete;
        ~TitleLock();
    };

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

    virtual void NotifyTask() noexcept = 0;
public:
    mutable common::mlog::MiniLogger<false> Logger;

    virtual ~WindowManager();
    virtual bool CheckCapsLock() const noexcept = 0;
    virtual void CreateNewWindow(CreatePayload& payload) = 0;
    virtual void BeforeWindowOpen(WindowHost_*) const {}
    virtual void AfterWindowOpen(WindowHost_*) const {}
    virtual void UpdateTitle(WindowHost_* host) const = 0;
    virtual void CloseWindow(WindowHost_* host) const = 0;
    virtual void ReleaseWindow(WindowHost_* host) = 0;
    virtual const void* GetWindowData(const WindowHost_* host, std::string_view name) const noexcept;

    void AddInvoke(std::function<void(void)>&& task);

    // void Invoke(std::function<void(WindowHost_&)> task);
};


}

}
