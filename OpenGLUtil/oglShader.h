#pragma once
#include "oglRely.h"

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
public:
	_oglShader(const ShaderType type, const string& txt);
	//_oglShader(const ShaderType type, FILE *fp) : _oglShader(type, file::readAllTxt(fp)) { };
	~_oglShader();

	void compile();
};


}
class OGLUAPI oglShader : public Wrapper<detail::_oglShader>
{
public:
	using Wrapper::Wrapper;
	static oglShader __cdecl loadFromFile(const ShaderType type, const fs::path& path);
	static vector<oglShader> __cdecl loadFromFiles(const u16string& fname);
	static vector<oglShader> __cdecl loadFromExSrc(const string& src);
};


}

