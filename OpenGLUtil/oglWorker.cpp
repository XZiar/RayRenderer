#include "oglPch.h"
#include "oglWorker.h"
#include "oglUtil.h"
#include <future>


namespace oglu
{
using std::string;
using std::u16string;

void oglWorker::Start()
{
    const auto curCtx = oglContext_::CurrentContext();
    ShareContext = curCtx->NewContext(true);
    IsolateContext = curCtx->NewContext(false);
    std::promise<void> pms1, pms2;
    ShareExecutor.Start([&]()
    {
        const auto& prefix = u"[oglShare]" + Name;
        common::SetThreadName(prefix);
        if (!ShareContext->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", prefix, ShareContext->GetDeviceContext(), ShareContext->Hrc, PlatFuncs::GetSystemError());
        }
        else
        {
            oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, ShareContext->GetDeviceContext(), ShareContext->Hrc, ShareContext->Capability->VersionString);
        }
        ShareContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms1.set_value();
    }, [&]()
    {
        const auto& prefix = u"[oglShare]" + Name;
        if (!ShareContext->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, ShareContext->GetDeviceContext(), ShareContext->Hrc, PlatFuncs::GetSystemError());
        }
        ShareContext.reset();
    });
    IsolateExecutor.Start([&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        common::SetThreadName(prefix);
        if (!IsolateContext->UseContext())
        {
            oglLog().error(u"{} with HDC[{}] HRC[{}], error: {}\n", prefix, IsolateContext->GetDeviceContext(), IsolateContext->Hrc, PlatFuncs::GetSystemError());
        }
        else
        {
            oglLog().info(u"{} use HDC[{}] HRC[{}], GL version {}\n", prefix, IsolateContext->GetDeviceContext(), IsolateContext->Hrc, IsolateContext->Capability->VersionString);
        }
        IsolateContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms2.set_value();
        }, [&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        if (!IsolateContext->UnloadContext())
        {
            oglLog().error(u"{} terminate with HDC[{}] HRC[{}], error: {}\n", prefix, IsolateContext->GetDeviceContext(), IsolateContext->Hrc, PlatFuncs::GetSystemError());
        }
        IsolateContext.reset();
    });
    pms1.get_future().wait();
    pms2.get_future().wait();
}


}