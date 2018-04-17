#include "oglRely.h"
#include "oglFBO.h"
#include "oglException.h"


namespace oglu::detail
{


_oglRenderBuffer::_oglRenderBuffer(const uint32_t width, const uint32_t height, const RBOFormat format) : Width(width), Height(height), InnerFormat(format)
{
    glGenRenderbuffers(1, &RBOId);
    glNamedRenderbufferStorageEXT(RBOId, (GLenum)InnerFormat, Width, Height);
}

_oglRenderBuffer::~_oglRenderBuffer()
{
    glDeleteRenderbuffers(1, &RBOId);
}

_oglFrameBuffer::_oglFrameBuffer()
{
    glGenFramebuffers(1, &FBOId);
    GLint maxAttach;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    ColorAttachemnts.resize(maxAttach);
}

_oglFrameBuffer::~_oglFrameBuffer()
{
    glDeleteFramebuffers(1, &FBOId);
}

bool _oglFrameBuffer::CheckIsComplete() const
{
    switch (glCheckNamedFramebufferStatusEXT(FBOId, GL_FRAMEBUFFER))
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;
    default:
        return false;
    }
}

void _oglFrameBuffer::AttackTexture(const oglTex2D& tex, const uint8_t attachment)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, tex->textureID, 0);
    ColorAttachemnts[attachment] = tex;
}

void _oglFrameBuffer::AttackTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    if (layer >= tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"texture layer overflow");
    glNamedFramebufferTextureLayerEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void _oglFrameBuffer::AttackRBO(const oglRBO& rbo, const uint8_t attachment)
{
    glNamedFramebufferRenderbufferEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, rbo->RBOId);
    ColorAttachemnts[attachment] = rbo;
}

void _oglFrameBuffer::AttackDepthTexture(const oglTex2D& tex)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    DepthAttachment = tex;
}

void _oglFrameBuffer::AttackDepthTexture(const oglRBO& rbo)
{
    glNamedFramebufferRenderbufferEXT(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    DepthAttachment = rbo;
}

void _oglFrameBuffer::AttackStencilTexture(const oglTex2D& tex)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    StencilAttachment = tex;
}

void _oglFrameBuffer::AttackStencilTexture(const oglRBO& rbo)
{
    glNamedFramebufferRenderbufferEXT(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    StencilAttachment = rbo;
}

}
