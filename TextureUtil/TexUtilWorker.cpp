#include "TexUtilPch.h"
#include "TexUtilWorker.h"


namespace oglu::texutil
{
using namespace oclu;
using namespace std::literals;

TexUtilWorker::TexUtilWorker(oglContext&& glContext, const oclContext& clContext) 
    : Executor(u"TexUtil"), GLContext(glContext), CLContext(clContext)
{
    Executor.Start([this]
    {
        common::SetThreadName(u"TexUtil");
        texLog().Success(u"TexUtil thread start running.\n");
        if (!GLContext->UseContext())
        {
            texLog().Error(u"TexUtil cannot use GL context\n");
            return;
        }
        texLog().Info(u"TexUtil use GL context with version {}\n", GLContext->Capability->VersionString);
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);

        if (CLContext)
        {
            CmdQue = oclCmdQue_::Create(CLContext, CLContext->GetGPUDevice());
            if (!CmdQue)
                COMMON_THROW(common::BaseException, u"clQueue initialized failed!");
        }
        else
            texLog().Warning(u"TexUtil has no shared CL context attached\n");
    }, [this]
    {
        CmdQue.reset();
        CLContext.reset();
        //exit
        if (!GLContext->UnloadContext())
            texLog().Error(u"TexUtil cannot terminate GL context\n");
        GLContext.reset();
    });
}

TexUtilWorker::~TexUtilWorker()
{
    Executor.Stop();
}


}