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

//upper for format, lower for type
enum class TextureDataFormat : uint8_t
{
	//normalized integer(uint8)
	R8 = 0x00, RG8 = 0x10, RGB8 = 0x20, BGR8 = 0x30, RGBA8 = 0x40, BGRA8 = 0x50,
	//non-normalized integer(uint8)
	R8U = 0x80, RG8U = 0x90, RGB8U = 0xa0, BGR8U = 0xb0, RGBA8U = 0xc0, BGRA8U = 0xd0,
	//float(FP32)
	Rf = 0x07, RGf = 0x17, RGBf = 0x27, BGRf = 0x37, RGBAf = 0x47, BGRAf = 0x57,
};
enum class TextureInnerFormat : GLint
{
	//normalized integer->as float[0,1]
	R8 = GL_R8, RG8 = GL_RG8, RGB8 = GL_RGB8, RGBA8 = GL_RGBA8,
	//non-normalized integer(uint8)
	R8U = GL_R8UI, RG8U = GL_RG8UI, RGB8U = GL_RGB8UI, RGBA8U = GL_RGBA8UI,
	//float(FP32)
	Rf = GL_R32F, RGf = GL_RG32F, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F,
	//compressed(S3TC/DXT1,S3TC/DXT3,S3TC/DXT5,RGTC,BPTC
	BC1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT, BC1A = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BC2 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	BC3 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BC4 = GL_COMPRESSED_RED_RGTC1, BC5 = GL_COMPRESSED_RG_RGTC2,
	BC6H = GL_COMPRESSED_RGBA_BPTC_UNORM, BC7 = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
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
	friend class oclu::inner::_oclMem;
	TextureType type;
	GLuint textureID = GL_INVALID_INDEX;
	oglBuffer innerBuf;
	static TextureManager& getTexMan();
	static uint8_t getDefaultPos();
	const uint8_t defPos;
	static void parseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype);
	static GLenum parseFormat(const TextureDataFormat dformat);
	void bind(const uint8_t pos) const;
	void unbind() const;
public:
	_oglTexture(const TextureType _type);
	~_oglTexture();

	void setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype);
	void setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void *);
	void setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglBuffer& buf);
	OPResult<> setBuffer(const TextureDataFormat dformat, const oglBuffer& tbo);
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

