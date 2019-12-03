#include "oglPch.h"
#include "oglVAO.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "oglProgram.h"



namespace oglu
{
using std::string;
using std::u16string;
using std::vector;

MAKE_ENABLER_IMPL(oglVAO_)


oglVAO_::VAOPrep::VAOPrep(oglVAO_* vao, const oglDrawProgram_& prog) noexcept : 
    VAO(vao), Prog(prog)
{
    VAO->bind();
}
oglVAO_::VAOPrep::~VAOPrep()
{
    if (VAO)
    {
        VAO->CheckCurrent();
        oglVAO_::unbind();
    }
}

GLint oglVAO_::VAOPrep::GetLoc(const std::string_view name) const noexcept
{
    return Prog.InputRess.GetLocation(name);
}
void oglVAO_::VAOPrep::GetLocs(const common::span<const std::string_view> names, const common::span<GLint> idxs) const noexcept
{
    size_t i = 0;
    for (const auto& name : names)
        idxs[i++] = Prog.InputRess.GetLocation(name);
}

static constexpr GLenum ParseVAValType(const VAValType type)
{
    switch (type)
    {
    case VAValType::Double:     return GL_DOUBLE;
    case VAValType::Float:      return GL_FLOAT;
    case VAValType::Half:       return GL_HALF_FLOAT;
    case VAValType::U32:        return GL_UNSIGNED_INT;
    case VAValType::I32:        return GL_INT;
    case VAValType::U16:        return GL_UNSIGNED_SHORT;
    case VAValType::I16:        return GL_SHORT;
    case VAValType::U8:         return GL_UNSIGNED_BYTE;
    case VAValType::I8:         return GL_BYTE;
    case VAValType::U10_2:      return GL_UNSIGNED_INT_2_10_10_10_REV;
    case VAValType::I10_2:      return GL_INT_2_10_10_10_REV;
    case VAValType::UF11_10:    return GL_UNSIGNED_INT_10F_11F_11F_REV;
    default:                    return GL_INVALID_ENUM;
    }
}

void oglVAO_::VAOPrep::SetInteger(const VAValType valType, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor)
{
    if (attridx != GLInvalidIndex)
    {
        CtxFunc->ogluEnableVertexArrayAttrib(VAO->VAOId, attridx);//vertex attr index
        CtxFunc->ogluVertexAttribIPointer(attridx, size, ParseVAValType(valType), stride, offset);
        if (divisor != 0)
            CtxFunc->ogluVertexAttribDivisor(attridx, divisor);
    }
}
void oglVAO_::VAOPrep::SetFloat(const VAValType valType, const bool isNormalize, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor)
{
    if (attridx != GLInvalidIndex)
    {
        CtxFunc->ogluEnableVertexArrayAttrib(VAO->VAOId, attridx);//vertex attr index
        CtxFunc->ogluVertexAttribPointer(attridx, size, ParseVAValType(valType), isNormalize, stride, offset);
        if (divisor != 0)
            CtxFunc->ogluVertexAttribDivisor(attridx, divisor);
    }
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetIndex(const oglEBO& ebo)
{
    VAO->CheckCurrent();
    ebo->bind();
    VAO->IndexBuffer = ebo;
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSize(const uint32_t offset, const uint32_t size)
{
    VAO->CheckCurrent();
    VAO->Count = (GLsizei)size;
    if (VAO->IndexBuffer)
    {
        VAO->Method = DrawMethod::Index;
        VAO->Offsets = (void*)uintptr_t(offset * VAO->IndexBuffer->IndexSize);
    }
    else
    {
        VAO->Method = DrawMethod::Array;
        VAO->Offsets = (GLint)offset;
    }
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSizes(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes)
{
    VAO->CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
    VAO->Count.emplace<vector<GLsizei>>(sizes.cbegin(), sizes.cend());
    if (VAO->IndexBuffer)
    {
        VAO->Method = DrawMethod::Indexs;
        const uint32_t idxSize = VAO->IndexBuffer->IndexSize;
        VAO->Offsets = common::linq::FromIterable(offsets)
            .Select([=](const uint32_t off) { return reinterpret_cast<const void*>(uintptr_t(off * idxSize)); })
            .ToVector();
    }
    else
    {
        VAO->Method = DrawMethod::Arrays;
        VAO->Offsets = common::linq::FromIterable(offsets)
            .Select([](const uint32_t off) { return static_cast<GLint>(off); })
            .ToVector();
    }
    return *this;
}

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawSizeFrom(const oglIBO& ibo, GLint offset, GLsizei size)
{
    if ((bool)VAO->IndexBuffer != ibo->IsIndexed())
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Unmatched ebo state and ibo's target.");
    if (offset > ibo->Count || offset < 0)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"offset exceed ebo size.");
    if (size == 0)
        size = ibo->Count - offset;
    else if (size + offset > ibo->Count)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"draw size exceed ebo size.");
    VAO->IndirectBuffer = ibo;
    VAO->Method = ibo->IsIndexed() ? DrawMethod::IndirectIndexes : DrawMethod::IndirectArrays;
    VAO->Count = size;
    VAO->Offsets = offset;
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
        drawIdVBO->SetName(u"ogluDrawID");
        drawIdVBO->WriteSpan(ids);
        oglLog().success(u"new DrawIdVBO generated.\n");
        return drawIdVBO;
    }
};
static DRAWIDCtxConfig DRAWID_CTX_CFG;

oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawId(const GLint attridx)
{
    VAO->CheckCurrent();
    return SetInteger<int32_t>(oglContext_::CurrentContext()->GetOrCreate<true>(DRAWID_CTX_CFG), attridx, sizeof(int32_t), 1, 0, 1);
}
oglVAO_::VAOPrep& oglVAO_::VAOPrep::SetDrawId()
{
    return SetDrawId(Prog.InputRess.GetLocation("@DrawID"));
}


void oglVAO_::bind() const noexcept
{
    CheckCurrent();
    CtxFunc->ogluBindVertexArray(VAOId);
}

void oglVAO_::unbind() noexcept
{
    CtxFunc->ogluBindVertexArray(0);
}

oglVAO_::oglVAO_(const VAODrawMode mode) noexcept : VAOId(GL_INVALID_INDEX), DrawMode(mode)
{
    CtxFunc->ogluGenVertexArrays(1, &VAOId);
}

oglVAO_::~oglVAO_() noexcept
{
    if (!EnsureValid()) return;
    CtxFunc->ogluDeleteVertexArrays(1, &VAOId);
}

oglVAO_::VAOPrep oglVAO_::Prepare(const std::shared_ptr<oglDrawProgram_>& prog) noexcept
{
    CheckCurrent();
    return VAOPrep(this, *prog);
}

void oglVAO_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->ogluSetObjectLabel(GL_VERTEX_ARRAY, VAOId, Name);
}

void oglVAO_::RangeDraw(const uint32_t size, const uint32_t offset) const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        CtxFunc->ogluDrawArrays(common::enum_cast(DrawMode), offset, size);
        break;
    case DrawMethod::Index:
        CtxFunc->ogluDrawElements(common::enum_cast(DrawMode), size, IndexBuffer->IndexType, (const void*)intptr_t(offset * IndexBuffer->IndexSize));
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
        CtxFunc->ogluDrawArrays(common::enum_cast(DrawMode), std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::Index:
        CtxFunc->ogluDrawElements(common::enum_cast(DrawMode), std::get<GLsizei>(Count), IndexBuffer->IndexType, std::get<const void*>(Offsets));
        break;
    case DrawMethod::Arrays:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            CtxFunc->ogluMultiDrawArrays(common::enum_cast(DrawMode), std::get<vector<GLint>>(Offsets).data(), sizes.data(), (GLsizei)sizes.size());
        } break;
    case DrawMethod::Indexs:
        {
            const auto& sizes = std::get<vector<GLsizei>>(Count);
            CtxFunc->ogluMultiDrawElements(common::enum_cast(DrawMode), sizes.data(), IndexBuffer->IndexType, std::get<vector<const void*>>(Offsets).data(), (GLsizei)sizes.size());
            //const auto& offsets = std::get<vector<const void*>>(Offsets).data();
            //for (size_t i = 0; i < sizes.size(); ++i)
            //{
            //    //glDrawElementsBaseVertex(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 0);
            //    //glDrawElementsInstancedBaseInstance(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0);
            //    glDrawElementsInstancedBaseVertexBaseInstance(common::enum_cast(DrawMode), sizes[i], IndexBuffer->IndexType, (void*)offsets[i], 1, 0, 0);
            //}
        } break;
    case DrawMethod::IndirectArrays:
        CtxFunc->ogluMultiDrawArraysIndirect(common::enum_cast(DrawMode), *IndirectBuffer, std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::IndirectIndexes:
        CtxFunc->ogluMultiDrawElementsIndirect(common::enum_cast(DrawMode), IndexBuffer->IndexType, *IndirectBuffer, std::get<GLint>(Offsets), std::get<GLsizei>(Count));
        break;
    case DrawMethod::UnPrepared:
        oglLog().error(u"drawing an unprepared VAO [{}]\n", VAOId);
        break;
    }
    unbind();
}

void oglVAO_::InstanceDraw(const uint32_t count, const uint32_t base) const noexcept
{
    CheckCurrent();
    bind();
    switch (Method)
    {
    case DrawMethod::Array:
        CtxFunc->ogluDrawArraysInstanced(common::enum_cast(DrawMode), std::get<GLint>(Offsets), std::get<GLsizei>(Count), count, base);
        break;
    case DrawMethod::Index:
        CtxFunc->ogluDrawElementsInstanced(common::enum_cast(DrawMode), std::get<GLsizei>(Count), IndexBuffer->IndexType, std::get<const void*>(Offsets), count, base);
        break;
    case DrawMethod::Arrays:
    {
        const auto& sizes = std::get<vector<GLsizei>>(Count);
        const auto& offsets = std::get<vector<GLint>>(Offsets);
        for (size_t i = 0; i < sizes.size(); i++)
        {
            CtxFunc->ogluDrawArraysInstanced(common::enum_cast(DrawMode), 
                offsets[i], sizes[i], count, base);
        }
    } break;
    case DrawMethod::Indexs:
    {
        const auto& sizes = std::get<vector<GLsizei>>(Count);
        const auto& offsets = std::get<vector<const void*>>(Offsets);
        for (size_t i = 0; i < sizes.size(); i++)
        {
            CtxFunc->ogluDrawElementsInstanced(common::enum_cast(DrawMode),
                sizes[i], IndexBuffer->IndexType, offsets[i], count, base);
        }
    } break;
    case DrawMethod::IndirectArrays:
    case DrawMethod::IndirectIndexes:
        oglLog().error(u"indirect draw not support when manually instance draw, this vao[{}] has mode [{}].\n", VAOId, (uint8_t)Method);
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
