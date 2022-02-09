#include "oglPch.h"
#include "oglContext.h"
#include "oglUtil.h"


namespace oglu
{
using namespace std::string_view_literals;
using std::string;
using std::u16string;
using std::vector;

BindingState::BindingState()
{
    //HRC = PlatFuncs::GetCurrentGLContext();
    CtxFunc->GetIntegerv(GL_CURRENT_PROGRAM, &Prog);
    CtxFunc->GetIntegerv(GL_VERTEX_ARRAY_BINDING, &VAO);
    CtxFunc->GetIntegerv(GL_FRAMEBUFFER_BINDING, &FBO);
    CtxFunc->GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &DFB);
    CtxFunc->GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &RFB);
    CtxFunc->GetIntegerv(GL_ARRAY_BUFFER_BINDING, &VBO);
    if (CtxFunc->SupportIndirectDraw)
        CtxFunc->GetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &IBO);
    else
        IBO = 0;
    CtxFunc->GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &EBO);
    CtxFunc->GetIntegerv(GL_TEXTURE_BINDING_2D, &Tex2D);
    CtxFunc->GetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &Tex2DArray);
    CtxFunc->GetIntegerv(GL_TEXTURE_BINDING_3D, &Tex3D);
}

thread_local oglContext InnerCurCtx{ };

#if defined(_DEBUG)
#define CHECKCURRENT() if (!InnerCurCtx || InnerCurCtx.get() != this) COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"operate without UseContext");
#else
#define CHECKCURRENT()
#endif


static std::map<void*, std::weak_ptr<oglContext_>> CTX_MAP;
static std::map<void*, std::weak_ptr<oglContext_>> EXTERN_CTX_MAP;
static common::RWSpinLock CTX_LOCK;
static common::SpinLocker EXTERN_CTX_LOCK;


static MsgLevel ParseLevel(const GLenum lv) noexcept
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
static void HandleMsg(MsgSrc msgFrom, MsgType msgType, MsgLevel msgLevel, GLsizei length, const GLchar* message, const void* userParam)
{
    const oglContext_::DBGLimit& limit = *reinterpret_cast<const oglContext_::DBGLimit*>(userParam);
    if (((limit.src & msgFrom) != MsgSrc::Empty)
        && ((limit.type & msgType) != MsgType::Empty)
        && (uint8_t)limit.minLV <= (uint8_t)msgLevel)
    {
        DebugMessage msg(msgFrom, msgType, msgLevel);
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

static void GLAPIENTRY HandleMsgARB(GLenum source, GLenum type, [[maybe_unused]]GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_MARKER || type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP)
        return;
    if (source == GL_DEBUG_SOURCE_THIRD_PARTY && (id >= OGLUMsgIdMin && id <= OGLUMsgIdMax))
        return;
    const auto msgFrom  = static_cast<MsgSrc>(1 << (source - GL_DEBUG_SOURCE_API));
    const auto msgType  = static_cast<MsgType>(type <= GL_DEBUG_TYPE_OTHER ?
        1 << (type - GL_DEBUG_TYPE_ERROR) : 0x40 << (type - GL_DEBUG_TYPE_MARKER));
    const auto msgLevel = ParseLevel(severity);
    HandleMsg(msgFrom, msgType, msgLevel, length, message, userParam);
}

static void GLAPIENTRY HandleMsgAMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
{
    if (category == GL_DEBUG_CATEGORY_APPLICATION_AMD && (id >= OGLUMsgIdMin && id <= OGLUMsgIdMax))
        return;
    MsgSrc  msgFrom;
    MsgType msgType;
    switch (category)
    {
    case GL_DEBUG_CATEGORY_API_ERROR_AMD: 
        msgFrom = MsgSrc::OpenGL,       msgType = MsgType::Error;       break;
    case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD: 
        msgFrom = MsgSrc::WindowSystem, msgType = MsgType::Error;       break;
    case GL_DEBUG_CATEGORY_DEPRECATION_AMD: 
        msgFrom = MsgSrc::OpenGL,       msgType = MsgType::Deprecated;  break;
    case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD: 
        msgFrom = MsgSrc::OpenGL,       msgType = MsgType::UndefBehav;  break;
    case GL_DEBUG_CATEGORY_PERFORMANCE_AMD: 
        msgFrom = MsgSrc::OpenGL,       msgType = MsgType::Performance; break;
    case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD: 
        msgFrom = MsgSrc::Compiler,     msgType = MsgType::Other;       break;
    case GL_DEBUG_CATEGORY_APPLICATION_AMD: 
        msgFrom = MsgSrc::Application,  msgType = MsgType::Other;       break;
    case GL_DEBUG_CATEGORY_OTHER_AMD: 
    default:
        msgFrom = MsgSrc::Other,        msgType = MsgType::Other;       break;
    }
    const auto msgLevel = ParseLevel(severity);
    HandleMsg(msgFrom, msgType, msgLevel, length, message, userParam);
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

oglContext_::oglContext_(const std::shared_ptr<detail::SharedContextCore>& sharedCore, const std::shared_ptr<GLHost>& host, void *hrc, const CtxFuncs* ctxFunc) :
    Host(host), Hrc(hrc), SharedCore(sharedCore), Capability(ctxFunc), XCompDevice(ctxFunc->XCompDevice)
{
    ctxFunc->GetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&DepthTestFunc));
    if (ctxFunc->IsEnabled(GL_CULL_FACE))
    {
        GLint cullingMode;
        ctxFunc->GetIntegerv(GL_CULL_FACE_MODE, &cullingMode);
        if (cullingMode == GL_FRONT_AND_BACK)
            FaceCulling = FaceCullingType::CullAll;
        else
        {
            GLint frontFace = GL_CCW;
            ctxFunc->GetIntegerv(GL_FRONT_FACE, &frontFace);
            FaceCulling = ((cullingMode == GL_BACK) ^ (frontFace == GL_CW)) ? FaceCullingType::CullCW : FaceCullingType::CullCCW;
        }
    }
    else
        FaceCulling = FaceCullingType::OFF;
}

const common::container::FrozenDenseSet<std::string_view>& oglContext_::GetPlatformExtensions() const noexcept
{ 
    return Host->Extensions;
}
std::optional<std::array<std::byte, 8>> oglContext_::GetLUID() const noexcept
{
    if (XCompDevice) return XCompDevice->Luid;
    return CtxFunc->GetLUID();
}
std::optional<std::array<std::byte, 16>> oglContext_::GetUUID() const noexcept
{
    if (XCompDevice) return XCompDevice->Guid;
    return CtxFunc->GetUUID();
}


void oglContext_::FinishGL()
{
    CHECKCURRENT();
    CtxFunc->Finish();
}

void oglContext_::SwapBuffer()
{
    CHECKCURRENT();
    Host->SwapBuffer();
}

void oglContext_::ForceSync()
{
    CHECKCURRENT();
    CtxFunc->Finish();
}

oglContext_::~oglContext_()
{
#if defined(_DEBUG)
    //if (!IsExternal)
        oglLog().debug(u"Here destroy glContext [{}].\n", Hrc);
#endif
    ResHandler.Release();
    //if (!IsRetain)
    //if (!IsExternal)
    {
        Host->DeleteGLContext(Hrc);
    }
}

bool oglContext_::UseContext(const bool force)
{
    if (!force)
    {
        if (InnerCurCtx && InnerCurCtx.get() == this)
            return true;
    }
    if (!Host->MakeGLContextCurrent(Hrc))
    {
        Host->ReportFailure(fmt::format(FMT_STRING(u"use context[{}]"sv), Hrc));
        return false;
    }
    InnerCurCtx = shared_from_this();
    return true;
}

bool oglContext_::UnloadContext()
{
    if (InnerCurCtx && InnerCurCtx.get() == this)
    {
        InnerCurCtx.reset();
        if (!Host->MakeGLContextCurrent(nullptr))
        {
            Host->ReportFailure(fmt::format(FMT_STRING(u"unload context[{}]"sv), Hrc));
            return false;
        }
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

void oglContext_::SetDebug(MsgSrc src, MsgType type, MsgLevel minLV)
{
    CHECKCURRENT();
    DbgLimit = { type, src, minLV };
    if (CtxFunc->SupportDebug)
    {
        CtxFunc->Enable(GL_DEBUG_OUTPUT);
        CtxFunc->Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        if (CtxFunc->DebugMessageCallback)
            CtxFunc->DebugMessageCallback(HandleMsgARB, &DbgLimit);
        else if (CtxFunc->DebugMessageCallbackAMD)
            CtxFunc->DebugMessageCallbackAMD(HandleMsgAMD, &DbgLimit);
    }
}

void oglContext_::SetDepthTest(const DepthTestType type)
{
    CHECKCURRENT();
    switch (type)
    {
    case DepthTestType::OFF: 
        CtxFunc->Disable(GL_DEPTH_TEST);
        break;
    case DepthTestType::Never:
    case DepthTestType::Always:
    case DepthTestType::Equal:
    case DepthTestType::NotEqual:
    case DepthTestType::Less:
    case DepthTestType::LessEqual:
    case DepthTestType::Greater:
    case DepthTestType::GreaterEqual:
        CtxFunc->Enable(GL_DEPTH_TEST);
        CtxFunc->DepthFunc(common::enum_cast(type));
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
        CtxFunc->Disable(GL_CULL_FACE); break;
    case FaceCullingType::CullCW:
        CtxFunc->Enable(GL_CULL_FACE); CtxFunc->CullFace(GL_BACK); CtxFunc->FrontFace(GL_CCW); break;
    case FaceCullingType::CullCCW:
        CtxFunc->Enable(GL_CULL_FACE); CtxFunc->CullFace(GL_BACK); CtxFunc->FrontFace(GL_CW); break;
    case FaceCullingType::CullAll:
        CtxFunc->Enable(GL_CULL_FACE); CtxFunc->CullFace(GL_FRONT_AND_BACK); break;
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
            CtxFunc->ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            CtxFunc->ClearDepth(0.0f);
        }
        else
        {
            CtxFunc->ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            CtxFunc->ClearDepth(1.0f);
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
    if (CtxFunc->SupportSRGBFrameBuffer)
    {
        if (isEnable)
            CtxFunc->Enable(GL_FRAMEBUFFER_SRGB);
        else
            CtxFunc->Disable(GL_FRAMEBUFFER_SRGB);
    }
    else
    {
        oglLog().warning(u"sRGB FBO not supported on the context, ignored");
    }
}


//void oglContext_::SetViewPort(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
//{
//    CHECKCURRENT();
//    CtxFunc->Viewport(x, y, (GLsizei)width, (GLsizei)height);
//}

mbase::IVec4 oglContext_::GetViewPort() const
{
    CHECKCURRENT();
    mbase::IVec4 viewport;
    CtxFunc->GetIntegerv(GL_VIEWPORT, viewport.Ptr());
    return viewport;
}

void oglContext_::MemBarrier(const GLMemBarrier mbar)
{
    CHECKCURRENT();
    CtxFunc->MemoryBarrier(common::enum_cast(mbar));
}

std::shared_ptr<const xcomp::RangeHolder> oglContext_::BeginRange(std::u16string_view msg) const noexcept
{
    Expects(InnerCurCtx && InnerCurCtx.get() == this);
    CtxFunc->PushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, OGLUMsgIdMin, msg);
    return shared_from_this();
}
void oglContext_::EndRange() const noexcept
{
    Expects(InnerCurCtx && InnerCurCtx.get() == this);
    CtxFunc->PopDebugGroup();
}
void oglContext_::AddMarker(std::u16string_view name) const noexcept
{
    Expects(InnerCurCtx && InnerCurCtx.get() == this);
    CtxFunc->InsertDebugMarker(OGLUMsgIdMin, name);
}

oglContext oglContext_::CurrentContext()
{
    return InnerCurCtx;
}



//oglContext oglContext_::Refresh()
//{
//    void* hrc = PlatFuncs::GetCurrentGLContext();
//    if (hrc == nullptr)
//    {
//        oglLog().debug(u"currently no GLContext\n");
//        return {};
//    }
//    Expects(PlatFunc);
//    oglContext ctx;
//    {
//        auto lock = CTX_LOCK.ReadScope();
//        ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock();
//    }
//    if (ctx)
//    {
//        InnerCurCtx = ctx;
//        return ctx;
//    }
//    else
//    {
//        {
//            auto lock = CTX_LOCK.WriteScope();
//            ctx = common::container::FindInMapOrDefault(CTX_MAP, hrc).lock(); // second check
//            if (!ctx) // need to create the wrapper
//            {
//#if defined(_WIN32)
//                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), 
//                    PlatFunc, hrc, true));
//#else
//                ctx = oglContext(new oglContext_(std::make_shared<detail::SharedContextCore>(), 
//                    PlatFunc, hrc, PlatFuncs::GetCurrentDrawable(), true));
//#endif
//                CTX_MAP.emplace(hrc, ctx);
//            }
//        }
//        InnerCurCtx = ctx;
//        ctx->Init(true);
//#if defined(_DEBUG) || 1
//        ctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
//#endif
//        {
//            const auto lock = EXTERN_CTX_LOCK.LockScope();
//            EXTERN_CTX_MAP.emplace(hrc, ctx);
//        }
//        return ctx;
//    }
//}


void oglContext_::PushToMap(oglContext ctx)
{
    auto lock = CTX_LOCK.WriteScope();
    CTX_MAP.emplace(ctx->Hrc, std::move(ctx));
}
oglContext oglContext_::NewContext(const bool isShared, uint32_t version) const
{
    CHECKCURRENT();
    CreateInfo cinfo;
    cinfo.Version = version;
    return Host->CreateContext(cinfo, isShared ? this : nullptr);
}

//bool oglContext_::ReleaseExternContext()
//{
//    return ReleaseExternContext(PlatFuncs::GetCurrentGLContext());
//}
//bool oglContext_::ReleaseExternContext(void* hrc)
//{
//    size_t dels = 0;
//    {
//        const auto lock = EXTERN_CTX_LOCK.LockScope();
//        dels = EXTERN_CTX_MAP.erase(hrc);
//    }
//    if (dels > 0)
//    {
//        auto lock2 = CTX_LOCK.WriteScope();
//        CTX_MAP.erase(hrc);
//        return true;
//    }
//    else
//    {
//        oglLog().warning(u"unretained HRC[{:p}] was request to release.\n", hrc);
//        return false;
//    }
//}


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

}