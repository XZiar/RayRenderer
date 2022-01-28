#include "oglPch.h"
#include "oglBuffer.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "BindingManager.h"


namespace oglu
{
using std::string;
using std::u16string;
using std::vector;

MAKE_ENABLER_IMPL(oglBuffer_::oglMapPtr_)
MAKE_ENABLER_IMPL(oglPixelBuffer_)
MAKE_ENABLER_IMPL(oglArrayBuffer_)
MAKE_ENABLER_IMPL(oglTextureBuffer_)
MAKE_ENABLER_IMPL(oglUniformBuffer_)
MAKE_ENABLER_IMPL(oglIndirectBuffer_)
MAKE_ENABLER_IMPL(oglElementBuffer_)


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


oglBuffer_::oglMapPtr_::oglMapPtr_(oglBuffer_* buf, const MapFlag flags) :
    Buffer(*buf), Flag(flags)
{
    GLenum access;
    if (HAS_FIELD(Flag, MapFlag::MapRead) && !HAS_FIELD(Flag, MapFlag::MapWrite))
        access = GL_READ_ONLY;
    else if (!HAS_FIELD(Flag, MapFlag::MapRead) && HAS_FIELD(Flag, MapFlag::MapWrite))
        access = GL_WRITE_ONLY;
    else
        access = GL_READ_WRITE;

    const auto ptr = CtxFunc->MapNamedBuffer(common::enum_cast(Buffer.BufferType), Buffer.BufferID, access);
    MemSpace = common::span<std::byte>(reinterpret_cast<std::byte*>(ptr), Buffer.BufSize);
}

oglBuffer_::oglMapPtr_::~oglMapPtr_()
{
    if (CtxFunc->UnmapNamedBuffer(common::enum_cast(Buffer.BufferType), Buffer.BufferID) == GL_FALSE)
        oglLog().error(u"unmap buffer [{}] with size[{}] failed.\n", Buffer.BufferID, MemSpace.size());
}


oglBuffer_::oglBuffer_(const BufferTypes type) noexcept :
    BufSize(0), BufferID(GL_INVALID_INDEX), BufferType(type)
{
    CtxFunc->GenBuffers(1, &BufferID);
    CtxFunc->BindBuffer(common::enum_cast(BufferType), BufferID);
    CtxFunc->BindBuffer(common::enum_cast(BufferType), 0);
}

oglBuffer_::~oglBuffer_() noexcept
{
    if (!EnsureValid()) return;
    if (PersistentPtr)
        PersistentPtr.reset();
    if (BufferID != GL_INVALID_INDEX)
        CtxFunc->DeleteBuffers(1, &BufferID);
    else
        oglLog().error(u"re-release oglBuffer [{}] of type [{}] with size [{}]\n", BufferID, (uint32_t)BufferType, BufSize);
}

void oglBuffer_::bind() const noexcept
{
    CheckCurrent();
    CtxFunc->BindBuffer(common::enum_cast(BufferType), BufferID);
    //if (BufferType== BufferType::Indirect)
        //oglLog().verbose(u"binding ibo[{}].\n", BufferID);
}

void oglBuffer_::unbind() const noexcept
{
    CheckCurrent();
    CtxFunc->BindBuffer(common::enum_cast(BufferType), 0);
}

void oglBuffer_::PersistentMap(MapFlag flags)
{
    flags |= MapFlag::CoherentMap | MapFlag::PersistentMap;
    CtxFunc->NamedBufferStorage(common::enum_cast(BufferType), BufferID, BufSize, nullptr, 
        common::enum_cast(flags & MapFlag::PrepareMask));
    PersistentPtr.emplace(this, flags);
}

oglMapPtr oglBuffer_::Map(const MapFlag flags)
{
    CheckCurrent();
    const bool newPersist = !PersistentPtr && HAS_FIELD(flags, MapFlag::PersistentMap);
    if (newPersist)
    {
        PersistentMap(flags);
        return oglMapPtr({}, std::shared_ptr<const oglMapPtr_>(shared_from_this(), &*PersistentPtr));
    }
    else
    {
        bind();
        return oglMapPtr(shared_from_this(), MAKE_ENABLER_SHARED(const oglMapPtr_, (this, flags)));
    }
}

void oglBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    CtxFunc->SetObjectLabel(GL_BUFFER, BufferID, Name);
}

common::span<std::byte> oglBuffer_::GetPersistentPtr() const
{
    return PersistentPtr->MemSpace;
}

void oglBuffer_::WriteSpan(const common::span<const std::byte> space, const BufferWriteMode mode)
{
    CheckCurrent();
    if (PersistentPtr && HAS_FIELD(PersistentPtr->Flag, MapFlag::MapWrite))
    {
        memcpy_s(PersistentPtr->MemSpace.data(), BufSize, space.data(), space.size());
    }
    else
    {
        CtxFunc->NamedBufferData(common::enum_cast(BufferType), BufferID, space.size(), space.data(), ParseBufferWriteMode(mode));
        BufSize = space.size();
    }
}

oglPixelBuffer_::~oglPixelBuffer_() noexcept {}
oglPBO oglPixelBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglPixelBuffer_, ());
}

oglArrayBuffer_::~oglArrayBuffer_() noexcept {}
oglVBO oglArrayBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglArrayBuffer_, ());
}


oglTextureBuffer_::oglTextureBuffer_() noexcept : oglBuffer_(BufferTypes::Texture)
{ }
oglTextureBuffer_::~oglTextureBuffer_() noexcept {}
oglTBO oglTextureBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglTextureBuffer_, ());
}


oglUniformBuffer_::oglUniformBuffer_(const size_t size) noexcept : oglBuffer_(BufferTypes::Uniform)
{
    BufSize = size;
    PersistentMap(MapFlag::MapWrite);
    vector<uint8_t> empty(size);
    WriteSpan(empty, BufferWriteMode::StreamDraw);
}

oglUniformBuffer_::~oglUniformBuffer_() noexcept
{
    if (!EnsureValid()) return;
    //force unbind ubo, since bufID may be reused after releasaed
    GetUBOMan().ReleaseRes(this);
}
oglUBO oglUniformBuffer_::Create(const size_t size)
{
    return MAKE_ENABLER_SHARED(oglUniformBuffer_, (size));
}


struct UBOCtxConfig : public CtxResConfig<false, std::unique_ptr<detail::ResourceBinder<oglUniformBuffer_>>>
{
    std::unique_ptr<detail::ResourceBinder<oglUniformBuffer_>> Construct() const
    { 
        return std::make_unique<detail::CachedResManager<oglUniformBuffer_>>(CtxFunc->MaxUBOUnits);
    }
};
static UBOCtxConfig UBO_CTXCFG;
detail::ResourceBinder<oglUniformBuffer_>& oglUniformBuffer_::GetUBOMan() noexcept
{
    return *oglContext_::CurrentContext()->GetOrCreate<false>(UBO_CTXCFG);
}

void oglUniformBuffer_::BindToUnit(const uint16_t pos) const
{
    CheckCurrent();
    CtxFunc->BindBufferBase(GL_UNIFORM_BUFFER, pos, BufferID);
}


bool oglIndirectBuffer_::IsIndexed() const
{
    return Commands.index() == 0;
}

oglIndirectBuffer_::oglIndirectBuffer_() noexcept : oglBuffer_(BufferTypes::Indirect)
{ }
oglIndirectBuffer_::~oglIndirectBuffer_() noexcept {}
oglIBO oglIndirectBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglIndirectBuffer_, ());
}

void oglIndirectBuffer_::WriteCommands(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes, const bool isIndexed)
{
    CheckCurrent();
    const auto count = offsets.size();
    if (count != sizes.size())
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"offset and size should be of the same size.");
    Count = (GLsizei)count;
    if (isIndexed)
    {
        vector<DrawElementsIndirectCommand> commands;
        for (size_t i = 0; i < count; ++i)
            commands.push_back(DrawElementsIndirectCommand{ sizes[i], 1, offsets[i], 0, (GLuint)i });
        WriteSpan(commands);
        Commands = std::move(commands);
    }
    else
    {
        vector<DrawArraysIndirectCommand> commands;
        for (size_t i = 0; i < count; ++i)
            commands.push_back(DrawArraysIndirectCommand{ sizes[i], 1, offsets[i], (GLuint)i });
        WriteSpan(commands);
        Commands = std::move(commands);
    }
}

void oglIndirectBuffer_::WriteCommands(const uint32_t offset, const uint32_t size, const bool isIndexed)
{
    CheckCurrent();
    Count = 1;
    if (isIndexed)
    {
        DrawElementsIndirectCommand command{ size, 1, offset, 0, 0 };
        Write(command);
        Commands = vector<DrawElementsIndirectCommand>{ command };
    }
    else
    {
        DrawArraysIndirectCommand command{ size, 1, offset, 0 };
        Write(command);
        Commands = vector<DrawArraysIndirectCommand>{ command };
    }
}


oglElementBuffer_::oglElementBuffer_() noexcept : 
    oglBuffer_(BufferTypes::Element), IndexType(GL_INVALID_ENUM), IndexSize(0) {}

oglElementBuffer_::~oglElementBuffer_() noexcept {}
oglEBO oglElementBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglElementBuffer_, ());
}

void oglElementBuffer_::SetSize(const uint8_t elesize)
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

common::span<std::byte> oglMapPtr::Get() const noexcept
{
    return Ptr->MemSpace;
}


}