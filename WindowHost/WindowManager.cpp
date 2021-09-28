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

struct ManagerBlock
{
    std::unique_ptr<WindowManager> Manager;
    std::atomic_bool IsRunnerOwned = false;
    ManagerBlock() : Manager(CreateManagerImpl())
    {
        try
        {
            Manager->Initialize();
        }
        catch (const common::BaseException& be)
        {
            Manager->Logger.error(u"GetError when initialize WindowManager:\n{}\n", be.Message());
            std::rethrow_exception(std::current_exception());
        }
    }
};
ManagerBlock& GetManagerBlock()
{
    static ManagerBlock Block;
    return Block;
}


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
    return *GetManagerBlock().Manager;
}

}


WindowRunner::~WindowRunner()
{
    detail::GetManagerBlock().IsRunnerOwned = false;
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


WindowRunner WindowHost_::Init()
{
    auto& block = detail::GetManagerBlock();
    if (!block.IsRunnerOwned.exchange(true))
        return block.Manager.get();
    return nullptr;
}

}