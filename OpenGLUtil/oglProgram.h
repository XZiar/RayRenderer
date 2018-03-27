#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"

namespace oglu
{

enum class TransformType : uint8_t { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI alignas(alignof(Vec4)) TransformOP : public common::AlignBase<alignof(Vec4)>
{
	Vec4 vec;
	TransformType type;
	TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};

namespace detail
{
class _oglProgram;
}

struct OGLUAPI ProgramResource
{
	friend detail::_oglProgram;
private:
	GLint getValue(const GLuint pid, const GLenum prop);
	void initData(const GLuint pid, const GLint idx);
public:
	GLint location = GL_INVALID_INDEX;
	//length of array
	GLuint len = 1;
	GLenum type;
	GLenum valtype;
	uint16_t size = 0;
	uint8_t ifidx;
	ProgramResource(const GLenum type_) :type(type_) { }
	const char* getTypeName() const noexcept;
	bool isUniformBlock() const noexcept { return type == GL_UNIFORM_BLOCK; }
	bool isAttrib() const noexcept { return type == GL_PROGRAM_INPUT; }
	bool isTexture() const noexcept;
};

struct OGLUAPI SubroutineResource
{
	friend detail::_oglProgram;
private:
	const GLenum Stage;
    const GLuint Id;
	SubroutineResource(const GLenum stage, const string& name, const GLuint id) : Stage(stage), Name(name), Id(id) {}
public:
    const string Name;
};

namespace detail
{


class OGLUAPI alignas(32) _oglProgram : public NonCopyable, public NonMovable, public common::AlignBase<32>
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
        //Subroutine are not kept by OGL, it's erased eachtime switch prog
        map<GLenum, vector<GLuint>> srCache;
		explicit ProgState(_oglProgram& prog_);
		void setTexture(TextureManager& texMan, const GLint pos, const oglTexture& tex) const;
		void setTexture(TextureManager& texMan) const;
		void setUBO(UBOManager& uboMan, const GLint pos, const oglUBO& ubo) const;
        void setUBO(UBOManager& uboMan) const;
        void setSubroutine() const;
	public:
		void end();
		ProgState& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0);
		//no check on pos, carefully use
		ProgState& setTexture(const oglTexture& tex, const GLuint pos);
		ProgState& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0);
		//no check on pos, carefully use
		ProgState& setUBO(const oglUBO& ubo, const GLuint pos);
        ProgState& setSubroutine(const SubroutineResource& sr);
        ProgState& setSubroutine(const string& sruname, const string& srname);
    };

	class OGLUAPI ProgDraw : public ProgState
	{
        friend _oglProgram;
    private:
        TextureManager & TexMan;
        UBOManager& UboMan;
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
        ProgDraw& setTexture(const oglTexture& tex, const GLuint pos)
		{
			return *(ProgDraw*)&ProgState::setTexture(tex, pos);
		}
        ProgDraw& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0)
		{
			return *(ProgDraw*)&ProgState::setUBO(ubo, name, idx);
		}
        ProgDraw& setUBO(const oglUBO& ubo, const GLuint pos)
		{
			return *(ProgDraw*)&ProgState::setUBO(ubo, pos);
		}
        ProgDraw& setSubroutine(const SubroutineResource& sr)
        {
            return *(ProgDraw*)&ProgState::setSubroutine(sr);
        }
        ProgDraw& setSubroutine(const string& sruname, const string& srname)
        {
            return *(ProgDraw*)&ProgState::setSubroutine(sruname, srname);
        }
	};

	

	GLuint programID = GL_INVALID_INDEX;
	vector<oglShader> shaders;
	map<string, ProgramResource> resMap;
	map<string, ProgramResource> texMap;
	map<string, ProgramResource> uboMap;
	map<string, ProgramResource> attrMap;
    map<string, vector<SubroutineResource>> subrMap;
    map<pair<GLenum, GLuint>, GLint> subrLookup;
	//map<string, GLint> locMap;
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
	void initSubroutines();
public:
	GLint Attr_Vert_Pos = GL_INVALID_INDEX;//Vertex Position
	GLint Attr_Vert_Norm = GL_INVALID_INDEX;//Vertex Normal
	GLint Attr_Vert_Texc = GL_INVALID_INDEX;//Vertex Texture Coordinate
	GLint Attr_Vert_Color = GL_INVALID_INDEX;//Vertex Color
	_oglProgram();
	~_oglProgram();

	void addShader(oglShader && shader);
	void link();
	void registerLocation(const string(&VertAttrName)[4], const string(&MatrixName)[5]);
	GLint getLoc(const string& name) const;
	optional<const ProgramResource*> getResource(const string& name) const;
	optional<const vector<SubroutineResource>*> getSubroutines(const string& name) const;
	void setProject(const Camera &, const int wdWidth, const int wdHeight);
	void setCamera(const Camera &);
	ProgDraw draw(const Mat4x4& modelMat, const Mat3x3& normMat);
	ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity());
	using topIT = vectorEx<TransformOP>::const_iterator;
	ProgDraw draw(topIT begin, topIT end);
	ProgState& globalState();
};


}


using oglProgram = Wrapper<detail::_oglProgram>;

}