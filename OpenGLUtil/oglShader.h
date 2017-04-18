#pragma once
#include "oglRely.h"
#include "oglInternal.h"

namespace oglu
{

enum class ShaderType : GLenum
{
	Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
	TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
	Compute = GL_COMPUTE_SHADER
};


namespace detail
{


class OGLUAPI _oglShader : public NonCopyable
{
private:
	friend class _oglProgram;
	const ShaderType shaderType;
	GLuint shaderID = GL_INVALID_INDEX;
	string src;
	static string loadFromFile(FILE *fp);
public:
	_oglShader(const ShaderType type, const string& txt);
	_oglShader(const ShaderType type, FILE *fp) : _oglShader(type, loadFromFile(fp)) { };
	~_oglShader();

	void compile();
};


}
class OGLUAPI oglShader : public Wrapper<detail::_oglShader>
{
public:
	using Wrapper::Wrapper;
	static oglShader __cdecl loadFromFile(const ShaderType type, const fs::path& path);
	static auto __cdecl loadFromFiles(const wstring& fname);
};
auto inline __cdecl oglShader::loadFromFiles(const wstring& fname)
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
	vectorEx<oglShader> shaders;
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
//using oglShader = Wrapper<detail::_oglShader>;

}

