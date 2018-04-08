#include "oglRely.h"
#include "oglVAO.h"
#include "oglException.h"
#include "oglUtil.h"

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

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset)
{
    if (attridx != GL_INVALID_INDEX)
    {
        vbo->bind();
        glEnableVertexAttribArray(attridx);//vertex attr index
        glVertexAttribPointer(attridx, size, GL_FLOAT, GL_FALSE, stride, (void*)intptr_t(offset));
    }
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglVBO& vbo, const GLint(&attridx)[3], const GLint offset)
{
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

_oglVAO::VAOPrep& _oglVAO::VAOPrep::Set(const oglVBO& vbo, const GLint(&attridx)[4], const GLint offset)
{
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
    return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::SetDrawSize(const uint32_t offset, const uint32_t size)
{
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
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"offset and size should be of the same size.");
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
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"Unmatched ebo state and ibo's target.");
    vao.IndirectBuffer = ibo;
    vao.Method = ibo->IsIndexed ? DrawMethod::IndirectIndexes : DrawMethod::IndirectArrays;
    vao.Count = std::monostate();
    vao.Offsets = std::monostate();
    ibo->bind();
    return *this;
}

void _oglVAO::bind() const noexcept
{
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
    glDeleteVertexArrays(1, &VAOId);
}

_oglVAO::VAOPrep _oglVAO::Prepare() noexcept
{
    return VAOPrep(*this);
}

void _oglVAO::Draw(const uint32_t size, const uint32_t offset) const noexcept
{
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
        glMultiDrawArraysIndirect((GLenum)DrawMode, 0, IndirectBuffer->Count, 0);
        break;
    case DrawMethod::IndirectIndexes:
        glMultiDrawElementsIndirect((GLenum)DrawMode, IndexBuffer->IndexType, 0, IndirectBuffer->Count, 0);
        break;
    }
    unbind();
}

void _oglVAO::Test() const noexcept
{
    bind();

    GLint vaoId = 0, vboId = 0, iboId = 0, eboId = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoId);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboId);
    glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &iboId);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboId);

    unbind();
    oglLog().debug(u"Current VAO[{}]'s binding: VAO[{}], VBO[{}], IBO[{}], EBO[{}]\n", VAOId, vaoId, vboId, iboId, eboId);
}

}
