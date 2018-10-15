#include "RenderCoreRely.h"
#include "PostProcessor.h"
#include "SceneManager.h"
#include "resource.h"


namespace rayr
{

using namespace oclu;
using namespace oglu;
using namespace std::literals;

static const u16string PostProcessorName = u"后处理";

void PostProcessor::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", Controllable::ArgType::RawValue)
        .RegistObject<false>(PostProcessorName);
    RegistItem<float>("Exposure", "", u"曝光补偿", ArgType::RawValue, std::pair(-4.0f, 4.0f), u"曝光补偿(ev)")
        .RegistGetter(&PostProcessor::GetExposure).RegistSetter(&PostProcessor::SetExposure);
}

PostProcessor::PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que, const uint32_t lutSize)
    : CLContext(ctx), CmdQue(que), LutSize(lutSize)
{
    LutTex.reset(LutSize, LutSize, LutSize, TextureInnerFormat::RGB10A2);
    LutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
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


    MiddleFrame.reset();
    ScreenBox.reset();
    const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
    Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
    ScreenBox->Write(DatVert, sizeof(DatVert));
    PostShader.reset(u"PostProcess", getShaderFromDLL(IDR_SHADER_POSTPROC));
    VAOScreen.reset(VAODrawMode::Triangles);
    VAOScreen->Prepare()
        .SetFloat(ScreenBox, PostShader->Program->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
        .SetFloat(ScreenBox, PostShader->Program->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
        .SetDrawSize(0, 6);
    PostShader->Program->State()
        .SetSubroutine("ToneMap", "ACES")
        .SetTexture(LutTex, "lut");

    RegistControllable();
}

PostProcessor::~PostProcessor()
{
}

void PostProcessor::SetExposure(const float exposure)
{
    Exposure = exposure; 
    UpdateDemand.Add(PostProcUpdate::LUT);
}

void PostProcessor::SetMidFrame(const uint16_t width, const uint16_t height, const bool needFloatDepth)
{
    MidFrameConfig.Width = width, MidFrameConfig.Height = height, MidFrameConfig.NeedFloatDepth = needFloatDepth;
    UpdateDemand.Add(PostProcUpdate::FBO);
}


bool PostProcessor::UpdateLUT()
{
    if (UpdateDemand.Extract(PostProcUpdate::LUT))
    {
        LutGenerator->SetUniform("exposure", std::pow(2.0f, Exposure));
        LutGenerator->Run(GroupCount[0], GroupCount[1], GroupCount[2]);
        return true;
    }
    return false;
}

bool PostProcessor::UpdateFBO()
{
    if (UpdateDemand.Extract(PostProcUpdate::FBO))
    {
        FBOTex.reset(MidFrameConfig.Width, MidFrameConfig.Height, TextureInnerFormat::RG11B10);
        FBOTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        MiddleFrame->AttachColorTexture(FBOTex, 0);
        oglRBO mainRBO(MidFrameConfig.Width, MidFrameConfig.Height, MidFrameConfig.NeedFloatDepth ? RBOFormat::Depth32Stencil8 : RBOFormat::Depth24Stencil8);
        MiddleFrame->AttachDepthStencilBuffer(mainRBO);
        dizzLog().info(u"FBO resize to [{}x{}], status:{}\n", MidFrameConfig.Width, MidFrameConfig.Height,
            MiddleFrame->CheckStatus() == FBOStatus::Complete ? u"complete" : u"not complete");
        return true;
    }
    return false;
}

void PostProcessor::OnPrepare(RenderPassContext& context)
{
    UpdateFBO();
    MiddleFrame->Use();
    GLContext->ClearFBO();
    context.SetTexture("MainFBTex", FBOTex);
    context.SetFrameBuffer("MainFB", MiddleFrame);
    UpdateLUT();
}

void PostProcessor::OnDraw(RenderPassContext& context)
{
    oglu::oglFBO::UseDefault();
    GLContext->SetSRGBFBO(false);

    const auto cam = context.GetScene()->GetCamera();
    const auto ow = cam->Width, oh = cam->Height;

    PostShader->Program->SetView(cam->GetView());
    PostShader->Program->SetVec("vecCamPos", cam->Position);

    const auto sw = MidFrameConfig.Width * oh / MidFrameConfig.Height;
    const auto widthscale = sw * 1.0f / ow;
    PostShader->Program->Draw().SetUniform("widthscale", widthscale).Draw(VAOScreen);

}


void PostProcessor::Serialize(SerializeUtil & context, ejson::JObject& jself) const
{
}
void PostProcessor::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
}
RESPAK_IMPL_COMP_DESERIALIZE(PostProcessor, oclu::oclContext, oclu::oclCmdQue, uint32_t)
{
    return std::any{};
}

}