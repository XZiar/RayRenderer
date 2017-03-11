#pragma once
#include "oglRely.h"
#include "oglBuffer.h"

namespace oglu
{

enum class ShaderType : GLenum
{
	Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
	TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
	Compute = GL_COMPUTE_SHADER
};

enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, TexBuf = GL_TEXTURE_BUFFER, };

enum class TextureFormat : GLenum
{
	RGB = GL_RGB, RGBA = GL_RGBA, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F
};

enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };

enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, Clamp = GL_CLAMP, };

enum class VAODrawMode : GLenum
{
	Triangles = GL_TRIANGLES
};


namespace inner
{


class OGLUAPI _oglShader : public NonCopyable, public NonMovable
{
private:
	friend class _oglProgram;
	ShaderType shaderType;
	GLuint shaderID = GL_INVALID_INDEX;
	string src;
	static string loadFromFile(FILE *fp);
public:
	_oglShader(const ShaderType type, const string& txt);
	_oglShader(const ShaderType type, FILE *fp) : _oglShader(type, loadFromFile(fp)) { };
	~_oglShader();

	OPResult<> compile();
};


class OGLUAPI _oglTexture : public NonCopyable, public NonMovable
{
private:
	friend class TextureManager;
	friend class _oglProgram;
	friend class oclu::_oclMem;
	TextureType type;
	GLuint textureID = GL_INVALID_INDEX;
	oglBuffer innerBuf;
	static TextureManager& getTexMan();
	static uint8_t getDefaultPos();
	const uint8_t defPos;
	static void parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype);
	void bind(const uint8_t pos) const;
	void unbind() const;
public:
	_oglTexture(const TextureType _type);
	~_oglTexture();

	void setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void *);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf);
	OPResult<> setBuffer(const TextureFormat format, const oglBuffer& tbo);
};


class OGLUAPI _oglVAO : public NonCopyable, public NonMovable
{
protected:
	friend class _oglProgram;
	enum class DrawMethod { UnPrepared, Array, Arrays, Index, Indexs };
	class OGLUAPI VAOPrep : public NonCopyable
	{
	private:
		friend class _oglVAO;
		_oglVAO& vao;
		VAOPrep(_oglVAO& vao_);
	public:
		void end();
		/*-param  vbo buffer, vertex attr index, stride(number), size(number), offset(byte)
		*Each group contains (stride) byte, (size) float are taken as an element, 1st element is at (offset) byte*/
		VAOPrep& set(const oglBuffer& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset);
		/*vbo's inner data is assumed to be Point, automatic set VertPos,VertNorm,TexPos*/
		VAOPrep& set(const oglBuffer& vbo, const GLint(&attridx)[3], const GLint offset);
		VAOPrep& setIndex(const oglEBO& ebo);
	};
	VAODrawMode vaoMode;
	DrawMethod drawMethod = DrawMethod::UnPrepared;
	GLuint vaoID = GL_INVALID_INDEX;
	oglEBO index;
	//IndexSize indexType;
	//uint8_t indexSizeof;
	vector<GLsizei> sizes;
	vector<void*> poffsets;
	vector<GLint> ioffsets;
	vector<uint32_t> offsets;
	//uint32_t offset, size;
	void bind() const;
	static void unbind();
	void initSize();
public:
	_oglVAO(const VAODrawMode);
	~_oglVAO();

	VAOPrep prepare();
	void setDrawSize(const uint32_t offset_, const uint32_t size_);
	void setDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_);
	void draw(const uint32_t size, const uint32_t offset = 0) const;
	void draw() const;
};


}


using oglShader = Wrapper<inner::_oglShader, true>;
using oglTexture = Wrapper<inner::_oglTexture, false>;
using oglVAO = Wrapper<inner::_oglVAO, false>;

}

