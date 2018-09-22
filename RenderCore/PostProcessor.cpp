#include "RenderCoreRely.h"
#include "PostProcessor.h"
#include "resource.h"


namespace rayr
{

using namespace oclu;
using namespace oglu;
constexpr static uint32_t LutSize = 32;

PostProcessor::PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que) : CLContext(ctx), GLContext(oglu::oglContext::CurrentContext()), CmdQue(que)
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
    LutTex.reset(LutSize, LutSize, LutSize, TextureInnerFormat::RGB10A2);
    LutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
    LutImg.reset(LutTex, oglu::TexImgUsage::WriteOnly);
    LutGenerator2.reset(u"ColorLut", getShaderFromDLL(IDR_SHADER_CLRLUTGL));
    LutGenerator2->State()
        .SetSubroutine("ToneMap","ACES")
        .SetImage(LutImg, "result");
    LutGenerator2->SetUniform("step", 1.0f / (LutSize - 1));
    LutGenerator2->SetUniform("exposure", 1.0f);
}

bool PostProcessor::UpdateLut()
{
    if (ShouldUpdate)
    {
        LutGenerator2->Run(LutSize, LutSize, LutSize);
    }
    const bool ret = ShouldUpdate;
    ShouldUpdate = false;
    return ret;
}

PostProcessor::~PostProcessor()
{
}

}