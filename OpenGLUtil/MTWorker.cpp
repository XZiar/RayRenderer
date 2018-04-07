#include "oglRely.h"
#include "MTWorker.h"
#include "oglUtil.h"
#include <thread>
#include <GL/wglew.h>

namespace oglu::detail
{

void MTWorker::start(oglContext&& context)
{
    Context = std::move(context);
    std::thread(&MTWorker::worker, this).detach();
}

common::PromiseResult<void> MTWorker::doWork(const AsyncTaskFunc& work, const u16string& taskName, const uint32_t stackSize)
{
    SimpleTimer callerTimer;
    callerTimer.Start();
    const auto pms = Executor.AddTask(work, taskName, stackSize);
    callerTimer.Stop();
    oglLog().debug(u"CALL {} add work cost {} us\n", Name, callerTimer.ElapseUs());
    return pms;
}

void MTWorker::worker()
{
    Executor.MainLoop([&]()
    {
        common::SetThreadName(Prefix);
        if (!Context->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", Prefix, Context->Hdc, Context->Hrc, GetLastError());
        }
        oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", Prefix, Context->Hdc, Context->Hrc, oglUtil::getVersion());
        Context->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
    }, [&]()
    {
        //exit
        if (!Context->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", Prefix, Context->Hdc, Context->Hrc, GetLastError());
        }
        Context = nullptr;
        //wglDeleteContext((HGLRC)Context->Hrc);
    });
}

}