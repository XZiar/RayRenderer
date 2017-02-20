#pragma once
#include "oglRely.h"
#include <GL/glew.h>

#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <vector>
#include <map>
#include <tuple>


namespace oglu
{
using std::string;
using miniBLAS::vector;
//using std::vector_;
using std::tuple;
using std::map;
using std::function;
using miniBLAS::AlignBase;
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
using b3d::Camera;
using namespace common;

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
	BufferType bufferType;
	GLuint bufferID = GL_INVALID_INDEX;
	void bind() const;
	void unbind() const;
public:
	_oglBuffer(const BufferType type);
	~_oglBuffer();

	void write(const void *dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
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
private:
	friend class _oglProgram;
	class OGLUAPI VAOPrep : public NonCopyable
	{
	private:
		friend class _oglVAO;
		_oglVAO& vao;
		VAOPrep(_oglVAO& vao_) :vao(vao_)
		{
			glBindVertexArray(vao.vaoID);
		}
	public:
		void end()
		{
			glBindVertexArray(0);
		}
		/*-param  vbo buffer, vertex attr index, stride(number), size(number), offset(byte)
		 *Each group contains (stride) byte, (size) float are taken as an element, 1st element is at (offset) byte*/
		VAOPrep& set(const oglBuffer& vbo, const GLuint attridx, const uint16_t stride, const uint8_t size, const GLint offset);
		/*vbo's inner data is assumed to be Point, automatic set VertPos,VertNorm,TexPos*/
		VAOPrep& set(const oglBuffer& vbo, const GLuint(&attridx)[3], const GLint offset);
		VAOPrep& setIndex(const oglBuffer& vbo, const IndexSize idxSize);
	};
	VAODrawMode vaoMode;
	GLuint vaoID = GL_INVALID_INDEX;
	oglBuffer index;
	IndexSize indexType;
	uint16_t offset, size;
public:
	_oglVAO(const VAODrawMode);
	~_oglVAO();

	VAOPrep prepare();
	void setDrawSize(const uint16_t offset_, const uint16_t size_);
};
using oglVAO = Wrapper<_oglVAO, false>;


enum class TransformType { RotateXYZ, Rotate, Translate, Scale };
struct alignas(Vec4) TransformOP : public AlignBase<>
{
	Vec4 vec;
	TransformType type;
	TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};
class TextureManager;
class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public NonMovable, public AlignBase<>
{
private:
	friend class TextureManager;
	class OGLUAPI ProgDraw : public NonCopyable
	{
	private:
		friend _oglProgram;
		const _oglProgram& prog;
		ProgDraw(const _oglProgram& prog_, const Mat4x4& modelMat);
	public:
		void end()
		{
		}
		/*draw vao
		*-param vao, size, offset*/
		ProgDraw& draw(const oglVAO& vao, const GLsizei size, const GLint offset = 0);
		ProgDraw& draw(const oglVAO& vao);
	};
	
	GLuint programID = GL_INVALID_INDEX;
	vector<oglShader> shaders;
	map<string, GLuint> uniLocs;
	map<string, GLuint> attrLocs;
	struct TexPair
	{
		oglTexture tex;
		GLuint tid;
		TexPair(const oglTexture& tex_ = oglTexture()) :tex(tex_), tid(tex_ ? tex_->textureID : UINT32_MAX) { }
	};
	vector<TexPair> texs;
	GLuint
		Uni_projMat = GL_INVALID_INDEX,
		Uni_viewMat = GL_INVALID_INDEX,
		Uni_modelMat = GL_INVALID_INDEX,
		Uni_mvpMat = GL_INVALID_INDEX,
		Uni_Texture = GL_INVALID_INDEX,
		Uni_camPos = GL_INVALID_INDEX;
	static TextureManager& getTexMan();
	static bool usethis(const _oglProgram& programID, const bool change = true);
	void setMat(const GLuint pos, const Mat4x4& mat) const;
public:
	GLuint
		Attr_Vert_Pos = GL_INVALID_INDEX,
		Attr_Vert_Norm = GL_INVALID_INDEX,
		Attr_Vert_Texc = GL_INVALID_INDEX,
		Attr_Vert_Color = GL_INVALID_INDEX;
	Mat4x4 matrix_Proj, matrix_View;
	_oglProgram();
	~_oglProgram();

	void addShader(oglShader && shader);
	//
	OPResult<> link(const string(&MatrixName)[4] = { "","","","" }, const string(&BasicUniform)[3] = { "tex","","" },
		const string(&VertAttrName)[4] = { "vertPos","","",""}, const uint8_t texcount = 16);
	GLuint getAttrLoc(const string &);
	GLuint getUniLoc(const string &);
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	void setTexture(const oglTexture& tex, const uint8_t pos);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity());
	using topIT = vector<TransformOP>::const_iterator;
	ProgDraw draw(topIT begin, topIT end);
};
using oglProgram = Wrapper<_oglProgram, true>;


class OGLUAPI oglUtil
{
public:
	static void init();
	static string getVersion();
	static OPResult<GLenum> getError();
	//load Vertex and Fragment Shader(with suffix of (.vert) and (.frag)
	static OPResult<string> loadShader(oglProgram& prog, const string& fname);
};


}

