#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"
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
    Empty = 0, 
    MASK_CATEGORY = 0xf000,
    CAT_INPUT = 0x0000, CAT_OUTPUT = 0x1000, CAT_UNIFORM = 0x2000, CAT_UBO = 0x3000,
    MASK_TYPE_CAT = 0x0f00,
    TYPE_PRIMITIVE = 0x000, TYPE_TEX = 0x100, TYPE_IMG = 0x200,
    MASK_TYPE_DETAIL = 0x00ff, MASK_FULLCAT = 0xff00,
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


class OGLUAPI MappingResourceSolver : public common::NonCopyable, public common::NonMovable
{
private:
    using MappingPair = std::pair<std::string_view, const ProgramResource*>;
    using IndexedPair = std::pair<GLint, const ProgramResource*>;
    std::string MappedNames;
    common::container::FrozenDenseSet<ProgramResource, ProgramResource::Lesser> Resources;
    common::container::FrozenDenseSet<IndexedPair,
        common::container::SetKeyLess<IndexedPair, &IndexedPair::first>> IndexedResources;
    common::container::FrozenDenseSet<MappingPair,
        common::container::SetKeyLess<MappingPair, &MappingPair::first>> Mappings;
public:
    MappingResourceSolver() { }
    void Init(const std::vector<ProgramResource>& resources, const std::map<std::string, std::string>& mappings);
    const ProgramResource* GetResource(const GLint location) const noexcept;
    const ProgramResource* GetResource(std::string_view name) const noexcept;
    GLint GetLocation(std::string_view name) const noexcept
    {
        const auto res = GetResource(name);
        return res ? res->location : GLInvalidIndex;
    }
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


class OGLUAPI oglProgram_ : public common::NonMovable, 
    public detail::oglCtxObject<true>, public std::enable_shared_from_this<oglProgram_>
{
    friend class TextureManager;
    friend class UBOManager;
    friend class ProgState;
    friend class ProgDraw;
private:
    enum class UniformType : uint8_t { FLOAT, UINT, INT, BOOL, VEC4, VEC3, VEC2, MAT4, MAT3 };
    static GLenum ParseUniformType(const UniformType type);
protected:
    class OGLUAPI oglProgStub : public detail::oglCtxObject<true>
    {
        friend class oglProgram_;
    private:
        std::map<ShaderType, oglShader> Shaders;
        ShaderExtInfo ExtInfo;
    public:
        oglProgStub();
        ~oglProgStub();
        void AddShader(oglShader shader);
        bool AddExtShaders(const std::string& src, const ShaderConfig& config = {}, const bool allowCompute = true, const bool allowDraw = true);
        oglDrawProgram LinkDrawProgram(const std::u16string& name);
        oglComputeProgram LinkComputeProgram(const std::u16string& name);
    };

    std::map<ShaderType, oglShader> Shaders;
    ShaderExtInfo ExtInfo;
    std::map<std::string, const ProgramResource*, std::less<>> ResNameMapping;
    std::set<ProgramResource, ProgramResource::Lesser> ProgRess, TexRess, ImgRess, UBORess;
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
    oglProgram_(const std::u16string& name, const oglProgStub* stub, const bool isDraw);
    void RecoverState();
    void InitLocs();
    void InitSubroutines();
    void FilterProperties();
    GLint GetLoc(const ProgramResource* res,  UniformType valtype) const;
    GLint GetLoc(const std::string_view name, UniformType valtype) const;

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

    [[nodiscard]] const std::set<ProgramResource, ProgramResource::Lesser>& getResources() const { return ProgRess; }
    [[nodiscard]] const std::set<ShaderExtProperty, ShaderExtProperty::Lesser>& getResourceProperties() const { return ExtInfo.Properties; }
    [[nodiscard]] const std::set<SubroutineResource, SubroutineResource::Lesser>& getSubroutineResources() const { return SubroutineRess; }
    [[nodiscard]] const std::map<ShaderType, oglShader>& getShaders() const { return Shaders; }
    [[nodiscard]] const std::map<GLint, UniformValue>& getCurUniforms() const { return UniValCache; }

    [[nodiscard]] GLint GetLoc(const std::string& name) const;
    [[nodiscard]] const ProgramResource* GetResource(const std::string& name) const { return common::container::FindInSet(ProgRess, name); }
    [[nodiscard]] const SubroutineResource* GetSubroutines(const std::string& name) const { return common::container::FindInSet(SubroutineRess, name); }
    [[nodiscard]] const SubroutineResource::Routine* GetSubroutine(const std::string& sruname);
    [[nodiscard]] const SubroutineResource::Routine* GetSubroutine(const SubroutineResource& sru);
    ProgState State() noexcept;template<typename T>
    void SetVec    (const T& name, const b3d::Coord2D& vec)        { SetUniform(GetLoc(name, UniformType::VEC2 ), vec); }
    template<typename T>
    void SetVec    (const T& name, const miniBLAS::Vec3& vec)      { SetUniform(GetLoc(name, UniformType::VEC3 ), vec); }
    template<typename T>
    void SetVec    (const T& name, const miniBLAS::Vec4& vec)      { SetUniform(GetLoc(name, UniformType::VEC4 ), vec); }
    template<typename T>
    void SetMat    (const T& name, const miniBLAS::Mat3x3& mat)    { SetUniform(GetLoc(name, UniformType::MAT3 ), mat); }
    template<typename T>
    void SetMat    (const T& name, const miniBLAS::Mat4x4& mat)    { SetUniform(GetLoc(name, UniformType::MAT4 ), mat); }
    template<typename T>
    void SetUniform(const T& name, const bool val)                 { SetUniform(GetLoc(name, UniformType::BOOL ), val); }
    template<typename T>
    void SetUniform(const T& name, const int32_t val)              { SetUniform(GetLoc(name, UniformType::INT  ), val); }
    template<typename T>
    void SetUniform(const T& name, const uint32_t val)             { SetUniform(GetLoc(name, UniformType::UINT ), val); }
    template<typename T>
    void SetUniform(const T& name, const float val)                { SetUniform(GetLoc(name, UniformType::FLOAT), val); }
    template<typename T>
    void SetVec    (const T& name, const float x, const float y)                               { SetVec(name, b3d::Coord2D  (x, y)      ); }
    template<typename T>
    void SetVec    (const T& name, const float x, const float y, const float z)                { SetVec(name, miniBLAS::Vec3(x, y, z)   ); }
    template<typename T>
    void SetVec    (const T& name, const float x, const float y, const float z, const float w) { SetVec(name, miniBLAS::Vec4(x, y, z, w)); }
    
    [[nodiscard]] static oglProgStub Create();
};


class OGLUAPI oglDrawProgram_ : public oglProgram_
{
    friend class ProgDraw;
    friend class oglProgram_::oglProgStub;
private:
    MAKE_ENABLER();
    oglDrawProgram_(const std::u16string& name, const oglProgStub* stub);
public:
    MappingResourceSolver InputRess, OutputRess;
    ~oglDrawProgram_() override;

    [[nodiscard]] ProgDraw Draw() noexcept;

    [[nodiscard]] static oglDrawProgram Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config = {});
};

class oglFrameBuffer_;
class OGLUAPI ProgDraw
{
    friend class oglDrawProgram_;
private:
    using FBOIntpType = std::pair<std::shared_ptr<oglFrameBuffer_>, common::RWSpinLock::ReadScopeType>;
    oglDrawProgram_& Prog;
    std::shared_ptr<oglFrameBuffer_> FBO;
    common::SpinLocker::ScopeType Lock;
    common::RWSpinLock::ReadScopeType FBOLock;
    detail::TextureManager& TexMan;
    detail::TexImgManager& ImgMan;
    detail::UBOManager& UboMan;
    std::map<GLuint, oglTexBase> TexCache;
    std::map<GLuint, oglImgBase> ImgCache;
    std::map<GLuint, oglUBO> UBOCache;
    std::map<const SubroutineResource*, const SubroutineResource::Routine*> SubroutineCache;
    std::map<GLuint, std::pair<GLint, ProgResType>> UniBindBackup;
    std::map<GLint, UniformValue> UniValBackup;
    ProgDraw(oglDrawProgram_* prog, FBOIntpType&& fboInfo) noexcept;
    ProgDraw(oglDrawProgram_* prog) noexcept;
    template<typename T>
    GLint GetLoc(const T& res, const GLenum valtype)
    {
        const GLint loc = Prog.GetLoc(res, valtype);
        if (loc != -1)
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
    ///<summary>draw vao range</summary>  
    ///<param name="size">elements count being drawed</param>
    ///<param name="offset">elements offset</param>
    ///<returns>self</returns>
    ProgDraw& DrawRange(const oglVAO& vao, const uint32_t size, const uint32_t offset = 0);
    ///<summary>instanced draw actual vao</summary>  
    ///<param name="count">instance count being drawed</param>
    ///<param name="base">base instance</param>
    ///<returns>self</returns>
    ProgDraw& DrawInstance(const oglVAO& vao, const uint32_t count, const uint32_t base = 0);
    ProgDraw& Draw(const oglVAO& vao);
    ProgDraw& SetTexture(const oglTexBase& tex, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetTexture(const oglTexBase& tex, const GLuint pos);
    ProgDraw& SetImage(const oglImgBase& img, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetImage(const oglImgBase& img, const GLuint pos);
    ProgDraw& SetUBO(const oglUBO& ubo, const std::string& name, const GLuint idx = 0);
    ProgDraw& SetUBO(const oglUBO& ubo, const GLuint pos);
    ProgDraw& SetSubroutine(const SubroutineResource::Routine* routine);
    ProgDraw& SetSubroutine(const std::string_view& subrName, const std::string_view& routineName);

    template<typename T, typename... Args> 
    ProgDraw& SetVec    (const T& res, Args&&... args) { Prog.SetVec    (res, std::forward<Args>(args)...); return *this; }
    template<typename T, typename... Args>
    ProgDraw& SetMat    (const T& res, Args&&... args) { Prog.SetMat    (res, std::forward<Args>(args)...); return *this; }
    template<typename T, typename... Args>
    ProgDraw& SetUniform(const T& res, Args&&... args) { Prog.SetUniform(res, std::forward<Args>(args)...); return *this; }
};


class OGLUAPI oglComputeProgram_ : public oglProgram_
{
    friend class oglProgram_::oglProgStub;
private:
    MAKE_ENABLER();
    oglComputeProgram_(const std::u16string& name, const oglProgStub* stub);
    std::array<uint32_t, 3> LocalSize = { 0, 0, 0 };
public:
    virtual ~oglComputeProgram_() override {}

    [[nodiscard]] const std::array<uint32_t, 3>& GetLocalSize() const { return LocalSize; }

    void Run(const uint32_t groupX, const uint32_t groupY = 1, const uint32_t groupZ = 1);

    [[nodiscard]] static oglComputeProgram Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config = {});
    [[nodiscard]] static bool CheckSupport();
};




}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif