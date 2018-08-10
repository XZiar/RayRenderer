#include "oglRely.h"
#include "MTWorker.h"
#include "oglUtil.h"
#if defined(_WIN32)
#   include "glew/wglew.h"
#   define GetError() GetLastError()
#else
#   include "glew/glxew.h"
#   define GetError() errno
#endif

namespace oglu::detail
{

void MTWorker::start(oglContext&& context)
{
    Context = std::move(context);
    Executor.Start([&]()
    {
        common::SetThreadName(Prefix);
        if (!Context->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", Prefix, Context->Hdc, Context->Hrc, GetError());
        }
        oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", Prefix, Context->Hdc, Context->Hrc, oglUtil::getVersion());
        Context->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
    }, [&]()
    {
        //exit
        if (!Context->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", Prefix, Context->Hdc, Context->Hrc, GetError());
        }
        Context.release();
    });
}

common::PromiseResult<void> MTWorker::DoWork(const AsyncTaskFunc& work, const u16string& taskName, const uint32_t stackSize)
{
    SimpleTimer callerTimer;
    callerTimer.Start();
    const auto pms = Executor.AddTask(work, taskName, stackSize);
    callerTimer.Stop();
    oglLog().debug(u"CALL {} add work cost {} us\n", Name, callerTimer.ElapseUs());
    return pms;
}

}