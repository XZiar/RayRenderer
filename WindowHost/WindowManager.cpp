#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ThreadEx.h"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"


namespace xziar::gui::detail
{
using common::loop::LoopBase;

std::shared_ptr<WindowManager> CreateManagerImpl(uintptr_t pms);


std::shared_ptr<WindowManager> WindowManager::CreateManager()
{
    std::promise<void> pms;
    const auto manager = CreateManagerImpl(reinterpret_cast<uintptr_t>(&pms));
    pms.get_future().get();
    return manager;
}

WindowManager::WindowManager(uintptr_t pms) :
    LoopBase(LoopBase::GetThreadedExecutor), 
    Logger(u"WindowManager", { common::mlog::GetConsoleBackend() })
{
    Start(reinterpret_cast<std::promise<void>*>(pms));
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


std::shared_ptr<WindowManager> WindowManager::Get()
{
    static auto manager = CreateManager();
    return manager;
}

}