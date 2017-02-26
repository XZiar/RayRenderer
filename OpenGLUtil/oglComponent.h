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
using oglShader = Wrapper<_oglShader, true>;


enum class BufferType : GLenum
{
	Array = GL_ARRAY_BUFFER, Element = GL_ELEMENT_ARRAY_BUFFER, Uniform = GL_UNIFORM_BUFFER, 
	Pixel = GL_PIXEL_UNPACK_BUFFER, Texture = GL_TEXTURE_BUFFER,
};
enum class BufferWriteMode : GLenum
{
	StreamDraw = GL_STREAM_DRAW, StreamRead = GL_STREAM_READ, StreamCopy = GL_STREAM_COPY,
	StaticDraw = GL_STATIC_DRAW, StaticRead = GL_STATIC_READ, StaticCopy = GL_STATIC_COPY,
	DynamicDraw = GL_DYNAMIC_DRAW, DynamicRead = GL_DYNAMIC_READ, DynamicCopy = GL_DYNAMIC_COPY,
};
class OGLUAPI _oglBuffer : public NonCopyable, public NonMovable
{
private:
	friend class oclu::_oclMem;
	friend class _oglTexture;
	friend class _oglVAO;
	friend class _oglProgram;
	friend class UBOManager;
	BufferType bufferType;
	GLuint bufferID = GL_INVALID_INDEX;
	void bind() const;
	void unbind() const;
public:
	_oglBuffer(const BufferType type);
	~_oglBuffer();

	void write(const void *dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
	template<class T>
	void write(const miniBLAS::vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		write(dat.data(), sizeof(T)*dat.size(), mode);
	}
};
using oglBuffer = Wrapper<_oglBuffer, false>;


enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, TexBuf = GL_TEXTURE_BUFFER, };
enum class TextureFormat : GLenum
{
	RGB = GL_RGB, RGBA = GL_RGBA, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F
};
enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };
enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, Clamp = GL_CLAMP, };
class OGLUAPI _oglTexture : public NonCopyable, public NonMovable
{
private:
	friend class _oglProgram;
	friend class TextureManager;
	friend class oclu::_oclMem;
	TextureType type;
	GLuint textureID = GL_INVALID_INDEX;
	oglBuffer innerBuf;
	static void parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype);
	void bind(const uint16_t pos = 0) const;
	void unbind() const;
public:
	_oglTexture(const TextureType _type);
	~_oglTexture();

	void setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void *);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf);
	OPResult<> setBuffer(const TextureFormat format, const oglBuffer& tbo);
};
using oglTexture = Wrapper<_oglTexture, false>;


enum class VAODrawMode : GLenum
{
	Triangles = GL_TRIANGLES
};
enum class IndexSize : GLenum { Byte = GL_UNSIGNED_BYTE, Short = GL_UNSIGNED_SHORT, Int = GL_UNSIGNED_INT };
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
		VAOPrep& setIndex(const oglBuffer& ebo, const IndexSize idxSize);
	};
	VAODrawMode vaoMode;
	DrawMethod drawMethod = DrawMethod::UnPrepared;
	GLuint vaoID = GL_INVALID_INDEX;
	oglBuffer index;
	IndexSize indexType;
	uint8_t indexSizeof;
	vector<GLsizei> sizes;
	vector<void*> poffsets;
	vector<GLint> ioffsets;
	vector<uint32_t> offsets;
	//uint32_t offset, size;
	void bind() const;
	void unbind() const;
	void initSize();
public:
	_oglVAO(const VAODrawMode);
	~_oglVAO();

	VAOPrep prepare();
	void setDrawSize(const uint32_t offset_, const uint32_t size_);
	void setDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_);
};
using oglVAO = Wrapper<_oglVAO, false>;

}

