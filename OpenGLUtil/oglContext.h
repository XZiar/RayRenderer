#pragma once
#include "oglRely.h"
#include "oglFBO.h"

namespace oglu
{
class oglUtil;
class oglContext;

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


struct OGLUAPI BindingState
{
    GLint progId = 0, vaoId = 0, fboId = 0, vboId = 0, iboId = 0, eboId = 0;
    BindingState();
};

namespace detail
{

class _oglProgram;
class MTWorker;

template<bool Shared>
struct ContextResourceHelper;

class OGLUAPI _oglContext : public common::NonCopyable, public std::enable_shared_from_this<_oglContext>
{
    friend class _oglProgram;
    friend class MTWorker;
    friend class ::oglu::oglUtil;
    friend class ::oglu::oglContext;
public:
    struct DBGLimit
    {
        MsgType type;
        MsgSrc src;
        MsgLevel minLV;
    };
private:
    void *Hdc, *Hrc;
    DBGLimit DbgLimit;
    const uint32_t Uid;
    FaceCullingType FaceCulling = FaceCullingType::OFF;
    DepthTestType DepthTestFunc = DepthTestType::Less;
    _oglContext(const uint32_t uid, void *hdc, void *hrc);
public:
    ~_oglContext();
    bool UseContext();
    bool UnloadContext();
    void SetDebug(MsgSrc src, MsgType type, MsgLevel minLV);
    void SetDepthTest(const DepthTestType type);
    void SetFaceCulling(const FaceCullingType type);
    DepthTestType GetDepthTest() { return DepthTestFunc; }
    FaceCullingType GetFaceCulling() { return FaceCulling; }

    void SetFBO(const oglFBO& fbo) const;
    void SetViewPort(const miniBLAS::VecI4& viewport) const { SetViewPort(viewport.x, viewport.y, viewport.z, viewport.w); }
    void SetViewPort(const int32_t x, const int32_t y, const int32_t width, const int32_t height) const;
    miniBLAS::VecI4 GetViewPort() const;
};

}

struct OGLUAPI DebugMessage
{
    friend class detail::_oglContext;
private:
    static MsgSrc CDECLCALL ParseSrc(const GLenum src)
    {
        return static_cast<MsgSrc>(1 << (src - GL_DEBUG_SOURCE_API));
    }
    static MsgType CDECLCALL ParseType(const GLenum type)
    {
        if (type <= GL_DEBUG_TYPE_OTHER)
            return static_cast<MsgType>(1 << (type - GL_DEBUG_TYPE_ERROR));
        else
            return static_cast<MsgType>(0x40 << (type - GL_DEBUG_TYPE_MARKER));
    }
    static MsgLevel CDECLCALL ParseLevel(const GLenum lv)
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
    const MsgType type;
    const MsgSrc from;
    const MsgLevel level;
    u16string msg;

    DebugMessage(const GLenum from_, const GLenum type_, const GLenum lv)
        :from(ParseSrc(from_)), type(ParseType(type_)), level(ParseLevel(lv)) { }
};


class OGLUAPI oglContext : public common::Wrapper<detail::_oglContext>
{
    template<bool Shared> friend struct ::oglu::detail::ContextResourceHelper;
    friend class detail::_oglContext;
private:
    static void* CurrentHRC();
    static oglContext& CurrentCtx();
    static uint32_t CurrentCtxUid();
public:
    using common::Wrapper<detail::_oglContext>::Wrapper;
    static oglContext CurrentContext();
    static void Refresh();
    static oglContext NewContext(const oglContext& ctx, const bool isShared = false, int *attribs = nullptr);
};

namespace detail
{


template<> struct ContextResourceHelper<true>
{
    using KeyType = uint32_t;
    static KeyType CurCtx() 
    {
        return oglContext::CurrentCtxUid();
    }
};
template<> struct ContextResourceHelper<false>
{
    using KeyType = void*;
    static KeyType CurCtx()
    {
        return oglContext::CurrentHRC();
    }
};

///<summary>Context-related resource map</summary>  
///<param name="Val">resource type</param>
///<param name="Shared">is the resource shared between shared context</param>
template<typename Val, bool Shared = true>
class ContextResource : public common::NonCopyable, public common::NonMovable
{
    static_assert(common::SharedPtrHelper<Val>::IsSharedPtr || std::is_trivial_v<Val>, "Val type should be trivial or shared_ptr.");
private:
    using KeyType = typename ContextResourceHelper<Shared>::KeyType;
    std::map<KeyType, Val> Map;
    common::RWSpinLock Lock;
public:
    std::optional<Val> TryGet(KeyType key)
    {
        Lock.LockRead();
        auto obj = common::container::FindInMap(Map, key, std::in_place);
        Lock.UnlockRead();
        return obj;
    }
    ///<summary>Try get current context's resource</summary>  
    ///<returns>optional of resource</returns>
    std::optional<Val> TryGet()
    {
        return TryGet(ContextResourceHelper<Shared>::CurCtx());
    }
    template<class Creator>
    Val GetOrInsert(KeyType key, Creator creator)
    {
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
            if (obj = common::container::FindInMap(Map, key, std::in_place))
            {
                Lock.UnlockWrite();
                return *obj;
            }
            auto& ret = Map.emplace(key, creator(key)).first->second;
            Lock.UnlockWrite();
            return ret;
        }
    }
    ///<summary>Get or create current context's resource</summary>  
    ///<param name="creator">callable that create resource with given key</param>
    ///<returns>resource</returns>
    template<class Creator>
    Val GetOrInsert(Creator creator) { return GetOrInsert(ContextResourceHelper<Shared>::CurCtx(), creator); }
    void Delete(KeyType key)
    {
        Lock.LockWrite();
        Map.erase(key);
        Lock.UnlockWrite();
    }
    void Delete() { Delete(ContextResourceHelper<Shared>::CurCtx()); }
    template<class Creator>
    void InsertOrAssign(uint32_t key, Creator creator)
    {
        Lock.LockWrite();
        if (auto obj = common::container::FindInMap(Map, key))
            *obj = creator(key);
        else
            Map.emplace(key, creator(key));
        Lock.UnlockWrite();
    }
    ///<summary>Insert or repalce current context's resource</summary>  
    ///<param name="creator">callable that create resource with given key</param>
    template<class Creator>
    void InsertOrAssign(Creator creator) { InsertOrAssign(ContextResourceHelper<Shared>::CurCtx(), creator); }
};

}

}
