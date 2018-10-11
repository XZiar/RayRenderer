#include "FontRely.h"
#include "FontViewer.h"
#include "resource.h"
#include "OpenGLUtil/PointEnhance.hpp"


namespace oglu
{

static const u16string FontViewControlName = u"文本";

void FontViewer::RegisterControllable()
{
    RegistItem<u16string>("Name", "", u"名称", Controllable::ArgType::RawValue)
        .RegistObject<false>(FontViewControlName);
    if (const auto res = prog->GetResource("fontColor"); res)
    {
        const GLint loc = res->location;
        RegistItem<miniBLAS::Vec4>("Color", "", u"颜色", ArgType::Color, {}, u"文字颜色")
            .RegistGetter([loc](const Controllable& self, const string&)
            { return std::get<miniBLAS::Vec4>(*common::container::FindInMap(dynamic_cast<const FontViewer&>(self).prog->getCurUniforms(), loc)); })
            .RegistSetter([res](Controllable& self, const string&, const ControlArg& val)
            { dynamic_cast<FontViewer&>(self).prog->SetVec(res, std::get<miniBLAS::Vec4>(val)); });
    }
    if (const auto res = prog->GetResource("distRange"); res)
    {
        const GLint loc = res->location;
        RegistItem<std::pair<float, float>>("Dist", "", u"边缘阈值", ArgType::RawValue, std::pair<float, float>(0.f, 1.f), u"sdf边缘阈值")
            .RegistGetter([loc](const Controllable& self, const string&)
            {
                const auto& c2d = std::get<b3d::Coord2D>(*common::container::FindInMap(dynamic_cast<const FontViewer&>(self).prog->getCurUniforms(), loc));
                return std::pair{ c2d.u, c2d.v };
            }).RegistSetter([res](Controllable& self, const string&, const ControlArg& val)
            {
                dynamic_cast<FontViewer&>(self).prog->SetVec(res, b3d::Coord2D(std::get<std::pair<float, float>>(val)));
            });
    }
}

FontViewer::FontViewer()
{
    prog.reset(u"FontViewer");
    try
    {
        prog->AddExtShaders(getShaderFromDLL(IDR_SHADER_PRINTFONT));
    }
    catch (OGLException& gle)
    {
        fntLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
        COMMON_THROW(BaseException, u"OpenGL compile fail");
    }
    try
    {
        prog->Link();
    }
    catch (OGLException& gle)
    {
        fntLog().error(u"Fail to link Program:\n{}\n", gle.message);
        COMMON_THROW(BaseException, u"link Program error");
    }

    viewVAO.reset(VAODrawMode::Triangles);
    viewRect.reset();
    {
        const Point pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }),
            pb({ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }),
            pc({ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }),
            pd({ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f });
        Point DatVert[] = { pa,pb,pc, pd,pc,pb };

        viewRect->Write(DatVert, sizeof(DatVert));
        viewVAO->Prepare()
            .SetFloat(viewRect, prog->GetLoc("@VertPos"), sizeof(Point), 2, 0)
            .SetFloat(viewRect, prog->GetLoc("@VertColor"), sizeof(Point), 3, sizeof(Vec3))
            .SetFloat(viewRect, prog->GetLoc("@VertTexc"), sizeof(Point), 2, 2 * sizeof(Vec3))
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