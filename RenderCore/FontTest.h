#pragma once
#include "RenderCoreRely.h"
#include "RenderPass.h"
#include "FontHelper/FontHelper.h"

namespace dizz
{
enum class FontUpdate : uint32_t { FONT = 0x1, TARGET = 0x2 };
MAKE_ENUM_BITFIELD(FontUpdate)

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI FontTester : public common::NonCopyable, public RenderPass
{
private:
    static inline std::u16string TestName = u"FontTest";
    std::shared_ptr<oglu::FontCreator> FontCreator;
    std::shared_ptr<oglu::FontViewer> FontViewer;
    oglu::oglTex2DD FontTex;
    fs::path FontPath;
    char32_t CharBegin = U'我';
    uint32_t CharCount = 16;
    common::AtomicBitfield<FontUpdate> UpdateDemand = FontUpdate::FONT | FontUpdate::TARGET;
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
            try
            {
                FontCreator->reloadFont(FontPath);
            }
            catch (const BaseException&)
            {
                //dizzLog().Error(u"Font Construct failure:\n{}\n", be.Message());
            }
        }
        if (UpdateDemand.Extract(FontUpdate::TARGET))
        {
            try
            {
                auto imgShow = FontCreator->clgraysdfs(CharBegin, CharCount);
                imgShow.FlipVertical(); // pre-flip
                FontTex->SetData(xziar::img::TextureFormat::R8, imgShow, true, false);
                FontTex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::ClampEdge);
                FontViewer->BindTexture(FontTex);
            }
            catch (const BaseException&)
            {
                //dizzLog().Error(u"Font Construct failure:\n{}\n", be.Message());
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
    FontTester(const oclu::oclContext ctx) : 
        FontCreator(std::make_shared<oglu::FontCreator>(ctx)), 
        FontViewer(std::make_shared<oglu::FontViewer>())
    {
        FontTex = oglu::oglTex2DDynamic_::Create();
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

    virtual void Serialize(xziar::respak::SerializeUtil&, xziar::ejson::JObject&) const override {}
    virtual void Deserialize(xziar::respak::DeserializeUtil&, const xziar::ejson::JObjectRef<true>&) override {}
    RESPAK_DECL_COMP_DESERIALIZE("dizz#FontTester")
};


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif



}
