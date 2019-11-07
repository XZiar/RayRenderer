#include "oglPch.h"
#include "oglVAO.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "oglProgram.h"
#include "DSAWrapper.h"



namespace oglu
{
using std::string;
using std::u16string;
using std::vector;

MAKE_ENABLER_IMPL(oglVAO_)


oglVAO_::VAOPrep::VAOPrep(oglVAO_& vao_) noexcept :vao(vao_), isEmpty(false)
{
    vao.bind();
}

void oglVAO_::VAOPrep::End() noexcept
{
    vao.CheckCurrent();
    if (!isEmpty)
    {
        isEmpty = true;
        oglVAO_::unbind();
    }
}

void oglVAO_::VAOPrep::SetInteger(const GLenum valType, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor)
{
    if (attridx != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx);//vertex attr index
        DSA->ogluVertexAttribIPointer(attridx, size, valType, stride, offset);
        DSA->ogluVertexAttribDivisor(attridx, divisor);
    }
}
void oglVAO_::VAOPrep::SetFloat(const GLenum valType, const bool isNormalize, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor)
{
    if (attridx != (GLint)GL_INVALID_INDEX)
    {
        DSA->ogluEnableVertexArrayAttrib(vao.VAOId, attridx);//vertex attr index
        DSA->ogluVertexAttribPointer(attridx, size, valType, isNormalize, stride, offset);
        DSA->ogluVertexAttribDivisor(attridx, divisor);
    }
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetIndex(const oglEBO& ebo)
{
    vao.CheckCurrent();
    ebo->bind();
    vao.IndexBuffer = ebo;
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSize(const uint32_t offset, const uint32_t size)
{
    vao.CheckCurrent();
    vao.Count = (GLsizei)size;
    if (vao.IndexBuffer)
    {
        vao.Method = DrawMethod::Index;
        vao.Offsets = (void*)uintptr_t(offset * vao.IndexBuffer->IndexSize);
    }
    else
    {
        vao.Method = DrawMethod::Array;
        vao.Offsets = (GLint)offset;
    }
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSizes(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes)
{
    vao.CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
    vao.Count.emplace<vector<GLsizei>>(sizes.cbegin(), sizes.cend());
    if (vao.IndexBuffer)
    {
        vao.Method = DrawMethod::Indexs;
        const uint32_t idxSize = vao.IndexBuffer->IndexSize;
        vao.Offsets = common::linq::FromIterable(offsets)
            .Select([=](const uint32_t off) { return reinterpret_cast<const void*>(intptr_t(off * idxSize)); })
            .ToVector();
    }
    else
    {
        vao.Method = DrawMethod::Arrays;
        vao.Offsets = common::linq::FromIterable(offsets)
            .Select([](const uint32_t off) { return static_cast<GLint>(off); })
            .ToVector();
    }
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSizeFrom(const oglIBO& ibo, GLint offset, GLsizei size)
{
    if ((bool)vao.IndexBuffer != ibo->IsIndexed())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Unmatched ebo state and ibo's target.");
    if (offset > ibo->Count || offset < 0)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"offset exceed ebo size.");
    if (size == 0)
        size = ibo->Count - offset;
    else if (size + offset > ibo->Count)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"draw size exceed ebo size.");
    vao.IndirectBuffer = ibo;
    vao.Method = ibo->IsIndexed() ? DrawMethod::IndirectIndexes : DrawMethod::IndirectArrays;
    vao.Count = size;
    vao.Offsets = offset;
    return *this;
}

struct DRAWIDCtxConfig : public CtxResConfig<true, oglVBO>
{
    oglVBO Construct() const
    {
        std::vector<uint32_t> ids(4096);
        for (uint32_t i = 0; i < 4096; ++i)
            ids[i] = i;
        auto drawIdVBO = oglArrayBuffer_::Create();
        drawIdVBO->WriteSpan(ids);
        oglLog().success(u"new DrawIdVBO generated.\n");
        return drawIdVBO;
    }
};
static DRAWIDCtxConfig DRAWID_CTX_CFG;

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawId(const GLint attridx)
{
    vao.CheckCurrent();
    return SetInteger<int32_t>(oglContext_::CurrentContext()->GetOrCreate<true>(DRAWID_CTX_CFG), attridx, sizeof(int32_t), 1, 0, 1);
}
oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawId(const std::shared_ptr<oglDrawProgram_>& prog)
{
    return SetDrawId(prog->GetLoc("@DrawID"));
}


void oglVAO_::bind() const noexcept
{
    CheckCurrent();
    DSA->ogluBindVertexArray(VAOId);
}

void oglVAO_::unbind() noexcept
{
    DSA->ogluBindVertexArray(0);
}

oglVAO_::oglVAO_(const VAODrawMode mode) noexcept : VAOId(GL_INVALID_INDEX), DrawMode(mode)
{
    DSA->ogluGenVertexArrays(1, &VAOId);
}

oglVAO_::~oglVAO_() noexcept
{
    if (!EnsureValid()) return;
    DSA->ogluDeleteVertexArrays(1, &VAOId);
}

oglVAO_::VAOPrep oglVAO_::Prepare() noexcept
{
    CheckCurrent();
    return VAOPrep(*this);
}

void oglVAO_::Draw(const uint32_t size, const uint32_t offset) const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        glDrawArrays(common::enum_cast(DrawMode), offset, size);
        break;
    case DrawMethod::Index:
        glDrawElements(common::enum_cast(DrawMode), size, IndexBuffer->IndexType, (const void*)intptr_t(offset * IndexBuffer->IndexSize));
        break;
    default:
        oglLog().error(u"Only array/index mode support ranged draw, this vao[{}] has mode [{}].\n", VAOId, (uint8_t)Method);
    }
    unbind();
}

void oglVAO_::Draw() const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        glDrawArrays(common::enum_cast(DrawMode), std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::Index:
        glDrawElements(common::enum_cast(DrawMode), std::get<GLsizei>(Count), IndexBuffer->IndexType, std::get<const void*>(Offsets));
        break;
    case DrawMethod::Arrays:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            DSA->ogluMultiDrawArrays(common::enum_cast(DrawMode), std::get<vector<GLint>>(Offsets).data(), sizes.data(), (GLsizei)sizes.size());
        } break;
    case DrawMethod::Indexs:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            DSA->ogluMultiDrawElements(common::enum_cast(DrawMode), sizes.data(), IndexBuffer->IndexType, std::get<vector<const void*>>(Offsets).data(), (GLsizei)sizes.size());
            //const auto& offsets = std::get<vector<const void*>>(Offsets).data();
            //for (size_t i = 0; i < sizes.size(); ++i)
            //{
            //    //glDrawElementsBaseVertex(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 0);
            //    //glDrawElementsInstancedBaseInstance(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0);
            //    glDrawElementsInstancedBaseVertexBaseInstance(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0, 0);
            //}
        } break;
    case DrawMethod::IndirectArrays:
        DSA->ogluMultiDrawArraysIndirect(common::enum_cast(DrawMode), IndirectBuffer, std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::IndirectIndexes:
        DSA->ogluMultiDrawElementsIndirect(common::enum_cast(DrawMode), IndexBuffer->IndexType, IndirectBuffer, std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::UnPrepared:
        oglLog().error(u"drawing an unprepared VAO [{}]\n", VAOId);
        break;
    }
    unbind();
}

void oglVAO_::Test() const noexcept
{
    CheckCurrent();
    bind();
    BindingState state;
    unbind();
    oglLog().debug(u"Current VAO[{}]'s binding: VAO[{}], VBO[{}], IBO[{}], EBO[{}]\n", VAOId, state.VAO, state.VBO, state.IBO, state.EBO);
}

oglVAO oglVAO_::Create(const VAODrawMode mode)
{
    return MAKE_ENABLER_SHARED(oglVAO_, (mode));
}

}
