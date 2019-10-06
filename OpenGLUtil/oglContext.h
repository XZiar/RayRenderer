#pragma once
#include "oglRely.h"
#include "3DElement.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{
class oglUtil;
class oglContext;
struct BindingState;
struct DSAFuncs;

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

namespace detail
{
class _oglProgram;

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
    operator T&() { return Res; }
    operator const T&() const { return Res; }
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
    friend class _oglContext;
    CtxResHandler ResHandler;
    const uint32_t Id;
    SharedContextCore();
    ~SharedContextCore();
};

///<summary>oglContext, all set/get method should be called after UseContext</summary>  
class OGLUAPI _oglContext : public common::NonCopyable, public std::enable_shared_from_this<_oglContext>
{
    friend class _oglProgram;
    friend class ::oglu::oglWorker;
    friend class ::oglu::oglUtil;
    friend class ::oglu::oglContext;
    friend struct ::oglu::BindingState;
    friend class ::oclu::GLInterop;
    friend class ::oclu::oclPlatform_;
    template<bool> friend class oglCtxObject;
public:
    struct DBGLimit
    {
        MsgType type;
        MsgSrc src;
        MsgLevel minLV;
    };
private:
    void *Hdc, *Hrc;
#if defined(_WIN32)
#else
    unsigned long DRW;
#endif
    CtxResHandler ResHandler;
    std::unique_ptr<DSAFuncs, void(*)(DSAFuncs*)> DSAs;
    common::container::FrozenDenseSet<string_view> Extensions;
    const std::shared_ptr<SharedContextCore> SharedCore;
    DBGLimit DbgLimit = { MsgType::All, MsgSrc::All, MsgLevel::Notfication };
    FaceCullingType FaceCulling = FaceCullingType::OFF;
    DepthTestType DepthTestFunc = DepthTestType::Less;
    uint32_t Version = 0;
    bool IsExternal;
    //bool IsRetain = false;
#if defined(_WIN32)
    _oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc, const bool external = false);
#else
    _oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw, const bool external = false);
#endif
    void Init(const bool isCurrent);
    void FinishGL();
public:
    ~_oglContext();
    const auto& GetExtensions() const { return Extensions; }

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
};

}


struct OGLUAPI DebugMessage
{
    friend class detail::_oglContext;
public:
    const MsgType Type;
    const MsgSrc From;
    const MsgLevel Level;
    u16string Msg;

    DebugMessage(const GLenum from, const GLenum type, const GLenum lv);
    ~DebugMessage();
};


class OGLUAPI oglContext : public common::Wrapper<detail::_oglContext>
{
    friend class detail::_oglContext;
    friend class oglUtil;
private:
    //static void BasicInit();
public:
    using common::Wrapper<detail::_oglContext>::Wrapper;
    static uint32_t GetLatestVersion();
    static oglContext CurrentContext();
    static oglContext Refresh();
    static oglContext NewContext(const oglContext& ctx, const bool isShared, const int32_t *attribs);
    static oglContext NewContext(const oglContext& ctx, const bool isShared = false, uint32_t version = 0);
    static bool ReleaseExternContext(void* hrc);
};

struct OGLUAPI BindingState
{
    void *HRC = nullptr;
    GLint Prog = 0, VAO = 0, FBO = 0, DFB = 0, RFB = 0, VBO = 0, IBO = 0, EBO = 0;
    GLint Tex2D = 0, Tex2DArray = 0, Tex3D = 0;
    BindingState();
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
