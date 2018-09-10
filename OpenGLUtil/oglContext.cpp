#include "oglRely.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "oglException.h"
#include "DSAWrapper.h"
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include "glew/wglew.h"
#   include "GL/wglext.h"
#else
#   include "glew/glxew.h"
#   include "GL/glxext.h"
//fucking X11 defines some terrible macro
#   undef Always
#   undef None
#endif

namespace oglu
{

BindingState::BindingState()
{
#if defined(_WIN32)
    HRC = wglGetCurrentContext();
#else
    HRC = glXGetCurrentContext();
#endif
    const oglContext ctx = oglContext::CurrentContext();
    glGetIntegerv(GL_CURRENT_PROGRAM, &progId);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoId);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboId);
    if (ctx->Version >= 40)
        glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &iboId);
    else
        iboId = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboId);
}

thread_local oglContext InnerCurCtx{ };

#if defined(_DEBUG)
#define CHECKCURRENT() if (!InnerCurCtx || InnerCurCtx.get() != this) COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#else
#define CHECKCURRENT()
#endif

static uint32_t& LatestVersion()
{
    static uint32_t version = 0;
    return version;
}

namespace detail
{

static void GLAPIENTRY onMsg(GLenum source, GLenum type, [[maybe_unused]]GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    DebugMessage msg(source, type, severity);
    const _oglContext::DBGLimit& limit = *reinterpret_cast<const _oglContext::DBGLimit*>(userParam);
    if (((limit.src & msg.From) != MsgSrc::Empty)
        && ((limit.type & msg.Type) != MsgType::Empty)
        && (uint8_t)limit.minLV <= (uint8_t)msg.Level)
    {
        auto theMsg = std::make_shared<DebugMessage>(msg);
        theMsg->Msg.assign(message, message + length);

        if (theMsg->Type == MsgType::Error)
        {
            oglLog().error(u"OpenGL ERROR\n{}\n", theMsg->Msg);
            BindingState state;
            oglLog().debug(u"Current Prog[{}], VAO[{}], FBO[{}] binding-state: VBO[{}], IBO[{}], EBO[{}]\n", state.progId, state.vaoId, state.fboId, state.vboId, state.iboId, state.eboId);
        }
        else
        {
            oglLog().verbose(u"OpenGL message\n{}\n", theMsg->Msg);
        }
    }
}

void CtxResHandler::Release()
{
    Lock.LockWrite();
    for (const auto&[key, val] : Resources)
        delete val;
    Resources.clear();
    Lock.UnlockWrite();
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

#if defined(_WIN32)
_oglContext::_oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc) : Hdc(hdc), Hrc(hrc),
    DSAs(new DSAFuncs(), [](DSAFuncs* ptr){ delete ptr; }), SharedCore(sharedCore) { }
#else
_oglContext::_oglContext(const std::shared_ptr<SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw) : Hdc(hdc), Hrc(hrc), DRW(drw),
    DSAs(new DSAFuncs(), [](DSAFuncs* ptr){ delete ptr; }), SharedCore(sharedCore) { }
#endif

void _oglContext::Init(const bool isCurrent)
{
    oglContext oldCtx;
    if (!isCurrent)
    {
        oldCtx = oglContext::CurrentContext();
        UseContext();
    }
    int32_t major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    Version = major * 10 + minor;
    auto& latestVer = LatestVersion();
    if (Version > latestVer)
    {
        oglLog().info(u"update API Version to [{}.{}]\n", Version / 10, Version % 10);
        glewInit();
        latestVer = Version;
    }
    glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&DepthTestFunc));
    if (glIsEnabled(GL_CULL_FACE))
    {
        GLint cullingMode;
        glGetIntegerv(GL_CULL_FACE_MODE, &cullingMode);
        if (cullingMode == GL_FRONT_AND_BACK)
            FaceCulling = FaceCullingType::CullAll;
        else
        {
            GLint frontFace = GL_CCW;
            glGetIntegerv(GL_FRONT_FACE, &frontFace);
            FaceCulling = ((cullingMode == GL_BACK) ^ (frontFace == GL_CW)) ? FaceCullingType::CullCW : FaceCullingType::CullCCW;
        }
    }
    else
        FaceCulling = FaceCullingType::OFF;

    InitDSAFuncs(*DSAs);
    Extensions = oglUtil::GetExtensions();

    if (!isCurrent)
        oldCtx->UseContext();
    else
        DSA = DSAs.get(); // DSA just initialized, set it
}

_oglContext::~_oglContext()
{
#if defined(_DEBUG)
    oglLog().debug(u"Here destroy glContext [{}].\n", Hrc);
#endif
    ResHandler.Release();
    if (!IsRetain)
    {
#if defined(_WIN32)
        wglDeleteContext((HGLRC)Hrc);
#else
        glXDestroyContext((Display*)Hdc, (GLXContext)Hrc);
#endif
    }
}

bool _oglContext::UseContext(const bool force)
{
    if (!force)
    {
        if (InnerCurCtx && InnerCurCtx.get() == this)
            return true;
    }
#if defined(_WIN32)
    if (!wglMakeCurrent((HDC)Hdc, (HGLRC)Hrc))
    {
        oglLog().error(u"Failed to use HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, GetLastError());
        return false;
    }
#else
    if (!glXMakeCurrent((Display*)Hdc, DRW, (GLXContext)Hrc))
    {
        oglLog().error(u"Failed to use Disp[{}] Drawable[{}] CTX[{}], error: {}\n", Hdc, DRW, Hrc, errno);
        return false;
    }
#endif
    DSA = DSAs.get(); //threadlocal of CurrentContext make sure lifetime
    InnerCurCtx = this->shared_from_this();
    return true;
}

bool _oglContext::UnloadContext()
{
    if (InnerCurCtx && InnerCurCtx.get() == this)
    {
        InnerCurCtx.release();
#if defined(_WIN32)
        if (!wglMakeCurrent((HDC)Hdc, nullptr))
        {
            oglLog().error(u"Failed to unload HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, GetLastError());
            return false;
        }
#else
        if (!glXMakeCurrent((Display*)Hdc, DRW, nullptr))
        {
            oglLog().error(u"Failed to unload Disp[{}] Drawable[{}] CTX[{}], error: {}\n", Hdc, DRW, Hrc, errno);
            return false;
        }
#endif
    }
    return true;
}

void _oglContext::Release()
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
void _oglContext::SetRetain(const bool isRetain)
{
    static std::set<oglContext> retainMap;
    static std::atomic_flag mapLock = {0};
    if(isRetain != IsRetain)
    {
        common::SpinLocker lock(mapLock);
        auto self = this->shared_from_this();
        if (isRetain)
            retainMap.insert(self);
        else
            retainMap.erase(self);
    }
    IsRetain = isRetain;
}

void _oglContext::SetDebug(MsgSrc src, MsgType type, MsgLevel minLV)
{
    CHECKCURRENT();
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    DbgLimit = { type, src, minLV };
    if (glDebugMessageCallback)
        glDebugMessageCallback(onMsg, &DbgLimit);
}

void _oglContext::SetDepthTest(const DepthTestType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case DepthTestType::OFF: glDisable(GL_DEPTH_TEST); break;
    case DepthTestType::Never:
    case DepthTestType::Always:
    case DepthTestType::Equal:
    case DepthTestType::NotEqual:
    case DepthTestType::Less:
    case DepthTestType::LessEqual:
    case DepthTestType::Greater:
    case DepthTestType::GreaterEqual:
        glEnable(GL_DEPTH_TEST); glDepthFunc((GLenum)type); break;
    default:
        oglLog().warning(u"Unsupported depth test type [{}]\n", (uint32_t)type);
        return;
    }
    DepthTestFunc = type;
}

void _oglContext::SetFaceCulling(const FaceCullingType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case FaceCullingType::OFF:
        glDisable(GL_CULL_FACE); break;
    case FaceCullingType::CullCW:
        glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW); break;
    case FaceCullingType::CullCCW:
        glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CW); break;
    case FaceCullingType::CullAll:
        glEnable(GL_CULL_FACE); glCullFace(GL_FRONT_AND_BACK); break;
    default:
        oglLog().warning(u"Unsupported face culling type [{}]\n", (uint32_t)type);
        return;
    }
    FaceCulling = type;
}

void _oglContext::SetDepthClip(const bool fix)
{
    CHECKCURRENT();
    if (fix)
    {
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glClearDepth(0.0f);
    }
    else
    {
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        glClearDepth(1.0f);
    }
}

void _oglContext::SetSRGBFBO(const bool isEnable)
{
    CHECKCURRENT();
    if (isEnable)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);
}

void _oglContext::ClearFBO()
{
    CHECKCURRENT();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void _oglContext::SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
{
    CHECKCURRENT();
    glViewport(x, y, (GLsizei)width, (GLsizei)height);
}

miniBLAS::VecI4 _oglContext::GetViewPort() const
{
    CHECKCURRENT();
    miniBLAS::VecI4 viewport;
    glGetIntegerv(GL_VIEWPORT, viewport);
    return viewport;
}


}


static std::map<void*, std::weak_ptr<detail::_oglContext>> CTX_MAP;
static common::RWSpinLock CTX_LOCK;

uint32_t oglContext::GetLatestVersion()
{
    return LatestVersion();
}
void oglContext::BasicInit()
{
#if !defined(_WIN32)
    glxewInit(); // for some glx API
#endif
}
oglContext oglContext::CurrentContext()
{
    return InnerCurCtx;
}

oglContext oglContext::Refresh()
{
    oglContext ctx;
#if defined(_WIN32)
    void *hrc = wglGetCurrentContext();
#else
    void *hrc = glXGetCurrentContext();
#endif
    CTX_LOCK.LockRead();
    ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock();
    if (ctx)
    {
        CTX_LOCK.UnlockRead();
        InnerCurCtx = ctx;
        return ctx;
    }
    else
    {
        CTX_LOCK.UnlockRead();
        CTX_LOCK.LockWrite();
        ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock(); // second check
        if (!ctx) // need to create the wrapper
        {
#if defined(_WIN32)
            ctx = oglContext(new detail::_oglContext(std::make_shared<detail::SharedContextCore>(), wglGetCurrentDC(), hrc));
#else
            ctx = oglContext(new detail::_oglContext(std::make_shared<detail::SharedContextCore>(), glXGetCurrentDisplay(), hrc, glXGetCurrentDrawable()));
#endif
            CTX_MAP.emplace(hrc, ctx);
        }
        InnerCurCtx = ctx;
        CTX_LOCK.UnlockWrite();
        ctx->Init(true);
        ctx->SetRetain(true);
        return ctx;
    }
}

#if !defined(_WIN32)
static int TmpXErrorHandler(Display* disp, XErrorEvent* evt)
{
    thread_local string txtBuf;
    txtBuf.resize(1024, '\0');
    XGetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
    txtBuf.resize(std::char_traits<char>::length(txtBuf.data()));
    oglLog().warning(u"X11 report an error with code[{}][{}]:\t{}\n", evt->error_code, evt->minor_code, 
        str::to_u16string(txtBuf, str::Charset::UTF8));
    return 0;
}
#endif
oglContext oglContext::NewContext(const oglContext& ctx, const bool isShared, const int32_t *attribs)
{
    oglContext newCtx;

#if defined(_WIN32)
    const auto newHrc = wglCreateContextAttribsARB((HDC)ctx->Hdc, isShared ? (HGLRC)ctx->Hrc : nullptr, attribs);
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by HDC[{}] HRC[{}] ({}), error: {}\n", ctx->Hdc, ctx->Hrc, isShared ? u"shared" : u"", GetLastError());
        return {};
    }
    newCtx.reset(new detail::_oglContext(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), ctx->Hdc, newHrc));
#else
    static int visual_attribs[] =
    {
        GLX_X_RENDERABLE, true,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        0
    };
    int num_fbc = 0;
    GLXFBConfig *fbc = glXChooseFBConfig((Display*)ctx->Hdc, DefaultScreen((Display*)ctx->Hdc), visual_attribs, &num_fbc);
    const auto oldHandler = XSetErrorHandler(&TmpXErrorHandler);
    const auto newHrc = glXCreateContextAttribsARB((Display*)ctx->Hdc, fbc[0], isShared ? (GLXContext)ctx->Hrc : nullptr, true, attribs);
    //XSetErrorHandler(oldHandler);
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by Display[{}] Drawable[{}] HRC[{}] ({}), error: {}\n", ctx->Hdc, ctx->DRW, ctx->Hrc, isShared ? u"shared" : u"", errno);
        return {};
    }
    newCtx.reset(new detail::_oglContext(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), ctx->Hdc, newHrc, ctx->DRW));
#endif
    newCtx->Init(false);

    CTX_LOCK.LockWrite();
    CTX_MAP.emplace(newHrc, newCtx.weakRef());
    CTX_LOCK.UnlockWrite();

    return newCtx;
}
oglContext oglContext::NewContext(const oglContext& ctx, const bool isShared, uint32_t version)
{
    if (version == 0) 
        version = LatestVersion();
#if defined(_WIN32)
    vector<int32_t> ctxAttrb
    {
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB
    };
    constexpr int32_t VerMajor = WGL_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t VerMinor = WGL_CONTEXT_MINOR_VERSION_ARB;
    // constexpr int32_t FlushFlag = WGL_CONTEXT_RELEASE_BEHAVIOR_ARB;
    // constexpr int32_t FlushVal = WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB;
    const bool supportFlushControl = WGLEW_ARB_context_flush_control == 1;
#else
    vector<int32_t> ctxAttrb
    {
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB
    };
    constexpr int32_t VerMajor = GLX_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t VerMinor = GLX_CONTEXT_MINOR_VERSION_ARB;
    // constexpr int32_t FlushFlag = GLX_CONTEXT_RELEASE_BEHAVIOR_ARB;
    // constexpr int32_t FlushVal = GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB;
    const bool supportFlushControl = false;// GLXEW_ARB_context_flush_control == 1;
#endif
    constexpr int32_t FlushFlag = 0x2097;
    constexpr int32_t FlushVal = 0;
    if (version != 0)
    {
        ctxAttrb.insert(ctxAttrb.end(),
            {
                VerMajor, static_cast<int32_t>(version / 10),
                VerMinor, static_cast<int32_t>(version % 10),
            });
    }
    if (GLEW_KHR_context_flush_control == GL_TRUE || supportFlushControl)
    {
        ctxAttrb.insert(ctxAttrb.end(), { FlushFlag, FlushVal }); // all the same
    }
    ctxAttrb.push_back(0);
    return NewContext(ctx, isShared, ctxAttrb.data());
}

namespace detail
{
template<>
std::weak_ptr<SharedContextCore> oglCtxObject<true>::GetCtx()
{
    return oglContext::CurrentContext()->SharedCore;
}
template<>
std::weak_ptr<_oglContext> oglCtxObject<false>::GetCtx()
{
    return oglContext::CurrentContext().weakRef();
}
template<>
void oglCtxObject<true>::CheckCurrent() const
{
#if defined(_DEBUG)
    if (!InnerCurCtx || Context.owner_before(InnerCurCtx->SharedCore) || InnerCurCtx->SharedCore.owner_before(Context))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#endif
}
template<>
void oglCtxObject<false>::CheckCurrent() const
{
#if defined(_DEBUG)
    if (Context.owner_before(InnerCurCtx) || InnerCurCtx.owner_before(Context))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
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

}