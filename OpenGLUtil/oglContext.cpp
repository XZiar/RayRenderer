#include "oglPch.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "oglException.h"


namespace oglu
{
using std::string;
using std::u16string;
using std::vector;

BindingState::BindingState()
{
    HRC = PlatFuncs::GetCurrentGLContext();
    CtxFunc->ogluGetIntegerv(GL_CURRENT_PROGRAM, &Prog);
    CtxFunc->ogluGetIntegerv(GL_VERTEX_ARRAY_BINDING, &VAO);
    CtxFunc->ogluGetIntegerv(GL_FRAMEBUFFER_BINDING, &FBO);
    CtxFunc->ogluGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &DFB);
    CtxFunc->ogluGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &RFB);
    CtxFunc->ogluGetIntegerv(GL_ARRAY_BUFFER_BINDING, &VBO);
    if (CtxFunc->SupportIndirectDraw)
        CtxFunc->ogluGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &IBO);
    else
        IBO = 0;
    CtxFunc->ogluGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &EBO);
    CtxFunc->ogluGetIntegerv(GL_TEXTURE_BINDING_2D, &Tex2D);
    CtxFunc->ogluGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &Tex2DArray);
    CtxFunc->ogluGetIntegerv(GL_TEXTURE_BINDING_3D, &Tex3D);
}

thread_local oglContext InnerCurCtx{ };

#if defined(_DEBUG)
#define CHECKCURRENT() if (!InnerCurCtx || InnerCurCtx.get() != this) COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#else
#define CHECKCURRENT()
#endif


static std::atomic_uint32_t LatestVersion = 0;
static std::map<void*, std::weak_ptr<oglContext_>> CTX_MAP;
static std::map<void*, std::weak_ptr<oglContext_>> EXTERN_CTX_MAP;
static common::RWSpinLock CTX_LOCK;
static common::SpinLocker EXTERN_CTX_LOCK;


static void GLAPIENTRY onMsg(GLenum source, GLenum type, [[maybe_unused]]GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_MARKER || type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP)
        return;
    if (source == GL_DEBUG_SOURCE_THIRD_PARTY && (id >= OGLUMsgIdMin && id <= OGLUMsgIdMax))
        return;
    DebugMessage msg(source, type, severity);
    const oglContext_::DBGLimit& limit = *reinterpret_cast<const oglContext_::DBGLimit*>(userParam);
    if (((limit.src & msg.From) != MsgSrc::Empty)
        && ((limit.type & msg.Type) != MsgType::Empty)
        && (uint8_t)limit.minLV <= (uint8_t)msg.Level)
    {
        msg.Msg.assign(message, message + length);

        if (msg.Type == MsgType::Error)
        {
            oglLog().error(u"OpenGL ERROR\n{}\n", msg.Msg);
            BindingState state;
            oglLog().debug(u"State: Prog[{}], VAO[{}], FBO[{}](D[{}]R[{}]) binding: VBO[{}], IBO[{}], EBO[{}]\n",
                state.Prog, state.VAO, state.FBO, state.DFB, state.RFB, state.VBO, state.IBO, state.EBO);
        }
        else
        {
            oglLog().verbose(u"OpenGL message\n{}\n", msg.Msg);
        }
    }
}


namespace detail
{
void CtxResHandler::Release()
{
    auto lock = Lock.WriteScope();
    for (auto& res : Resources)
        delete res.second;
    Resources.clear();
}
CtxResHandler::~CtxResHandler()
{
    Release();
}

static std::atomic_uint32_t SharedContextUID = 0;
SharedContextCore::SharedContextCore() : Id(SharedContextUID++) {}
SharedContextCore::~SharedContextCore()
{
    oglLog().debug(u"Here destroy GLContext Shared core [{}].\n", Id);
    ResHandler.Release();
}

}

#if defined(_WIN32)
oglContext_::oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, const bool external) : 
    Hdc(hdc), Hrc(hrc), SharedCore(sharedCore), IsExternal(external) { }
#else
oglContext_::oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw, const bool external) : 
    Hdc(hdc), Hrc(hrc), DRW(drw), SharedCore(sharedCore), IsExternal(external) { }
#endif

void oglContext_::Init(const bool isCurrent)
{
    oglContext oldCtx;
    if (!isCurrent)
    {
        oldCtx = oglContext_::CurrentContext();
        UseContext();
    }
    Capability = CtxFunc;
    if (common::UpdateAtomicMaximum(LatestVersion, CtxFunc->Version))
        oglLog().info(u"update API Version to [{}.{}]\n", CtxFunc->Version / 10, CtxFunc->Version % 10);

    CtxFunc->ogluGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&DepthTestFunc));
    if (CtxFunc->ogluIsEnabled(GL_CULL_FACE))
    {
        GLint cullingMode;
        CtxFunc->ogluGetIntegerv(GL_CULL_FACE_MODE, &cullingMode);
        if (cullingMode == GL_FRONT_AND_BACK)
            FaceCulling = FaceCullingType::CullAll;
        else
        {
            GLint frontFace = GL_CCW;
            CtxFunc->ogluGetIntegerv(GL_FRONT_FACE, &frontFace);
            FaceCulling = ((cullingMode == GL_BACK) ^ (frontFace == GL_CW)) ? FaceCullingType::CullCW : FaceCullingType::CullCCW;
        }
    }
    else
        FaceCulling = FaceCullingType::OFF;

    if (!isCurrent)
        oldCtx->UseContext();
}

void oglContext_::FinishGL()
{
    CHECKCURRENT();
    CtxFunc->ogluFinish();
}

oglContext_::~oglContext_()
{
#if defined(_DEBUG)
    if (!IsExternal)
        oglLog().debug(u"Here destroy glContext [{}].\n", Hrc);
#endif
    ResHandler.Release();
    //if (!IsRetain)
    if (!IsExternal)
    {
        PlatFuncs::DeleteGLContext(Hdc, Hrc);
    }
}

bool oglContext_::UseContext(const bool force)
{
    if (!force)
    {
        if (InnerCurCtx && InnerCurCtx.get() == this)
            return true;
    }
#if defined(_WIN32)
    if (!PlatFuncs::MakeGLContextCurrent(Hdc, Hrc))
    {
        oglLog().error(u"Failed to use HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, PlatFuncs::GetSystemError());
        return false;
    }
#else
    if (!PlatFuncs::MakeGLContextCurrent(Hdc, DRW, Hrc))
    {
        oglLog().error(u"Failed to use Disp[{}] Drawable[{}] CTX[{}], error: {}\n", Hdc, DRW, Hrc, PlatFuncs::GetSystemError());
        return false;
    }
#endif
    InnerCurCtx = this->shared_from_this();
    return true;
}

bool oglContext_::UnloadContext()
{
    if (InnerCurCtx && InnerCurCtx.get() == this)
    {
        InnerCurCtx.reset();
#if defined(_WIN32)
        if (!PlatFuncs::MakeGLContextCurrent(Hdc, nullptr))
        {
            oglLog().error(u"Failed to unload HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, PlatFuncs::GetSystemError());
            return false;
        }
#else
        if (!PlatFuncs::MakeGLContextCurrent(Hdc, DRW, nullptr))
        {
            oglLog().error(u"Failed to unload Disp[{}] Drawable[{}] CTX[{}], error: {}\n", Hdc, DRW, Hrc, PlatFuncs::GetSystemError());
            return false;
        }
#endif
    }
    return true;
}

void oglContext_::Release()
{
    UseContext();
    std::vector<const CtxResCfg*> deletes;
    //Lock.LockRead();
    for (auto it = ResHandler.Resources.begin(); it != ResHandler.Resources.end();)
    {
        if (it->first->IsEagerRelease())
        {
            delete it->second;
            deletes.push_back(it->first);
        }
        else
            ++it;
    }
    //Lock.UnlockRead();
    //Lock.LockWrite();
    for (const auto key : deletes)
        ResHandler.Resources.erase(key);
    //Lock.UnlockWrite();
    UnloadContext();
}
//void oglContext_::SetRetain(const bool isRetain)
//{
//    static std::set<oglContext> retainMap;
//    static std::atomic_flag mapLock = { };
//    if(isRetain != IsRetain)
//    {
//        common::SpinLocker lock(mapLock);
//        auto self = this->shared_from_this();
//        if (isRetain)
//            retainMap.insert(self);
//        else
//            retainMap.erase(self);
//    }
//    IsRetain = isRetain;
//}

void oglContext_::SetDebug(MsgSrc src, MsgType type, MsgLevel minLV)
{
    CHECKCURRENT();
    DbgLimit = { type, src, minLV };
    if (CtxFunc->SupportDebug)
    {
        CtxFunc->ogluEnable(GL_DEBUG_OUTPUT);
        CtxFunc->ogluEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        CtxFunc->ogluDebugMessageCallback(onMsg, &DbgLimit);
    }
}

void oglContext_::SetDepthTest(const DepthTestType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case DepthTestType::OFF: 
        CtxFunc->ogluDisable(GL_DEPTH_TEST);
        break;
    case DepthTestType::Never:
    case DepthTestType::Always:
    case DepthTestType::Equal:
    case DepthTestType::NotEqual:
    case DepthTestType::Less:
    case DepthTestType::LessEqual:
    case DepthTestType::Greater:
    case DepthTestType::GreaterEqual:
        CtxFunc->ogluEnable(GL_DEPTH_TEST);
        CtxFunc->ogluDepthFunc(common::enum_cast(type));
        break;
    default:
        oglLog().warning(u"Unsupported depth test type [{}]\n", (uint32_t)type);
        return;
    }
    DepthTestFunc = type;
}

void oglContext_::SetFaceCulling(const FaceCullingType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case FaceCullingType::OFF:
        CtxFunc->ogluDisable(GL_CULL_FACE); break;
    case FaceCullingType::CullCW:
        CtxFunc->ogluEnable(GL_CULL_FACE); CtxFunc->ogluCullFace(GL_BACK); CtxFunc->ogluFrontFace(GL_CCW); break;
    case FaceCullingType::CullCCW:
        CtxFunc->ogluEnable(GL_CULL_FACE); CtxFunc->ogluCullFace(GL_BACK); CtxFunc->ogluFrontFace(GL_CW); break;
    case FaceCullingType::CullAll:
        CtxFunc->ogluEnable(GL_CULL_FACE); CtxFunc->ogluCullFace(GL_FRONT_AND_BACK); break;
    default:
        oglLog().warning(u"Unsupported face culling type [{}]\n", (uint32_t)type);
        return;
    }
    FaceCulling = type;
}

void oglContext_::SetDepthClip(const bool fix)
{
    CHECKCURRENT();
    if (CtxFunc->SupportClipControl)
    {
        if (fix)
        {
            CtxFunc->ogluClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            CtxFunc->ogluClearDepth(0.0f);
        }
        else
        {
            CtxFunc->ogluClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            CtxFunc->ogluClearDepth(1.0f);
        }
    }
    else
    {
        oglLog().warning(u"ClipControl not supported on the context, ignored");
    }
}

void oglContext_::SetSRGBFBO(const bool isEnable)
{
    CHECKCURRENT();
    if (CtxFunc->SupportSRGB)
    {
        if (isEnable)
            CtxFunc->ogluEnable(GL_FRAMEBUFFER_SRGB);
        else
            CtxFunc->ogluDisable(GL_FRAMEBUFFER_SRGB);
    }
    else
    {
        oglLog().warning(u"sRGB FBO not supported on the context, ignored");
    }
}


//void oglContext_::SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
//{
//    CHECKCURRENT();
//    CtxFunc->ogluViewport(x, y, (GLsizei)width, (GLsizei)height);
//}

miniBLAS::VecI4 oglContext_::GetViewPort() const
{
    CHECKCURRENT();
    miniBLAS::VecI4 viewport;
    CtxFunc->ogluGetIntegerv(GL_VIEWPORT, viewport);
    return viewport;
}

void oglContext_::MemBarrier(const GLMemBarrier mbar)
{
    CHECKCURRENT();
    CtxFunc->ogluMemoryBarrier(common::enum_cast(mbar));
}

struct oglContext_::oglMarker
{
    oglContext Context;
    std::shared_ptr<oglContext_::oglMarker> Previous;
    oglMarker(oglContext_* ctx) : Context(ctx->shared_from_this()), 
        Previous(ctx->CurrentRangeMarker.lock())
    { }
    ~oglMarker()
    {
        CtxFunc->ogluPopDebugGroup();
        Context->CurrentRangeMarker = Previous;
    }
};
std::shared_ptr<void> oglContext_::DeclareRange(std::u16string_view name)
{
    CHECKCURRENT();
    CtxFunc->ogluPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, OGLUMsgIdMin, name);
    const auto marker = std::make_shared<oglMarker>(this);
    CurrentRangeMarker = marker;
    return marker;
}

uint32_t oglContext_::GetLatestVersion()
{
    return LatestVersion;
}
oglContext oglContext_::CurrentContext()
{
    return InnerCurCtx;
}



oglContext oglContext_::Refresh()
{
    void* hrc = PlatFuncs::GetCurrentGLContext();
    if (hrc == nullptr)
    {
        oglLog().debug(u"currently no GLContext\n");
        return {};
    }
    oglContext ctx;
    {
        auto lock = CTX_LOCK.ReadScope();
        ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock();
    }
    if (ctx)
    {
        InnerCurCtx = ctx;
        return ctx;
    }
    else
    {
        {
            auto lock = CTX_LOCK.WriteScope();
            ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock(); // second check
            if (!ctx) // need to create the wrapper
            {
#if defined(_WIN32)
                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), 
                    PlatFuncs::GetCurrentDeviceContext(), hrc, true));
#else
                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), 
                    PlatFuncs::GetCurrentDeviceContext(), hrc, PlatFuncs::GetCurrentDrawable(), true));
#endif
                CTX_MAP.emplace(hrc, ctx);
            }
        }
        InnerCurCtx = ctx;
        ctx->Init(true);
#if defined(_DEBUG) || 1
        ctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
#endif
        {
            const auto lock = EXTERN_CTX_LOCK.LockScope();
            EXTERN_CTX_MAP.emplace(hrc, ctx);
        }
        return ctx;
    }
}


oglContext oglContext_::NewContext(const oglContext& ctx, const bool isShared, const int32_t *attribs)
{
    oglContext newCtx;
    const auto newHrc = PlatFuncs::CreateNewContext(ctx.get(), isShared, attribs);
#if defined(_WIN32)
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by HDC[{}] HRC[{}] ({}), error: {}\n", 
            ctx->Hdc, ctx->Hrc, isShared ? u"shared" : u"", PlatFuncs::GetSystemError());
        return {};
    }
    newCtx.reset(new oglContext_(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), 
        ctx->Hdc, newHrc));
#else
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by Display[{}] Drawable[{}] HRC[{}] ({}), error: {}\n", 
            ctx->Hdc, ctx->DRW, ctx->Hrc, isShared ? u"shared" : u"", PlatFuncs::GetSystemError());
        return {};
    }
    newCtx.reset(new oglContext_(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), 
        ctx->Hdc, newHrc, ctx->DRW));
#endif
    newCtx->Init(false);

    {
        auto lock = CTX_LOCK.WriteScope();
        CTX_MAP.emplace(newHrc, newCtx);
    }

    return newCtx;
}
oglContext oglContext_::NewContext(const oglContext& ctx, const bool isShared, uint32_t version)
{
    if (version == 0) 
        version = LatestVersion;
    const auto ctxAttrb = PlatFuncs::GenerateContextAttrib(version, 
        ctx->Capability->Extensions.Has("GL_KHR_context_flush_control"));
    return NewContext(ctx, isShared, ctxAttrb.data());
}

bool oglContext_::ReleaseExternContext()
{
    return ReleaseExternContext(PlatFuncs::GetCurrentGLContext());
}
bool oglContext_::ReleaseExternContext(void* hrc)
{
    size_t dels = 0;
    {
        const auto lock = EXTERN_CTX_LOCK.LockScope();
        dels = EXTERN_CTX_MAP.erase(hrc);
    }
    if (dels > 0)
    {
        auto lock2 = CTX_LOCK.WriteScope();
        CTX_MAP.erase(hrc);
        return true;
    }
    else
    {
        oglLog().warning(u"unretained HRC[{:p}] was request to release.\n", hrc);
        return false;
    }
}


namespace detail
{
template<>
std::weak_ptr<SharedContextCore> oglCtxObject<true>::GetCtx()
{
    return oglContext_::CurrentContext()->SharedCore;
}
template<>
std::weak_ptr<oglContext_> oglCtxObject<false>::GetCtx()
{
    return oglContext_::CurrentContext();
}
template<>
void oglCtxObject<true>::CheckCurrent() const
{
#if defined(_DEBUG)
    if (!InnerCurCtx || Context.owner_before(InnerCurCtx->SharedCore) || InnerCurCtx->SharedCore.owner_before(Context))
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#endif
}
template<>
void oglCtxObject<false>::CheckCurrent() const
{
#if defined(_DEBUG)
    if (Context.owner_before(InnerCurCtx) || InnerCurCtx.owner_before(Context))
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#endif
}
template<>
bool oglCtxObject<true>::EnsureValid()
{
    const bool result = InnerCurCtx && InnerCurCtx->SharedCore == Context.lock();
#if defined(_DEBUG)
    if (!result)
        oglLog().warning(u"oglCtxObject(shared) is now invalid due to released-context or uncurrent-context.\n");
#endif
    return result;
}
template<>
bool oglCtxObject<false>::EnsureValid()
{
    const auto ctx = Context.lock();
    if (ctx)
    {
        ctx->UseContext();
        return true;
    }
#if defined(_DEBUG)
    oglLog().warning(u"oglCtxObject(shared) is now invalid due to released-context.\n");
#endif
    return false;
}
}

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

DebugMessage::DebugMessage(const GLenum from, const GLenum type, const GLenum lv)
    :Type(ParseType(type)), From(ParseSrc(from)), Level(ParseLevel(lv)) { }

DebugMessage::~DebugMessage(){ }

}