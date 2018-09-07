#include "oglRely.h"
#include "oglVAO.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "DSAWrapper.h"

namespace oglu::detail
{


_oglVAO::VAOPrep::VAOPrep(_oglVAO& vao_) noexcept :vao(vao_), isEmpty(false)
{
    vao.bind();
}

void _oglVAO::VAOPrep::End() noexcept
{
    vao.CheckCurrent();
    if (!isEmpty)
    {
        isEmpty = true;
        _oglVAO::unbind();
    }
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetInteger(const GLenum valType, const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor)
{
    vao.CheckCurrent();
    if (attridx != (GLint)GL_INVALID_INDEX)
    {
        vbo->bind();
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx);//vertex attr index
        glVertexAttribIPointer(attridx, size, valType, stride, (const void*)intptr_t(offset));
        glVertexAttribDivisor(attridx, divisor);
    }
    return *this;
}
_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetFloat(const GLenum valType, const bool isInteger, const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor)
{
    vao.CheckCurrent();
    if (attridx != (GLint)GL_INVALID_INDEX)
    {
        vbo->bind();
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx);//vertex attr index
        glVertexAttribPointer(attridx, size, valType, isInteger ? GL_TRUE : GL_FALSE, stride, (const void*)intptr_t(offset));
        glVertexAttribDivisor(attridx, divisor);
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetPoints(const oglVBO& vbo, const GLint(&attridx)[3], const GLint offset)
{
    vao.CheckCurrent();
    vbo->bind();
    if (attridx[0] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[0]);//VertPos
        glVertexAttribPointer(attridx[0], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset));
    }
    if (attridx[1] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[1]);//VertPos
        glVertexAttribPointer(attridx[1], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset + 16));
    }
    if (attridx[2] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[2]);//TexPos
        glVertexAttribPointer(attridx[2], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)intptr_t(offset + 32));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetPointExs(const oglVBO& vbo, const GLint(&attridx)[4], const GLint offset)
{
    vao.CheckCurrent();
    vbo->bind();
    if (attridx[0] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[0]);//VertPos
        glVertexAttribPointer(attridx[0], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset));
    }
    if (attridx[1] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[1]);//VertNorm
        glVertexAttribPointer(attridx[1], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 16));
    }
    if (attridx[2] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[2]);//TexPos
        glVertexAttribPointer(attridx[2], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 32));
    }
    if (attridx[3] != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx[3]);//VertTan
        glVertexAttribPointer(attridx[3], 4, GL_FLOAT, GL_FALSE, sizeof(b3d::PointEx), (void*)intptr_t(offset + 48));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetIndex(const oglEBO& ebo)
{
    vao.CheckCurrent();
    ebo->bind();
    vao.IndexBuffer = ebo;
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetDrawSize(const uint32_t offset, const uint32_t size)
{
    vao.CheckCurrent();
    vao.Count = (GLsizei)size;
    if (vao.IndexBuffer)
    {
        vao.Method = DrawMethod::Index;
        vao.Offsets = (void*)intptr_t(offset * vao.IndexBuffer->IndexSize);
    }
    else
    {
        vao.Method = DrawMethod::Array;
        vao.Offsets = (GLint)offset;
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetDrawSize(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes)
{
    vao.CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
    vao.Count.emplace<vector<GLsizei>>(sizes.cbegin(), sizes.cend());
    if (vao.IndexBuffer)
    {
        vao.Method = DrawMethod::Indexs;
        vector<const void*> tmpOffsets;
        const uint32_t idxSize = vao.IndexBuffer->IndexSize;
        std::transform(offsets.cbegin(), offsets.cend(), std::back_inserter(tmpOffsets),
            [=](const uint32_t off) { return (const void*)intptr_t(off * idxSize); });
        vao.Offsets = std::move(tmpOffsets);
    }
    else
    {
        vao.Method = DrawMethod::Arrays;
        vector<GLint> tmpOffsets;
        std::transform(offsets.cbegin(), offsets.cend(), std::back_inserter(tmpOffsets),
            [](const uint32_t off) { return (GLint)off; });
        vao.Offsets = std::move(tmpOffsets);
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetDrawSize(const oglIBO& ibo)
{
    if ((bool)vao.IndexBuffer != ibo->IsIndexed)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Unmatched ebo state and ibo's target.");
    vao.IndirectBuffer = ibo;
    vao.Method = ibo->IsIndexed ? DrawMethod::IndirectIndexes : DrawMethod::IndirectArrays;
    vao.Count = std::monostate();
    vao.Offsets = std::monostate();
    return *this;
}

void _oglVAO::bind() const noexcept
{
    CheckCurrent();
    glBindVertexArray(VAOId);
}

void _oglVAO::unbind() noexcept
{
    glBindVertexArray(0);
}

_oglVAO::_oglVAO(const VAODrawMode mode) noexcept :DrawMode(mode)
{
    glGenVertexArrays(1, &VAOId);
}

_oglVAO::~_oglVAO() noexcept
{
    if (!EnsureValid()) return;
    glDeleteVertexArrays(1, &VAOId);
}

_oglVAO::VAOPrep _oglVAO::Prepare() noexcept
{
    CheckCurrent();
    return VAOPrep(*this);
}

void _oglVAO::Draw(const uint32_t size, const uint32_t offset) const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        glDrawArrays((GLenum)DrawMode, offset, size);
        break;
    case DrawMethod::Index:
        glDrawElements((GLenum)DrawMode, size, IndexBuffer->IndexType, (const void*)intptr_t(offset * IndexBuffer->IndexSize));
        break;
    default:
        oglLog().error(u"Only array/index mode support ranged draw, this vao[{}] has mode [{}].\n", VAOId, (uint8_t)Method);
    }
    unbind();
}

void _oglVAO::Draw() const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        glDrawArrays((GLenum)DrawMode, std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::Index:
        glDrawElements((GLenum)DrawMode, std::get<GLsizei>(Count), IndexBuffer->IndexType, std::get<const void*>(Offsets));
        break;
    case DrawMethod::Arrays:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            glMultiDrawArrays((GLenum)DrawMode, std::get<vector<GLint>>(Offsets).data(), sizes.data(), (GLsizei)sizes.size());
        } break;
    case DrawMethod::Indexs:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            glMultiDrawElements((GLenum)DrawMode, sizes.data(), IndexBuffer->IndexType, std::get<vector<const void*>>(Offsets).data(), (GLsizei)sizes.size());
            //const auto& offsets = std::get<vector<const void*>>(Offsets).data();
            //for (size_t i = 0; i < sizes.size(); ++i)
            //{
            //    //glDrawElementsBaseVertex((GLenum)DrawMode, sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 0);
            //    //glDrawElementsInstancedBaseInstance((GLenum)DrawMode, sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0);
            //    glDrawElementsInstancedBaseVertexBaseInstance((GLenum)DrawMode, sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0, 0);
            //}
        } break;
    case DrawMethod::IndirectArrays:
        IndirectBuffer->bind(); //IBO not included in VAO
        glMultiDrawArraysIndirect((GLenum)DrawMode, 0, IndirectBuffer->Count, 0);
        break;
    case DrawMethod::IndirectIndexes:
        IndirectBuffer->bind(); //IBO not included in VAO
        glMultiDrawElementsIndirect((GLenum)DrawMode, IndexBuffer->IndexType, 0, IndirectBuffer->Count, 0);
        break;
    case DrawMethod::UnPrepared:
        oglLog().error(u"drawing an unprepared VAO [{}]\n", VAOId);
        break;
    }
    unbind();
}

void _oglVAO::Test() const noexcept
{
    CheckCurrent();
    bind();
    BindingState state;
    unbind();
    oglLog().debug(u"Current VAO[{}]'s binding: VAO[{}], VBO[{}], IBO[{}], EBO[{}]\n", VAOId, state.vaoId, state.vboId, state.iboId, state.eboId);
}

}
