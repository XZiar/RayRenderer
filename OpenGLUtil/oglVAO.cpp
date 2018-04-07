#include "oglRely.h"
#include "oglVAO.h"
#include "oglException.h"

namespace oglu::detail
{


_oglVAO::VAOPrep::VAOPrep(_oglVAO& vao_) noexcept :vao(vao_), isEmpty(false)
{
    vao.bind();
}

void _oglVAO::VAOPrep::End() noexcept
{
    if (!isEmpty)
    {
        isEmpty = true;
        _oglVAO::unbind();
    }
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglBuffer& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset)
{
    if (vbo->BufType != BufferType::Array)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Binding buffer must be VBO([BufferType::Array]).");
    if (attridx != GL_INVALID_INDEX)
    {
        vbo->bind();
        glEnableVertexAttribArray(attridx);//vertex attr index
        glVertexAttribPointer(attridx, size, GL_FLOAT, GL_FALSE, stride, (void*)intptr_t(offset));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglBuffer& vbo, const GLint(&attridx)[3], const GLint offset)
{
    if (vbo->BufType != BufferType::Array)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Binding buffer must be VBO([BufferType::Array]).");
    vbo->bind();
    if (attridx[0] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[0]);//VertPos
        glVertexAttribPointer(attridx[0], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset));
    }
    if (attridx[1] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[1]);//VertNorm
        glVertexAttribPointer(attridx[1], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset + 16));
    }
    if (attridx[2] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[2]);//TexPos
        glVertexAttribPointer(attridx[2], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset + 32));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglBuffer& vbo, const GLint(&attridx)[4], const GLint offset)
{
    if (vbo->BufType != BufferType::Array)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Binding buffer must be VBO([BufferType::Array]).\n");
    vbo->bind();
    if (attridx[0] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[0]);//VertPos
        glVertexAttribPointer(attridx[0], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset));
    }
    if (attridx[1] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[1]);//VertNorm
        glVertexAttribPointer(attridx[1], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 16));
    }
    if (attridx[2] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[2]);//TexPos
        glVertexAttribPointer(attridx[2], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 32));
    }
    if (attridx[4] != GL_INVALID_INDEX)
    {
        glEnableVertexAttribArray(attridx[3]);//VertTan
        glVertexAttribPointer(attridx[3], 4, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 48));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetIndex(const oglEBO& ebo)
{
    ebo->bind();
    vao.IndexBuffer = ebo;
    vao.InitSize();
    return *this;
}


void _oglVAO::bind() const noexcept
{
    glBindVertexArray(vaoID);
}

void _oglVAO::unbind() noexcept
{
    glBindVertexArray(0);
}

void _oglVAO::InitSize()
{
    if (IndexBuffer)
    {
        drawMethod = sizes.size() > 1 ? DrawMethod::Indexs : DrawMethod::Index;
        poffsets.clear();
        for (const auto& off : offsets)
            poffsets.push_back((void*)intptr_t(off * IndexBuffer->IndexSize));
    }
    else
    {
        drawMethod = sizes.size() > 1 ? DrawMethod::Arrays : DrawMethod::Array;
        ioffsets.clear();
        for (const auto& off : offsets)
            ioffsets.push_back((GLint)off);
    }
}

_oglVAO::_oglVAO(const VAODrawMode _mode) noexcept :vaoMode(_mode)
{
    glGenVertexArrays(1, &vaoID);
}

_oglVAO::~_oglVAO() noexcept
{
    glDeleteVertexArrays(1, &vaoID);
}

_oglVAO::VAOPrep _oglVAO::Prepare() noexcept
{
    return VAOPrep(*this);
}

void _oglVAO::SetDrawSize(const uint32_t offset_, const uint32_t size_)
{
    sizes = { (GLsizei)size_ };
    offsets = { offset_ };
    InitSize();
}

void _oglVAO::SetDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_)
{
    offsets = offset_;
    sizes.clear();
    for (const auto& s : size_)
        sizes.push_back((GLsizei)s);
    InitSize();
}

void _oglVAO::Draw(const uint32_t size, const uint32_t offset) const noexcept
{
    bind();
    if (IndexBuffer)
        glDrawElements((GLenum)vaoMode, size, IndexBuffer->IndexType, (void*)intptr_t(offset * IndexBuffer->IndexSize));
    else
        glDrawArrays((GLenum)vaoMode, offset, size);
    unbind();
}

void _oglVAO::Draw() const noexcept
{
    bind();
    switch (drawMethod)
    {
    case DrawMethod::Array:
        glDrawArrays((GLenum)vaoMode, ioffsets[0], sizes[0]);
        break;
    case DrawMethod::Index:
        glDrawElements((GLenum)vaoMode, sizes[0], IndexBuffer->IndexType, poffsets[0]);
        break;
    case DrawMethod::Arrays:
        glMultiDrawArrays((GLenum)vaoMode, ioffsets.data(), sizes.data(), (GLsizei)sizes.size());
        break;
    case DrawMethod::Indexs:
        glMultiDrawElements((GLenum)vaoMode, sizes.data(), IndexBuffer->IndexType, poffsets.data(), (GLsizei)sizes.size());
        break;
    }
    unbind();
}


}
