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
    const wstring name, prefix;
    common::asyexe::AsyncManager Executor;
    HDC hdc;
    HGLRC hrc;
    void worker()
    {
        Executor.MainLoop([&]() 
        {
            const std::string threadName = str::to_string(prefix, str::Charset::UTF7, str::Charset::UTF16LE);
            common::SetThreadName(threadName);
            if (!wglMakeCurrent(hdc, hrc))
            {
                oglLog().error(L"{} with HDC[{}] HRC[{}], error: {}\n", prefix, (void*)hdc, (void*)hrc, GetLastError());
            }
            oglLog().info(L"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, (void*)hdc, (void*)hrc, oglUtil::getVersion());
            oglUtil::setDebug(0x2f, 0x2f, MsgLevel::Notfication);
        }, [&]() 
        {
            //exit
            if (!wglMakeCurrent(hdc, nullptr))
            {
                oglLog().error(L"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, (void*)hdc, (void*)hrc, GetLastError());
            }
            wglDeleteContext(hrc);
        });
    }
public:
    MTWorker(wstring name_) : name(name_), prefix(L"OGLU-Worker " + name_), Executor(L"OGLU-" + name_)
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
    common::PromiseResult<void> doWork(const AsyncTaskFunc& work, const std::wstring& taskName)
    {
        SimpleTimer callerTimer;
        callerTimer.Start();
        const auto pms = Executor.AddTask(work, taskName);
        callerTimer.Stop();
        oglLog().debug(L"CALL {} add work cost {} us\n", name, callerTimer.ElapseUs());
        return pms;
    }
};



}