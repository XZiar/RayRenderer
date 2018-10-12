#pragma once
#include "RenderCoreRely.h"
#include "RenderPass.h"

namespace rayr
{

class PostProcessor : public NonCopyable, public DefaultRenderPass
{
private:
    struct FBOConfig
    {
        uint16_t Width, Height;
        bool NeedFloatDepth;
    };
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    oglu::oglTex2DS FBOTex;
    oglu::oglTex3DS LutTex;
    oglu::oglImg3D LutImg;
    oglu::oglComputeProgram LutGenerator;
    oglu::oglVBO ScreenBox;
    array<uint32_t, 3> GroupCount;
    const uint32_t LutSize;
    FBOConfig MidFrameConfig;
    float Exposure = 0.0f;
    bool NeedUpdateLUT = true, NeedUpdateFBO = true;
    void RegistControllable();
protected:
public:
    oglu::oglFBO MiddleFrame;
    oglu::oglVAO VAOScreen;
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;
    PostProcessor(const oclu::oclContext ctx, const oclu::oclCmdQue& que, const uint32_t lutSize = 32);
    ~PostProcessor();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"PostProcess"sv;
    }
    constexpr uint32_t GetLutSize() const { return LutSize; }
    std::shared_ptr<GLShader> GetShader() const { return Shader; }
    float GetExposure() const { return Exposure; }
    void SetExposure(const float exposure);
    void SetMidFrame(const uint16_t width, const uint16_t height, const bool needFloatDepth);

    bool UpdateLUT();
    bool UpdateFBO();
    bool CheckNeedUpdate() const { return NeedUpdateLUT; }
};

}
