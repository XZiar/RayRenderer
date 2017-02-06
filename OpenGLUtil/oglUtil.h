#pragma once

#include "oglRely.h"
#include <GL/glew.h>

#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <iterator>
#include <algorithm>
#include <vector>
#include <map>
#include <tuple>




namespace oglu
{
using std::string;
using std::vector;
using std::vector_;
using std::tuple;
using std::map;
using std::function;
using miniBLAS::AlignBase;
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
using b3d::Camera;
using b3d::Material;
using b3d::Light;



enum class ShaderType : GLenum
{
	Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
	TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
	Compute = GL_COMPUTE_SHADER
};

class OGLUAPI _oglShader : public NonCopyable
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
	_oglShader(_oglShader &&) = default;
	_oglShader & operator= (_oglShader &&) = default;
	~_oglShader();

	OPResult<> compile();
};
using oglShader = Wrapper<_oglShader>;


enum class BufferType : GLenum
{
	Array = GL_ARRAY_BUFFER, Element = GL_ELEMENT_ARRAY_BUFFER, Uniform = GL_UNIFORM_BUFFER, Pixel = GL_PIXEL_UNPACK_BUFFER
};
enum class DrawMode : GLenum
{
	StreamDraw = GL_STREAM_DRAW, StreamRead = GL_STREAM_READ, StreamCopy = GL_STREAM_COPY,
	StaticDraw = GL_STATIC_DRAW, StaticRead = GL_STATIC_READ, StaticCopy = GL_STATIC_COPY,
	DynamicDraw = GL_DYNAMIC_DRAW, DynamicRead = GL_DYNAMIC_READ, DynamicCopy = GL_DYNAMIC_COPY,
};
class OGLUAPI _oglBuffer
{
private:
	friend class _oglVAO;
	friend class oclu::_oclMem;
	friend class _oglTexture;
	BufferType bufferType;
	GLuint bID = GL_INVALID_INDEX;
public:
	_oglBuffer(const BufferType type);
	~_oglBuffer();

	void write(const void *, const size_t, const DrawMode = DrawMode::StaticDraw);
};
using oglBuffer = Wrapper<_oglBuffer>;


enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, };
enum class TextureFormat : GLenum
{
	RGB = GL_RGB, RGBA = GL_RGBA, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F
};
enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };
enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, Clamp = GL_CLAMP, };
class OGLUAPI _oglTexture
{
private:
	friend class _oglProgram;
	friend class TextureManager;
	friend class oclu::_oclMem;
	TextureType type;
	GLuint tID = GL_INVALID_INDEX;
	static void parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype);
	void bind(const uint16_t pos = 0) const;
	void unbind() const;
public:
	_oglTexture(const TextureType _type);
	~_oglTexture();
	void setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void *);
	void setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf);
};
using oglTexture = Wrapper<_oglTexture>;


enum class VAODrawMode : GLenum
{
	Triangles = GL_TRIANGLES
};
class OGLUAPI _oglVAO
{
private:
	friend class _oglProgram;
	class OGLUAPI VAOPrep : public NonCopyable
	{
	private:
		friend class _oglVAO;
		const _oglVAO& vao;
		VAOPrep(const _oglVAO& vao_) :vao(vao_)
		{
			glBindVertexArray(vao.vaoID);
		}
	public:
		void end()
		{
			glBindVertexArray(0);
		}
		/*-param  vbo buffer, vertex attr index, size(number), stride(number), offset(byte)
		 *Each group contains (stride) byte, (size) float are taken as an element, 1st element is at (offset) byte*/
		VAOPrep& set(const oglBuffer& vbo, const GLuint attridx, const uint16_t stride, const uint8_t size, const GLint offset);
	};
	VAODrawMode vaoMode;
	GLuint vaoID = GL_INVALID_INDEX;
public:
	_oglVAO(const VAODrawMode);
	~_oglVAO();

	VAOPrep prepare()
	{
		return VAOPrep(*this);
	}
};
using oglVAO = Wrapper<_oglVAO>;


enum class TransformType { Rotate, Translate, Scale };
using TransformOP = std::pair<TransformType, Vec3>;
class TextureManager;
class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public AlignBase<>
{
private:
	friend class TextureManager;
	class OGLUAPI ProgDraw : public NonCopyable
	{
	private:
		friend _oglProgram;
		const _oglProgram& prog;
		ProgDraw(const _oglProgram& prog_, const Mat4x4& modelMat, const Mat4x4& normMat);
	public:
		void end()
		{
		}
		/*draw vao
		*-param vao, size, offset*/
		ProgDraw& draw(const oglVAO& vao, const GLsizei size, const GLint offset = 0);
	};
	
	GLuint programID = GL_INVALID_INDEX;
	vector<oglShader> shaders;
	map<string, GLint> uniLocs;
	struct TexPair
	{
		oglTexture tex;
		GLuint tid;
		TexPair(const oglTexture& tex_ = oglTexture()) :tex(tex_), tid(tex_ ? tex_->tID : UINT32_MAX) { }
	};
	vector<TexPair> texs;
	GLuint ID_lgtVBO, ID_mtVBO;
	GLuint
		Uni_projMat = GL_INVALID_INDEX,
		Uni_viewMat = GL_INVALID_INDEX,
		Uni_modelMat = GL_INVALID_INDEX,
		Uni_normMat = GL_INVALID_INDEX,
		Uni_camPos = GL_INVALID_INDEX;
	GLuint
		Uni_Texture = GL_INVALID_INDEX,
		Uni_Light = GL_INVALID_INDEX,
		Uni_Material = GL_INVALID_INDEX;
	static TextureManager& getTexMan();
	static bool usethis(const _oglProgram& programID, const bool change = true);
	void setMat(const GLuint pos, const Mat4x4& mat) const;
public:
	GLuint
		Attr_Vert_Pos = 0,
		Attr_Vert_Norm = 1,
		Attr_Vert_Color = GL_INVALID_INDEX,
		Attr_Vert_Texc = 2;
	Mat4x4 matrix_Proj, matrix_View;
	_oglProgram();
	~_oglProgram();

	void addShader(oglShader && shader);
	OPResult<> link(const string(&MatrixName)[4] = { "","","","" }, const string(&BasicUniform)[3] = { "tex","","" }, const uint8_t texcount = 16);
	GLint getUniLoc(const string &);
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	void setLight(const uint8_t id, const Light &);
	void setMaterial(const Material &);
	void setTexture(const oglTexture& tex, const uint8_t pos);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity(), const Mat4x4& normMat = Mat4x4::identity());
	template<class IT, class = typename std::enable_if<std::is_same<std::iterator_traits<TI>::value_type, TransformOP>::type>>
	ProgDraw draw(IT begin, IT end)
	{
		Mat4x4 matModel = Mat4x4::identity(), matNorm = Mat4x4::identity();
		for (TI cur = begin; cur != end; ++cur)
		{
			const TransformOP& trans = *cur;
			switch (trans.first)
			{
			case TransformType::Rotate:
				const auto rMat = Mat4x4(Mat3x3::RotateMat(trans.second));
				matModel *= rMat;
				matNorm *= rMat;
				break;
			case TransformType::Translate:
				matModel *= Mat4x4::TranslateMat(trans.second);
				break;
			case TransformType::Scale:
				matModel *= Mat3x3::ScaleMat(trans.second);
			}
		}
		return ProgDraw(*this);
	}
};
using oglProgram = Wrapper<_oglProgram>;


class OGLUAPI oglUtil
{
public:
	static void init();
	static OPResult<GLenum> getError();
	//load Vertex and Fragment Shader(with suffix of (.vert) and (.frag)
	static OPResult<string> loadShader(oglProgram& prog, const string& fname);
};


}

