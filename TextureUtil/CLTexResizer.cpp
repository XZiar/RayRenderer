#include "TexUtilRely.h"
#include "CLTexResizer.h"
#include "resource.h"
#include <future>
#include <thread>


namespace oglu::texutil
{

CLTexResizer::CLTexResizer(const oglContext& glContext) : Executor(u"GLTexResizer"), GLContext(glContext)
{
    Executor.Start([this]
    {
        common::SetThreadName(u"GLTexResizer");
        texLog().success(u"TexResize thread start running.\n");
        if (!GLContext->UseContext())
        {
            texLog().error(u"CLTexResizer cannot use GL context\n");
            return;
        }
        texLog().info(u"CLTexResizer use GL context with version {}\n", oglUtil::getVersion());
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        CLContext = oclu::oclUtil::CreateGLSharedContext(GLContext);
        if (!CLContext)
        {
            texLog().error(u"CLTexResizer cannot create shared CL context from given OGL context\n");
            return;
        }
        CLContext->onMessage = [](const u16string& errtxt)
        {
            texLog().error(u"Error from context:\t{}\n", errtxt);
        };
        ComQue.reset(CLContext, CLContext->Devices[0]);
        if (!ComQue)
            COMMON_THROW(BaseException, u"clQueue initialized failed!");

        oclu::oclProgram clProg(CLContext, getShaderFromDLL(IDR_SHADER_CLRESIZER));
        try
        {
            const string options = CLContext->vendor == oclu::Vendor::NVIDIA ? "-cl-kernel-arg-info -cl-fast-relaxed-math -cl-nv-verbose -DNVIDIA" : "-cl-fast-relaxed-math";
            clProg->Build(options);
        }
        catch (oclu::OCLException& cle)
        {
            texLog().error(u"Fail to build opencl Program:\n{}\n", cle.message);
            COMMON_THROW(BaseException, u"build Program error");
        }
        KernelResizer = clProg->GetKernel("resizer");
    }, [this] 
    {
        KernelResizer.release();
        ComQue.release();
        CLContext.release();
        GLContext->UnloadContext();
        GLContext.release();
    });
}

CLTexResizer::~CLTexResizer()
{
    Executor.Stop();
}

}
