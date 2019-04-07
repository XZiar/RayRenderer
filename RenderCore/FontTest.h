#pragma once
#include "RenderCoreRely.h"
#include "RenderCoreUtil.hpp"
#include "RenderPass.h"
#include "FontHelper/FontHelper.h"

namespace rayr
{
enum class FontUpdate : uint32_t { FONT = 0x1, TARGET = 0x2 };
MAKE_ENUM_BITFIELD(FontUpdate)

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class RAYCOREAPI FontTester : public NonCopyable, public RenderPass
{
private:
    static inline std::u16string TestName = u"FontTest";
    Wrapper<oglu::FontCreator> FontCreator;
    Wrapper<oglu::FontViewer> FontViewer;
    oglu::oglTex2DD FontTex;
    fs::path FontPath;
    char32_t CharBegin = U'我';
    uint32_t CharCount = 16;
    AtomicBitfiled<FontUpdate> UpdateDemand = FontUpdate::FONT | FontUpdate::TARGET;
    void RegistControllable()
    {
        RegistItem<std::u16string>("Name", "", u"名称")
            .RegistObject<false>(TestName);
    }
protected:
    virtual void OnPrepare(RenderPassContext&) override
    {
        using namespace oglu;
        if (UpdateDemand.Extract(FontUpdate::FONT))
        {
            FontCreator->reloadFont(FontPath);
        }
        if (UpdateDemand.Extract(FontUpdate::TARGET))
        {
            try
            {
                auto imgShow = FontCreator->clgraysdfs(CharBegin, CharCount);
                imgShow.FlipVertical(); // pre-flip
                FontTex->SetData(TextureInnerFormat::R8, imgShow, true, false);
                FontTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::ClampEdge);
                FontViewer->BindTexture(FontTex);
            }
            catch (BaseException&)
            {
                //dizzLog().error(u"Font Construct failure:\n{}\n", be.message);
            }
        }
    }
    virtual void OnDraw(RenderPassContext&) override
    {
        GLContext->SetSRGBFBO(false);
        FontViewer->Draw();
    }
public:
    oglu::oglFBO MiddleFrame;
    oglu::oglVAO VAOScreen;
    FontTester(const oclu::oclContext ctx) : FontCreator(ctx), FontViewer(std::in_place)
    {
        FontTex.reset();
        RegistControllable();
    }
    ~FontTester() {}
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"FontTester"sv;
    }

    void SetText(const char32_t chBegin, const uint32_t count)
    {
        CharBegin = chBegin;
        CharCount = count;
        UpdateDemand.Add(FontUpdate::TARGET);
    }
    void SetFont(const fs::path& fontpath)
    {
        FontPath = fontpath;
        UpdateDemand.Add(FontUpdate::FONT | FontUpdate::TARGET);
    }

    virtual void Serialize(SerializeUtil&, ejson::JObject&) const override {}
    virtual void Deserialize(DeserializeUtil&, const ejson::JObjectRef<true>&) override {}
    RESPAK_DECL_COMP_DESERIALIZE("rayr#FontTester")
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif



}
