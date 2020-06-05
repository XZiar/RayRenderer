#pragma once
#include "RenderCoreRely.h"
#include "RenderPass.h"
#include "GLShader.h"

namespace rayr
{

enum class PostProcUpdate : uint32_t { LUT = 0x1, FBO = 0x2 };
MAKE_ENUM_BITFIELD(PostProcUpdate)

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RAYCOREAPI PostProcessor : public common::NonCopyable, public RenderPass
{
private:
    struct FBOConfig
    {
        uint16_t Width, Height;
        bool NeedFloatDepth;
    };
    class LutGen;
    class ComputeLutGen;
    class RenderLutGen;

    oglu::oglTex2DS FBOTex;
    oglu::oglTex3DS LutTex;
    std::unique_ptr<LutGen> LutGenerator;
    std::shared_ptr<GLShader> PostShader;
    oglu::oglVBO ScreenBox;
    common::AtomicBitfield<PostProcUpdate> UpdateDemand = PostProcUpdate::LUT | PostProcUpdate::FBO;
    const uint32_t LutSize;
    FBOConfig MidFrameConfig;
    float Exposure = 0.0f;
    bool EnablePostProcess = true;
    void RegistControllable();
    void FixMidFrame();
protected:
    PostProcessor(const uint32_t lutSize, const string& postSrc);
public:
    oglu::oglFBO2D MiddleFrame;
    oglu::oglVAO VAOScreen;
    virtual void OnPrepare(RenderPassContext& context) override;
    virtual void OnDraw(RenderPassContext& context) override;
    PostProcessor(const uint32_t lutSize = 32);
    ~PostProcessor();
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"PostProcess"sv;
    }
    uint32_t GetLutSize() const { return LutSize; }
    std::shared_ptr<GLShader> GetShader() const { return PostShader; }
    float GetExposure() const { return Exposure; }
    void SetExposure(const float exposure);
    void SetMidFrame(const uint16_t width, const uint16_t height, const bool needFloatDepth);
    void SetEnable(const bool isEnable);

    bool UpdateLUT();
    bool UpdateFBO();

    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#PostProcessor")
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}
