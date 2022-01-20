#pragma once
#include "oglRely.h"
#include "oglShader.h"
#include "oglBuffer.h"
#include "oglTexture.h"
#include "oglVAO.h"

#if COMMON_COMPILER_MSVC
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


//using UniformValue = std::variant<mbase::Vec2, mbase::Vec3, mbase::Vec4, mbase::Mat3, mbase::Mat4, bool, float, int32_t, uint32_t>;
struct UniformValue
{
    enum class Types : uint32_t { None, Vec2, Vec3, Vec4, Mat3, Mat4, Bool, F32, I32, U32 };
    Types Type;
    std::array<uint32_t, 16> DataStore;
    template<typename T> 
    forceinline T* Ptr() noexcept 
    { 
        return reinterpret_cast<T*>(DataStore.data());
    }
    template<typename T>
    forceinline const T* Ptr() const noexcept
    {
        return reinterpret_cast<const T*>(DataStore.data());
    }
#define CASE_TYPE(te, type) static_assert(sizeof(type) <= sizeof(DataStore));\
    UniformValue(const type& data) noexcept : Type(Types::te)
    CASE_TYPE(Vec2, mbase::Vec2) { data.SaveAll(Ptr<float>()); }
    CASE_TYPE(Vec3, mbase::Vec3) { data.SaveAll(Ptr<float>()); }
    CASE_TYPE(Vec4, mbase::Vec4) { data.SaveAll(Ptr<float>()); }
    CASE_TYPE(Mat3, mbase::Mat3) { data.SaveAll(Ptr<float>()); }
    CASE_TYPE(Mat4, mbase::Mat4) { data.SaveAll(Ptr<float>()); }
    CASE_TYPE(Bool, bool)        { Ptr<uint32_t>()[0] = data; }
    CASE_TYPE(F32,  float)       { Ptr<   float>()[0] = data; }
    CASE_TYPE(I32,  int32_t)     { Ptr< int32_t>()[0] = data; }
    CASE_TYPE(U32,  uint32_t)    { Ptr<uint32_t>()[0] = data; }
#undef CASE_TYPE
    template<Types T> 
    auto Get() const noexcept
    {
        if constexpr (T == Types::Vec2)
            return mbase::Vec2::LoadAll(Ptr<float>());
        else if constexpr (T == Types::Vec3)
            return mbase::Vec3::LoadAll(Ptr<float>());
        else if constexpr (T == Types::Vec4)
            return mbase::Vec4::LoadAll(Ptr<float>());
        else if constexpr (T == Types::Mat3)
            return mbase::Mat3::LoadAll(Ptr<float>());
        else if constexpr (T == Types::Mat4)
            return mbase::Mat4::LoadAll(Ptr<float>());
        else if constexpr (T == Types::Bool)
            return bool(Ptr<uint32_t>()[0]);
        else if constexpr (T == Types::F32)
            return Ptr<float>()[0];
        else if constexpr (T == Types::I32)
            return Ptr<int32_t>()[0];
        else if constexpr (T == Types::U32)
            return Ptr<uint32_t>()[0];
        else
            static_assert(!common::AlwaysTrue2<T>, "Unsupport type");
    }
    template<typename T>
    auto Visit(T&& visitor) const noexcept
    {
        switch (Type)
        {
        case Types::Vec2:   return visitor(Get<Types::Vec2>());
        case Types::Vec3:   return visitor(Get<Types::Vec3>());
        case Types::Vec4:   return visitor(Get<Types::Vec4>());
        case Types::Mat3:   return visitor(Get<Types::Mat3>());
        case Types::Mat4:   return visitor(Get<Types::Mat4>());
        case Types::Bool:   return visitor(Get<Types::Bool>());
        case Types::F32:    return visitor(Get<Types::F32> ());
        case Types::I32:    return visitor(Get<Types::I32> ());
        case Types::U32:    return visitor(Get<Types::U32> ());
        default:            return visitor(Get<Types::Vec2>());
        }
    }
};


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
    uint32_t Type;
    uint32_t Valtype;
    int32_t location;
    //length of array
    mutable uint32_t len;
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
        std::string_view Name;
        using Lesser = common::container::SetKeyLess<Routine, &Routine::Name>;
    private:
        const SubroutineResource* Host;
        uint32_t Id;
        constexpr Routine(const SubroutineResource* host, const std::string_view name, const uint32_t id) noexcept 
            : Name(name), Host(host), Id(id) { }
    };
    SubroutineResource(const uint32_t sType, const int32_t location, const std::string_view name) noexcept
        : Name(name), Stage(sType), UniLoc(location) {}
private:
    std::string Name;
    mutable std::string SRNames;
    mutable common::container::FrozenDenseSet<Routine, Routine::Lesser> Routines;
    uint32_t Stage;
    int32_t UniLoc;
public:
    using Lesser = common::container::SetKeyLess<SubroutineResource, &SubroutineResource::Name>;
    constexpr const std::string& GetName() const noexcept { return Name; }
    constexpr const std::vector<Routine>& GetRoutines() const noexcept { return Routines.RawData(); }
};


class OGLUAPI MappingResource : public common::NonCopyable, public common::NonMovable
{
private:
    using MappingPair = std::pair<std::string_view, const ProgramResource*>;
    using IndexedPair = std::pair<int32_t, const ProgramResource*>;
    std::string MappedNames;
    common::container::FrozenDenseSet<ProgramResource, ProgramResource::Lesser> Resources;
    common::container::FrozenDenseSet<IndexedPair,
        common::container::SetKeyLess<IndexedPair, &IndexedPair::first>> IndexedResources;
    common::container::FrozenDenseSet<MappingPair,
        common::container::SetKeyLess<MappingPair, &MappingPair::first>> Mappings;
public:
    MappingResource() { }
    void Init(std::vector<ProgramResource>&& resources, const std::map<std::string, std::string>& mappings);
    const ProgramResource* GetResource(const int32_t location) const noexcept;
    const ProgramResource* GetResource(std::string_view name) const noexcept;
    forceinline const ProgramResource* GetResource(const ProgramResource* res) const noexcept
    {
        const auto& ress = Resources.RawData();
        return (&ress[0] <= res) && (&ress.back() >= res) ? res : nullptr;
    }
    int32_t GetLocation(std::string_view name) const noexcept
    {
        const auto res = GetResource(name);
        return res ? res->location : GLInvalidIndex;
    }
    std::vector<std::pair<std::string_view, int32_t>> GetBindingNames() const noexcept;

    decltype(auto) begin() const noexcept { return Resources.begin(); }
    decltype(auto) end()   const noexcept { return Resources.end();   }
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
    ProgState& SetTexture(const oglTexBase& tex, const std::string_view name, const uint32_t idx = 0);
    ProgState& SetImage(const oglImgBase& img, const std::string_view name, const uint32_t idx = 0);
    ProgState& SetUBO(const oglUBO& ubo, const std::string_view name, const uint32_t idx = 0);
    ProgState& SetSubroutine(const SubroutineResource::Routine* routine);
    ProgState& SetSubroutine(const std::string_view subrName, const std::string_view routineName);
};


class OGLUAPI oglProgram_ : public common::NonMovable, 
    public detail::oglCtxObject<true>, public std::enable_shared_from_this<oglProgram_>
{
    friend class TextureManager;
    friend class UBOManager;
    friend class ProgState;
    friend class ProgDraw;
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
    std::optional<ShaderExtInfo> TmpExtInfo;
    std::set<ShaderExtProperty, ShaderExtProperty::Lesser> ShaderProps;
    std::set<ProgramResource, ProgramResource::Lesser> ProgRess;
    MappingResource UniformRess, UBORess, TexRess, ImgRess;
    std::set<SubroutineResource, SubroutineResource::Lesser> SubroutineRess;
    std::map<int32_t, UniformValue> UniValCache;
    std::vector<int32_t> UniBindCache; 
    std::map<uint32_t, oglTexBase> TexBindings;
    std::map<uint32_t, oglImgBase> ImgBindings;
    std::map<uint32_t, oglUBO> UBOBindings;
    std::map<const SubroutineResource*, const SubroutineResource::Routine*> SubroutineBindings;
    std::map<uint32_t, std::vector<uint32_t>> SubroutineSettings;
    uint32_t ProgramID = 0; //zero means invalid program

    static bool usethis(oglProgram_& prog, const bool change = true);
    oglProgram_(const std::u16string& name, const oglProgStub* stub, const bool isDraw);
    void InitLocs(const ShaderExtInfo& extInfo);
    void InitSubroutines(const ShaderExtInfo& extInfo);
    void FilterProperties(const ShaderExtInfo& extInfo);

    std::pair<const SubroutineResource*, const SubroutineResource::Routine*> LocateSubroutine
        (const std::string_view subrName, const std::string_view routineName) const noexcept;

    void SetSubroutines() noexcept;
    void SetBindings() noexcept;
    void SetSubroutine(const SubroutineResource::Routine* routine);
    void SetSubroutine(const SubroutineResource* subr, const SubroutineResource::Routine* routine);

    void SetVec_(const ProgramResource* res, const mbase::Vec2& vec, const bool keep = true);
    void SetVec_(const ProgramResource* res, const mbase::Vec3& vec, const bool keep = true);
    void SetVec_(const ProgramResource* res, const mbase::Vec4& vec, const bool keep = true);
    void SetMat_(const ProgramResource* res, const mbase::Mat4& mat, const bool keep = true);
    void SetMat_(const ProgramResource* res, const mbase::Mat3& mat, const bool keep = true);
    void SetVal_(const ProgramResource* res, const bool         val, const bool keep = true);
    void SetVal_(const ProgramResource* res, const float        val, const bool keep = true);
    void SetVal_(const ProgramResource* res, const int32_t      val, const bool keep = true);
    void SetVal_(const ProgramResource* res, const uint32_t     val, const bool keep = true);

public:
    std::u16string Name;
    virtual ~oglProgram_();

    [[nodiscard]] const std::set<ProgramResource, ProgramResource::Lesser>& getResources() const { return ProgRess; }
    [[nodiscard]] const std::set<ShaderExtProperty, ShaderExtProperty::Lesser>& getResourceProperties() const { return ShaderProps; }
    [[nodiscard]] const std::set<SubroutineResource, SubroutineResource::Lesser>& getSubroutineResources() const { return SubroutineRess; }
    [[nodiscard]] const std::map<ShaderType, oglShader>& getShaders() const { return Shaders; }
    [[nodiscard]] const std::map<int32_t, UniformValue>& getCurUniforms() const { return UniValCache; }

    [[nodiscard]] const ProgramResource* GetResource(const std::string& name) const { return common::container::FindInSet(ProgRess, name); }
    [[nodiscard]] const SubroutineResource* GetSubroutines(const std::string& name) const { return common::container::FindInSet(SubroutineRess, name); }
    [[nodiscard]] const SubroutineResource::Routine* GetSubroutine(const std::string& sruname);
    [[nodiscard]] const SubroutineResource::Routine* GetSubroutine(const SubroutineResource& sru);
    ProgState State() noexcept;
    template<typename K, typename V>
    forceinline void SetVec(const K& name, const V& vec)
    { 
        SetVec_(UniformRess.GetResource(name), vec);
    }
    template<typename K, typename V>
    forceinline void SetMat(const K& name, const V& mat)
    { 
        SetMat_(UniformRess.GetResource(name), mat);
    }
    template<typename K, typename V>
    forceinline void SetVal(const K& name, const V val)
    { 
        SetVal_(UniformRess.GetResource(name), val);
    }
    template<typename K>
    forceinline void SetVec(const K& name, const float x, const float y)
    { 
        SetVec_(UniformRess.GetResource(name), mbase::Vec2(x, y)      );
    }
    template<typename K>
    forceinline void SetVec(const K& name, const float x, const float y, const float z)
    { 
        SetVec_(UniformRess.GetResource(name), mbase::Vec3(x, y, z)   );
    }
    template<typename K>
    forceinline void SetVec(const K& name, const float x, const float y, const float z, const float w)
    { 
        SetVec_(UniformRess.GetResource(name), mbase::Vec4(x, y, z, w));
    }
    
    [[nodiscard]] static oglProgStub Create();
};


class OGLUAPI oglDrawProgram_ : public oglProgram_
{
    friend class ProgDraw;
    friend class oglProgram_::oglProgStub;
private:
    MAKE_ENABLER();
    std::vector<std::pair<std::string_view, int32_t>> OutputBindings;
    oglDrawProgram_(const std::u16string& name, const oglProgStub* stub);
public:
    MappingResource InputRess, OutputRess;
    ~oglDrawProgram_() override;

    [[nodiscard]] ProgDraw Draw() noexcept;

    [[nodiscard]] static oglDrawProgram Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config = {});
};

class oglFrameBuffer_;
class OGLUAPI ProgDraw
{
    friend class oglDrawProgram_;
private:
    using FBOPairType = std::pair<std::shared_ptr<oglFrameBuffer_>, common::RWSpinLock::ReadScopeType>;
    oglDrawProgram_& Prog;
    oglFrameBuffer_& FBO;
    common::SpinLocker::ScopeType Lock;
    common::RWSpinLock::ReadScopeType FBOLock;
    detail::ResourceBinder<oglTexBase_>& TexMan;
    detail::ResourceBinder<oglImgBase_>& ImgMan;
    detail::ResourceBinder<oglUniformBuffer_>& UboMan;
    std::map<uint32_t, oglTexBase> TexBindings;
    std::map<uint32_t, oglImgBase> ImgBindings;
    std::map<uint32_t, oglUBO> UBOBindings;
    std::map<const SubroutineResource*, const SubroutineResource::Routine*> SubroutineCache;
    std::map<const ProgramResource*, UniformValue> UniValBackup;
    ProgDraw(oglDrawProgram_* prog, FBOPairType&& fboInfo) noexcept;
    ProgDraw(oglDrawProgram_* prog) noexcept;
    template<typename T>
    const ProgramResource* BeforeSetVal(const T& name)
    {
        const auto res = Prog.UniformRess.GetResource(name);
        if (res && res->location != GLInvalidIndex)
            if (const auto it = common::container::FindInMap(Prog.UniValCache, res->location))
                UniValBackup.insert_or_assign(res, *it);
        return res;
    }
    void SetBindings() noexcept;
public:
    ProgDraw(ProgDraw&& other) noexcept = default;
    ~ProgDraw();
    ///<summary>restore current drawing state</summary>  
    ///<param name="quick">whether perform quick restore</param>
    ///<returns>self</returns>
    ProgDraw& Restore();
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

    ProgDraw& SetTexture(const oglTexBase& tex, const std::string_view name, const uint32_t idx = 0);
    ProgDraw& SetImage(const oglImgBase& img, const std::string_view name, const uint32_t idx = 0);
    ProgDraw& SetUBO(const oglUBO& ubo, const std::string_view name, const uint32_t idx = 0);
    ProgDraw& SetSubroutine(const SubroutineResource::Routine* routine);
    ProgDraw& SetSubroutine(const std::string_view subrName, const std::string_view routineName);

    template<typename T, typename Arg> 
    ProgDraw& SetVec(const T& name, Arg&& arg)
    { 
        Prog.SetVec_(BeforeSetVal(name), std::forward<Arg>(arg), false);
        return *this;
    }
    template<typename T, typename Arg>
    ProgDraw& SetMat(const T& name, Arg&& arg)
    { 
        Prog.SetMat_(BeforeSetVal(name), std::forward<Arg>(arg), false);
        return *this;
    }
    template<typename T, typename... Args>
    ProgDraw& SetVal(const T& name, Args&&... args)
    {
        const auto res = BeforeSetVal(name);
        if constexpr(sizeof...(Args) == 1)
            Prog.SetVal_(res, std::forward<Args>(args)..., false);
        else if constexpr (sizeof...(Args) == 2)
            Prog.SetVal_(res, mbase::Vec2(std::forward<Args>(args)...), false);
        else if constexpr (sizeof...(Args) == 3)
            Prog.SetVal_(res, mbase::Vec3(std::forward<Args>(args)...), false);
        else if constexpr (sizeof...(Args) == 4)
            Prog.SetVal_(res, mbase::Vec4(std::forward<Args>(args)...), false);
        return *this;
    }
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


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif