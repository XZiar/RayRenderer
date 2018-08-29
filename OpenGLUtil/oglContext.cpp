#include "oglRely.h"
#include "oglContext.h"
#include "oglUtil.h"
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include "glew/wglew.h"
#else
#   define GLEW_NO_GLU
#   include "glew/glxew.h"
//fucking X11 defines some terrible macro
#   undef Always
#endif

namespace oglu
{

BindingState::BindingState()
{
    glGetIntegerv(GL_CURRENT_PROGRAM, &progId);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoId);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboId);
    glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &iboId);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboId);
}

namespace detail
{


static void GLAPIENTRY onMsg(GLenum source, GLenum type, [[maybe_unused]]GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    DebugMessage msg(source, type, severity);
    const _oglContext::DBGLimit& limit = *(_oglContext::DBGLimit*)userParam;
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

#if defined(_WIN32)
_oglContext::_oglContext(const uint32_t uid, void *hdc, void *hrc) : Hdc(hdc), Hrc(hrc), Uid(uid)
#else
_oglContext::_oglContext(const uint32_t uid, void *hdc, void *hrc, unsigned long drw) : Hdc(hdc), Hrc(hrc), DRW(drw), Uid(uid)
#endif
{
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
}

_oglContext::~_oglContext()
{
#if defined(_WIN32)
    wglDeleteContext((HGLRC)Hrc);
#else
    glXDestroyContext((Display*)Hdc, (GLXContext)Hrc);
#endif
}

bool _oglContext::UseContext()
{
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
    oglContext::CurrentCtx() = this->shared_from_this();
    return true;
}

bool _oglContext::UnloadContext()
{
    if (&*oglContext::CurrentCtx() == this)
    {
        oglContext::CurrentCtx().release();
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

void _oglContext::SetDebug(MsgSrc src, MsgType type, MsgLevel minLV)
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    DbgLimit = { type, src, minLV };
    glDebugMessageCallback(onMsg, &DbgLimit);
}

void _oglContext::SetDepthTest(const DepthTestType type)
{
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

bool _oglContext::SetFBO(const oglFBO& fbo)
{
    if (FrameBuffer != fbo)
    {
        if (fbo)
            glBindFramebuffer(GL_FRAMEBUFFER, fbo->FBOId);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        FrameBuffer = fbo;
        return true;
    }
    return false;
}
void _oglContext::SetSRGBFBO(const bool isEnable)
{
    if (isEnable)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);
}

void _oglContext::ClearFBO()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void _oglContext::SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
{
    glViewport(x, y, (GLsizei)width, (GLsizei)height);
}

miniBLAS::VecI4 _oglContext::GetViewPort() const
{
    miniBLAS::VecI4 viewport;
    glGetIntegerv(GL_VIEWPORT, viewport);
    return viewport;
}


}


static std::map<void*, oglContext> CTX_MAP;
static common::RWSpinLock CTX_LOCK;
static std::atomic_uint32_t CTX_UID = 1;

void* oglContext::CurrentHRC()
{
    const auto& ctx = CurrentCtx();
    return ctx ? ctx->Hrc : nullptr;
}
oglContext& oglContext::CurrentCtx()
{
    thread_local oglContext ctx { };
    return ctx;
}
uint32_t oglContext::CurrentCtxUid()
{
    const auto& ctx = CurrentCtx();
    return ctx ? ctx->Uid : 0;
}

static std::pair<uint8_t, uint8_t>& LatestVersion()
{
    static std::pair<uint8_t, uint8_t> version { uint8_t(0),uint8_t(0) };
    return version;
}
bool oglContext::RefreshVersion()
{
    int32_t major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);   
    auto& curVer = LatestVersion();
    if (major * 10 + minor > curVer.first * 10 + curVer.second)
    {
        curVer.first = (uint8_t)major, curVer.second = (uint8_t)minor;
        oglLog().info(u"update API Version to [{}.{}]\n", curVer.first, curVer.second);
        return true;
    }
    return false;
}
std::pair<uint8_t, uint8_t> oglContext::GetLatestVersion()
{
    return LatestVersion();
}
oglContext oglContext::CurrentContext()
{
#if defined(_WIN32)
    void *hrc = wglGetCurrentContext();
#else
    void *hrc = glXGetCurrentContext();
#endif
    CTX_LOCK.LockRead();
    oglContext ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc);
    if (ctx)
    {
        CTX_LOCK.UnlockRead();
        CurrentCtx() = ctx;
        return ctx;
    }
    else
    {
        CTX_LOCK.UnlockRead();
        CTX_LOCK.LockWrite();
        ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc); // second check
        if (!ctx) // need to create the wrapper
        {
#if defined(_WIN32)
            ctx = oglContext(new detail::_oglContext(CTX_UID++, wglGetCurrentDC(), hrc));
#else
            ctx = oglContext(new detail::_oglContext(CTX_UID++, glXGetCurrentDisplay(), hrc, glXGetCurrentDrawable()));
#endif
            CTX_MAP.emplace(hrc, ctx);
        }
        CurrentCtx() = ctx;
        CTX_LOCK.UnlockWrite();
        return ctx;
    }
}

void oglContext::Refresh()
{
    CurrentContext();
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
    newCtx.reset(new detail::_oglContext(isShared ? ctx->Uid : CTX_UID++, ctx->Hdc, newHrc));
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
        None
    };
    int num_fbc = 0;
    GLXFBConfig *fbc = glXChooseFBConfig((Display*)ctx->Hdc, DefaultScreen((Display*)ctx->Hdc), visual_attribs, &num_fbc);
    const auto oldHandler = XSetErrorHandler(&TmpXErrorHandler);
    const auto newHrc = glXCreateContextAttribsARB((Display*)ctx->Hdc, fbc[0], isShared ? (GLXContext)ctx->Hrc : nullptr, true, attribs);
    XSetErrorHandler(oldHandler);
    if (!newHrc)
    {
        oglLog().error(u"failed to create context by Display[{}] Drawable[{}] HRC[{}] ({}), error: {}\n", ctx->Hdc, ctx->DRW, ctx->Hrc, isShared ? u"shared" : u"", errno);
        return {};
    }
    newCtx.reset(new detail::_oglContext(isShared ? ctx->Uid : CTX_UID++, ctx->Hdc, newHrc, ctx->DRW));
#endif
    newCtx->UseContext();
    if (RefreshVersion())
        oglUtil::init();
    ctx->UseContext();
    return newCtx;
}
oglContext oglContext::NewContext(const oglContext& ctx, const bool isShared, std::pair<uint8_t, uint8_t> version)
{
    if (version.first == 0) version.first = LatestVersion().first;
    if (version.second == 0) version.second = LatestVersion().second;
#if defined(_WIN32)
    vector<int32_t> ctxAttrb
    {
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB
    };
    constexpr int32_t verMajor = WGL_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t verMinor = WGL_CONTEXT_MINOR_VERSION_ARB;
#else
    vector<int32_t> ctxAttrb
    {
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB
    };
    constexpr int32_t verMajor = GLX_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t verMinor = GLX_CONTEXT_MINOR_VERSION_ARB;
#endif
    if (version.first != 0 && version.second != 0)
    {
        ctxAttrb.insert(ctxAttrb.end(),
        {
            verMajor, version.first,
            verMinor, version.second,
        });
    }
    ctxAttrb.push_back(0);
    return NewContext(ctx, isShared, ctxAttrb.data());
}


}