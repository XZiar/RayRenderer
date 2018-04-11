#include "oglRely.h"
#include "oglContext.h"
#include <atomic>
#include <GL/wglew.h>


namespace oglu
{

BindingState::BindingState()
{
    glGetIntegerv(GL_CURRENT_PROGRAM, &progId);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoId);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboId);
    glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &iboId);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboId);
}

namespace detail
{


static void GLAPIENTRY onMsg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    DebugMessage msg(source, type, severity);
    const _oglContext::DBGLimit& limit = *(_oglContext::DBGLimit*)userParam;
    if (((limit.src & msg.from) != MsgSrc::Empty)
        && ((limit.type & msg.type) != MsgType::Empty)
        && (uint8_t)limit.minLV <= (uint8_t)msg.level)
    {
        auto theMsg = std::make_shared<DebugMessage>(msg);
        theMsg->msg.assign(message, message + length);

        if (theMsg->type == MsgType::Error)
        {
            oglLog().error(u"OpenGL ERROR\n{}\n", theMsg->msg);
            BindingState state;
            oglLog().debug(u"Current Prog[{}] binding-state: VAO[{}], VBO[{}], IBO[{}], EBO[{}]\n", state.progId, state.vaoId, state.vboId, state.iboId, state.eboId);
        }
        else
        {
            oglLog().verbose(u"OpenGL message\n{}\n", theMsg->msg);
        }
    }
}

_oglContext::_oglContext(const uint32_t uid, void *hdc, void *hrc) : Hdc(hdc), Hrc(hrc), Uid(uid)
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
    wglDeleteContext((HGLRC)Hrc);
}

bool _oglContext::UseContext()
{
    if (!wglMakeCurrent((HDC)Hdc, (HGLRC)Hrc))
    {
        oglLog().error(u"Failed to use HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, GetLastError());
        return false;
    }
    else
    {
        oglContext::CurrentCtx() = this->shared_from_this();
        return true;
    }
}

bool _oglContext::UnloadContext()
{
    if (!wglMakeCurrent((HDC)Hdc, nullptr))
    {
        oglLog().error(u"Failed to unload HDC[{}] HRC[{}], error: {}\n", Hdc, Hrc, GetLastError());
        return false;
    }
    else
    {
        oglContext::CurrentCtx() = nullptr;
        return true;
    }
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
    thread_local oglContext ctx = nullptr;
    return ctx;
}
uint32_t oglContext::CurrentCtxUid()
{
    const auto& ctx = CurrentCtx();
    return ctx ? ctx->Uid : 0;
}

oglContext oglContext::CurrentContext()
{
    void *hrc = wglGetCurrentContext();
    CTX_LOCK.LockRead();
    if (auto ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc))
    {
        CTX_LOCK.UnlockRead();
        CurrentCtx() = ctx;
        return ctx;
    }
    else
    {
        CTX_LOCK.UnlockRead();
        CTX_LOCK.LockWrite();
        if (ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc))
        { }
        else
        {
            ctx = oglContext(new detail::_oglContext(CTX_UID++, wglGetCurrentDC(), hrc));
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

oglContext oglContext::NewContext(const oglContext& ctx, const bool isShared, int *attribs)
{
    static int ctxAttrb[] =
    {
        /*WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,*/
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        0
    };
    int *attrs = attribs ? attribs : ctxAttrb;
    auto newHrc = wglCreateContextAttribsARB((HDC)ctx->Hdc, isShared ? (HGLRC)ctx->Hrc : nullptr, ctxAttrb);
    return oglContext(new detail::_oglContext(isShared ? ctx->Uid : CTX_UID++, ctx->Hdc, newHrc));
}


}