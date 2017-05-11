#include "FontRely.h"
#include "FontViewer.h"
#include <cmath>
#include "resource.h"


namespace oglu
{

namespace detail
{

FontViewerProgram::FontViewerProgram()
{
	prog.reset();
	auto shaders = oglShader::loadFromExSrc(getShaderFromDLL(IDR_SHADER_PRINTFONT));
	for (auto shader : shaders)
	{
		try
		{
			shader->compile();
			prog->addShader(std::move(shader));
		}
		catch (OGLException& gle)
		{
			fntLog().error(L"OpenGL compile fail:\n{}\n", gle.message);
			COMMON_THROW(BaseException, L"OpenGL compile fail", std::any(shader));
		}
	}
	try
	{
		prog->link();
		prog->registerLocation({ "vertPos","","vertTexc","vertColor" }, { "","","","","" });
	}
	catch (OGLException& gle)
	{
		fntLog().error(L"Fail to link Program:\n{}\n", gle.message);
		COMMON_THROW(BaseException, L"link Program error");
	}
}

}


detail::FontViewerProgram& FontViewer::getProgram()
{
	static detail::FontViewerProgram fvProg;
	return fvProg;
}

FontViewer::FontViewer()
{
	using b3d::Point;
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
		viewVAO->prepare().set(viewRect, getProgram().prog->Attr_Vert_Pos, sizeof(Point), 2, 0)
			.set(viewRect, getProgram().prog->Attr_Vert_Color, sizeof(Point), 3, sizeof(Vec3))
			.set(viewRect, getProgram().prog->Attr_Vert_Texc, sizeof(Point), 2, 2 * sizeof(Vec3)).end();
	}
	getProgram().prog->useSubroutine("fontRenderer", "sdfMid");
	//getProgram().prog->useSubroutine("fontRenderer", "plainFont");
}

void FontViewer::draw()
{
	getProgram().prog->draw().draw(viewVAO).end();
}

void FontViewer::bindTexture(const oglTexture& tex)
{
	getProgram().prog->globalState().setTexture(tex, "tex").end();
}

}