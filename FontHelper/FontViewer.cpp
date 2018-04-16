#include "FontRely.h"
#include "FontViewer.h"
#include <cmath>
#include "resource.h"


namespace oglu
{


FontViewer::FontViewer()
{
    using b3d::Point;

    prog.reset(u"FontViewer");
    try
    {
        prog->AddExtShaders(getShaderFromDLL(IDR_SHADER_PRINTFONT));
    }
    catch (OGLException& gle)
    {
        fntLog().error(u"OpenGL compile fail:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"OpenGL compile fail");
    }
    try
    {
        prog->Link();
    }
    catch (OGLException& gle)
    {
        fntLog().error(u"Fail to link Program:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"link Program error");
    }

    viewVAO.reset(VAODrawMode::Triangles);
    viewRect.reset();
    {
        const Point pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }),
            pb({ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }),
            pc({ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }),
            pd({ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f });
        Point DatVert[] = { pa,pb,pc, pd,pc,pb };

        viewRect->Write(DatVert, sizeof(DatVert));
        viewVAO->Prepare()
            .SetFloat(viewRect, prog->Attr_Vert_Pos, sizeof(Point), 2, 0)
            .SetFloat(viewRect, prog->Attr_Vert_Color, sizeof(Point), 3, sizeof(Vec3))
            .SetFloat(viewRect, prog->Attr_Vert_Texc, sizeof(Point), 2, 2 * sizeof(Vec3))
            .SetDrawSize(0, 6);
    }
    prog->State().SetSubroutine("fontRenderer", "sdfMid");
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