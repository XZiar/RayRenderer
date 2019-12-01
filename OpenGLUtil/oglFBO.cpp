#include "oglPch.h"
#include "oglFBO.h"
#include "oglContext.h"
#include "oglException.h"


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
    CtxFunc->ogluCreateRenderbuffers(1, &RBOId);
    CtxFunc->ogluNamedRenderbufferStorage(RBOId, ParseRBOFormat(InnerFormat), Width, Height);
}
oglRBO oglRenderBuffer_::Create(const uint32_t width, const uint32_t height, const RBOFormat format)
{
    return MAKE_ENABLER_SHARED(oglRenderBuffer_, (width, height, format));
}

oglRenderBuffer_::~oglRenderBuffer_()
{
    if (!EnsureValid()) return;
    CtxFunc->ogluDeleteRenderbuffers(1, &RBOId);
}

void oglRenderBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->ogluSetObjectLabel(GL_RENDERBUFFER, RBOId, Name);
}


oglFrameBuffer_::FBOClear::FBOClear(oglFrameBuffer_* fbo) : NewFBO(*fbo), OldFBOId(CtxFunc->DrawFBO)
{
    NewFBO.Use();
}

oglFrameBuffer_::FBOClear::~FBOClear()
{
    CtxFunc->ogluBindFramebuffer(GL_DRAW_FRAMEBUFFER, OldFBOId);
}

oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearColors(const b3d::Vec4& color)
{
    CtxFunc->ogluClearColor(color.x, color.y, color.z, color.w);
    CtxFunc->ogluClear(GL_COLOR_BUFFER_BIT);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearDepth(const float depth)
{
    CtxFunc->ogluClearNamedFramebufferfv(NewFBO.FBOId, GL_DEPTH, 0, &depth);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearStencil(const GLint stencil)
{
    CtxFunc->ogluClearNamedFramebufferiv(NewFBO.FBOId, GL_STENCIL, 0, &stencil);
    return *this;
}
oglFrameBuffer_::FBOClear& oglFrameBuffer_::FBOClear::ClearDepthStencil(const float depth, const GLint stencil)
{
    CtxFunc->ogluClearNamedFramebufferDepthStencil(NewFBO.FBOId, depth, stencil);
    return *this;
}


oglFrameBuffer_::oglFrameBuffer_(const GLuint id) : FBOId(id), Width(0), Height(0)
{ }
oglFrameBuffer_::~oglFrameBuffer_()
{ }

GLenum oglFrameBuffer_::CheckIfBinded() const
{
    if (CtxFunc->DrawFBO == FBOId) return GL_DRAW_FRAMEBUFFER;
    if (CtxFunc->ReadFBO == FBOId) return GL_READ_FRAMEBUFFER;
    return GLInvalidEnum;
}

bool oglFrameBuffer_::SetViewPort(const uint32_t width, const uint32_t height)
{
    if (width == Width && height == Height)
        return false;
    Width = width, Height = height;
    if (CtxFunc->DrawFBO == FBOId)
    {
        CtxFunc->ogluViewport(0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height));
        return false;
    }
    return true;
}

FBOStatus oglFrameBuffer_::CheckStatus() const
{
    CheckCurrent();
    switch (CtxFunc->ogluCheckNamedFramebufferStatus(FBOId, GL_DRAW_FRAMEBUFFER))
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
    CtxFunc->ogluBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOId);
    CtxFunc->ogluViewport(0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height));
}


void oglFrameBuffer_::DiscardColor(const uint8_t attachment)
{
    const GLenum slot = GL_COLOR_ATTACHMENT0 + attachment;
    CtxFunc->ogluInvalidateNamedFramebufferData(FBOId, 1, &slot);
}
void oglFrameBuffer_::DiscardDepth()
{
    const GLenum slot = GL_DEPTH_ATTACHMENT;
    CtxFunc->ogluInvalidateNamedFramebufferData(FBOId, 1, &slot);
}
void oglFrameBuffer_::DiscardStencil()
{
    const GLenum slot = GL_STENCIL_ATTACHMENT;
    CtxFunc->ogluInvalidateNamedFramebufferData(FBOId, 1, &slot);
}

oglFrameBuffer_::FBOClear oglFrameBuffer_::Clear()
{
    return FBOClear(this);
}

void oglFrameBuffer_::ClearAll()
{
    FBOClear(this).ClearAll();
}


static void BlitColor(const GLuint from, const GLuint to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    const auto& [x0, y0, x1, y1] = rect;
    CtxFunc->ogluBlitNamedFramebuffer(from, to, x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
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


oglDefaultFrameBuffer_::oglDefaultFrameBuffer_() : oglFrameBuffer_(0)
{
    const auto viewport = oglContext_::CurrentContext()->GetViewPort(); 
    Width = viewport.z; Height = viewport.w;
    GLint isSrgb = GL_LINEAR;
    CtxFunc->ogluGetNamedFramebufferAttachmentParameteriv(0, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &isSrgb);
    IsSrgbColor = isSrgb == GL_SRGB;
}
oglDefaultFrameBuffer_::~oglDefaultFrameBuffer_()
{ }

bool oglDefaultFrameBuffer_::IsSrgb() const
{
    return IsSrgbColor;
}


void oglDefaultFrameBuffer_::SetWindowSize(const uint32_t width, const uint32_t height)
{
    SetViewPort(width, height);
}

oglFrameBuffer_::FBOClear oglDefaultFrameBuffer_::Clear()
{
    return FBOClear(this);
}

struct DefFBOCtxConfig : public CtxResConfig<false, oglDefaultFBO>
{
    oglDefaultFBO Construct() const 
    {
        return MAKE_ENABLER_SHARED(oglDefaultFrameBuffer_, ());
    }
};
static DefFBOCtxConfig DEFFBO_CTXCFG;

oglDefaultFBO oglDefaultFrameBuffer_::Get()
{
    return oglContext_::CurrentContext()->GetOrCreate<false>(DEFFBO_CTXCFG);
}


oglCustomFrameBuffer_::oglCustomFrameBuffer_() : oglFrameBuffer_(GL_INVALID_INDEX)
{
    CtxFunc->ogluCreateFramebuffers(1, &FBOId);
    ColorAttachemnts.resize(CtxFunc->MaxColorAttachment);
}
oglCustomFrameBuffer_::~oglCustomFrameBuffer_()
{
    if (!EnsureValid()) return;
    CtxFunc->ogluDeleteFramebuffers(1, &FBOId);
}

bool oglCustomFrameBuffer_::IsSrgb() const
{
    return IsSrgbColor.value();
}

void oglCustomFrameBuffer_::CheckAttachmentMatch(const uint32_t width, const uint32_t height, const bool isSrgb, const bool checkSrgb)
{
    if (Width != 0 && Width != width)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's width  not match");
    if (Height != 0 && Height != height)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's height not match");
    SetViewPort(width, height);
    if (checkSrgb)
    {
        if (!IsSrgbColor.has_value())
            IsSrgbColor = isSrgb;
        else if (IsSrgbColor.value() != isSrgb)
            oglLog().warning(u"Framebuffer[{}]'s attachment has different state of srgb", Name);
    }
}

GLuint oglCustomFrameBuffer_::GetID(const oglRBO& rbo)
{
    return rbo->RBOId;
}
GLuint oglCustomFrameBuffer_::GetID(const oglTexBase& tex)
{
    return tex->TextureID;
}

void oglCustomFrameBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->ogluSetObjectLabel(GL_FRAMEBUFFER, FBOId, Name);
}

oglCustomFrameBuffer_::FBOClear oglCustomFrameBuffer_::Clear()
{
    return FBOClear(this);
}

std::pair<GLuint, GLuint> oglCustomFrameBuffer_::DebugBinding() const
{
    return oglCustomFrameBuffer_::DebugBinding(FBOId);
}

std::pair<GLuint, GLuint> oglCustomFrameBuffer_::DebugBinding(GLuint id)
{
    GLint clrId = 0, depId = 0;
    CtxFunc->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &clrId);
    CtxFunc->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depId);
    return std::pair<GLuint, GLuint>(clrId, depId);
}


oglFrameBuffer2D_::oglFrameBuffer2D_() : oglCustomFrameBuffer_()
{ }
oglFrameBuffer2D_::~oglFrameBuffer2D_()
{ }

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment)
{
    CheckCurrent();
    CheckAttachmentMatch(tex, true);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    Expects(layer < tex->Layers); // u"texture layer overflow"
    CheckCurrent();
    CheckAttachmentMatch(tex, true);
    CtxFunc->ogluNamedFramebufferTextureLayer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void oglFrameBuffer2D_::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_COLOR); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo, true);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, GetID(rbo));
    ColorAttachemnts[attachment] = rbo;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo, false);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo, false);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    StencilAttachment = rbo;
}

void oglFrameBuffer2D_::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckAttachmentMatch(rbo, false);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
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

void oglLayeredFrameBuffer_::AttachColorTexture(const oglTex3D& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex, true);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_3D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglLayeredFrameBuffer_::AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex, true);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglLayeredFrameBuffer_::AttachDepthTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachDepthTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachStencilTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglLayeredFrameBuffer_::AttachStencilTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckAttachmentMatch(tex, false);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    StencilAttachment = tex;
}

oglFBOLayered oglLayeredFrameBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglLayeredFrameBuffer_, ());
}




}