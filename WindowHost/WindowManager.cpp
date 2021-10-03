#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ThreadEx.h"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include <mutex>
#include <future>


namespace xziar::gui
{

namespace detail
{
using common::loop::LoopBase;

std::unique_ptr<WindowManager> CreateManagerImpl();


WindowManager::WindowManager() : RunningFlag(false),
    Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{ }
WindowManager::~WindowManager()
{
    StopNewThread();
}

void WindowManager::StartNewThread()
{
    std::promise<void> pms;
    MainThread = std::thread([&]()
        {
            common::SetThreadName(u"WindowManager");
            Prepare();
            pms.set_value();
            MessageLoop();
            Terminate();
        });
    pms.get_future().get();
}

void WindowManager::StopNewThread()
{
    if (MainThread.joinable()) // ensure MainThread invalid
    {
        MainThread.join();
    }
}

void WindowManager::StartInplace(common::BasicPromise<void>* pms)
{
    Prepare();
    if (pms)
        pms->SetData();
    MessageLoop();
    Terminate();
}

common::span<const std::string_view> WindowManager::GetFeature() const noexcept
{
    return {};
}
const void* WindowManager::GetWindowData(const WindowHost_*, std::string_view) const noexcept
{
    return nullptr;
}


bool WindowManager::UnregisterHost(WindowHost_* host)
{
    for (auto it = WindowList.begin(); it != WindowList.end(); ++it)
    {
        if (it->second == host)
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

void WindowManager::Initialize() { }
void WindowManager::DeInitialize() noexcept { }
void WindowManager::Prepare() noexcept { }
void WindowManager::Terminate() noexcept { }

void WindowManager::AddInvoke(std::function<void(void)>&& task)
{
    InvokeList.AppendNode(new InvokeNode(std::move(task)));
    NotifyTask();
}

WindowManager& WindowManager::Get()
{
    static auto Manager = []()
    {
        auto man = CreateManagerImpl();
        try
        {
            man->Initialize();
        }
        catch (const common::BaseException& be)
        {
            man->Logger.error(u"GetError when initialize WindowManager:\n{}\n", be.Message());
            std::rethrow_exception(std::current_exception());
        }
        return man;
    }();
    return *Manager;
}

}


bool WindowRunner::RunInplace(common::BasicPromise<void>* pms) const
{
    Expects(Manager);
    if (!Manager->RunningFlag.exchange(true))
    {
        Manager->StartInplace(pms);
        return true;
    }
    return false;
}
bool WindowRunner::RunNewThread() const
{
    Expects(Manager && Manager->SupportNewThread());
    if (!Manager->RunningFlag.exchange(true))
    {
        Manager->StartNewThread();
        return true;
    }
    return false;
}
bool WindowRunner::SupportNewThread() const noexcept 
{
    Expects(Manager);
    return Manager->SupportNewThread();
}
bool WindowRunner::CheckFeature(std::string_view feat) const noexcept
{
    Expects(Manager);
    const auto feats = Manager->GetFeature();
    return std::find(feats.begin(), feats.end(), feat) != feats.end();
}

WindowRunner WindowHost_::Init()
{
    return &detail::WindowManager::Get();
}

}