#include "FontPch.h"
#include "FontViewer.h"
#include "resource.h"
#include "OpenGLUtil/PointEnhance.hpp"


namespace oglu
{
using std::string;
using std::u16string;
using common::BaseException;


static const u16string FontViewControlName = u"文本";

void FontViewer::RegisterControllable()
{
    Controllable::EnumSet<uint64_t> kk({ {0UL, u"A"}, {1UL, u"B"}, {2UL, u"C"} });
    kk.GetEnumNames();
    kk.ConvertTo(2);
    kk.ConvertFrom(u"");

    RegistItem<u16string>("Name", "", u"名称", Controllable::ArgType::RawValue)
        .RegistObject<false>(FontViewControlName);
    if (const auto res = prog->GetResource("fontColor"); res)
    {
        const auto loc = res->location;
        RegistItem<mbase::Vec4>("Color", "", u"颜色", ArgType::Color, {}, u"文字颜色")
            .RegistGetterProxy<FontViewer>([loc](const FontViewer& self)
            { 
                return std::get<mbase::Vec4>(*common::container::FindInMap(self.prog->getCurUniforms(), loc));
            })
            .RegistSetterProxy<FontViewer>([res](FontViewer & self, const ControlArg& val)
            { 
                self.prog->SetVec(res, std::get<mbase::Vec4>(val));
            });
    }
    if (const auto res = prog->GetResource("distRange"); res)
    {
        const auto loc = res->location;
        RegistItem<std::pair<float, float>>("Dist", "", u"边缘阈值", ArgType::RawValue, std::pair<float, float>(0.f, 1.f), u"sdf边缘阈值")
            .RegistGetterProxy<FontViewer>([loc](const FontViewer & self)
            {
                const auto& c2d = std::get<mbase::Vec2>(*common::container::FindInMap(self.prog->getCurUniforms(), loc));
                return std::pair{ c2d.X, c2d.Y };
            })
            .RegistSetterProxy<FontViewer>([res](FontViewer & self, const ControlArg& val)
            {
                /*const auto dat = std::get<std::pair<float, float>>(val);
                self.prog->SetVec(res, mbase::Vec2(dat.first, dat.second));*/
                self.prog->SetVec(res, mbase::Vec2(std::get<std::pair<float, float>>(val)));
            });
    }
}

FontViewer::FontViewer()
{
    try
    {
        prog = oglDrawProgram_::Create(u"FontViewer", LoadShaderFromDLL(IDR_SHADER_PRINTFONT));
    }
    catch (const OGLException& gle)
    {
        fntLog().error(u"Fail to create glProgram:{}\n{}\n", gle.Message(), gle.GetDetailMessage());
        COMMON_THROW(BaseException, u"OpenGL compile fail");
    }

    viewVAO = oglVAO_::Create(VAODrawMode::Triangles);
    viewRect = oglArrayBuffer_::Create();
    {
        const Point 
            pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, mbase::Vec2{ 0.0f, 0.0f }),
            pb({  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, mbase::Vec2{ 1.0f, 0.0f }),
            pc({ -1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, mbase::Vec2{ 0.0f, 1.0f }),
            pd({  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, mbase::Vec2{ 1.0f, 1.0f });
        Point DatVert[] = { pa,pb,pc, pd,pc,pb };

        viewRect->WriteSpan(DatVert);
        viewVAO->Prepare(prog)
            .SetFloat(viewRect, "@VertPos",   sizeof(Point), 2, 0 * sizeof(mbase::Vec3))
            .SetFloat(viewRect, "@VertColor", sizeof(Point), 3, 1 * sizeof(mbase::Vec3))
            .SetFloat(viewRect, "@VertTexc",  sizeof(Point), 2, 2 * sizeof(mbase::Vec3))
            .SetDrawSize(0, 6);
    }
    prog->State().SetSubroutine("fontRenderer", "sdfMid");
    RegisterControllable();
}

void FontViewer::Draw()
{
    prog->Draw().Draw(viewVAO);
}

void FontViewer::BindTexture(const oglTex2D& tex)
{
    prog->State().SetTexture(tex, "tex");
}

}