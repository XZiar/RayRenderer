#include "oglRely.h"
#include "oglBuffer.h"
#include "oglContext.h"
#include "BindingManager.h"

namespace oglu::detail
{
static ContextResource<std::shared_ptr<UBOManager>, false> CTX_UBO_MAN;

_oglBuffer::_oglBuffer(const BufferType _type) noexcept : BufType(_type)
{
    glGenBuffers(1, &bufferID);
}

_oglBuffer::~_oglBuffer() noexcept
{
    if (MappedPtr != nullptr)
    {
        bind();
        if (glUnmapBuffer((GLenum)BufType) == GL_FALSE)
            oglLog().warning(u"unmap buffer [{}] with size[{}] and flag[{}] failed.\n", bufferID, BufSize, (GLenum)BufFlag);
        unbind();
    }
    if (bufferID != GL_INVALID_INDEX)
        glDeleteBuffers(1, &bufferID);
}

void _oglBuffer::bind() const noexcept
{
    glBindBuffer((GLenum)BufType, bufferID);
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
    MappedPtr = glMapBuffer((GLenum)BufType, access);
}

void _oglBuffer::Write(const void * const dat, const size_t size, const BufferWriteMode mode)
{
    if (MappedPtr == nullptr)
    {
        bind();
        glBufferData((GLenum)BufType, size, dat, (GLenum)mode);
        BufSize = size;
        if (BufType == BufferType::Pixel)//in case of invalid operation
            unbind();
    }
    else
    {
        memcpy_s(MappedPtr, size, dat, size);
    }
}

_oglTextureBuffer::_oglTextureBuffer() noexcept : _oglBuffer(BufferType::Uniform)
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
    //manually release
    glDeleteBuffers(1, &bufferID);
    bufferID = GL_INVALID_INDEX;
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


_oglElementBuffer::_oglElementBuffer() noexcept : _oglBuffer(BufferType::Element)
{
}

}