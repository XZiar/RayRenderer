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
    const auto manager = CreateManagerImpl();
    manager->Start();
    return manager;
}

WindowManager::WindowManager() : Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{ }
WindowManager::~WindowManager()
{
    Stop();
}

void WindowManager::Start()
{
    std::promise<void> pms;
    MainThread = std::thread([&]()
        {
            common::SetThreadName(u"WindowManager");
            try
            {
                Initialize();
            }
            catch (common::BaseException & be)
            {
                Logger.error(u"GetError when initialize WindowManager:\n{}\n", be.message);
                pms.set_exception(std::current_exception());
            }
            pms.set_value();

            MessageLoop();

            Terminate();
        });
    pms.get_future().get();
}

void WindowManager::Stop()
{
    if (MainThread.joinable()) // ensure MainThread invalid
    {
        MainThread.join();
    }
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