#include "oglRely.h"
#include "oglBuffer.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"
#include "DSAWrapper.h"

namespace oglu::detail
{

_oglMapPtr::_oglMapPtr(_oglBuffer& buf, const MapFlag flags) : BufId(buf.bufferID), Size(buf.BufSize)
{
    GLenum access;
    if (HAS_FIELD(flags, MapFlag::MapRead) && !HAS_FIELD(flags, MapFlag::MapWrite))
        access = GL_READ_ONLY;
    else if (!HAS_FIELD(flags, MapFlag::MapRead) && HAS_FIELD(flags, MapFlag::MapWrite))
        access = GL_WRITE_ONLY;
    else
        access = GL_READ_WRITE;

    Pointer = DSA->ogluMapNamedBuffer(BufId, access);
}

_oglMapPtr::~_oglMapPtr()
{
    if (DSA->ogluUnmapNamedBuffer(BufId) == GL_FALSE)
        oglLog().error(u"unmap buffer [{}] with size[{}] failed.\n", BufId, Size);
}


_oglBuffer::_oglBuffer(const BufferType _type) noexcept : BufType(_type)
{
    glGenBuffers(1, &bufferID);
    glBindBuffer((GLenum)BufType, bufferID);
    glBindBuffer((GLenum)BufType, 0);
}

_oglBuffer::~_oglBuffer() noexcept
{
    if (!EnsureValid()) return;
    if (PersistentPtr)
        PersistentPtr.reset();
    if (bufferID != GL_INVALID_INDEX)
        glDeleteBuffers(1, &bufferID);
    else
        oglLog().error(u"re-release oglBuffer [{}] of type [{}] with size [{}]\n", bufferID, (uint32_t)BufType, BufSize);
}

void _oglBuffer::bind() const noexcept
{
    CheckCurrent();
    glBindBuffer((GLenum)BufType, bufferID);
    //if (BufType == BufferType::Indirect)
        //oglLog().verbose(u"binding ibo[{}].\n", bufferID);
}

void _oglBuffer::unbind() const noexcept
{
    CheckCurrent();
    glBindBuffer((GLenum)BufType, 0);
}

oglMapPtr _oglBuffer::Map(const MapFlag flags)
{
    CheckCurrent();
    bind();
    const bool newPersist = !PersistentPtr && HAS_FIELD(flags, MapFlag::PersistentMap);
    if (newPersist)
        glBufferStorage((GLenum)BufType, BufSize, nullptr, (GLenum)(flags & MapFlag::PrepareMask));
    oglMapPtr ptr(new _oglMapPtr(*this, flags));
    if (newPersist)
        PersistentPtr = ptr;
    return ptr;
}

void _oglBuffer::Write(const void * const dat, const size_t size, const BufferWriteMode mode)
{
    CheckCurrent();
    if (PersistentPtr)
    {
        memcpy_s(PersistentPtr, BufSize, dat, size);
    }
    else
    {
        DSA->ogluNamedBufferData(bufferID, size, dat, (GLenum)mode);
        BufSize = size;
    }
}

_oglTextureBuffer::_oglTextureBuffer() noexcept : _oglBuffer(BufferType::Texture)
{
}


_oglUniformBuffer::_oglUniformBuffer(const size_t size) noexcept : _oglBuffer(BufferType::Uniform)
{
    BufSize = size;
    Map(MapFlag::CoherentMap | MapFlag::PersistentMap | MapFlag::MapWrite);
    vector<uint8_t> empty(size);
    Write(empty, BufferWriteMode::StreamDraw);
}

_oglUniformBuffer::~_oglUniformBuffer() noexcept
{
    if (!EnsureValid()) return;
    //force unbind ubo, since bufID may be reused after releasaed
    getUBOMan().forcePop(bufferID);
}

struct UBOCtxConfig : public CtxResConfig<false, UBOManager>
{
    UBOManager Construct() const { return {}; }
};
static UBOCtxConfig UBO_CTXCFG;
UBOManager& _oglUniformBuffer::getUBOMan()
{
    return oglContext::CurrentContext()->GetOrCreate<false>(UBO_CTXCFG);
}

void _oglUniformBuffer::bind(const uint16_t pos) const
{
    CheckCurrent();
    glBindBufferBase(GL_UNIFORM_BUFFER, pos, bufferID);
}


_oglIndirectBuffer::_oglIndirectBuffer() noexcept : _oglBuffer(BufferType::Indirect)
{ }

void _oglIndirectBuffer::WriteCommands(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes, const bool isIndexed)
{
    CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
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
    CheckCurrent();
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