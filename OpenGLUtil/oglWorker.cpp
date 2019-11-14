#include "oglPch.h"
#include "oglWorker.h"
#include "oglUtil.h"


namespace oglu
{
using std::string;
using std::u16string;

void oglWorker::Start()
{
    const auto curCtx = oglContext_::CurrentContext();
    ShareContext = oglContext_::NewContext(curCtx, true);
    IsolateContext = oglContext_::NewContext(curCtx, false);
    std::promise<void> pms1, pms2;
    ShareExecutor.Start([&]()
    {
        const auto& prefix = u"[oglShare]" + Name;
        common::SetThreadName(prefix);
        if (!ShareContext->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", prefix, ShareContext->Hdc, ShareContext->Hrc, PlatFuncs::GetSystemError());
        }
        oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, ShareContext->Hdc, ShareContext->Hrc, oglUtil::GetVersionStr());
        ShareContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms1.set_value();
    }, [&]()
    {
        const auto& prefix = u"[oglShare]" + Name;
        if (!ShareContext->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, ShareContext->Hdc, ShareContext->Hrc, PlatFuncs::GetSystemError());
        }
        ShareContext.reset();
    });
    IsolateExecutor.Start([&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        common::SetThreadName(prefix);
        if (!IsolateContext->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", prefix, IsolateContext->Hdc, IsolateContext->Hrc, PlatFuncs::GetSystemError());
        }
        oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, IsolateContext->Hdc, IsolateContext->Hrc, oglUtil::GetVersionStr());
        IsolateContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms2.set_value();
        }, [&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        if (!IsolateContext->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, IsolateContext->Hdc, IsolateContext->Hrc, PlatFuncs::GetSystemError());
        }
        IsolateContext.reset();
    });
    pms1.get_future().wait();
    pms2.get_future().wait();
}


}