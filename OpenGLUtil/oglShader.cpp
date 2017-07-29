#include "oglRely.h"
#include "oglException.h"
#include "oglShader.h"
#include "BindingManager.h"
#include <string_view>

namespace oglu
{
namespace detail
{

_oglShader::_oglShader(const ShaderType type, const string & txt) : shaderType(type), src(txt)
{
	auto ptr = txt.c_str();
	shaderID = glCreateShader(GLenum(type));
	glShaderSource(shaderID, 1, &ptr, NULL);
}

_oglShader::~_oglShader()
{
	if (shaderID != GL_INVALID_INDEX)
		glDeleteShader(shaderID);
}

void _oglShader::compile()
{
	glCompileShader(shaderID);

	GLint result;
	char logstr[4096] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(logstr), NULL, logstr);
		COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, to_wstring(logstr));
	}
}

}

oglShader __cdecl oglShader::loadFromFile(const ShaderType type, const fs::path& path)
{
	using namespace common::file;
	string txt = FileObject::OpenThrow(path, (OpenFlag)((uint8_t)OpenFlag::BINARY | (uint8_t)OpenFlag::READ))
		.ReadAllText();
	oglShader shader(type, txt);
	return shader;
}

vector<oglShader> __cdecl oglShader::loadFromFiles(const wstring& fname)
{
	static pair<wstring, ShaderType> types[] =
	{
		{ L".vert",ShaderType::Vertex },
		{ L".frag",ShaderType::Fragment },
		{ L".geom",ShaderType::Geometry },
		{ L".comp",ShaderType::Compute },
		{ L".tscl",ShaderType::TessCtrl },
		{ L".tsev",ShaderType::TessEval }
	};
	vector<oglShader> shaders;
	for (const auto& type : types)
	{
		fs::path fpath = fname + type.first;
		try
		{
			auto shader = loadFromFile(type.second, fpath);
			shaders.push_back(shader);
		}
		catch (FileException& fe)
		{
			oglLog().warning(L"skip loading {} due to Exception[{}]", fpath.wstring(), fe.message);
		}
	}
	return shaders;
}


vector<oglShader> __cdecl oglShader::loadFromExSrc(const string& src)
{
	using std::string_view;
	vector<oglShader> shaders;
	vector<string_view> params;
	string_view part1, part2;
	for (size_t from = 0, to = string::npos; ;from = to + 1)
	{
		to = src.find_first_of("\r\n", from);
		if (to == string::npos)
			COMMON_THROW(BaseException, L"Invalid shader source");
		if (to - from < 6)
			continue;
		string_view config(&src[from], to - from);
		if(!str::begin_with(config, "//@@$$"))
			continue;
		//has found
		if (from)
			part1 = string_view(src.data(), from);
		if (to < src.length())
			part2 = string_view(&src[to], src.length() - to);
		config.remove_prefix(6);
		params = str::split(config, '|', false);
		break;
	}
	static string glDefs = R"(
#if defined(OGLU_VERT)
#define GLVARY out
#elif defined(OGLU_FRAG)
#define GLVARY in
#else
#define GLVARY 
#endif
)";
	for (const auto& sv : params)
	{
		switch (hash_(sv))
		{
		case "VERT"_hash:
			{
				oglShader shader(ShaderType::Vertex, str::concat<char>(part1, "#define OGLU_VERT\n", glDefs, part2));
				shaders.push_back(shader);
			}break;
		case "FRAG"_hash:
			{
				oglShader shader(ShaderType::Fragment, str::concat<char>(part1, "#define OGLU_FRAG\n", glDefs, part2));
				shaders.push_back(shader);
			}break;
		case "GEOM"_hash:
			{
				oglShader shader(ShaderType::Geometry, str::concat<char>(part1, "#define OGLU_GEOM\n", glDefs, part2));
				shaders.push_back(shader);
			}break;
		default:
			break;
		}
	}
	return shaders;
}

}
