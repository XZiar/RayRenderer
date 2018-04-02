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
        prog->link();
        prog->registerLocation({ "vertPos","","vertTexc","vertColor" }, { "","","","","" });
    }
    catch (OGLException& gle)
    {
        fntLog().error(u"Fail to link Program:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"link Program error");
    }

    viewVAO.reset(VAODrawMode::Triangles);
    viewRect.reset(BufferType::Array);
    {
        const Point pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }),
            pb({ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }),
            pc({ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }),
            pd({ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f });
        Point DatVert[] = { pa,pb,pc, pd,pc,pb };

        viewRect->write(DatVert, sizeof(DatVert));
        viewVAO->setDrawSize(0, 6);
        viewVAO->prepare().set(viewRect, prog->Attr_Vert_Pos, sizeof(Point), 2, 0)
            .set(viewRect, prog->Attr_Vert_Color, sizeof(Point), 3, sizeof(Vec3))
            .set(viewRect, prog->Attr_Vert_Texc, sizeof(Point), 2, 2 * sizeof(Vec3)).end();
    }
    prog->SetUniform("distLowbound", 0.44f);
    prog->SetUniform("distHighbound", 0.57f);
    prog->globalState().setSubroutine("fontRenderer", "sdfMid").end();
    //prog->globalState().setSubroutine("fontRenderer", "plainFont").end();
}

void FontViewer::draw()
{
    prog->draw().draw(viewVAO).end();
}

void FontViewer::bindTexture(const oglTexture& tex)
{
    prog->globalState().setTexture(tex, "tex").end();
}

}