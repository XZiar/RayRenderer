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


namespace inner
{


FontViewerProgram::FontViewerProgram()
{
	prog.reset();
	{
		oglShader vert(ShaderType::Vertex, getShaderFromDLL(IDR_SHADER_FONTVERT));
		try
		{
			vert->compile();
			prog->addShader(std::move(vert));
		}
		catch (BaseException& be)
		{
			//basLog().error(L"Fail to compile Vertex Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Vertex Shader error");
		}
		oglShader frag(ShaderType::Fragment, getShaderFromDLL(IDR_SHADER_FONTFRAG));
		try
		{
			frag->compile();
			prog->addShader(std::move(frag));
		}
		catch (BaseException& be)
		{
			//basLog().error(L"Fail to compile Fragment Shader:\n{}\n", be.message);
			COMMON_THROW(BaseException, L"compile Fragment Shader error");
		}
	}
	try
	{
		prog->link();
		prog->registerLocation({ "vertPos","","vertTexc","vertColor" }, { "","","","","" });
	}
	catch (BaseException& be)
	{
		//basLog().error(L"Fail to link Program:\n{}\n", be.message);
		COMMON_THROW(BaseException, L"link Program error");
	}
}


}



FontCreater::FontCreater(Path& fontpath)
{

}

oglu::inner::FontViewerProgram& FontViewer::getProgram()
{
	static inner::FontViewerProgram fvProg;
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
