#pragma once

#include "oglRely.h"
#include "oglUtil.h"
#include "common/ThreadEx.inl"
#include "common/AsyncExecutor/AsyncManager.h"
#include <GL/wglew.h>
#include <thread>

namespace oglu::detail
{


using common::asyexe::AsyncTaskFunc;
class MTWorker
{
private:
    const u16string Name, Prefix;
    common::asyexe::AsyncManager Executor;
    HDC hdc;
    HGLRC hrc;
    void worker()
    {
        Executor.MainLoop([&]() 
        {
            common::SetThreadName(Prefix);
            if (!wglMakeCurrent(hdc, hrc))
            {
                oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", Prefix, (void*)hdc, (void*)hrc, GetLastError());
            }
            oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", Prefix, (void*)hdc, (void*)hrc, oglUtil::getVersion());
            oglUtil::setDebug(0x2f, 0x2f, MsgLevel::Notfication);
        }, [&]() 
        {
            //exit
            if (!wglMakeCurrent(hdc, nullptr))
            {
                oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", Prefix, (void*)hdc, (void*)hrc, GetLastError());
            }
            wglDeleteContext(hrc);
        });
    }
public:
    MTWorker(u16string name_) : Name(name_), Prefix(u"OGLU-Worker " + name_), Executor(u"OGLU-" + name_)
    {
    }
    ~MTWorker()
    {
        Executor.Terminate();
    }
    void start(HDC hdc_, HGLRC hrc_)
    {
        hdc = hdc_, hrc = hrc_;
        std::thread(&MTWorker::worker, this).detach();
    }
    common::PromiseResult<void> doWork(const AsyncTaskFunc& work, const u16string& taskName)
    {
        SimpleTimer callerTimer;
        callerTimer.Start();
        const auto pms = Executor.AddTask(work, taskName);
        callerTimer.Stop();
        oglLog().debug(u"CALL {} add work cost {} us\n", Name, callerTimer.ElapseUs());
        return pms;
    }
};



}