#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"
#include "oglUtil.h"
#include "3DElement.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324 4275)
#endif

namespace oglu
{
class ProgDraw;
class oglProgram_;
using oglProgram        = std::shared_ptr<oglProgram_>;
class oglDrawProgram_;
using oglDrawProgram    = std::shared_ptr<oglDrawProgram_>;
class oglComputeProgram_;
using oglComputeProgram = std::shared_ptr<oglComputeProgram_>;


using UniformValue = std::variant<miniBLAS::Vec3, miniBLAS::Vec4, miniBLAS::Mat3x3, miniBLAS::Mat4x4, b3d::Coord2D, bool, int32_t, uint32_t, float>;


enum class ProgResType : uint16_t 
{ 
    Empty = 0, CategoryMask = 0xf000, TypeMask = 0x0fff, TypeCatMask = 0x0f00, FullCatMask = 0xff00, RawTypeMask = 0x00ff,
    InputCat = 0x0000, UniformCat = 0x1000, UBOCat = 0x2000,
    PrimitiveType = 0x000, TexType = 0x100, ImgType = 0x200,
    Tex1D = 0x100, Tex2D = 0x110, Tex3D = 0x120, TexCube = 0x130, Tex1DArray = 0x140, Tex2DArray = 0x150,
    Img1D = 0x200, Img2D = 0x210, Img3D = 0x220, ImgCube = 0x230, Img1DArray = 0x240, Img2DArray = 0x250,
};
MAKE_ENUM_BITFIELD(ProgResType)

struct OGLUAPI ProgramResource
{
    friend class oglProgram_;
    friend class ProgState;
    friend class ProgDraw;
public:
    std::string Name;
    GLenum Type;
    GLenum Valtype;
    GLint location;
    //length of array
    mutable GLuint len;
    uint16_t size;
    ProgResType ResType;
    uint8_t ifidx;
    std::string_view GetTypeName() const noexcept;
    std::string_view GetValTypeName() const noexcept;
    using Lesser = common::container::SetKeyLess<ProgramResource, &ProgramResource::Name>;
};

class OGLUAPI SubroutineResource
{
    friend class oglProgram_;
    friend class ProgState;
    friend class ProgDraw;
public:
    struct OGLUAPI Routine
    {
        friend class SubroutineResource;
        friend class oglProgram_;
        friend class ProgState;
        friend class ProgDraw;
        const std::string Name;
    private:
        const GLuint Id;
        Routine(const std::string& name, const GLuint id) : Name(name), Id(id) {}
    };
    const std::string Name;
    const std::vector<Routine> Routines;
    const ShaderType Stage;
private:
    const GLint UniLoc;
public:
    SubroutineResource(const ShaderType sType, const GLint location, const std::string& name, std::vector<Routine>&& routines)
        : Name(name), Routines(std::move(routines)), Stage(sType), UniLoc(location) {}
    using Lesser = common::container::SetKeyLess<SubroutineResource, &SubroutineResource::Name>;
};


enum class ProgramMappingTarget : uint64_t 
{ 
    ProjectMat = "ProjectMat"_hash,
    ViewMat    = "ViewMat"_hash,
    ModelMat   = "ModelMat"_hash,
    MVPMat     = "MVPMat"_hash,
    MVPNormMat = "MVPNormMat"_hash,
    CamPosVec  = "CamPosVec"_hash,
    VertPos    = "VertPos"_hash,
    VertNorm   = "VertNorm"_hash,
    VertTexc   = "VertTexc"_hash,
    VertColor  = "VertColor"_hash,
    VertTan    = "VertTan"_hash,
    DrawID     = "DrawID"_hash,
};


///<summary>ProgState, use to batch set program binding states</summary>  
class OGLUAPI ProgState : public common::NonCopyable
{
    friend class oglProgram_;
private:
    oglProgram_& Prog;
    ProgState(oglProgram_& prog) : Prog(prog) { }
    ProgState(ProgState&& other) = default;
public:
    ~ProgState();
    ProgState& SetTexture(const oglTexBase& tex, const std::string& name, const GLuint idx = 0);
    //no check on pos, carefully use
    ProgState& SetTexture(const oglTexBase& tex, const GLuint pos);
    ProgState& SetImage(const oglImgBase& img, const std::string& name, const GLuint idx = 0);
    //no check on pos, carefully use
    ProgState& SetImage(const oglImgBase& img, const GLuint pos);
    ProgState& SetUBO(const oglUBO& ubo, const std::string& name, const GLuint idx = 0);
    //no check on pos, carefully use
    ProgState& SetUBO(const oglUBO& ubo, const GLuint pos);
    ProgState& SetSubroutine(const SubroutineResource::Routine* routine);
    ProgState& SetSubroutine(const std::string_view& subrName, const std::string_view& routineName);
};


class OGLUAPI oglProgram_ : public common::NonMovable, public common::AlignBase<32>, 
    public detail::oglCtxObject<true>, public std::enable_shared_from_this<oglProgram_>
{
    friend class TextureManager;
    friend class UBOManager;
    friend class ProgState;
    friend class ProgDraw;
protected:
    std::map<ShaderType, oglShader> Shaders;
    ShaderExtInfo ExtInfo;
    std::map<std::string, const ProgramResource*, std::less<>> ResNameMapping;
    std::set<ProgramResource, ProgramResource::Lesser> ProgRess, TexRess, ImgRess, UBORess, AttrRess;
    std::set<SubroutineResource, SubroutineResource::Lesser> SubroutineRess;
    std::map<const SubroutineResource::Routine*, const SubroutineResource*> subrLookup;
    std::map<GLint, UniformValue> UniValCache;
    std::vector<GLint> UniBindCache; 
    std::map<GLuint, oglTexBase> TexBindings;
    std::map<GLuint, oglImgBase> ImgBindings;
    std::map<GLuint, oglUBO> UBOBindings;
    std::map<const SubroutineResource*, const SubroutineResource::Routine*> SubroutineBindings;
    std::map<ShaderType, std::vector<GLuint>> SubroutineSettings;
    GLuint ProgramID = 0; //zero means invalid program

    static bool usethis(oglProgram_& prog, const bool change = true);
    oglProgram_(const std::u16string& name);
    void RecoverState();
    void InitLocs();
    void InitSubroutines();
    void FilterProperties();
    virtual void OnPrepare() = 0;
    GLint GetLoc(const ProgramResource* res, GLenum valtype) const;
    GLint GetLoc(const std::string& name,         GLenum valtype) const;

    void SetTexture(detail::TextureManager& texMan, const GLint pos, const oglTexBase& tex, const bool shouldPin = false);
    void SetTexture(detail::TextureManager& texMan, const std::map<GLuint, oglTexBase>& texs, const bool shouldPin = false);
    void SetImage(detail::TexImgManager& texMan, const GLint pos, const oglImgBase& tex, const bool shouldPin = false);
    void SetImage(detail::TexImgManager& texMan, const std::map<GLuint, oglImgBase>& texs, const bool shouldPin = false);
    void SetUBO(detail::UBOManager& uboMan, const GLint pos, const oglUBO& ubo, const bool shouldPin = false);
    void SetUBO(detail::UBOManager& uboMan, const std::map<GLuint, oglUBO>& ubos, const bool shouldPin = false);
    void SetSubroutine();
    void SetSubroutine(const SubroutineResource* subr, const SubroutineResource::Routine* routine);

    void SetUniform(const GLint pos, const b3d::Coord2D& vec,     const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Vec3& vec,   const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Vec4& vec,   const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Mat4x4& mat, const bool keep = true);
    void SetUniform(const GLint pos, const miniBLAS::Mat3x3& mat, const bool keep = true);
    void SetUniform(const GLint pos, const bool val,              const bool keep = true);
    void SetUniform(const GLint pos, const int32_t val,           const bool keep = true);
    void SetUniform(const GLint pos, const uint32_t val,          const bool keep = true);
    void SetUniform(const GLint pos, const float val,             const bool keep = true);
public:
    std::u16string Name;
    virtual ~oglProgram_();
    virtual bool AddExtShaders(const std::string& src, const ShaderConfig& config = {}) = 0;

    const std::set<ProgramResource, ProgramResource::Lesser>& getResources() const { return ProgRess; }
    const std::set<ShaderExtProperty, ShaderExtProperty::Lesser>& getResourceProperties() const { return ExtInfo.Properties; }
    const std::set<SubroutineResource, SubroutineResource::Lesser>& getSubroutineResources() const { return SubroutineRess; }
    const std::map<ShaderType, oglShader>& getShaders() const { return Shaders; }
    const std::map<GLint, UniformValue>& getCurUniforms() const { return UniValCache; }

    void AddShader(const oglShader& shader);
    void Link();
    GLint GetLoc(const std::string& name) const;
    const ProgramResource* GetResource(const std::string& name) const { return common::container::FindInSet(ProgRess, name); }
    const SubroutineResource* GetSubroutines(const std::string& name) const { return common::container::FindInSet(SubroutineRess, name); }
    const SubroutineResource::Routine* GetSubroutine(const std::string& sruname);
    const SubroutineResource::Routine* GetSubroutine(const SubroutineResource& sru);
    ProgState State() noexcept;
    void SetVec    (const ProgramResource* res, const float x, const float y)                               { SetVec(res, b3d::Coord2D  (x, y)      ); }
    void SetVec    (const ProgramResource* res, const float x, const float y, const float z)                { SetVec(res, miniBLAS::Vec3(x, y, z)   ); }
    void SetVec    (const ProgramResource* res, const float x, const float y, const float z, const float w) { SetVec(res, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec    (const ProgramResource* res, const b3d::Coord2D& vec)        { SetUniform(GetLoc(res, GL_FLOAT_VEC2  ), vec); }
    void SetVec    (const ProgramResource* res, const miniBLAS::Vec3& vec)      { SetUniform(GetLoc(res, GL_FLOAT_VEC3  ), vec); }
    void SetVec    (const ProgramResource* res, const miniBLAS::Vec4& vec)      { SetUniform(GetLoc(res, GL_FLOAT_VEC4  ), vec); }
    void SetMat    (const ProgramResource* res, const miniBLAS::Mat3x3& mat)    { SetUniform(GetLoc(res, GL_FLOAT_MAT3  ), mat); }
    void SetMat    (const ProgramResource* res, const miniBLAS::Mat4x4& mat)    { SetUniform(GetLoc(res, GL_FLOAT_MAT4  ), mat); }
    void SetUniform(const ProgramResource* res, const bool val)                 { SetUniform(GetLoc(res, GL_BOOL        ), val); }
    void SetUniform(const ProgramResource* res, const int32_t val)              { SetUniform(GetLoc(res, GL_INT         ), val); }
    void SetUniform(const ProgramResource* res, const uint32_t val)             { SetUniform(GetLoc(res, GL_UNSIGNED_INT), val); }
    void SetUniform(const ProgramResource* res, const float val)                { SetUniform(GetLoc(res, GL_FLOAT       ), val); }
    void SetVec    (const std::string& name, const float x, const float y)                               { SetVec(name, b3d::Coord2D  (x, y)      ); }
    void SetVec    (const std::string& name, const float x, const float y, const float z)                { SetVec(name, miniBLAS::Vec3(x, y, z)   ); }
    void SetVec    (const std::string& name, const float x, const float y, const float z, const float w) { SetVec(name, miniBLAS::Vec4(x, y, z, w)); }
    void SetVec    (const std::string& name, const b3d::Coord2D& vec)        { SetUniform(GetLoc(name, GL_FLOAT_VEC2  ), vec); }
    void SetVec    (const std::string& name, const miniBLAS::Vec3& vec)      { SetUniform(GetLoc(name, GL_FLOAT_VEC3  ), vec); }
    void SetVec    (const std::string& name, const miniBLAS::Vec4& vec)      { SetUniform(GetLoc(name, GL_FLOAT_VEC4  ), vec); }
    void SetMat    (const std::string& name, const miniBLAS::Mat3x3& mat)    { SetUniform(GetLoc(name, GL_FLOAT_MAT3  ), mat); }
    void SetMat    (const std::string& name, const miniBLAS::Mat4x4& mat)    { SetUniform(GetLoc(name, GL_FLOAT_MAT4  ), mat); }
    void SetUniform(const std::string& name, const bool val)                 { SetUniform(GetLoc(name, GL_BOOL        ), val); }
    void SetUniform(const std::string& name, const int32_t val)              { SetUniform(GetLoc(name, GL_INT         ), val); }
    void SetUniform(const std::string& name, const uint32_t val)             { SetUniform(GetLoc(name, GL_UNSIGNED_INT), val); }
    void SetUniform(const std::string& name, const float val)                { SetUniform(GetLoc(name, GL_FLOAT       ), val); }
};


class OGLUAPI oglDrawProgram_ : public oglProgram_
{
    friend class ProgDraw;
private:
    MAKE_ENABLER();
    using oglProgram_::oglProgram_;

    b3d::Mat4x4 matrix_Proj, matrix_View;
    GLint
        Uni_projMat   = GL_INVALID_INDEX,
        Uni_viewMat   = GL_INVALID_INDEX,
        Uni_modelMat  = GL_INVALID_INDEX,
        Uni_normalMat = GL_INVALID_INDEX,
        Uni_mvpMat    = GL_INVALID_INDEX,
        Uni_camPos    = GL_INVALID_INDEX;
    virtual void OnPrepare() override;
public:
    virtual ~oglDrawProgram_() override {}

    virtual bool AddExtShaders(const std::string& src, const ShaderConfig& config = {}) override;
    //void RegisterLocation();
    void SetProject(const b3d::Mat4x4 &);
    void SetView(const b3d::Mat4x4 &);

    ProgDraw Draw(const b3d::Mat4x4& modelMat, const b3d::Mat3x3& normMat) noexcept;
    ProgDraw Draw(const b3d::Mat4x4& modelMat = b3d::Mat4x4::identity()) noexcept;
    template<typename Iterator>
    ProgDraw Draw(const Iterator& begin, const Iterator& end) noexcept;

    static oglDrawProgram Create(const std::u16string& name);
};

class OGLUAPI ProgDraw
{
    friend class oglDrawProgram_;
private:
    oglDrawProgram_& Prog;
    detail::TextureManager& TexMan;
    detail::TexImgManager& ImgMan;
    detail::UBOManager& UboMan;
    std::map<GLuint, oglTexBase> TexCache;
    std::map<GLuint, oglImgBase> ImgCache;
    std::map<GLuint, oglUBO> UBOCache;
    std::map<const SubroutineResource*, const SubroutineResource::Routine*> SubroutineCache;
    std::map<GLuint, std::pair<GLint, ProgResType>> UniBindBackup;
    std::map<GLint, UniformValue> UniValBackup;
    ProgDraw(oglDrawProgram_& prog_, const b3d::Mat4x4& modelMat, const b3d::Mat3x3& normMat) noexcept;
    template<typename T>
    GLint GetLoc(const T& res, const GLenum valtype)
    {
        const GLint loc = Prog.GetLoc(res, valtype);
        if (loc != (GLint)GL_INVALID_INDEX)
            if (const auto it = common::container::FindInMap(Prog.UniValCache, loc))
                UniValBackup.insert_or_assign(loc, *it);
        return loc;
    }
public:
    ProgDraw(ProgDraw&& other) noexcept = default;
    ~ProgDraw();
    ///<summary>restore current drawing state</summary>  
    ///<param name="quick">whether perform quick restore</param>
    ///<returns>self</returns>
    ProgDraw& Restore(const bool quick = false);
    std::weak_ptr<oglDrawProgram_> GetProg() const noexcept;
    ProgDraw& SetPosition(const b3d::Mat4x4& modelMat, const b3d::Mat3x3& normMat);
    ProgDraw& SetPosition(const b3d::Mat4x4& modelMat) { return SetPosition(modelMat, (b3d::Mat3x3)modelMat); }
    ///<summary>draw actual vao</summary>  
    ///<param name="size">elements count being drawed</param>
    ///<param name="offset">elements offset</param>
    ///<returns>self</returns>
    ProgDraw& Draw(const oglVAO& vao, const uint32_t size, const uint32_t offset = 0);
    ProgDraw& Draw(const oglVAO& vao);
    ProgDraw& SetTexture(const oglTexBase& tex, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetTexture(const oglTexBase& tex, const GLuint pos);
    ProgDraw& SetImage(const oglImgBase& img, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetImage(const oglImgBase& img, const GLuint pos);
    ProgDraw& SetUBO(const oglUBO& ubo, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetUBO(const oglUBO& ubo, const GLuint pos);
    ProgDraw& SetSubroutine(const SubroutineResource::Routine* routine);
    ProgDraw& SetSubroutine(const std::string_view& subrName, const std::string_view& routineName);

    ProgDraw& SetVec    (const ProgramResource* res, const float x, const float y)                               { SetVec(res, b3d::Coord2D  (x, y)      ); return *this; }
    ProgDraw& SetVec    (const ProgramResource* res, const float x, const float y, const float z)                { SetVec(res, miniBLAS::Vec3(x, y, z)   ); return *this; }
    ProgDraw& SetVec    (const ProgramResource* res, const float x, const float y, const float z, const float w) { SetVec(res, miniBLAS::Vec4(x, y, z, w)); return *this; }
    ProgDraw& SetVec    (const ProgramResource* res, const b3d::Coord2D& vec)     { Prog.SetUniform(GetLoc(res, GL_FLOAT_VEC2  ), vec       ); return *this; }
    ProgDraw& SetVec    (const ProgramResource* res, const miniBLAS::Vec3& vec)   { Prog.SetUniform(GetLoc(res, GL_FLOAT_VEC3  ), vec, false); return *this; }
    ProgDraw& SetVec    (const ProgramResource* res, const miniBLAS::Vec4& vec)   { Prog.SetUniform(GetLoc(res, GL_FLOAT_VEC4  ), vec, false); return *this; }
    ProgDraw& SetMat    (const ProgramResource* res, const miniBLAS::Mat3x3& mat) { Prog.SetUniform(GetLoc(res, GL_FLOAT_MAT3  ), mat, false); return *this; }
    ProgDraw& SetMat    (const ProgramResource* res, const miniBLAS::Mat4x4& mat) { Prog.SetUniform(GetLoc(res, GL_FLOAT_MAT4  ), mat, false); return *this; }
    ProgDraw& SetUniform(const ProgramResource* res, const bool val)              { Prog.SetUniform(GetLoc(res, GL_BOOL        ), val, false); return *this; }
    ProgDraw& SetUniform(const ProgramResource* res, const int32_t val)           { Prog.SetUniform(GetLoc(res, GL_INT         ), val, false); return *this; }
    ProgDraw& SetUniform(const ProgramResource* res, const uint32_t val)          { Prog.SetUniform(GetLoc(res, GL_UNSIGNED_INT), val, false); return *this; }
    ProgDraw& SetUniform(const ProgramResource* res, const float val)             { Prog.SetUniform(GetLoc(res, GL_FLOAT       ), val, false); return *this; }
    ProgDraw& SetVec    (const std::string& name, const float x, const float y)                               { SetVec(name, b3d::Coord2D  (x, y)      ); return *this; }
    ProgDraw& SetVec    (const std::string& name, const float x, const float y, const float z)                { SetVec(name, miniBLAS::Vec3(x, y, z)   ); return *this; }
    ProgDraw& SetVec    (const std::string& name, const float x, const float y, const float z, const float w) { SetVec(name, miniBLAS::Vec4(x, y, z, w)); return *this; }
    ProgDraw& SetVec    (const std::string& name, const b3d::Coord2D& vec)     { Prog.SetUniform(GetLoc(name, GL_FLOAT_VEC2  ), vec       ); return *this; }
    ProgDraw& SetVec    (const std::string& name, const miniBLAS::Vec3& vec)   { Prog.SetUniform(GetLoc(name, GL_FLOAT_VEC3  ), vec, false); return *this; }
    ProgDraw& SetVec    (const std::string& name, const miniBLAS::Vec4& vec)   { Prog.SetUniform(GetLoc(name, GL_FLOAT_VEC4  ), vec, false); return *this; }
    ProgDraw& SetMat    (const std::string& name, const miniBLAS::Mat3x3& mat) { Prog.SetUniform(GetLoc(name, GL_FLOAT_MAT3  ), mat, false); return *this; }
    ProgDraw& SetMat    (const std::string& name, const miniBLAS::Mat4x4& mat) { Prog.SetUniform(GetLoc(name, GL_FLOAT_MAT4  ), mat, false); return *this; }
    ProgDraw& SetUniform(const std::string& name, const bool val)              { Prog.SetUniform(GetLoc(name, GL_BOOL        ), val, false); return *this; }
    ProgDraw& SetUniform(const std::string& name, const int32_t val)           { Prog.SetUniform(GetLoc(name, GL_INT         ), val, false); return *this; }
    ProgDraw& SetUniform(const std::string& name, const uint32_t val)          { Prog.SetUniform(GetLoc(name, GL_UNSIGNED_INT), val, false); return *this; }
    ProgDraw& SetUniform(const std::string& name, const float val)             { Prog.SetUniform(GetLoc(name, GL_FLOAT       ), val, false); return *this; }
};

template<typename Iterator>
ProgDraw oglDrawProgram_::Draw(const Iterator& begin, const Iterator& end) noexcept
{
    static_assert(std::is_same_v<TransformOP, std::iterator_traits<Iterator>::value_type>, "Element insinde the range should be TransformOP.");
    b3d::Mat4x4 matModel = b3d::Mat4x4::identity();
    b3d::Mat3x3 matNormal = b3d::Mat3x3::identity();
    for (auto cur = begin; cur != end; ++cur)
    {
        const TransformOP& trans = *cur;
        oglUtil::applyTransform(matModel, matNormal, trans);
    }
    return Draw(matModel, matNormal);
}


class OGLUAPI oglComputeProgram_ : public oglProgram_
{
private:
    MAKE_ENABLER();
    using oglProgram_::oglProgram_;
    std::array<uint32_t, 3> LocalSize = { 0, 0, 0 };
    virtual void OnPrepare() override;
    using oglProgram_::AddShader;
public:
    virtual ~oglComputeProgram_() override {}

    virtual bool AddExtShaders(const std::string& src, const ShaderConfig& config = {}) override;
    const std::array<uint32_t, 3>& GetLocalSize() const { return LocalSize; }

    void Run(const uint32_t groupX, const uint32_t groupY = 1, const uint32_t groupZ = 1);

    static oglComputeProgram Create(const std::u16string& name);
};




}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif