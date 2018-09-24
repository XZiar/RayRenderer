#include "RenderCoreRely.h"
#include "PostProcessor.h"
#include "resource.h"


namespace rayr
{

using namespace oclu;
using namespace oglu;

PostProcessor::PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que, const uint32_t lutSize) 
    : Controllable(u"后处理"),
    CLContext(ctx), GLContext(oglu::oglContext::CurrentContext()), CmdQue(que), LutSize(lutSize)
{
    //RegistControlItemDirect<float>("exposure", "", u"曝光补偿", Exposure, std::pair(-4.0f, 4.0f), u"");
    RegistControlItemInDirect<float, PostProcessor>("exposure", "", u"曝光补偿", 
        &PostProcessor::GetExposure, &PostProcessor::SetExposure, ArgType::RawValue, std::pair(-4.0f, 4.0f), u"曝光补偿(ev)");

    LutTex.reset(LutSize, LutSize, LutSize, TextureInnerFormat::RGB10A2);
    LutTexView = LutTex->GetTextureView();
    LutTexView->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
    LutImg.reset(LutTex, oglu::TexImgUsage::WriteOnly);
    ShaderConfig config;
    config.Routines["ToneMap"] = "ACES";
    LutGenerator.reset(u"ColorLut");
    LutGenerator->AddExtShaders(getShaderFromDLL(IDR_SHADER_CLRLUTGL), config);
    LutGenerator->Link();
    const auto& localSize = LutGenerator->GetLocalSize();
    GroupCount = { LutSize / localSize[0], LutSize / localSize[1], LutSize / localSize[2] };
    LutGenerator->State()
        .SetImage(LutImg, "result");
    LutGenerator->SetUniform("step", 1.0f / (LutSize - 1));
    LutGenerator->SetUniform("exposure", 1.0f);
}

PostProcessor::~PostProcessor()
{
}

bool PostProcessor::UpdateLut()
{
    if (ShouldUpdate)
    {
        LutGenerator->SetUniform("exposure", std::pow(2.0f, Exposure));
        LutGenerator->Run(GroupCount[0], GroupCount[1], GroupCount[2]);
    }
    const bool ret = ShouldUpdate;
    ShouldUpdate = false;
    return ret;
}


}