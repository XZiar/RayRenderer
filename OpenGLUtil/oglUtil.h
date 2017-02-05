#pragma once

#include "../3dBasic/3dElement.hpp"

#include <GL/glew.h>

#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <iterator>
#include <string>
#include <vector>
#include <map>
#include <tuple>


#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define OGLUTPL _declspec(dllexport) 
#else
#   define OGLUAPI _declspec(dllimport)
#   define OGLUTPL
#endif

namespace oclu
{
class _oclMem;
}

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


struct OGLUAPI NonCopyable
{
	NonCopyable() { }
	NonCopyable(const NonCopyable & other) = delete;
	NonCopyable(NonCopyable && other) = default;
	NonCopyable& operator= (const NonCopyable & other) = delete;
	NonCopyable& operator= (NonCopyable && other) = default;
};


template<class T, bool isNonCopy = std::is_base_of<NonCopyable, T>::value>
class OGLUTPL Wrapper;

template<class T>
class OGLUTPL Wrapper<T, true>
{
private:
	T *instance = nullptr;
public:
	Wrapper() { }
	template<class... ARGS>
	Wrapper(ARGS... args) : instance(new T(args...)) { }
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T>& other) = delete;
	Wrapper(Wrapper<T>&& other)
	{
		*this = std::move(other);
	}
	Wrapper& operator=(const Wrapper& other) = delete;
	Wrapper& operator=(Wrapper&& other)
	{
		release();
		instance = other.instance;
		other.instance = nullptr;
		return *this;
	}
	
	void release()
	{
		if (instance != nullptr)
		{
			delete instance;
			instance = nullptr;
		}
	}

	template<class... ARGS>
	void reset(ARGS... args)
	{
		release();
		instance = new T(args...);
	}
	void reset()
	{
		release();
		instance = new T();
	}

	T* operator-> ()
	{
		return instance;
	}
	const T* operator-> () const
	{
		return instance;
	}
	operator bool()
	{
		return instance != nullptr;
	}
	operator const bool() const
	{
		return instance != nullptr;
	}
};

template<class T>
class OGLUTPL Wrapper<T, false>
{
private:
	struct ControlBlock
	{
		T *instance = nullptr;;
		uint32_t count = 0;
		ControlBlock(T *dat) :instance(dat) { }
		~ControlBlock() { delete instance; }
	};
	ControlBlock *cb = nullptr;;
	void create(T* instance)
	{
		cb = new ControlBlock(instance);
	}
public:
	Wrapper() : cb(nullptr) { }
	template<class... ARGS>
	Wrapper(ARGS... args)
	{
		create(new T(args...));
	}
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T>& other)
	{
		*this = std::forward(other);
	}
	Wrapper(Wrapper<T>&& other)
	{
		*this = std::move(other);
	}
	Wrapper& operator=(const Wrapper<T>& other)
	{
		release();
		cb = other.cb;
		if (cb != nullptr)
			cb->count++;
		return *this;
	}
	Wrapper& operator=(Wrapper<T>&& other)
	{
		release();
		cb = other.cb;
		other.cb = nullptr;
		return *this;
	}

	void release()
	{
		if (cb != nullptr)
		{
			if (!(--cb->count))
			{
				delete cb;
				cb = nullptr;
			}
		}
	}

	template<class... ARGS>
	void reset(ARGS... args)
	{
		release();
		create(new T(args...));
	}
	void reset()
	{
		release();
		create(new T());
	}

	T* operator-> ()
	{
		return cb->instance;
	}
	const T* operator-> () const
	{
		return cb->instance;
	}
	operator bool()
	{
		return cb != nullptr;
	}
	operator const bool() const
	{
		return cb != nullptr;
	}
};

class _Init
{
private:
	friend class _oglProgram;
	_Init()
	{
		glewInit();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
};

template<class T = char>
class OGLUTPL OPResult
{
private:
	bool isSuc;
public:
	string msg;
	T data;
	OPResult(const bool isSuc_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_) { }
	OPResult(const bool isSuc_, const T& dat_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_), data(dat_) { }
	operator bool() { return isSuc; }
};

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
	GLuint shaderID = 0;
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
enum class TexturePropType { Wrap, Filter };
enum class TexturePropVal : GLint
{
	Linear = GL_LINEAR, Nearest = GL_NEAREST,
	Repeat = GL_REPEAT, Clamp = GL_CLAMP,
};
class OGLUAPI _oglTexture
{
private:
	friend class _oglProgram;
	friend class oclu::_oclMem;
	TextureType type;
	static void parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype);
public:
	GLuint tID;
	_oglTexture(const TextureType _type);
	~_oglTexture();
	template<uint8_t N>
	void setProperty(const tuple<TexturePropType, TexturePropVal>(&params)[N])
	{
		glBindTexture((GLenum)type, tID);
		for (uint8_t a = 0; a < N; ++a)
		{
			TexturePropType ptype; TexturePropVal pval;
			std::tie(ptype, pval) = params[a];
			GLenum pname1, pname2;
			if (ptype == TexturePropType::Wrap)
				pname1 = GL_TEXTURE_WRAP_S, pname2 = GL_TEXTURE_WRAP_T;
			else if (ptype == TexturePropType::Filter)
				pname1 = GL_TEXTURE_MAG_FILTER, pname2 = GL_TEXTURE_MIN_FILTER;
			else
				return;
			glTexParameteri((GLenum)type, pname1, (GLint)pval);
			glTexParameteri((GLenum)type, pname2, (GLint)pval);
		}
		glBindTexture((GLenum)type, 0);
	}
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
	GLuint vaoID;
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
class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public AlignBase<>
{
private:
	class OGLUAPI ProgDraw : public NonCopyable
	{
	private:
		friend _oglProgram;
		const _oglProgram& prog;
		ProgDraw(const _oglProgram& prog_, const Mat4x4& modelMat, const Mat4x4& normMat) :prog(prog_)
		{
			_oglProgram::usethis(prog);
			prog.setMat(prog.Uni_modelMat, modelMat);
			prog.setMat(prog.Uni_normMat, normMat);
		}
	public:
		void end()
		{
		}
		/*draw vao
		*-param vao, size, offset*/
		ProgDraw& draw(const oglVAO& vao, const GLsizei size, const GLint offset = 0);
	};
	GLuint programID = 0;
	vector<oglShader> shaders;
	map<string, GLint> uniLocs;
	vector<oglTexture> texs;
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
	static void usethis(const _oglProgram& programID);
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
	static OPResult<GLenum> getError();
	//load Vertex and Fragment Shader(with suffix of (.vert) and (.frag)
	static OPResult<string> loadShader(oglProgram& prog, const string& fname);
};


}

