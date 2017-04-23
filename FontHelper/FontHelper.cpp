#include "FontRely.h"
#include "FontInternal.h"
#include "resource.h"
#include "FontHelper.h"
#include "../common/ResourceHelper.h"


namespace oglu
{

static string getShaderFromDLL(int32_t id)
{
	std::vector<uint8_t> data;
	if (ResourceHelper::getData(data, L"SHADER", id) != ResourceHelper::Result::Success)
		return "";
	data.push_back('\0');
	return string((const char*)data.data());
}


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



FontCreater::FontCreater(const fs::path& fontpath) : ft2(fontpath)
{

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
		const Point pa({ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }),
			pb({ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }),
			pc({ -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }),
			pd({ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f });
		Point DatVert[] = { pa,pb,pc, pd,pc,pb };

		viewRect->write(DatVert, sizeof(DatVert));
		viewVAO->setDrawSize(0, 6);
		viewVAO->prepare().set(viewRect, getProgram().prog->Attr_Vert_Pos, sizeof(Point), 2, 0)
			.set(viewRect, getProgram().prog->Attr_Vert_Color, sizeof(Point), 3, sizeof(Vec3))
			.set(viewRect, getProgram().prog->Attr_Vert_Texc, sizeof(Point), 2, 2 * sizeof(Vec3)).end();
	}
}

void FontViewer::draw()
{
	getProgram().prog->draw().draw(viewVAO).end();
}

}
