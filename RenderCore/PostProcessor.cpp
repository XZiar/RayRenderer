#include "RenderCorePch.h"
#include "PostProcessor.h"
#include "SceneManager.h"
#include "resource.h"


namespace dizz
{
using namespace oclu;
using namespace oglu;
using namespace std::literals;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


static const u16string PostProcessorName = u"后处理";

void PostProcessor::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue)
        .RegistObject<false>(PostProcessorName);
    RegistItem<float>("Exposure", "", u"曝光补偿", ArgType::RawValue, std::pair(-4.0f, 4.0f), u"曝光补偿(ev)")
        .RegistGetter(&PostProcessor::GetExposure).RegistSetter(&PostProcessor::SetExposure);
    RegistItem<bool>("IsEnable", "", u"启用")
        .RegistMember(&PostProcessor::EnablePostProcess);
    constexpr auto NotifyMidFrame = [](PostProcessor& control) { control.FixMidFrame(); };
    RegistItem<uint16_t>("MidWidth", "MidFrame", u"宽", ArgType::RawValue, std::pair(64, 4096))
        .RegistMemberProxy<PostProcessor>([](auto & control) -> auto & { return control.MidFrameConfig.Width; }, NotifyMidFrame);
    RegistItem<uint16_t>("MidHeight", "MidFrame", u"高", ArgType::RawValue, std::pair(64, 4096))
        .RegistMemberProxy<PostProcessor>([](auto & control) -> auto & { return control.MidFrameConfig.Height; }, NotifyMidFrame);
    RegistItem<bool>("IsFloatDepth", "MidFrame", u"浮点深度", ArgType::RawValue, {}, u"使用浮点深度缓冲")
        .RegistMemberProxy<PostProcessor>([](auto & control) -> auto & { return control.MidFrameConfig.NeedFloatDepth; }, NotifyMidFrame);
    AddCategory("MidFrame", u"渲染纹理");
}

class PostProcessor::LutGen
{
public:
    virtual ~LutGen() { }
    virtual void UpdateLUT(const float exposure) = 0;
};
class PostProcessor::ComputeLutGen : public PostProcessor::LutGen
{
private:
    oglu::oglComputeProgram LutGenerator;
    std::array<uint32_t, 3> GroupCount;
    oglu::oglImg3D LutImg;
    oglu::oglTex3DS LutTex;
public:
    ComputeLutGen(const uint32_t lutSize, const oglu::oglTex3DS& tex, const string& lutSrc)
    {
        LutTex = tex;
        ShaderConfig config;
        config.Routines["ToneMap"] = "ACES";
        LutGenerator = oglu::oglComputeProgram_::Create(u"ColorLut", lutSrc, config);
        const auto& localSize = LutGenerator->GetLocalSize();
        GroupCount = { lutSize / localSize[0], lutSize / localSize[1], lutSize / localSize[2] };
        LutImg = oglu::oglImg3D_::Create(tex, oglu::TexImgUsage::WriteOnly);
        LutGenerator->State()
            .SetImage(LutImg, "result");
        LutGenerator->SetVal("step", 1.0f / (lutSize - 1));
        LutGenerator->SetVal("exposure", 1.0f);
    }
    ComputeLutGen(const uint32_t lutSize, const oglu::oglTex3DS tex) : 
        ComputeLutGen(lutSize, tex, LoadShaderFromDLL(IDR_SHADER_CLRLUTGL))
    { }
    ~ComputeLutGen() override { }

    void UpdateLUT(const float exposure) override
    {
        LutGenerator->SetVal("exposure", std::pow(2.0f, exposure));
        LutGenerator->Run(GroupCount[0], GroupCount[1], GroupCount[2]);
    }
};
class PostProcessor::RenderLutGen : public PostProcessor::LutGen
{
private:
    oglu::oglDrawProgram LutGenerator;
    oglu::oglVBO ScreenBox;
    oglu::oglFBOLayered LUTFrame;
    oglu::oglVAO VAOScreen;
    oglu::oglTex3DS LutTex;
    uint32_t LutSize;
public:
    RenderLutGen(const uint32_t lutSize, const oglu::oglTex3DS& tex, const string& lutSrc) : 
        LutTex(tex), LutSize(lutSize)
    {
        ShaderConfig config;
        config.Routines["ToneMap"] = "ACES";
        LutGenerator = oglu::oglDrawProgram_::Create(u"ColorLut", lutSrc, config);

        ScreenBox = oglu::oglArrayBuffer_::Create();
        const mbase::Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        mbase::Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
        ScreenBox->WriteSpan(DatVert);
        ScreenBox->SetName(u"LutScreenBox");
        VAOScreen = oglu::oglVAO_::Create(VAODrawMode::Triangles);
        VAOScreen->SetName(u"LutVAO");
        VAOScreen->Prepare(LutGenerator)
            .SetFloat(ScreenBox, "@VertPos",  sizeof(mbase::Vec4), 2, sizeof(float) * 0)
            .SetFloat(ScreenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);

        LUTFrame = oglu::oglLayeredFrameBuffer_::Create();
        LUTFrame->SetName(u"LutFBO");
        LUTFrame->AttachColorTexture(tex);
        dizzLog().info(u"LUT FBO status:{}\n", LUTFrame->CheckStatus() == FBOStatus::Complete ? u"complete" : u"not complete");

        LutGenerator->SetVal("step", 1.0f / (LutSize - 1));
        LutGenerator->SetVal("exposure", 1.0f);
        LutGenerator->SetVal("lutSize", (int32_t)LutSize);
    }
    RenderLutGen(const uint32_t lutSize, const oglu::oglTex3DS tex) :
        RenderLutGen(lutSize, tex, LoadShaderFromDLL(IDR_SHADER_CLRLUTGL))
    {
    }
    ~RenderLutGen() override { }

    void UpdateLUT(const float exposure) override
    {
        LUTFrame->Use();
        LutGenerator->SetVal("exposure", std::pow(2.0f, exposure));
        LutGenerator->Draw()
            .DrawInstance(VAOScreen, LutSize);
            //.Draw(VAOScreen);

        //const auto lutvals = LutTex->GetData(xziar::img::TextureFormat::RGB10A2);
    }
};

PostProcessor::PostProcessor(const uint32_t lutSize)
    : PostProcessor(lutSize, LoadShaderFromDLL(IDR_SHADER_POSTPROC)) {}
PostProcessor::PostProcessor(const uint32_t lutSize, const string& postSrc)
    : LutSize(lutSize), MidFrameConfig({ 64,64,true })
{
    LutTex = oglu::oglTex3DStatic_::Create(LutSize, LutSize, LutSize, xziar::img::TextureFormat::RGB10A2);
    LutTex->SetName(u"PostProcLUT");
    LutTex->SetProperty(oglu::TextureFilterVal::Linear, oglu::TextureWrapVal::ClampEdge);
    
    if (oglu::oglComputeProgram_::CheckSupport() && oglu::oglImgBase_::CheckSupport())
    {
        try
        {
            LutGenerator = std::make_unique<ComputeLutGen>(LutSize, LutTex);
        }
        catch (const BaseException & be)
        {
            dizzLog().warning(u"unable to create copmpute lut generator:\n {}\n", be.Message());
        }
    }
    if (!LutGenerator)
    {
        LutGenerator = std::make_unique<RenderLutGen>(LutSize, LutTex);
    }

    ScreenBox = oglu::oglArrayBuffer_::Create();
    ScreenBox->SetName(u"PostProcScreenBox");
    const mbase::Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
    mbase::Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb };
    ScreenBox->WriteSpan(DatVert);
    PostShader = std::make_shared<GLShader>(u"PostProcess", postSrc);
    VAOScreen = oglu::oglVAO_::Create(VAODrawMode::Triangles);
    VAOScreen->SetName(u"PostProcVAO");
    VAOScreen->Prepare(PostShader->Program)
        .SetFloat(ScreenBox, "@VertPos",  sizeof(mbase::Vec4), 2, sizeof(float) * 0)
        .SetFloat(ScreenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
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
    MidFrameConfig = { width,height,needFloatDepth };
    FixMidFrame();
}

void PostProcessor::FixMidFrame()
{
    MidFrameConfig.Width &= uint16_t(0xffc0); MidFrameConfig.Height &= uint16_t(0xffc0);
    UpdateDemand.Add(PostProcUpdate::FBO);
}

void PostProcessor::SetEnable(const bool isEnable)
{
    EnablePostProcess = isEnable;
}


bool PostProcessor::UpdateLUT()
{
    if (UpdateDemand.Extract(PostProcUpdate::LUT))
    {
        LutGenerator->UpdateLUT(Exposure);
        return true;
    }
    return false;
}

bool PostProcessor::UpdateFBO()
{
    if (UpdateDemand.Extract(PostProcUpdate::FBO))
    {
        MiddleFrame = oglu::oglFrameBuffer2D_::Create();
        MiddleFrame->SetName(u"PostProcFBO");
        FBOTex = oglu::oglTex2DStatic_::Create(MidFrameConfig.Width, MidFrameConfig.Height, xziar::img::TextureFormat::RG11B10);
        FBOTex->SetName(u"PostProcFBTex");
        FBOTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        PostShader->Program->State().SetTexture(FBOTex, "scene");
        MiddleFrame->AttachColorTexture(FBOTex);
        auto mainRBO = oglRenderBuffer_::Create(MidFrameConfig.Width, MidFrameConfig.Height, MidFrameConfig.NeedFloatDepth ? RBOFormat::Depth32Stencil8 : RBOFormat::Depth24Stencil8);
        mainRBO->SetName(u"PostProcRBO");
        MiddleFrame->AttachDepthStencilBuffer(mainRBO);
        dizzLog().info(u"FBO resize to [{}x{}], status:{}\n", MidFrameConfig.Width, MidFrameConfig.Height,
            MiddleFrame->CheckStatus() == FBOStatus::Complete ? u"complete" : u"not complete");
        return true;
    }
    return false;
}

void PostProcessor::OnPrepare(RenderPassContext& context)
{
    if (EnablePostProcess)
    {
        const auto maker = oglContext_::CurrentContext()->DeclareRange(u"PostProcess-Prepare");
        UpdateFBO();
        UpdateLUT();
        MiddleFrame->Use();
        MiddleFrame->ClearAll();
        context.SetTexture("MainFBTex", FBOTex);
        context.SetFrameBuffer("MainFB", MiddleFrame);
    }
}

void PostProcessor::OnDraw(RenderPassContext& context)
{
    if (EnablePostProcess)
    {
        const auto maker = oglContext_::CurrentContext()->DeclareRange(u"PostProcess-Draw");
        oglu::oglDefaultFrameBuffer_::Get()->Use();
        GLContext->SetSRGBFBO(false);

        const auto cam = context.GetScene()->GetCamera();
        //const auto ow = cam->Width, oh = cam->Height;

        //PostShader->Program->SetView(cam->GetView());
        PostShader->Program->SetVec("vecCamPos", cam->Position);

        PostShader->Program->Draw().Draw(VAOScreen);
    }
}


void PostProcessor::Serialize(SerializeUtil&, xziar::ejson::JObject& jself) const
{
    jself.Add(EJ_FIELD(LutSize))
         .Add(EJ_FIELD(Exposure));
}
void PostProcessor::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object)
{
    object.TryGet(EJ_FIELD(Exposure));
}
RESPAK_IMPL_COMP_DESERIALIZE(PostProcessor, uint32_t)
{
    const auto lutSize = object.Get<uint32_t>("LutSize");
    return std::any(std::tuple(lutSize));
}

}