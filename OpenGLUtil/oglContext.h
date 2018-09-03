#pragma once
#include "oglRely.h"
#include "oglFBO.h"

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

enum class DepthTestType : GLenum 
{
    OFF = GL_INVALID_INDEX, Never = GL_NEVER, Equal = GL_EQUAL, NotEqual = GL_NOTEQUAL, Always = GL_ALWAYS,
    Less = GL_LESS, LessEqual = GL_LEQUAL, Greater = GL_GREATER, GreaterEqual = GL_GEQUAL
};
enum class FaceCullingType : uint8_t { OFF, CullCW, CullCCW, CullAll };


namespace detail
{
class _oglProgram;

struct OGLUAPI SharedContextCore
{
    std::vector<std::function<void(uint32_t)>> OnDestroy;
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
    friend class ::oclu::detail::_oclPlatform;
    template<typename, bool> friend class ContextResource;
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
    std::unique_ptr<DSAFuncs, void(*)(DSAFuncs*)> DSAs;
    std::vector<std::function<void(void*)>> OnDestroy;
    common::container::FrozenDenseSet<string_view> Extensions;
    const std::shared_ptr<SharedContextCore> SharedCore;
    DBGLimit DbgLimit;
    oglFBO FrameBuffer;
    FaceCullingType FaceCulling = FaceCullingType::OFF;
    DepthTestType DepthTestFunc = DepthTestType::Less;
    uint32_t Version;
#if defined(_WIN32)
    _oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc);
#else
    _oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw);
#endif
    void Init(const bool isCurrent);
public:
    ~_oglContext();
    bool UseContext(const bool force = false);
    bool UnloadContext();

    void SetDebug(MsgSrc src, MsgType type, MsgLevel minLV);
    void SetDepthTest(const DepthTestType type);
    void SetFaceCulling(const FaceCullingType type);
    void SetDepthClip(const bool fix);
    DepthTestType GetDepthTest() const { return DepthTestFunc; }
    FaceCullingType GetFaceCulling() const { return FaceCulling; }

    bool SetFBO(const oglFBO& fbo = {});
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
private:
    static MsgSrc ParseSrc(const GLenum src)
    {
        return static_cast<MsgSrc>(1 << (src - GL_DEBUG_SOURCE_API));
    }
    static MsgType ParseType(const GLenum type)
    {
        if (type <= GL_DEBUG_TYPE_OTHER)
            return static_cast<MsgType>(1 << (type - GL_DEBUG_TYPE_ERROR));
        else
            return static_cast<MsgType>(0x40 << (type - GL_DEBUG_TYPE_MARKER));
    }
    static MsgLevel ParseLevel(const GLenum lv)
    {
        switch (lv)
        {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return MsgLevel::Notfication;
        case GL_DEBUG_SEVERITY_LOW:
            return MsgLevel::Low;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return MsgLevel::Medium;
        case GL_DEBUG_SEVERITY_HIGH:
            return MsgLevel::High;
        default:
            return MsgLevel::Notfication;
        }
    }
public:
    const MsgType Type;
    const MsgSrc From;
    const MsgLevel Level;
    u16string Msg;

    DebugMessage(const GLenum from, const GLenum type, const GLenum lv)
        :Type(ParseType(type)), From(ParseSrc(from)), Level(ParseLevel(lv)) { }
};


class OGLUAPI oglContext : public common::Wrapper<detail::_oglContext>
{
    friend class detail::_oglContext;
    friend class oglUtil;
private:
    static void BasicInit();
public:
    using common::Wrapper<detail::_oglContext>::Wrapper;
    static uint32_t GetLatestVersion();
    static oglContext CurrentContext();
    static oglContext Refresh();
    static oglContext NewContext(const oglContext& ctx, const bool isShared, const int32_t *attribs);
    static oglContext NewContext(const oglContext& ctx, const bool isShared = false, uint32_t version = 0);
};

struct OGLUAPI BindingState
{
    GLint progId = 0, vaoId = 0, fboId = 0, vboId = 0, iboId = 0, eboId = 0;
    BindingState(const oglContext& ctx);
    BindingState() : BindingState(oglContext::CurrentContext()) {}
};

namespace detail
{

///<summary>Context-related resource map</summary>  
///<param name="Val">resource type</param>
///<param name="Shared">is the resource shared between shared context</param>
template<typename Val, bool Shared = true>
class ContextResource : public common::NonCopyable, public common::NonMovable
{
    static_assert(common::SharedPtrHelper<Val>::IsSharedPtr || std::is_trivial_v<Val>, "Val type should be trivial or shared_ptr.");
private:
    using KeyType = std::conditional_t<Shared, uint32_t, void*>;
    std::map<KeyType, Val> Map;
    common::RWSpinLock Lock;
    KeyType GetKey(const oglContext& ctx)
    {
        if constexpr (Shared)
            return ctx->SharedCore->Id;
        else
            return ctx->Hrc;
    }
    void Delete(KeyType key)
    {
        Lock.LockWrite();
        Map.erase(key);
        Lock.UnlockWrite();
    }
    template<class Creator>
    void InsertOrAssign(KeyType key, Creator creator)
    {
        Lock.LockWrite();
        if (auto obj = common::container::FindInMap(Map, key))
            *obj = creator(key);
        else
            Map.emplace(key, creator(key));
        Lock.UnlockWrite();
    }
public:
    ///<summary>Try get current context's resource</summary>  
    ///<returns>optional of resource</returns>
    std::optional<Val> TryGet()
    {
        const auto key = GetKey(oglContext::CurrentContext());
        Lock.LockRead();
        auto obj = common::container::FindInMap(Map, key, std::in_place);
        Lock.UnlockRead();
        return obj;
    }
    ///<summary>Get or create current context's resource</summary>  
    ///<param name="creator">callable that create resource with given key</param>
    ///<returns>resource</returns>
    template<class Creator>
    Val GetOrInsert(Creator creator) 
    { 
        const auto curCtx = oglContext::CurrentContext();
        const auto key = GetKey(curCtx);
        Lock.LockRead();
        if (auto obj = common::container::FindInMap(Map, key, std::in_place))
        {
            Lock.UnlockRead();
            return *obj;
        }
        else
        {
            Lock.UnlockRead();
            Lock.LockWrite();
            if (obj = common::container::FindInMap(Map, key, std::in_place); obj)
            {
                Lock.UnlockWrite();
                return *obj;
            }
            if constexpr (Shared)
                curCtx->SharedCore->OnDestroy.push_back([this](uint32_t ctxKey) { Delete(ctxKey); });
            else
                curCtx->OnDestroy.push_back([this](void* ctxKey) { Delete(ctxKey); });
            auto& ret = Map.emplace(key, creator(key)).first->second;
            Lock.UnlockWrite();
            return ret;
        }
    }
    //void Delete() { Delete(GetKey(oglContext::CurrentContext())); }
    ///<summary>Insert or repalce current context's resource</summary>  
    ///<param name="creator">callable that create resource with given key</param>
    template<class Creator>
    void InsertOrAssign(Creator creator) { InsertOrAssign(GetKey(oglContext::CurrentContext()), creator); }
};

}

}
