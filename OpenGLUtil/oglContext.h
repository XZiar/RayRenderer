#pragma once
#include "oglRely.h"
#include "3DElement.hpp"
#include "common/ContainerEx.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{
class oglUtil;
struct BindingState;
class PlatFuncs;
struct DSAFuncs;
class oglContext_;
using oglContext = std::shared_ptr<oglContext_>;

enum class MsgSrc :uint8_t 
{
    OpenGL = 0x1, System = 0x2, Compiler = 0x4, ThreeRD = 0x8, Application = 0x10, Other = 0x20, 
    Empty = 0x0, All = 0x2f
};
MAKE_ENUM_BITFIELD(MsgSrc)
enum class MsgType :uint16_t
{
    Error = 0x1, Deprecated = 0x2, UndefBehav = 0x4, NonPortable = 0x8, Performance = 0x10, Other = 0x20,
    Maker = 0x40, PushGroup = 0x80, PopGroup = 0x100,
    Empty = 0x0, All = 0x1ff
};
MAKE_ENUM_BITFIELD(MsgType)
enum class MsgLevel :uint8_t { High = 3, Medium = 2, Low = 1, Notfication = 0 };
MAKE_ENUM_RANGE(MsgLevel)

struct OGLUAPI DebugMessage
{
    friend class oglContext_;
public:
    const MsgType Type;
    const MsgSrc From;
    const MsgLevel Level;
    std::u16string Msg;

    DebugMessage(const GLenum from, const GLenum type, const GLenum lv);
    ~DebugMessage();
};


struct OGLUAPI BindingState
{
    void* HRC = nullptr;
    GLint Prog = 0, VAO = 0, FBO = 0, DFB = 0, RFB = 0, VBO = 0, IBO = 0, EBO = 0;
    GLint Tex2D = 0, Tex2DArray = 0, Tex3D = 0;
    BindingState();
};


enum class DepthTestType : GLenum 
{
    OFF          = 0xFFFFFFFF/*GL_INVALID_INDEX*/,
    Never        = 0x0200/*GL_NEVER*/,
    Equal        = 0x0202/*GL_EQUAL*/,
    NotEqual     = 0x0205/*GL_NOTEQUAL*/,
    Always       = 0x0207/*GL_ALWAYS*/,
    Less         = 0x0201/*GL_LESS*/,
    LessEqual    = 0x0203/*GL_LEQUAL*/,
    Greater      = 0x0204/*GL_GREATER*/,
    GreaterEqual = 0x0206/*GL_GEQUAL*/
};
enum class FaceCullingType : uint8_t { OFF, CullCW, CullCCW, CullAll };

enum class GLMemBarrier : GLbitfield 
{
    VertAttrib = 0x00000001     /*GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT*/,
    EBO        = 0x00000002     /*GL_ELEMENT_ARRAY_BARRIER_BIT*/,
    Unifrom    = 0x00000004     /*GL_UNIFORM_BARRIER_BIT*/,
    TexFetch   = 0x00000008     /*GL_TEXTURE_FETCH_BARRIER_BIT*/,
    Image      = 0x00000020     /*GL_SHADER_IMAGE_ACCESS_BARRIER_BIT*/,
    Command    = 0x00000040     /*GL_COMMAND_BARRIER_BIT*/,
    PBO        = 0x00000080     /*GL_PIXEL_BUFFER_BARRIER_BIT*/,
    TexUpdate  = 0x00000100     /*GL_TEXTURE_UPDATE_BARRIER_BIT*/,
    Buffer     = 0x00000200     /*GL_BUFFER_UPDATE_BARRIER_BIT*/,
    FBO        = 0x00000400     /*GL_FRAMEBUFFER_BARRIER_BIT*/,
    TransFB    = 0x00000800     /*GL_TRANSFORM_FEEDBACK_BARRIER_BIT*/,
    Atomic     = 0x00001000     /*GL_ATOMIC_COUNTER_BARRIER_BIT*/,
    SSBO       = 0x00002000     /*GL_SHADER_STORAGE_BARRIER_BIT*/,
    BufMap     = 0x00004000     /*GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT*/,
    Query      = 0x00008000     /*GL_QUERY_BUFFER_BARRIER_BIT*/,
    All        = 0xFFFFFFFF     /*GL_ALL_BARRIER_BITS*/,
};
MAKE_ENUM_BITFIELD(GLMemBarrier)


struct CtxResCfg
{
    virtual bool IsEagerRelease() const = 0;
};
template<bool EagerRelease, typename T>
struct CtxResConfig : public CtxResCfg
{
    virtual bool IsEagerRelease() const override { return EagerRelease; }
    virtual T Construct() const = 0;
};

class oglProgram_;

namespace detail
{
struct ContextResource
{
    virtual ~ContextResource()
    { }
};
template<typename T>
struct AnyCtxRes : public ContextResource
{
private:
    T Res;
public:
    template<typename... Args>
    AnyCtxRes(Args&&... args) : Res(std::forward<Args>(args)...) {}
    virtual ~AnyCtxRes() override {}
    T* Ptr() { return &Res; }
    const T* Ptr() const { return &Res; }
    operator T& () { return Res; }
    operator const T& () const { return Res; }
    T* operator->() { return &Res; }
    const T* operator->() const { return &Res; }
};

class OGLUAPI CtxResHandler
{
public:
    std::map<const CtxResCfg*, ContextResource*> Resources;
    common::RWSpinLock Lock;
    ~CtxResHandler();
    ///<summary>Get or create current context's resource</summary>  
    ///<param name="creator">callable that create resource with given key</param>
    ///<returns>resource(pointer)</returns>
    template<typename T, bool Dummy>
    T& GetOrCreate(const CtxResConfig<Dummy, T>& cfg)
    {
        ContextResource** obj = nullptr;
        {
            auto lock = Lock.ReadScope();
            obj = common::container::FindInMap(Resources, &cfg);
        }
        if (obj)
            return *dynamic_cast<AnyCtxRes<T>*>(*obj);
        T ele = cfg.Construct();// create first, in case chained creation
        {
            auto lock = Lock.WriteScope();
            obj = common::container::FindInMap(Resources, &cfg);
            if (!obj)
                obj = &Resources.emplace(&cfg, new AnyCtxRes<T>(std::move(ele))).first->second;
            return *dynamic_cast<AnyCtxRes<T>*>(*obj);
        }
    }
    void Release();
};

struct OGLUAPI SharedContextCore
{
    friend class oglContext_;
    CtxResHandler ResHandler;
    const uint32_t Id;
    SharedContextCore();
    ~SharedContextCore();
};
}


///<summary>oglContext, all set/get method should be called after UseContext</summary>  
class OGLUAPI oglContext_ : public common::NonCopyable, public std::enable_shared_from_this<oglContext_>
{
    friend class oglProgram_;
    friend class oglWorker;
    friend class oglUtil;
    friend class PlatFuncs;
    friend struct BindingState;
    friend class ::oclu::GLInterop;
    friend class ::oclu::oclPlatform_;
    template<bool> friend class detail::oglCtxObject;
public:
    struct DBGLimit
    {
        MsgType type;
        MsgSrc src;
        MsgLevel minLV;
    };
private:
    MAKE_ENABLER();
    void *Hdc, *Hrc;
#if defined(_WIN32)
#else
    unsigned long DRW;
#endif
    detail::CtxResHandler ResHandler;
    const std::shared_ptr<detail::SharedContextCore> SharedCore;
    const common::container::FrozenDenseSet<std::string_view>* Extensions = nullptr;
    DBGLimit DbgLimit = { MsgType::All, MsgSrc::All, MsgLevel::Notfication };
    FaceCullingType FaceCulling = FaceCullingType::OFF;
    DepthTestType DepthTestFunc = DepthTestType::Less;
    uint32_t Version = 0;
    bool IsExternal;
    //bool IsRetain = false;
#if defined(_WIN32)
    oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, const bool external = false);
#else
    oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw, const bool external = false);
#endif
    void Init(const bool isCurrent);
    void FinishGL();
public:
    ~oglContext_();
    const auto& GetExtensions() const { return *Extensions; }

    bool UseContext(const bool force = false);
    bool UnloadContext();
    void Release();
    //void SetRetain(const bool isRetain);
    template<bool IsShared, typename T, bool Dummy>
    T& GetOrCreate(const CtxResConfig<Dummy, T>& cfg)
    {
        if constexpr (IsShared)
            return SharedCore->ResHandler.GetOrCreate(cfg);
        else
            return ResHandler.GetOrCreate(cfg);
    }

    void SetDebug(MsgSrc src, MsgType type, MsgLevel minLV);
    void SetDepthTest(const DepthTestType type);
    void SetFaceCulling(const FaceCullingType type);
    void SetDepthClip(const bool fix);
    DepthTestType GetDepthTest() const { return DepthTestFunc; }
    FaceCullingType GetFaceCulling() const { return FaceCulling; }

    void SetSRGBFBO(const bool isEnable);
    void ClearFBO();
    
    void SetViewPort(const miniBLAS::VecI4& viewport) { SetViewPort(viewport.x, viewport.y, viewport.z, viewport.w); }
    void SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height);
    miniBLAS::VecI4 GetViewPort() const;
    void MemBarrier(const GLMemBarrier mbar);



    static uint32_t GetLatestVersion();
    static oglContext CurrentContext();
    static oglContext Refresh();
    static oglContext NewContext(const oglContext& ctx, const bool isShared, const int32_t* attribs);
    static oglContext NewContext(const oglContext& ctx, const bool isShared = false, uint32_t version = 0);
    static bool ReleaseExternContext();
    static bool ReleaseExternContext(void* hrc);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
