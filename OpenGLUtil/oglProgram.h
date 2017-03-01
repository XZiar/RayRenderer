#pragma once
#include "oglRely.h"
#include "oglComponent.h"

namespace oglu
{

enum class TransformType { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI alignas(Vec4) TransformOP : public AlignBase<>
{
	Vec4 vec;
	TransformType type;
	TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};
//class TextureManager;
class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public NonMovable, public AlignBase<>
{
private:
	friend class TextureManager;
	friend class UBOManager;
	class OGLUAPI ProgDraw : public NonCopyable
	{
	private:
		friend _oglProgram;
		_oglProgram& prog;
		map<GLuint, oglTexture> texCache;
		map<GLuint, oglBuffer> uboCache;
		ProgDraw(_oglProgram& prog_, const Mat4x4& modelMat);
		void setTexture(const GLint pos, const oglTexture& tex);
		void setTexture();
		void setUBO(const GLint pos, const oglBuffer& ubo);
		void setUBO();
		void drawIndex(const oglVAO& vao, const GLsizei size, const void *offset);
		void drawIndexs(const oglVAO& vao, const GLsizei count, const GLsizei *size, const void * const *offset);
		void drawArray(const oglVAO& vao, const GLsizei size, const GLint offset);
		void drawArrays(const oglVAO& vao, const GLsizei count, const GLsizei *size, const GLint *offset);
	public:
		void end()
		{
		}
		ProgDraw& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0, const bool immediate = false);
		//no check on pos, carefully use
		ProgDraw& setTexture(const oglTexture& tex, const GLuint pos, const bool immediate = false);
		ProgDraw& setUBO(const oglBuffer& ubo, const string& name, const GLuint idx = 0, const bool immediate = false);
		//no check on pos, carefully use
		ProgDraw& setUBO(const oglBuffer& ubo, const GLuint pos, const bool immediate = false);
		/*draw vao
		*-param vao, size, offset*/
		ProgDraw& draw(const oglVAO& vao, const uint32_t size, const uint32_t offset = 0);
		ProgDraw& draw(const oglVAO& vao);
	};

	struct DataInfo
	{
	private:
		GLint getValue(const GLuint pid, const GLenum prop);
	public:
		//enum class 
		const char* getTypeName() const
		{
			static const char name[][8] = { "UniBlk","Uniform","Attrib" };
			switch (type)
			{
			case GL_UNIFORM_BLOCK:
				return name[0];
			case GL_UNIFORM:
				return name[1];
			case GL_PROGRAM_INPUT:
				return name[2];
			default:
				return nullptr;
			}
		}
		GLint location = GL_INVALID_INDEX;
		//length of array
		GLint len = 1;
		GLenum type;
		GLenum valtype;
		uint16_t size = 0;
		uint8_t ifidx;
		DataInfo(const GLenum type_) :type(type_) { }
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
		Uni_mvpMat = GL_INVALID_INDEX,
		Uni_Texture = GL_INVALID_INDEX,
		Uni_camPos = GL_INVALID_INDEX;
	Mat4x4 matrix_Proj, matrix_View;
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
	OPResult<> link(const string(&MatrixName)[4] = { "","","","" }, const string(&VertAttrName)[4] = { "vertPos","","","" });
	GLint getLoc(const string& name) const;
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity());
	using topIT = vector<TransformOP>::const_iterator;
	ProgDraw draw(topIT begin, topIT end);
};
using oglProgram = Wrapper<_oglProgram, true>;

}