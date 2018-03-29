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
    friend class detail::_oglProgram;
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
    const char* GetTypeName() const noexcept;
    const char* GetValTypeName() const noexcept;
    bool isUniformBlock() const noexcept { return type == GL_UNIFORM_BLOCK; }
    bool isAttrib() const noexcept { return type == GL_PROGRAM_INPUT; }
    bool isTexture() const noexcept;
};

class OGLUAPI SubroutineResource
{
    friend class detail::_oglProgram;
public:
    struct OGLUAPI Routine
    {
        friend class SubroutineResource;
        friend class ::oglu::detail::_oglProgram;
        const string Name;
    private:
        const GLuint Id;
        const GLenum Stage;
        Routine(const string& name, const GLuint id, const GLenum stage) : Name(name), Id(id), Stage(stage) {}
    };
private:
    const GLenum Stage;
    const GLint UniLoc;
public:
    SubroutineResource(const GLenum stage, const GLint location, const string& name, vector<Routine>&& routines)
        : Stage(stage), UniLoc(location), Name(name), Routines(std::move(routines)) {}
    const string Name;
    const vector<Routine> Routines;
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
        friend class _oglProgram;
    private:
        void init();
    protected:
        _oglProgram& prog;
        map<GLuint, oglTexture> texCache;
        map<GLuint, oglUBO> uboCache;
        //Subroutine are not kept by OGL, it's erased eachtime switch prog
        map<const SubroutineResource*, const SubroutineResource::Routine*> srSettings;
        map<GLenum, vector<GLuint>> srCache;
        explicit ProgState(_oglProgram& prog_);
        void setTexture(TextureManager& texMan, const GLint pos, const oglTexture& tex, const bool shouldPin = false) const;
        void setTexture(TextureManager& texMan, const bool shouldPin = false) const;
        void setUBO(UBOManager& uboMan, const GLint pos, const oglUBO& ubo, const bool shouldPin = false) const;
        void setUBO(UBOManager& uboMan, const bool shouldPin = false) const;
        void setSubroutine() const;
    public:
        void end();
        ProgState& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0);
        //no check on pos, carefully use
        ProgState& setTexture(const oglTexture& tex, const GLuint pos);
        ProgState& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0);
        //no check on pos, carefully use
        ProgState& setUBO(const oglUBO& ubo, const GLuint pos);
        ProgState& setSubroutine(const SubroutineResource::Routine& sr);
        ProgState& setSubroutine(const string& sruname, const string& srname);
        ProgState& getSubroutine(const string& sruname, string& srname);
    };

    class OGLUAPI ProgDraw : protected ProgState
    {
        friend _oglProgram;
    private:
        TextureManager & TexMan;
        UBOManager& UboMan;
        map<GLuint, std::pair<GLint, bool>> UniformBackup;
        ProgDraw(const ProgState& pstate, const Mat4x4& modelMat, const Mat3x3& normMat);
    public:
        void end();
        /*draw vao
        *-param vao, size, offset*/
        ProgDraw& draw(const oglVAO& vao, const uint32_t size, const uint32_t offset = 0);
        ProgDraw& draw(const oglVAO& vao);
        ProgDraw& setTexture(const oglTexture& tex, const string& name, const GLuint idx = 0);
        ProgDraw& setTexture(const oglTexture& tex, const GLuint pos);
        ProgDraw& setUBO(const oglUBO& ubo, const string& name, const GLuint idx = 0);
        ProgDraw& setUBO(const oglUBO& ubo, const GLuint pos);
        ProgDraw& setSubroutine(const SubroutineResource::Routine& sr)
        {
            return *(ProgDraw*)&ProgState::setSubroutine(sr);
        }
        ProgDraw& setSubroutine(const string& sruname, const string& srname)
        {
            return *(ProgDraw*)&ProgState::setSubroutine(sruname, srname);
        }
    };


    GLuint programID = 0; //zero means invalid program
    vector<oglShader> shaders;
    map<string, ProgramResource> resMap;
    map<string, ProgramResource> texMap;
    map<string, ProgramResource> uboMap;
    map<string, ProgramResource> attrMap;
    map<string, SubroutineResource> subrMap;
    map<const SubroutineResource::Routine*, const SubroutineResource*> subrLookup;
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
    void RecoverState();
    void setMat(const GLint pos, const Mat4x4& mat) const;
    void initLocs();
    void initSubroutines();
public:
    GLint Attr_Vert_Pos = GL_INVALID_INDEX;//Vertex Position
    GLint Attr_Vert_Norm = GL_INVALID_INDEX;//Vertex Normal
    GLint Attr_Vert_Texc = GL_INVALID_INDEX;//Vertex Texture Coordinate
    GLint Attr_Vert_Color = GL_INVALID_INDEX;//Vertex Color
    u16string Name;
    _oglProgram(const u16string& name);
    ~_oglProgram();

    void addShader(oglShader && shader);
    void link();
    void registerLocation(const string(&VertAttrName)[4], const string(&MatrixName)[5]);
    GLint getLoc(const string& name) const;
    const map<const string, const ProgramResource>& getResources() const { return *(const map<const string, const ProgramResource>*)&resMap; }
    const ProgramResource* getResource(const string& name) const;
    const map<const string, const SubroutineResource>& getSubroutineResources() const { return *(const map<const string, const SubroutineResource>*)&subrMap; }
    const SubroutineResource* getSubroutines(const string& name) const;
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