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
		ProgDraw(_oglProgram& prog_, const Mat4x4& modelMat);
		void drawIndex(const oglVAO& vao, const GLsizei size, const void *offset);
		void drawIndexs(const oglVAO& vao, const GLsizei count, const GLsizei *size, const void * const *offset);
		void drawArray(const oglVAO& vao, const GLsizei size, const GLint offset);
		void drawArrays(const oglVAO& vao, const GLsizei count, const GLsizei *size, const GLint *offset);
	public:
		void end()
		{
		}
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
		enum class IFType : GLenum { UniformBlock = GL_UNIFORM_BLOCK, Uniform = GL_UNIFORM, Attrib = GL_PROGRAM_INPUT };
		const char* getTypeName() const
		{
			static const char name[][8] = { "UniBlk","Uniform","Attrib" };
			switch (iftype)
			{
			case IFType::UniformBlock:
				return name[0];
			case IFType::Uniform:
				return name[1];
			case IFType::Attrib:
				return name[2];
			default:
				return nullptr;
			}
		}
		GLint location = GL_INVALID_INDEX;
		GLint len = 0;
		IFType iftype;
		GLenum type;
		uint16_t size = 0;
		uint8_t ifidx;
		DataInfo(const IFType iftype_) :iftype(iftype_) { }
		bool isArray() const { return len > 0; }
		void initData(const GLuint pid, const GLint idx);
	};

	GLuint programID = GL_INVALID_INDEX;
	vector<oglShader> shaders;
	map<string, DataInfo> dataMap;
	vector<oglTexture> texs;
	vector<GLint> texvals;
	vector<oglBuffer> ubos;
	vector<GLint> ubovals;
	GLint
		Uni_projMat = GL_INVALID_INDEX,
		Uni_viewMat = GL_INVALID_INDEX,
		Uni_modelMat = GL_INVALID_INDEX,
		Uni_mvpMat = GL_INVALID_INDEX,
		Uni_Texture = GL_INVALID_INDEX,
		Uni_camPos = GL_INVALID_INDEX;
	static bool usethis(_oglProgram& programID, const bool change = true);
	void setMat(const GLint pos, const Mat4x4& mat) const;
	void initLocs();
	void recoverTexMap();
public:
	GLint
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
		const string(&VertAttrName)[4] = { "vertPos","","","" });
	GLint getLoc(const string& name) const;
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	void setTexture(const oglTexture& tex, const uint8_t pos);
	void setUBO(const oglBuffer& ubo, const uint8_t pos);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity());
	using topIT = vector<TransformOP>::const_iterator;
	ProgDraw draw(topIT begin, topIT end);
};
using oglProgram = Wrapper<_oglProgram, true>;

}