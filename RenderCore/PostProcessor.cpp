#include "RenderCoreRely.h"
#include "PostProcessor.h"
#include "resource.h"


namespace rayr
{

using namespace oclu;
using namespace oglu;

PostProcessor::PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que) : CLContext(ctx), CmdQue(que)
{
    LutProg.reset(CLContext, getShaderFromDLL(IDR_SHADER_CLRLUTCL));
    try
    {
        oclu::CLProgConfig config;
        LutProg->Build(config);
        LutGenerator = LutProg->GetKernel("GenerateLUT_ACES_SRGB");
    }
    catch (OCLException& cle)
    {
        u16string buildLog;
        if (cle.data.has_value())
            buildLog = std::any_cast<u16string>(cle.data);
        basLog().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
    }
    LutTex.reset(64, 64, 64, TextureInnerFormat::RGB10A2);
    LutGenerator->SetSimpleArg(0, 1.0f / 64);
    try
    {
        LutImg.reset(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, LutTex);
        LutImgRaw.reset(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, 64, 64, 64, TextureDataFormat::RGB10A2);
        LutGenerator->SetArg(1, LutImgRaw);
    }
    catch (const BaseException& be)
    {
        basLog().error(u"Failed to create clImage:\n {}\n", be.message);
    }
}

void PostProcessor::UpdateLut()
{
    if (ShouldUpdate)
    {
        LutGenerator->SetSimpleArg(2, Exposure);
        LutGenerator->Run<3>(CmdQue, { 64,64,64 }, false)->wait();
    }
    ShouldUpdate = false;
}

PostProcessor::~PostProcessor()
{
}

}