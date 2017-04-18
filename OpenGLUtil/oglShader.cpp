#include "oglRely.h"
#include "oglException.h"
#include "oglInternal.h"
#include "oglShader.h"
#include "BindingManager.h"

namespace oglu
{
namespace detail
{

string _oglShader::loadFromFile(FILE * fp)
{
	fseek(fp, 0, SEEK_END);
	const size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *dat = new char[fsize + 1];
	fread(dat, fsize, 1, fp);
	dat[fsize] = '\0';
	string txt(dat);
	delete[] dat;

	return txt;
}

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
	if (!fs::exists(path))
		COMMON_THROW(FileException, FileException::Reason::NotExist, path, L"cannot open file to load shader");
	FILE *fp = nullptr;
	if (_wfopen_s(&fp, path.c_str(), L"rb") != 0)
		COMMON_THROW(FileException, FileException::Reason::OpenFail, path, L"cannot open file to load shader");
	fseek(fp, 0, SEEK_END);
	const size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	vector<uint8_t> data(fsize + 1);
	fread(data.data(), fsize, 1, fp);
	fclose(fp);
	data[fsize] = '\0';
	string txt((const char*)data.data());
	oglShader shader(type, txt);
	return shader;
}




}
