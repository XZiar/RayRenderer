#include "oglPch.h"
#include "oglFBO.h"
#include "oglContext.h"
#include "oglException.h"


namespace oglu
{
MAKE_ENABLER_IMPL(oglRenderBuffer_)
MAKE_ENABLER_IMPL(oglDefaultFrameBuffer_)
MAKE_ENABLER_IMPL(oglFrameBuffer2D_)
MAKE_ENABLER_IMPL(oglFrameBuffer3D_)


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


oglFrameBuffer_::oglFrameBuffer_() : FBOId(GL_INVALID_INDEX), Width(0), Height(0)
{
    CtxFunc->ogluCreateFramebuffers(1, &FBOId);
    ColorAttachemnts.resize(CtxFunc->MaxColorAttachment);
}
oglFrameBuffer_::oglFrameBuffer_(const GLuint id) : FBOId(id), Width(0), Height(0)
{ }

oglFrameBuffer_::~oglFrameBuffer_()
{
    if (!EnsureValid()) return;
    CtxFunc->ogluDeleteFramebuffers(1, &FBOId);
}

void oglFrameBuffer_::CheckSizeMatch(const uint32_t width, const uint32_t height)
{
    if (Width != 0 && Width != width)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's width  not match");
    if (Height != 0 && Height != height)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Framebuffer's height not match");
    Width = width; Height = height;
    RefreshViewPort();
}

void oglFrameBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->ogluSetObjectLabel(GL_FRAMEBUFFER, FBOId, Name);
}

GLuint oglFrameBuffer_::GetID(const oglRBO& rbo)
{
    return rbo->RBOId;
}
GLuint oglFrameBuffer_::GetID(const oglTexBase& tex)
{
    return tex->TextureID;
}

void oglFrameBuffer_::RefreshViewPort() const
{
    if (CtxFunc->DrawFBO == FBOId)
        CtxFunc->ogluViewport(0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height));
}

FBOStatus oglFrameBuffer_::CheckStatus() const
{
    CheckCurrent();
    switch (CtxFunc->ogluCheckNamedFramebufferStatus(FBOId, GL_FRAMEBUFFER))
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

void oglFrameBuffer_::Use() const
{
    CheckCurrent();
    CtxFunc->ogluBindFramebuffer(GL_FRAMEBUFFER, FBOId);
    RefreshViewPort();
}

void oglFrameBuffer_::Discard()
{

}

void oglFrameBuffer_::Clear()
{

}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding() const
{
    return oglFrameBuffer_::DebugBinding(FBOId);
}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding(GLuint id)
{
    GLint clrId = 0, depId = 0;
    CtxFunc->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &clrId);
    CtxFunc->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depId);
    return std::pair<GLuint, GLuint>(clrId, depId);
}


static void BlitColor(const GLuint from, const GLuint to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    const auto& [x0, y0, x1, y1] = rect;
    CtxFunc->ogluBlitNamedFramebuffer(from, to, x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void oglFrameBuffer2DBase_::BlitColorTo(const oglFBO2DBase& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    to->CheckCurrent();
    BlitColor(FBOId, to->FBOId, rect);
}
void oglFrameBuffer2DBase_::BlitColorFrom(const oglFBO2DBase& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    from->CheckCurrent();
    BlitColor(from->FBOId, FBOId, rect);
}


oglDefaultFrameBuffer_::oglDefaultFrameBuffer_() : oglFrameBuffer2DBase_(0)
{
    const auto viewport = oglContext_::CurrentContext()->GetViewPort();
    Width = viewport.z; Height = viewport.w;
}

oglDefaultFrameBuffer_::~oglDefaultFrameBuffer_()
{ }

void oglDefaultFrameBuffer_::SetWindowSize(const uint32_t width, const uint32_t height)
{
    Width = width; Height = height;
    RefreshViewPort();
}

struct DefFBOCtxConfig : public CtxResConfig<false, oglDefaultFBO>
{
    oglDefaultFBO Construct() const { return MAKE_ENABLER_SHARED(oglDefaultFrameBuffer_, ()); }
};
static DefFBOCtxConfig DEFFBO_CTXCFG;

oglDefaultFBO oglDefaultFrameBuffer_::Get()
{
    return oglContext_::CurrentContext()->GetOrCreate<false>(DEFFBO_CTXCFG);
}


oglFrameBuffer2D_::oglFrameBuffer2D_() : oglFrameBuffer2DBase_()
{ }
oglFrameBuffer2D_::~oglFrameBuffer2D_()
{ }

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment)
{
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    Expects(layer < tex->Layers); // u"texture layer overflow"
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTextureLayer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void oglFrameBuffer2D_::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_COLOR); // u"rbo type missmatch"
    CheckCurrent();
    CheckSizeMatch(rbo);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, GetID(rbo));
    ColorAttachemnts[attachment] = rbo;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH); // u"rbo type missmatch"
    CheckCurrent();
    CheckSizeMatch(rbo);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglTex2D& tex)
{
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckSizeMatch(rbo);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    StencilAttachment = rbo;
}

void oglFrameBuffer2D_::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    Expects(rbo->GetType() == RBOFormat::TYPE_DEPTH_STENCIL); // u"rbo type missmatch"
    CheckCurrent();
    CheckSizeMatch(rbo);
    CtxFunc->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo; StencilAttachment = rbo;
}

oglFBO2D oglFrameBuffer2D_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer2D_, ());
}


oglFrameBuffer3D_::oglFrameBuffer3D_() : oglFrameBuffer_()
{ }
oglFrameBuffer3D_::~oglFrameBuffer3D_()
{ }

void oglFrameBuffer3D_::CheckLayerMatch(const uint32_t layer)
{
    if (LayerCount != 0 && LayerCount != layer)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Layered Framebuffer's layer not match");
    LayerCount = layer;
}

void oglFrameBuffer3D_::AttachColorTexture(const oglTex3D& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_3D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer3D_::AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer3D_::AttachDepthTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer3D_::AttachDepthTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer3D_::AttachStencilTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer3D_::AttachStencilTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    CheckSizeMatch(tex);
    CtxFunc->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    StencilAttachment = tex;
}

oglFBO3D oglFrameBuffer3D_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer3D_, ());
}



}