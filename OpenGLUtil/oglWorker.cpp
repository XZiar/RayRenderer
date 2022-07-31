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
            oglLog().Error(u"{} failed with DC[{}] RC[{}]\n", prefix, ShareContext->Host->GetDeviceContext(), ShareContext->Hrc);
        }
        else
        {
            oglLog().Info(u"{} success with DC[{}] RC[{}], GL version {}\n", prefix, ShareContext->Host->GetDeviceContext(), ShareContext->Hrc, ShareContext->Capability->VersionString);
        }
        ShareContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms1.set_value();
    }, [&]()
    {
        const auto& prefix = u"[oglShare]" + Name;
        if (!ShareContext->UnloadContext())
        {
            oglLog().Error(u"{} failed with HDC[{}] HRC[{}]\n", prefix, ShareContext->Host->GetDeviceContext(), ShareContext->Hrc);
        }
        ShareContext.reset();
    });
    IsolateExecutor.Start([&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        common::SetThreadName(prefix);
        if (!IsolateContext->UseContext())
        {
            oglLog().Error(u"{} failed with DC[{}] RC[{}]\n", prefix, ShareContext->Host->GetDeviceContext(), ShareContext->Hrc);
        }
        else
        {
            oglLog().Info(u"{} success with DC[{}] RC[{}], GL version {}\n", prefix, ShareContext->Host->GetDeviceContext(), ShareContext->Hrc, ShareContext->Capability->VersionString);
        }
        IsolateContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        pms2.set_value();
        }, [&]()
    {
        const auto& prefix = u"[oglIsolate]" + Name;
        if (!IsolateContext->UnloadContext())
        {
            oglLog().Error(u"{} failed with DC[{}] RC[{}]\n", prefix, IsolateContext->Host->GetDeviceContext(), IsolateContext->Hrc);
        }
        IsolateContext.reset();
    });
    pms1.get_future().wait();
    pms2.get_future().wait();
}


}