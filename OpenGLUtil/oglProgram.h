#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"
#include <variant>

namespace oglu
{

using UniformValue = std::variant<miniBLAS::Vec3, miniBLAS::Vec4, miniBLAS::Mat3x3, miniBLAS::Mat4x4, b3d::Coord2D, bool, int32_t, uint32_t, float>;

namespace detail
{
class _oglProgram;
class ProgState;
class ProgDraw;
}

struct OGLUAPI ProgramResource : public common::container::NamedSetValue<ProgramResource, string>
{
    friend class detail::_oglProgram;
    friend class detail::ProgState;
    friend class detail::ProgDraw;
private:
    GLint getValue(const GLuint pid, const GLenum prop);
    void initData(const GLuint pid, const GLint idx);
public:
    GLint location = GL_INVALID_INDEX;
    //length of array
    mutable GLuint len = 1;
    GLenum type;
    GLenum valtype;
    uint16_t size = 0;
    uint8_t ifidx;
    const string Name;
    ProgramResource(const GLenum type_, const string& name) :type(type_), Name(name) { }
    const char* GetTypeName() const noexcept;
    const char* GetValTypeName() const noexcept;
    bool isUniformBlock() const noexcept { return type == GL_UNIFORM_BLOCK; }
    bool isAttrib() const noexcept { return type == GL_PROGRAM_INPUT; }
    bool isTexture() const noexcept;
};

class OGLUAPI SubroutineResource : public common::container::NamedSetValue<SubroutineResource, string>
{
    friend class detail::_oglProgram;
    friend class detail::ProgState;
    friend class detail::ProgDraw;
public:
    struct OGLUAPI Routine
    {
        friend class SubroutineResource;
        friend class ::oglu::detail::_oglProgram;
        friend class ::oglu::detail::ProgState;
        friend class ::oglu::detail::ProgDraw;
        const string Name;
    private:
        const GLuint Id;
        Routine(const string& name, const GLuint id) : Name(name), Id(id) {}
    };
private:
    const GLint UniLoc;
public:
    SubroutineResource(const GLenum stage, const GLint location, const string& name, vector<Routine>&& routines)
        : Stage(ShaderType(stage)), UniLoc(location), Name(name), Routines(std::move(routines)) {}
    const ShaderType Stage;
    const string Name;
    const vector<Routine> Routines;
};


namespace detail
{


class OGLUAPI ProgState : public NonCopyable
{
    friend class _oglProgram;
    friend class ProgDraw;
private:
    void init();
protected:
    _oglProgram & prog;
    map<GLuint, oglTexture> texCache;
    map<GLuint, oglUBO> uboCache;
    //Subroutine are not kept by OGL, it's erased eachtime switch prog
    map<const SubroutineResource*, const SubroutineResource::Routine*> srSettings;
    map<ShaderType, vector<GLuint>> srCache;
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

class OGLUAPI alignas(32) _oglProgram final : public NonCopyable, public NonMovable, public common::AlignBase<32>, public std::enable_shared_from_this<_oglProgram>
{
    friend class TextureManager;
    friend class UBOManager;
    friend class ProgState;
    friend class ProgDraw;
private:
    GLuint programID = 0; //zero means invalid program
    set<oglShader> shaders;
    set<ShaderExtProperty, std::less<>> ShaderProperties;
    set<ProgramResource, std::less<>> ProgRess;
    set<ProgramResource, std::less<>> TexRess;
    set<ProgramResource, std::less<>> UBORess;
    set<ProgramResource, std::less<>> AttrRess;
    set<SubroutineResource, std::less<>> SubroutineRess;
    map<const SubroutineResource::Routine*, const SubroutineResource*> subrLookup;
    vector<GLint> uniCache;
    map<GLint, UniformValue> UniValCache;
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
    void InitLocs();
    void InitSubroutines();
    void FilterProperties();
    GLint GetLoc(const ProgramResource* res, GLenum valtype) const;
    GLint GetLoc(const string& name, GLenum valtype) const;

    void SetUniform(const GLint pos, const b3d::Coord2D& vec, const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Vec3& vec, const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Vec4& vec, const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Mat4x4& mat, const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Mat3x3& mat, const bool keep = true);
    void SetUniform(const GLint pos, const bool val, const bool keep = true);
    void SetUniform(const GLint pos, const int32_t val, const bool keep = true);
    void SetUniform(const GLint pos, const uint32_t val, const bool keep = true);
    void SetUniform(const GLint pos, const float val, const bool keep = true);
public:
    GLint Attr_Vert_Pos = GL_INVALID_INDEX;//Vertex Position
    GLint Attr_Vert_Norm = GL_INVALID_INDEX;//Vertex Normal
    GLint Attr_Vert_Texc = GL_INVALID_INDEX;//Vertex Texture Coordinate
    GLint Attr_Vert_Color = GL_INVALID_INDEX;//Vertex Color
    u16string Name;
    _oglProgram(const u16string& name);
    ~_oglProgram();

    const set<ProgramResource, std::less<>>& getResources() const { return ProgRess; }
    const set<ShaderExtProperty, std::less<>>& getResourceProperties() const { return ShaderProperties; }
    const set<SubroutineResource, std::less<>>& getSubroutineResources() const { return SubroutineRess; }
    const set<oglShader>& getShaders() const { return shaders; }
    const map<GLint, UniformValue>& getCurUniforms() const { return UniValCache; }

    void addShader(const oglShader& shader);
    void AddExtShaders(const string& src);
    void link();
    void registerLocation(const string(&VertAttrName)[4], const string(&MatrixName)[5]);
    GLint getLoc(const string& name) const;
    const ProgramResource* getResource(const string& name) const;
    const SubroutineResource* getSubroutines(const string& name) const;
    void setProject(const Camera &, const int wdWidth, const int wdHeight);
    void setCamera(const Camera &);

    ProgDraw draw(const Mat4x4& modelMat, const Mat3x3& normMat) noexcept;
    ProgDraw draw(const Mat4x4& modelMat = Mat4x4::identity()) noexcept;
    using topIT = vectorEx<TransformOP>::const_iterator;
    ProgDraw draw(topIT begin, topIT end) noexcept;
    ProgState& globalState() noexcept;

    void SetVec(const ProgramResource* res, const float x, const float y) { SetVec(res, b3d::Coord2D(x, y)); }
    void SetVec(const ProgramResource* res, const float x, const float y, const float z) { SetVec(res, miniBLAS::Vec3(x, y, z)); }
    void SetVec(const ProgramResource* res, const float x, const float y, const float z, const float w) { SetVec(res, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec(const ProgramResource* res, const b3d::Coord2D& vec) { SetUniform(GetLoc(res, GL_FLOAT_VEC2), vec); }
    void SetVec(const ProgramResource* res, const miniBLAS::Vec3& vec) { SetUniform(GetLoc(res, GL_FLOAT_VEC3), vec); }
    void SetVec(const ProgramResource* res, const miniBLAS::Vec4& vec) { SetUniform(GetLoc(res, GL_FLOAT_VEC4), vec); }
    void SetMat(const ProgramResource* res, const miniBLAS::Mat3x3& mat) { SetUniform(GetLoc(res, GL_FLOAT_MAT3), mat); }
    void SetMat(const ProgramResource* res, const miniBLAS::Mat4x4& mat) { SetUniform(GetLoc(res, GL_FLOAT_MAT4), mat); }
    void SetUniform(const ProgramResource* res, const bool val) { SetUniform(GetLoc(res, GL_BOOL), val); }
    void SetUniform(const ProgramResource* res, const int32_t val) { SetUniform(GetLoc(res, GL_INT), val); }
    void SetUniform(const ProgramResource* res, const uint32_t val) { SetUniform(GetLoc(res, GL_UNSIGNED_INT), val); }
    void SetUniform(const ProgramResource* res, const float val) { SetUniform(GetLoc(res, GL_FLOAT), val); }
    void SetVec(const string& name, const float x, const float y) { SetVec(name, b3d::Coord2D(x, y)); }
    void SetVec(const string& name, const float x, const float y, const float z) { SetVec(name, miniBLAS::Vec3(x, y, z)); }
    void SetVec(const string& name, const float x, const float y, const float z, const float w) { SetVec(name, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec(const string& name, const b3d::Coord2D& vec) { SetUniform(GetLoc(name, GL_FLOAT_VEC2), vec); }
    void SetVec(const string& name, const miniBLAS::Vec3& vec) { SetUniform(GetLoc(name, GL_FLOAT_VEC3), vec); }
    void SetVec(const string& name, const miniBLAS::Vec4& vec) { SetUniform(GetLoc(name, GL_FLOAT_VEC4), vec); }
    void SetMat(const string& name, const miniBLAS::Mat3x3& mat) { SetUniform(GetLoc(name, GL_FLOAT_MAT3), mat); }
    void SetMat(const string& name, const miniBLAS::Mat4x4& mat) { SetUniform(GetLoc(name, GL_FLOAT_MAT4), mat); }
    void SetUniform(const string& name, const bool val) { SetUniform(GetLoc(name, GL_BOOL), val); }
    void SetUniform(const string& name, const int32_t val) { SetUniform(GetLoc(name, GL_INT), val); }
    void SetUniform(const string& name, const uint32_t val) { SetUniform(GetLoc(name, GL_UNSIGNED_INT), val); }
    void SetUniform(const string& name, const float val) { SetUniform(GetLoc(name, GL_FLOAT), val); }
};


class OGLUAPI ProgDraw : protected ProgState
{
    friend class _oglProgram;
private:
    TextureManager & TexMan;
    UBOManager& UboMan;
    map<GLuint, std::pair<GLint, bool>> UniBindBackup;
    map<GLint, UniformValue> UniValBackup;
    ProgDraw(const ProgState& pstate, const Mat4x4& modelMat, const Mat3x3& normMat) noexcept;
    template<typename T>
    GLint GetLoc(const T& res, const GLenum valtype)
    {
        const GLint loc = prog.GetLoc(res, valtype);
        if (loc != GL_INVALID_INDEX)
            if (const auto it = common::container::FindInMap(prog.UniValCache, loc))
                UniValBackup.insert_or_assign(loc, *it);
        return loc;
    }
public:
    ProgDraw(ProgDraw&& other) = default;
    ~ProgDraw();
    ProgDraw& Restore();
    std::weak_ptr<_oglProgram> GetProg() const noexcept;
    ProgDraw& SetPosition(const Mat4x4& modelMat, const Mat3x3& normMat);
    ProgDraw& SetPosition(const Mat4x4& modelMat) { return SetPosition(modelMat, (Mat3x3)modelMat); }
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
        ProgState::setSubroutine(sr); return *this;
    }
    ProgDraw& setSubroutine(const string& sruname, const string& srname)
    {
        ProgState::setSubroutine(sruname, srname); return *this;
    }

    void SetVec(const ProgramResource* res, const float x, const float y) { SetVec(res, b3d::Coord2D(x, y)); }
    void SetVec(const ProgramResource* res, const float x, const float y, const float z) { SetVec(res, miniBLAS::Vec3(x, y, z)); }
    void SetVec(const ProgramResource* res, const float x, const float y, const float z, const float w) { SetVec(res, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec(const ProgramResource* res, const b3d::Coord2D& vec) { prog.SetUniform(GetLoc(res, GL_FLOAT_VEC2), vec); }
    void SetVec(const ProgramResource* res, const miniBLAS::Vec3& vec) { prog.SetUniform(GetLoc(res, GL_FLOAT_VEC3), vec, false); }
    void SetVec(const ProgramResource* res, const miniBLAS::Vec4& vec) { prog.SetUniform(GetLoc(res, GL_FLOAT_VEC4), vec, false); }
    void SetMat(const ProgramResource* res, const miniBLAS::Mat3x3& mat) { prog.SetUniform(GetLoc(res, GL_FLOAT_MAT3), mat, false); }
    void SetMat(const ProgramResource* res, const miniBLAS::Mat4x4& mat) { prog.SetUniform(GetLoc(res, GL_FLOAT_MAT4), mat, false); }
    void SetUniform(const ProgramResource* res, const bool val) { prog.SetUniform(GetLoc(res, GL_BOOL), val, false); }
    void SetUniform(const ProgramResource* res, const int32_t val) { prog.SetUniform(GetLoc(res, GL_INT), val, false); }
    void SetUniform(const ProgramResource* res, const uint32_t val) { prog.SetUniform(GetLoc(res, GL_UNSIGNED_INT), val, false); }
    void SetUniform(const ProgramResource* res, const float val) { prog.SetUniform(GetLoc(res, GL_FLOAT), val, false); }
    void SetVec(const string& name, const float x, const float y) { SetVec(name, b3d::Coord2D(x, y)); }
    void SetVec(const string& name, const float x, const float y, const float z) { SetVec(name, miniBLAS::Vec3(x, y, z)); }
    void SetVec(const string& name, const float x, const float y, const float z, const float w) { SetVec(name, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec(const string& name, const b3d::Coord2D& vec) { prog.SetUniform(GetLoc(name, GL_FLOAT_VEC2), vec); }
    void SetVec(const string& name, const miniBLAS::Vec3& vec) { prog.SetUniform(GetLoc(name, GL_FLOAT_VEC3), vec, false); }
    void SetVec(const string& name, const miniBLAS::Vec4& vec) { prog.SetUniform(GetLoc(name, GL_FLOAT_VEC4), vec, false); }
    void SetMat(const string& name, const miniBLAS::Mat3x3& mat) { prog.SetUniform(GetLoc(name, GL_FLOAT_MAT3), mat, false); }
    void SetMat(const string& name, const miniBLAS::Mat4x4& mat) { prog.SetUniform(GetLoc(name, GL_FLOAT_MAT4), mat, false); }
    void SetUniform(const string& name, const bool val) { prog.SetUniform(GetLoc(name, GL_BOOL), val, false); }
    void SetUniform(const string& name, const int32_t val) { prog.SetUniform(GetLoc(name, GL_INT), val, false); }
    void SetUniform(const string& name, const uint32_t val) { prog.SetUniform(GetLoc(name, GL_UNSIGNED_INT), val, false); }
    void SetUniform(const string& name, const float val) { prog.SetUniform(GetLoc(name, GL_FLOAT), val, false); }
};


}


using oglProgram = Wrapper<detail::_oglProgram>;

}