#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/Exceptions.h"
#include "common/ContainerEx.hpp"
#include <mutex>
#include <future>


namespace xziar::gui
{

namespace detail
{
common::mlog::MiniLogger<false>& wdLog()
{
    static common::mlog::MiniLogger<false> wdlog(u"WindowHost", { common::mlog::GetConsoleBackend() });
    return wdlog;
}
}

auto& AllBackends() noexcept
{
    static std::vector<std::unique_ptr<WindowBackend>> Backends;
    return Backends;
}
void detail::RegisterBackend(std::unique_ptr<WindowBackend> backend) noexcept
{
    AllBackends().emplace_back(std::move(backend));
}
common::span<WindowBackend* const> WindowBackend::GetBackends() noexcept
{
    static_assert(sizeof(WindowBackend*) == sizeof(std::unique_ptr<WindowBackend>));
    const auto& backends = AllBackends();
    const auto ptr = reinterpret_cast<WindowBackend* const*>(backends.data());
    return { ptr, backends.size() };
}

enum class BackendStatus : uint8_t
{
    None = 0, Inited = 1, Running = 2
};
struct WindowBackend::Pimpl
{
    std::thread MainThread;
    std::promise<void> RunningPromise;
    std::atomic<uint8_t> Status;
    bool SupportNewThread : 1;
    Pimpl(bool supportNewThread) noexcept : Status(0), SupportNewThread(supportNewThread) {}
};

WindowBackend::WindowBackend(bool supportNewThread) noexcept : Impl(std::make_unique<Pimpl>(supportNewThread))
{
}
WindowBackend::~WindowBackend()
{}

void WindowBackend::OnInitialize(const void*) 
{
    [[maybe_unused]] const auto ret = common::TransitAtomicEnum(Impl->Status, BackendStatus::None, BackendStatus::Inited);
    Ensures(ret);
}
void WindowBackend::OnDeInitialize() noexcept 
{
    [[maybe_unused]] const auto ret = common::TransitAtomicEnum(Impl->Status, BackendStatus::Inited, BackendStatus::None);
    Ensures(ret);
}
void WindowBackend::OnPrepare() noexcept { }
void WindowBackend::OnTerminate() noexcept { }

void WindowBackend::CheckIfInited()
{
    if (Impl->Status >= common::enum_cast(BackendStatus::Inited))
        COMMON_THROW(common::BaseException, u"WindowHostBackend already initialized!").Attach("status", Impl->Status.load());
}
bool WindowBackend::Run(bool isNewThread, common::BasicPromise<void>* pms)
{
    if (isNewThread && !Impl->SupportNewThread)
        COMMON_THROW(common::BaseException, u"WindowHostBackend does not support run in new thread!");
    if (common::TransitAtomicEnum(Impl->Status, BackendStatus::Inited, BackendStatus::Running))
    {
        Impl->RunningPromise = {};
        if (isNewThread)
        { // new thread
            std::promise<void> pms2;
            Impl->MainThread = std::thread([&]()
                {
                    common::ThreadObject::GetCurrentThreadObject().SetName(u"WindowManager");
                    OnPrepare();
                    if (pms)
                        pms->SetData();
                    pms2.set_value();
                    OnMessageLoop();
                    OnTerminate();
                    common::TransitAtomicEnum(Impl->Status, BackendStatus::Running, BackendStatus::Inited);
                    Impl->RunningPromise.set_value();
                });
            pms2.get_future().get();
        }
        else
        { // in place
            OnPrepare();
            if (pms)
                pms->SetData();
            OnMessageLoop();
            OnTerminate();
            common::TransitAtomicEnum(Impl->Status, BackendStatus::Running, BackendStatus::Inited);
            Impl->RunningPromise.set_value();
        }
        return true;
    }
    return false;
}

bool WindowBackend::Stop()
{
    if (Impl->Status == common::enum_cast(BackendStatus::Running))
    {
        if (RequestStop())
        {
            Impl->RunningPromise.get_future().get();
            if (Impl->MainThread.joinable()) // ensure MainThread invalid
            {
                Impl->MainThread.join();
            }
            return true;
        }
    }
    return false;
}


namespace detail
{
using common::loop::LoopBase;


WindowManager::WindowManager() : Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{ }
WindowManager::~WindowManager()
{
    //StopNewThread();
}


const void* WindowManager::GetWindowData(const WindowHost_*, std::string_view) const noexcept
{
    return nullptr;
}

bool WindowManager::UnregisterHost(WindowHost_* host)
{
    for (auto it = WindowList.begin(); it != WindowList.end(); ++it)
    {
        if (it->second.get() == host)
        {
            WindowList.erase(it);
            return true;
        }
    }
    return false;
}

void WindowManager::HandleTask()
{
    while (!InvokeList.IsEmpty())
    {
        auto task = InvokeList.Begin();
        task->Task();
        InvokeList.PopNode(task);
    }
}

void WindowManager::AddInvoke(std::function<void(void)>&& task)
{
    InvokeList.AppendNode(new InvokeNode(std::move(task)));
    NotifyTask();
}


}


}