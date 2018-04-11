#include "oglRely.h"
#include "oglBuffer.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"

namespace oglu::detail
{
static ContextResource<std::shared_ptr<UBOManager>, false> CTX_UBO_MAN;

_oglBuffer::_oglBuffer(const BufferType _type) noexcept : BufType(_type)
{
    glGenBuffers(1, &bufferID);
    bind();
    unbind();
}

_oglBuffer::~_oglBuffer() noexcept
{
    if (MappedPtr != nullptr)
    {
        if (glUnmapNamedBufferEXT(bufferID) == GL_FALSE)
            oglLog().error(u"unmap buffer [{}] with size[{}] and flag[{}] failed.\n", bufferID, BufSize, (GLenum)BufFlag);
        MappedPtr = nullptr;
    }
    if (bufferID != GL_INVALID_INDEX)
        glDeleteBuffers(1, &bufferID);
    else
        oglLog().error(u"re-release oglBuffer [{}] of type [{}] with size [{}]\n", bufferID, (uint32_t)BufType, BufSize);
}

void _oglBuffer::bind() const noexcept
{
    glBindBuffer((GLenum)BufType, bufferID);
    //if (BufType == BufferType::Indirect)
        //oglLog().verbose(u"binding ibo[{}].\n", bufferID);
}

void _oglBuffer::unbind() const noexcept
{
    glBindBuffer((GLenum)BufType, 0);
}

void _oglBuffer::PersistentMap(const size_t size, const BufferFlags flags)
{
    BufFlag = flags | BufferFlags::PersistentMap;
    bind();
    glBufferStorage((GLenum)BufType, size, nullptr, (GLenum)(BufFlag & BufferFlags::PrepareMask));
    BufSize = size;
    GLenum access;
    if (HAS_FIELD(BufFlag, BufferFlags::MapRead) && !HAS_FIELD(BufFlag, BufferFlags::MapWrite))
        access = GL_READ_ONLY;
    else if (!HAS_FIELD(BufFlag, BufferFlags::MapRead) && HAS_FIELD(BufFlag, BufferFlags::MapWrite))
        access = GL_WRITE_ONLY;
    else
        access = GL_READ_WRITE;
    MappedPtr = glMapNamedBufferEXT(bufferID, access);
}

void _oglBuffer::Write(const void * const dat, const size_t size, const BufferWriteMode mode)
{
    if (MappedPtr == nullptr)
    {
        glNamedBufferDataEXT(bufferID, size, dat, (GLenum)mode);
        BufSize = size;
    }
    else
    {
        memcpy_s(MappedPtr, size, dat, size);
    }
}

_oglTextureBuffer::_oglTextureBuffer() noexcept : _oglBuffer(BufferType::Texture)
{
}


_oglUniformBuffer::_oglUniformBuffer(const size_t size) noexcept : _oglBuffer(BufferType::Uniform)
{
    PersistentMap(size, BufferFlags::PersistentMap | BufferFlags::CoherentMap | BufferFlags::MapWrite);
    vector<uint8_t> empty(size);
    Write(empty, BufferWriteMode::StreamDraw);
}

_oglUniformBuffer::~_oglUniformBuffer()
{
    //force unbind ubo, since bufID may be reused after releasaed
    getUBOMan().forcePop(bufferID);
}

UBOManager& _oglUniformBuffer::getUBOMan()
{
    const auto uboman = CTX_UBO_MAN.GetOrInsert([](auto dummy) { return std::make_shared<UBOManager>(); });
    return *uboman;
}

void _oglUniformBuffer::bind(const uint16_t pos) const
{
    glBindBufferBase(GL_UNIFORM_BUFFER, pos, bufferID);
}


_oglIndirectBuffer::_oglIndirectBuffer() noexcept : _oglBuffer(BufferType::Indirect)
{ }

void _oglIndirectBuffer::WriteCommands(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes, const bool isIndexed)
{
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"offset and size should be of the same size.");
    Count = (GLsizei)count;
    IsIndexed = isIndexed;
    if (isIndexed)
    {
        vector<DrawElementsIndirectCommand> commands;
        for (size_t i = 0; i < count; ++i)
            commands.push_back(DrawElementsIndirectCommand{ sizes[i], 1, offsets[i], 0, (GLuint)i });
        Write(commands);
        Commands = std::move(commands);
    }
    else
    {
        vector<DrawArraysIndirectCommand> commands;
        for (size_t i = 0; i < count; ++i)
            commands.push_back(DrawArraysIndirectCommand{ sizes[i], 1, offsets[i], (GLuint)i });
        Write(commands);
        Commands = std::move(commands);
    }
}

void _oglIndirectBuffer::WriteCommands(const uint32_t offset, const uint32_t size, const bool isIndexed)
{
    Count = 1;
    IsIndexed = isIndexed;
    if (isIndexed)
    {
        DrawElementsIndirectCommand command{ size, 1, offset, 0, 0 };
        Write(&command, sizeof(DrawElementsIndirectCommand));
        Commands = vector<DrawElementsIndirectCommand>{ command };
    }
    else
    {
        DrawArraysIndirectCommand command{ size, 1, offset, 0 };
        Write(&command, sizeof(DrawArraysIndirectCommand));
        Commands = vector<DrawArraysIndirectCommand>{ command };
    }
}


}