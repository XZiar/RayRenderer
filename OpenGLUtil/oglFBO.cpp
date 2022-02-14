#include "oglPch.h"
#include "oglFBO.h"
#include "oglContext.h"


namespace oglu
{
MAKE_ENABLER_IMPL(oglRenderBuffer_)
MAKE_ENABLER_IMPL(oglDefaultFrameBuffer_)
MAKE_ENABLER_IMPL(oglFrameBuffer2D_)
MAKE_ENABLER_IMPL(oglLayeredFrameBuffer_)


static GLenum ParseRBOFormat(const RBOFormat format)
{
    switch (format)
    {
    case RBOFormat::Depth:              return GL_DEPTH_COMPONENT;
    case RBOFormat::Depth16:            return GL_DEPTH_COMPONENT16;
    case RBOFormat::Depth24:            return GL_DEPTH_COMPONENT24;
    case RBOFormat::Depth32:            return GL_DEPTH_COMPONENT32;
    case RBOFormat::Stencil:            return GL_STENCIL_INDEX;
    case RBOFormat::Stencil1:           return GL_STENCIL_INDEX1;
    case RBOFormat::Stencil8:           return GL_STENCIL_INDEX8;
    case RBOFormat::Stencil16:          return GL_STENCIL_INDEX16;
    case RBOFormat::Depth24Stencil8:    return GL_DEPTH24_STENCIL8;
    case RBOFormat::Depth32Stencil8:    return GL_DEPTH32F_STENCIL8;
    case RBOFormat::RGBA8:              return GL_RGBA;
    case RBOFormat::RGBA8U:             return GL_RGBA8UI;
    case RBOFormat::RGBAf:              return GL_RGBA32F;
    case RBOFormat::RGB10A2:            return GL_RGB10_A2;
    default: COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Unknown RBOFormat.");
    }
}


oglRenderBuffer_::oglRenderBuffer_(const uint32_t width, const uint32_t height, const RBOFormat format)
    : RBOId(GL_INVALID_INDEX), InnerFormat(format), Width(width), Height(height)
{
    CtxFunc->CreateRenderbuffers(1, &RBOId);
    CtxFunc->NamedRenderbufferStorage(RBOId, ParseRBOFormat(InnerFormat), Width, Height);
}
oglRBO oglRenderBuffer_::Create(const uint32_t width, const uint32_t height, const RBOFormat format)
{
    return MAKE_ENABLER_SHARED(oglRenderBuffer_, (width, height, format));
}

oglRenderBuffer_::~oglRenderBuffer_()
{
    if (!EnsureValid()) return;
    CtxFunc->DeleteRenderbuffers(1, &RBOId);
}

void oglRenderBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->SetObjectLabel(GL_RENDERBUFFER, RBOId, Name);
}


struct FBOCtxInfo
{
    oglDefaultFBO DefaultFBO;
    oglFBO CurrentFBO;
    common::RWSpinLock Lock;
    FBOCtxInfo()
    {
        DefaultFBO = MAKE_ENABLER_SHARED(oglDefaultFrameBuffer_, ());
        CurrentFBO = DefaultFBO;
    }
    void Use(oglFrameBuffer_* fbo)
    {
        const auto lock = Lock.WriteScope();
        CurrentFBO = fbo->shared_from_this();
    }
};
struct FBOCtxConfig : public CtxResConfig<false, std::unique_ptr<FBOCtxInfo>>
{
    std::unique_ptr<FBOCtxInfo> Construct() const
    {
        return std::make_unique<FBOCtxInfo>();
    }
};
static FBOCtxConfig FBO_CTXCFG;


common::RWSpinLock::ReadScopeType oglFrameBuffer_::FBOClear::AcquireLock(oglFrameBuffer_& fbo)
{
    fbo.Use();
    return oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG)->Lock.ReadScope();
}
oglFrameBuffer_::FBOClear::FBOClear(oglFrameBuffer_* fbo) : 
    NewFBO(*fbo), OldFBOId(CtxFunc->DrawFBO), Lock(AcquireLock(NewFBO))
{ }
oglFrameBuffer_::FBOClear::~FBOClear()
{
    CtxFunc->BindFramebuffer(GL_DRAW_FRAMEBUFFER, OldFBOId);
}

oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearColors(const mbase::Vec4& color)
{
    CtxFunc->ClearColor(color.X, color.Y, color.Z, color.W);
    CtxFunc->Clear(GL_COLOR_BUFFER_BIT);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearDepth(const float depth)
{
    CtxFunc->ClearNamedFramebufferfv(NewFBOId(), GL_DEPTH, 0, &depth);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearStencil(const GLint stencil)
{
    CtxFunc->ClearNamedFramebufferiv(NewFBOId(), GL_STENCIL, 0, &stencil);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearDepthStencil(const float depth, const GLint stencil)
{
    CtxFunc->ClearNamedFramebufferDepthStencil(NewFBOId(), depth, stencil);
    return *this;
}


oglFrameBuffer_::oglFrameBuffer_(const GLuint id) : FBOId(id), Width(0), Height(0)
{
    DrawBindings.resize(CtxFunc->MaxDrawBuffers);
}
oglFrameBuffer_::~oglFrameBuffer_()
{ }

std::pair<oglFBO, common::RWSpinLock::ReadScopeType> oglFrameBuffer_::LockCurFBOLock()
{
    auto& info = oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG);
    return { info->CurrentFBO, info->Lock.ReadScope() };
}

GLenum oglFrameBuffer_::CheckIfBinded() const
{
    if (CtxFunc->DrawFBO == FBOId) return GL_DRAW_FRAMEBUFFER;
    if (CtxFunc->ReadFBO == FBOId) return GL_READ_FRAMEBUFFER;
    return GLInvalidEnum;
}

bool oglFrameBuffer_::CheckIsSrgb(const GLenum attachment) const
{
    GLint isSrgb = GL_LINEAR;
    CtxFunc->GetNamedFramebufferAttachmentParameteriv(FBOId, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &isSrgb);
    return isSrgb == GL_SRGB;
}

bool oglFrameBuffer_::SetViewPort(const uint32_t width, const uint32_t height)
{
    if (width == Width && height == Height)
        return false;
    Width = width, Height = height;
    if (CtxFunc->DrawFBO == FBOId)
    {
        CtxFunc->Viewport(0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height));
        return false;
    }
    return true;
}

void oglFrameBuffer_::BindDraws(const common::span<GLenum> bindings)
{
    Expects(bindings.size() < static_cast<size_t>(CtxFunc->MaxDrawBuffers)); // cannot bind such many buffers at the same time
    const auto nochange = common::linq::FromIterable(DrawBindings)
        .Pair(common::linq::FromIterable(bindings))
        .AllIf([](const auto& pair) { return pair.first == pair.second; });
    if (!nochange)
    {
        CtxFunc->DrawBuffers((GLsizei)bindings.size(), bindings.data());
        DrawBindings.assign(bindings.data(), bindings.data() + bindings.size());
    }
}

FBOStatus oglFrameBuffer_::CheckStatus() const
{
    CheckCurrent();
    switch (CtxFunc->CheckNamedFramebufferStatus(FBOId, GL_DRAW_FRAMEBUFFER))
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return FBOStatus::Complete;
    case GL_FRAMEBUFFER_UNDEFINED:
        return FBOStatus::Undefined;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return FBOStatus::Unsupported;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return FBOStatus::IncompleteAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return FBOStatus::IncompleteDrawBuffer;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return FBOStatus::MissingAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return FBOStatus::IncompleteReadBuffer;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return FBOStatus::IncompleteMultiSample;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return FBOStatus::IncompleteLayerTargets;
    default:
        return FBOStatus::Unknown;
    }
}

void oglFrameBuffer_::Use()
{
    CheckCurrent();
    CtxFunc->BindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOId);
    CtxFunc->Viewport(0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height));
    oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG)->Use(this);
}

oglFrameBuffer_::FBOClear oglFrameBuffer_::Clear()
{
    return FBOClear(this);
}

void oglFrameBuffer_::ClearAll()
{
    const auto oldFBOId = CtxFunc->DrawFBO;
    Use();
    CtxFunc->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CtxFunc->BindFramebuffer(GL_DRAW_FRAMEBUFFER, oldFBOId);
}


static void BlitColor(const GLuint from, const GLuint to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    const auto& [x0, y0, x1, y1] = rect;
    CtxFunc->BlitNamedFramebuffer(from, to, x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}
void oglFrameBuffer_::BlitColorTo(const oglFBO& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    to->CheckCurrent();
    BlitColor(FBOId, to->FBOId, rect);
}
void oglFrameBuffer_::BlitColorFrom(const oglFBO& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    from->CheckCurrent();
    BlitColor(from->FBOId, FBOId, rect);
}


oglDefaultFrameBuffer_::FBOClear& oglDefaultFrameBuffer_::FBOClear::DiscardColors()
{
    const GLenum slot = GL_COLOR;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglDefaultFrameBuffer_::FBOClear& oglDefaultFrameBuffer_::FBOClear::DiscardDepth()
{
    const GLenum slot = GL_DEPTH;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglDefaultFrameBuffer_::FBOClear& oglDefaultFrameBuffer_::FBOClear::DiscardStencil()
{
    const GLenum slot = GL_STENCIL;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglDefaultFrameBuffer_::FBOClear& oglDefaultFrameBuffer_::FBOClear::DiscardDepthStencil()
{
    const GLenum slot[] = { GL_DEPTH, GL_STENCIL };
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 2, slot);
    return *this;
}


oglDefaultFrameBuffer_::oglDefaultFrameBuffer_() : oglFrameBuffer_(0)
{
    DrawBindings[0] = GL_BACK;
    const auto viewport = oglContext_::CurrentContext()->GetViewPort(); 
    Width = viewport.Z; Height = viewport.W;
    IsSrgbColor = CheckIsSrgb(GL_BACK);
}
oglDefaultFrameBuffer_::~oglDefaultFrameBuffer_()
{ }

bool oglDefaultFrameBuffer_::IsSrgb() const
{
    return IsSrgbColor;
}

GLenum oglDefaultFrameBuffer_::GetAttachPoint(const std::string_view name) const
{
    switch (common::DJBHash::HashC(name))
    {
    case "default"_hash:
    case "back"_hash:
        return GL_BACK;
    case "left"_hash:
        return GL_BACK_LEFT;
    case "right"_hash:
        return GL_BACK_RIGHT;
    default:
        return GL_NONE;
    }
}


void oglDefaultFrameBuffer_::SetWindowSize(const uint32_t width, const uint32_t height)
{
    SetViewPort(width, height);
}

oglDefaultFrameBuffer_::FBOClear oglDefaultFrameBuffer_::Clear()
{
    return FBOClear(this);
}

oglDefaultFBO oglDefaultFrameBuffer_::Get()
{
    return oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG)->DefaultFBO;
}


oglCustomFrameBuffer_::FBOClear::FBOClear(oglCustomFrameBuffer_* fbo) : 
    oglFrameBuffer_::FBOClear(fbo), TheFBO(*fbo)
{
    ColorClears.resize(fbo->ColorAttachemnts.size());
}
oglCustomFrameBuffer_::FBOClear::~FBOClear()
{
    std::vector<GLenum> targets;
    GLenum att = GL_COLOR_ATTACHMENT0;
    for (const auto& clr : ColorClears)
    {
        if (clr.has_value())
            targets.push_back(att);
        att++;
    }
    BindDraws(targets);
    //CtxFunc->DrawBuffers((GLsizei)targets.size(), targets.data());
    uint8_t idx = 0;
    for (const auto& clr : ColorClears)
    {
        if (clr.has_value())
            CtxFunc->ClearNamedFramebufferfv(NewFBOId(), GL_COLOR, GL_DRAW_BUFFER0 + (idx++), &clr->X);
    }
}
oglCustomFrameBuffer_::FBOClear& oglCustomFrameBuffer_::FBOClear::ClearColor(const uint8_t attachment, const mbase::Vec4& color)
{
    Expects(attachment < TheFBO.ColorAttachemnts.size()); // u"attachment index overflow"
    ColorClears[attachment] = color;
    return *this;
}
oglCustomFrameBuffer_::FBOClear& oglCustomFrameBuffer_::FBOClear::DiscardColor(const uint8_t attachment)
{
    Expects(attachment < TheFBO.ColorAttachemnts.size()); // u"attachment index overflow"
    const GLenum slot = GL_COLOR_ATTACHMENT0 + attachment;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglCustomFrameBuffer_::FBOClear& oglCustomFrameBuffer_::FBOClear::DiscardDepth()
{
    const GLenum slot = GL_DEPTH_ATTACHMENT;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglCustomFrameBuffer_::FBOClear& oglCustomFrameBuffer_::FBOClear::DiscardStencil()
{
    const GLenum slot = GL_STENCIL_ATTACHMENT;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}
oglCustomFrameBuffer_::FBOClear& oglCustomFrameBuffer_::FBOClear::DiscardDepthStencil()
{
    const GLenum slot = GL_DEPTH_STENCIL_ATTACHMENT;
    CtxFunc->InvalidateNamedFramebufferData(NewFBOId(), 1, &slot);
    return *this;
}


oglCustomFrameBuffer_::oglCustomFrameBuffer_() : oglFrameBuffer_(GL_INVALID_INDEX)
{
    CtxFunc->CreateFramebuffers(1, &FBOId);
    ColorAttachemnts.resize(CtxFunc->MaxColorAttachment);
    ColorAttachNames.resize(CtxFunc->MaxColorAttachment);
    DrawBindings[0] = GL_COLOR_ATTACHMENT0;
}
oglCustomFrameBuffer_::~oglCustomFrameBuffer_()
{
    if (!EnsureValid()) return;
    CtxFunc->DeleteFramebuffers(1, &FBOId);
}

bool oglCustomFrameBuffer_::IsSrgb(const uint8_t attachment) const
{
    Expects(attachment < ColorAttachemnts.size()); // u"attachment index overflow"
    Expects(ColorAttachemnts[attachment].index() != 0); // u"attachment should not be empty"
    return CheckIsSrgb(GL_COLOR_ATTACHMENT0 + attachment);
}

GLenum oglCustomFrameBuffer_::GetAttachPoint(const std::string_view name) const
{
    GLenum ret = GL_COLOR_ATTACHMENT0;
    for (const auto& tarName : ColorAttachNames)
    {
        if (name == tarName)
            return ret;
        ret++;
    }
    return GL_NONE;
}

void oglCustomFrameBuffer_::CheckAttachmentMatch(const uint32_t width, const uint32_t height)
{
    if (Width != 0 && Width != width)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's width  not match");
    if (Height != 0 && Height != height)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's height not match");
    SetViewPort(width, height);
}

void oglCustomFrameBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->SetObjectLabel(GL_FRAMEBUFFER, FBOId, Name);
}

oglCustomFrameBuffer_::FBOClear oglCustomFrameBuffer_::Clear()
{
    return oglCustomFrameBuffer_::FBOClear(this);
}

std::pair<GLuint, GLuint> oglCustomFrameBuffer_::DebugBinding() const
{
    return oglCustomFrameBuffer_::DebugBinding(FBOId);
}

std::pair<GLuint, GLuint> oglCustomFrameBuffer_::DebugBinding(GLuint id)
{
    GLint clrId = 0, depId = 0;
    CtxFunc->GetNamedFramebufferAttachmentParameteriv(id, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &clrId);
    CtxFunc->GetNamedFramebufferAttachmentParameteriv(id, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depId);
    return std::pair<GLuint, GLuint>(clrId, depId);
}


oglFrameBuffer2D_::oglFrameBuffer2D_() : oglCustomFrameBuffer_()
{ }
oglFrameBuffer2D_::~oglFrameBuffer2D_()
{ }

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment, const std::string_view name)
{
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, GetID(tex), 0);
    oglCustomFrameBuffer_::AttachColorTexture(attachment, name, tex);
}

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment, const std::string_view name)
{
    Expects(layer < tex->Layers); // u"texture layer overflow"
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTextureLayer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0, layer);
    oglCustomFrameBuffer_::AttachColorTexture(attachment, name, std::make_pair(tex, layer));
}

void oglFrameBuffer2D_::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment, const std::string_view name)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_COLOR); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo);
    CtxFunc->NamedFramebufferRenderbuffer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, GetID(rbo));
    oglCustomFrameBuffer_::AttachColorTexture(attachment, name, rbo);
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo);
    CtxFunc->NamedFramebufferRenderbuffer(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo);
    CtxFunc->NamedFramebufferRenderbuffer(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    StencilAttachment = rbo;
}

void oglFrameBuffer2D_::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo);
    CtxFunc->NamedFramebufferRenderbuffer(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo; StencilAttachment = rbo;
}

oglFBO2D oglFrameBuffer2D_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer2D_, ());
}


oglLayeredFrameBuffer_::oglLayeredFrameBuffer_() : oglCustomFrameBuffer_()
{ }
oglLayeredFrameBuffer_::~oglLayeredFrameBuffer_()
{ }

void oglLayeredFrameBuffer_::CheckLayerMatch(const uint32_t layer)
{
    if (LayerCount != 0 && LayerCount != layer)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Layered Framebuffer's layer not match");
    LayerCount = layer;
}

void oglLayeredFrameBuffer_::AttachColorTexture(const oglTex3D& tex, const uint8_t attachment, const std::string_view name)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_3D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
    oglCustomFrameBuffer_::AttachColorTexture(attachment, name, tex);
}

void oglLayeredFrameBuffer_::AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment, const std::string_view name)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
    oglCustomFrameBuffer_::AttachColorTexture(attachment, name, tex);
}

void oglLayeredFrameBuffer_::AttachDepthTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachDepthTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachStencilTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachStencilTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex);
    CtxFunc->NamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    StencilAttachment = tex;
}

oglFBOLayered oglLayeredFrameBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglLayeredFrameBuffer_, ());
}




}