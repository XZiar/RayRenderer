#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ThreadEx.h"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"


namespace xziar::gui::detail
{
using common::loop::LoopBase;

std::shared_ptr<WindowManager> CreateManagerImpl();


std::shared_ptr<WindowManager> WindowManager::CreateManager()
{
    std::promise<void> pms;
    const auto manager = CreateManagerImpl();
    manager->Start(&pms);
    pms.get_future().get();
    return manager;
}

WindowManager::WindowManager() :
    LoopBase(LoopBase::GetThreadedExecutor), 
    Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{ }

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

WindowManager::~WindowManager()
{
    Stop();
}


bool WindowManager::OnStart(std::any cookie) noexcept
{
    auto& pms = *std::any_cast<std::promise<void>*>(cookie);
    common::SetThreadName(u"WindowManager");
    try
    {
        Initialize();
    }
    catch (common::BaseException & be)
    {
        Logger.error(u"GetError when initialize WindowManager:\n{}\n", be.message);
        pms.set_exception(std::current_exception());
        return false;
    }
    pms.set_value();
    return true;
}

void WindowManager::OnStop() noexcept
{
    Terminate();
}

#if COMMON_OS_WIN
#endif

LoopBase::LoopState WindowManager::OnLoop()
{
    MessageLoop();
    return LoopBase::LoopState::Finish;
}


void WindowManager::AddInvoke(std::function<void(void)>&& task)
{
    InvokeList.AppendNode(new InvokeNode(std::move(task)));
    NotifyTask();
}

std::shared_ptr<WindowManager> WindowManager::Get()
{
    static auto manager = CreateManager();
    return manager;
}

}