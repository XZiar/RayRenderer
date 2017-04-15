#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"

namespace oglu
{

enum class TransformType { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI alignas(Vec4) TransformOP : public AlignBase<>
{
	Vec4 vec;
	TransformType type;
	TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};

namespace inner
{


class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public NonMovable, public AlignBase<>
{
private:
	friend class TextureManager;
	friend class UBOManager;
	class OGLUAPI ProgState : public NonCopyable
	{
	protected:
		friend _oglProgram;
		_oglProgram& prog;
		map<GLuint, oglTexture> texCache;
		map<GLuint, oglUBO> uboCache;
		explicit ProgState(_oglProgram& prog_);
		void setTexture(const GLint pos, const oglTexture& tex) const;
		void setTexture() const;
		void setUBO(const GLint pos, const oglUBO& ubo) const;
		void setUBO() const;
	public:
		void end();
		ProgState& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0);
		//no check on pos, carefully use
		ProgState& setTexture(const oglTexture& tex, const GLuint pos);
		ProgState& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0);
		//no check on pos, carefully use
		ProgState& setUBO(const oglUBO& ubo, const GLuint pos);
	};

	class OGLUAPI ProgDraw : public ProgState
	{
	private:
		friend _oglProgram;
		ProgDraw(const ProgState& pstate, const Mat4x4& modelMat, const Mat3x3& normMat);
	public:
		void end();
		/*draw vao
		*-param vao, size, offset*/
		ProgDraw& draw(const oglVAO& vao, const uint32_t size, const uint32_t offset = 0);
		ProgDraw& draw(const oglVAO& vao);
		ProgDraw& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0)
		{
			return *(ProgDraw*)&ProgState::setTexture(tex, name, idx);
		}
		ProgState& setTexture(const oglTexture& tex, const GLuint pos)
		{
			return *(ProgDraw*)&ProgState::setTexture(tex, pos);
		}
		ProgState& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0)
		{
			return *(ProgDraw*)&ProgState::setUBO(ubo, name, idx);
		}
		ProgState& setUBO(const oglUBO& ubo, const GLuint pos)
		{
			return *(ProgDraw*)&ProgState::setUBO(ubo, pos);
		}
	};

	struct DataInfo
	{
	private:
		GLint getValue(const GLuint pid, const GLenum prop);
	public:
		GLint location = GL_INVALID_INDEX;
		//length of array
		GLuint len = 1;
		GLenum type;
		GLenum valtype;
		uint16_t size = 0;
		uint8_t ifidx;
		DataInfo(const GLenum type_) :type(type_) { }
		const char* getTypeName() const;
		void initData(const GLuint pid, const GLint idx);
		bool isUniformBlock() const { return type == GL_UNIFORM_BLOCK; }
		bool isAttrib() const { return type == GL_PROGRAM_INPUT; }
		bool isTexture() const;
	};

	GLuint programID = GL_INVALID_INDEX;
	vector<oglShader> shaders;
	map<string, DataInfo> texMap;
	map<string, DataInfo> uboMap;
	map<string, DataInfo> attrMap;
	map<string, GLint> locMap;
	vector<GLint> uniCache;
	GLint
		Uni_projMat = GL_INVALID_INDEX,
		Uni_viewMat = GL_INVALID_INDEX,
		Uni_modelMat = GL_INVALID_INDEX,
		Uni_normalMat = GL_INVALID_INDEX,
		Uni_mvpMat = GL_INVALID_INDEX,
		Uni_Texture = GL_INVALID_INDEX,
		Uni_camPos = GL_INVALID_INDEX;
	Mat4x4 matrix_Proj, matrix_View;
	ProgState gState;
	static bool usethis(_oglProgram& programID, const bool change = true);
	void setMat(const GLint pos, const Mat4x4& mat) const;
	void initLocs();
public:
	GLint Attr_Vert_Pos = GL_INVALID_INDEX;//Vertex Position
	GLint Attr_Vert_Norm = GL_INVALID_INDEX;//Vertex Normal
	GLint Attr_Vert_Texc = GL_INVALID_INDEX;//Vertex Texture Coordinate
	GLint Attr_Vert_Color = GL_INVALID_INDEX;//Vertex Color
	_oglProgram();
	~_oglProgram();

	void addShader(oglShader && shader);
	//
	OPResult<> link();
	void registerLocation(const string(&VertAttrName)[4], const string(&MatrixName)[5]);
	GLint getLoc(const string& name) const;
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	ProgDraw draw(const Mat4x4& modelMat, const Mat3x3& normMat);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity());
	using topIT = vector<TransformOP>::const_iterator;
	ProgDraw draw(topIT begin, topIT end);
	ProgState& globalState();
};


}


using oglProgram = Wrapper<inner::_oglProgram>;

}