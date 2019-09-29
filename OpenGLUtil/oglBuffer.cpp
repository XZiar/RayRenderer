#include "oglRely.h"
#include "oglBuffer.h"
#include "oglContext.h"
#include "oglException.h"
#include "BindingManager.h"
#include "DSAWrapper.h"

namespace oglu::detail
{

constexpr static GLenum ParseBufferWriteMode(const BufferWriteMode type)
{
    switch (type)
    {
    case BufferWriteMode::StreamDraw:   return GL_STREAM_DRAW;
    case BufferWriteMode::StreamRead:   return GL_STREAM_READ;
    case BufferWriteMode::StreamCopy:   return GL_STREAM_COPY;
    case BufferWriteMode::StaticDraw:   return GL_STATIC_DRAW;
    case BufferWriteMode::StaticRead:   return GL_STATIC_READ;
    case BufferWriteMode::StaticCopy:   return GL_STATIC_COPY;
    case BufferWriteMode::DynamicDraw:  return GL_DYNAMIC_DRAW;
    case BufferWriteMode::DynamicRead:  return GL_DYNAMIC_READ;
    case BufferWriteMode::DynamicCopy:  return GL_DYNAMIC_COPY;
    default:                            return GL_INVALID_ENUM;
    }
}


_oglMapPtr::_oglMapPtr(_oglBuffer& buf, const MapFlag flags) : BufferID(buf.BufferID), Size(buf.BufSize)
{
    GLenum access;
    if (HAS_FIELD(flags, MapFlag::MapRead) && !HAS_FIELD(flags, MapFlag::MapWrite))
        access = GL_READ_ONLY;
    else if (!HAS_FIELD(flags, MapFlag::MapRead) && HAS_FIELD(flags, MapFlag::MapWrite))
        access = GL_WRITE_ONLY;
    else
        access = GL_READ_WRITE;

    Pointer = DSA->ogluMapNamedBuffer(BufferID, access);
}

_oglMapPtr::~_oglMapPtr()
{
    if (DSA->ogluUnmapNamedBuffer(BufferID) == GL_FALSE)
        oglLog().error(u"unmap buffer [{}] with size[{}] failed.\n", BufferID, Size);
}


_oglBuffer::_oglBuffer(const BufferTypes type) noexcept :
    BufSize(0), BufferID(GL_INVALID_INDEX), BufferType(type)
{
    glGenBuffers(1, &BufferID);
    glBindBuffer(common::enum_cast(BufferType), BufferID);
    glBindBuffer(common::enum_cast(BufferType), 0);
}

_oglBuffer::~_oglBuffer() noexcept
{
    if (!EnsureValid()) return;
    if (PersistentPtr)
        PersistentPtr.reset();
    if (BufferID != GL_INVALID_INDEX)
        glDeleteBuffers(1, &BufferID);
    else
        oglLog().error(u"re-release oglBuffer [{}] of type [{}] with size [{}]\n", BufferID, (uint32_t)BufferType, BufSize);
}

void _oglBuffer::bind() const noexcept
{
    CheckCurrent();
    glBindBuffer(common::enum_cast(BufferType), BufferID);
    //if (BufferType== BufferType::Indirect)
        //oglLog().verbose(u"binding ibo[{}].\n", BufferID);
}

void _oglBuffer::unbind() const noexcept
{
    CheckCurrent();
    glBindBuffer(common::enum_cast(BufferType), 0);
}

oglMapPtr _oglBuffer::Map(const MapFlag flags)
{
    CheckCurrent();
    bind();
    const bool newPersist = !PersistentPtr && HAS_FIELD(flags, MapFlag::PersistentMap);
    if (newPersist)
        glBufferStorage(common::enum_cast(BufferType), BufSize, nullptr, static_cast<GLenum>(flags & MapFlag::PrepareMask));
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
        DSA->ogluNamedBufferData(BufferID, size, dat, ParseBufferWriteMode(mode));
        BufSize = size;
    }
}

_oglPixelBuffer::~_oglPixelBuffer() noexcept {}
_oglArrayBuffer::~_oglArrayBuffer() noexcept {}


_oglTextureBuffer::_oglTextureBuffer() noexcept : _oglBuffer(BufferTypes::Texture)
{
}
_oglTextureBuffer::~_oglTextureBuffer() noexcept {}


_oglUniformBuffer::_oglUniformBuffer(const size_t size) noexcept : _oglBuffer(BufferTypes::Uniform)
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
    getUBOMan().forcePop(BufferID);
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
    glBindBufferBase(GL_UNIFORM_BUFFER, pos, BufferID);
}


bool _oglIndirectBuffer::IsIndexed() const
{
    return Commands.index() == 0;
}

_oglIndirectBuffer::_oglIndirectBuffer() noexcept : _oglBuffer(BufferTypes::Indirect)
{ }
_oglIndirectBuffer::~_oglIndirectBuffer() noexcept {}

void _oglIndirectBuffer::WriteCommands(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes, const bool isIndexed)
{
    CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
    Count = (GLsizei)count;
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


_oglElementBuffer::_oglElementBuffer() noexcept : 
    _oglBuffer(BufferTypes::Element), IndexType(GL_INVALID_ENUM), IndexSize(0) {}

_oglElementBuffer::~_oglElementBuffer() noexcept {}

void _oglElementBuffer::SetSize(const uint8_t elesize)
{
    switch (IndexSize = elesize)
    {
    case 1:
        IndexType = GL_UNSIGNED_BYTE; return;
    case 2:
        IndexType = GL_UNSIGNED_SHORT; return;
    case 4:
        IndexType = GL_UNSIGNED_INT; return;
    }
}

}