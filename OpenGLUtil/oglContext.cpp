#include "oglPch.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "oglException.h"
#include "DSAWrapper.h"
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include "glew/wglew.h"
#   include "GL/wglext.h"
#   pragma message("Compiling OpenGLUtil with wgl-ext[" STRINGIZE(WGL_WGLEXT_VERSION) "]")
#else
#   include "glew/glxew.h"
#   include "GL/glxext.h"
#   pragma message("Compiling OpenGLUtil with glx-ext[" STRINGIZE(GLX_GLXEXT_VERSION) "]")
//fucking X11 defines some terrible macro
#   undef Always
#   undef None
#endif

namespace oglu
{
using std::string;
using std::u16string;
using std::vector;

BindingState::BindingState()
{
#if defined(_WIN32)
    HRC = wglGetCurrentContext();
#else
    HRC = glXGetCurrentContext();
#endif
    const oglContext ctx = oglContext_::CurrentContext();
    glGetIntegerv(GL_CURRENT_PROGRAM, &Prog);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &VAO);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &FBO);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &DFB);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &RFB);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &VBO);
    if (ctx->Version >= 40)
        glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &IBO);
    else
        IBO = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &EBO);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &Tex2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &Tex2DArray);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &Tex3D);
}

thread_local oglContext InnerCurCtx{ };

#if defined(_DEBUG)
#define CHECKCURRENT() if (!InnerCurCtx || InnerCurCtx.get() != this) COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#else
#define CHECKCURRENT()
#endif


static std::atomic_uint32_t LatestVersion = 0;
static std::map<void*, std::weak_ptr<oglContext_>> CTX_MAP;
static std::map<void*, std::weak_ptr<oglContext_>> EXTERN_CTX_MAP;
static common::RWSpinLock CTX_LOCK;
static common::SpinLocker EXTERN_CTX_LOCK, VER_LOCK;


static void GLAPIENTRY onMsg(GLenum source, GLenum type, [[maybe_unused]]GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
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
oglContext_::oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, const bool external) : Hdc(hdc), Hrc(hrc),
    DSAs(new DSAFuncs(), [](DSAFuncs* ptr){ delete ptr; }), SharedCore(sharedCore), IsExternal(external) { }
#else
oglContext_::oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, void *hdc, void *hrc, unsigned long drw, const bool external) : Hdc(hdc), Hrc(hrc), DRW(drw),
    DSAs(new DSAFuncs(), [](DSAFuncs* ptr){ delete ptr; }), SharedCore(sharedCore), IsExternal(external) { }
#endif

void oglContext_::Init(const bool isCurrent)
{
    oglContext oldCtx;
    if (!isCurrent)
    {
        oldCtx = oglContext_::CurrentContext();
        UseContext();
    }
    int32_t major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    Version = major * 10 + minor;
    {
        const auto lock = VER_LOCK.LockScope();
        if (Version > LatestVersion)
        {
            LatestVersion = Version;
            oglLog().info(u"update API Version to [{}.{}]\n", major, minor);
            glewInit();
        }
        InitDSAFuncs(*DSAs);
        Extensions = oglUtil::GetExtensions();
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

    if (!isCurrent)
        oldCtx->UseContext();
    else
        DSA = DSAs.get(); // DSA just initialized, set it
}

void oglContext_::FinishGL()
{
    CHECKCURRENT();
    glFinish();
}

oglContext_::~oglContext_()
{
#if defined(_DEBUG)
    oglLog().debug(u"Here destroy glContext [{}].\n", Hrc);
#endif
    ResHandler.Release();
    //if (!IsRetain)
    if (!IsExternal)
    {
#if defined(_WIN32)
        wglDeleteContext((HGLRC)Hrc);
#else
        glXDestroyContext((Display*)Hdc, (GLXContext)Hrc);
#endif
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

bool oglContext_::UnloadContext()
{
    if (InnerCurCtx && InnerCurCtx.get() == this)
    {
        InnerCurCtx.reset();
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
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    DbgLimit = { type, src, minLV };
    if (DSA->ogluDebugMessageCallback)
        DSA->ogluDebugMessageCallback(onMsg, &DbgLimit);
}

void oglContext_::SetDepthTest(const DepthTestType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case DepthTestType::OFF: 
        glDisable(GL_DEPTH_TEST); 
        break;
    case DepthTestType::Never:
    case DepthTestType::Always:
    case DepthTestType::Equal:
    case DepthTestType::NotEqual:
    case DepthTestType::Less:
    case DepthTestType::LessEqual:
    case DepthTestType::Greater:
    case DepthTestType::GreaterEqual:
        glEnable(GL_DEPTH_TEST); 
        glDepthFunc(common::enum_cast(type));
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

void oglContext_::SetDepthClip(const bool fix)
{
    CHECKCURRENT();
    if (fix)
    {
        DSA->ogluClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glClearDepth(0.0f);
    }
    else
    {
        DSA->ogluClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        glClearDepth(1.0f);
    }
}

void oglContext_::SetSRGBFBO(const bool isEnable)
{
    CHECKCURRENT();
    if (isEnable)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);
}

void oglContext_::ClearFBO()
{
    CHECKCURRENT();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void oglContext_::SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
{
    CHECKCURRENT();
    glViewport(x, y, (GLsizei)width, (GLsizei)height);
}

miniBLAS::VecI4 oglContext_::GetViewPort() const
{
    CHECKCURRENT();
    miniBLAS::VecI4 viewport;
    glGetIntegerv(GL_VIEWPORT, viewport);
    return viewport;
}


uint32_t oglContext_::GetLatestVersion()
{
    return LatestVersion;
}
oglContext oglContext_::CurrentContext()
{
    return InnerCurCtx;
}

struct EssentialInit
{
    EssentialInit()
    {
#if !defined(_WIN32)
        glxewInit(); // for some glx API
#endif
    }
};
oglContext oglContext_::Refresh()
{
    [[maybe_unused]]static EssentialInit EINIT;
    oglContext ctx;
#if defined(_WIN32)
    void *hrc = wglGetCurrentContext();
#else
    void *hrc = glXGetCurrentContext();
#endif
    if (hrc == nullptr)
    {
        oglLog().debug(u"currently no GLContext\n");
        return ctx;
    }
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
                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), wglGetCurrentDC(), hrc, true));
#else
                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), glXGetCurrentDisplay(), hrc, glXGetCurrentDrawable(), true));
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

#if !defined(_WIN32)
static int TmpXErrorHandler(Display* disp, XErrorEvent* evt)
{
    thread_local string txtBuf;
    txtBuf.resize(1024, '\0');
    XGetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
    txtBuf.resize(std::char_traits<char>::length(txtBuf.data()));
    oglLog().warning(u"X11 report an error with code[{}][{}]:\t{}\n", evt->error_code, evt->minor_code, 
        common::strchset::to_u16string(txtBuf, common::str::Charset::UTF8));
    return 0;
}
#endif
oglContext oglContext_::NewContext(const oglContext& ctx, const bool isShared, const int32_t *attribs)
{
    oglContext newCtx;

#if defined(_WIN32)
    const auto newHrc = wglCreateContextAttribsARB((HDC)ctx->Hdc, isShared ? (HGLRC)ctx->Hrc : nullptr, attribs);
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by HDC[{}] HRC[{}] ({}), error: {}\n", ctx->Hdc, ctx->Hrc, isShared ? u"shared" : u"", GetLastError());
        return {};
    }
    newCtx.reset(new oglContext_(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), ctx->Hdc, newHrc));
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
    newCtx.reset(new oglContext_(isShared ? ctx->SharedCore : std::make_shared<detail::SharedContextCore>(), ctx->Hdc, newHrc, ctx->DRW));
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
    if (GLEW_KHR_context_flush_control == GL_TRUE && supportFlushControl)
    {
        ctxAttrb.insert(ctxAttrb.end(), { FlushFlag, FlushVal }); // all the same
    }
    ctxAttrb.push_back(0);
    return NewContext(ctx, isShared, ctxAttrb.data());
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
        oglLog().warning(u"unretained HRC[{:p}] was requesr release.\n", hrc);
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