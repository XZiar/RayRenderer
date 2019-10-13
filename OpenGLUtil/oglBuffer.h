#pragma once
#include "oglRely.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{

enum class BufferTypes : GLenum
{
    Array           = 0x8892/*GL_ARRAY_BUFFER*/,
    Element         = 0x8893/*GL_ELEMENT_ARRAY_BUFFER*/,
    Uniform         = 0x8A11/*GL_UNIFORM_BUFFER*/,
    ShaderStorage   = 0x90D2/*GL_SHADER_STORAGE_BUFFER*/,
    Pixel           = 0x88EC/*GL_PIXEL_UNPACK_BUFFER*/,
    Texture         = 0x8C2A/*GL_TEXTURE_BUFFER*/,
    Indirect        = 0x8F3F/*GL_DRAW_INDIRECT_BUFFER*/
}; 

enum class BufferWriteMode : uint8_t
{
    FREQ_MASK = 0x0f, ACCESS_MASK = 0xf0,
    FREQ_STREAM = 0x01, FREQ_STATIC = 0x02, FREQ_DYNAMIC = 0x03,
    ACCESS_DRAW = 0x10, ACCESS_READ = 0x20, ACCESS_COPY  = 0x30,
    StreamDraw  = FREQ_STREAM  | ACCESS_DRAW, StreamRead  = FREQ_STREAM  | ACCESS_READ, StreamCopy  = FREQ_STREAM  | ACCESS_COPY,
    StaticDraw  = FREQ_STATIC  | ACCESS_DRAW, StaticRead  = FREQ_STATIC  | ACCESS_READ, StaticCopy  = FREQ_STATIC  | ACCESS_COPY,
    DynamicDraw = FREQ_DYNAMIC | ACCESS_DRAW, DynamicRead = FREQ_DYNAMIC | ACCESS_READ, DynamicCopy = FREQ_DYNAMIC | ACCESS_COPY,
};
MAKE_ENUM_BITFIELD(BufferWriteMode)

enum class MapFlag : uint16_t
{
    MapRead         = 0x0001/*GL_MAP_READ_BIT*/,             MapWrite        = 0x0002/*GL_MAP_WRITE_BIT*/, 
    PersistentMap   = 0x0040/*GL_MAP_PERSISTENT_BIT*/,       CoherentMap     = 0x0080/*GL_MAP_COHERENT_BIT*/,
    DynamicStorage  = 0x0100/*GL_DYNAMIC_STORAGE_BIT*/,      ClientStorage   = 0x0200/*GL_CLIENT_STORAGE_BIT*/,
    InvalidateRange = 0x0004/*GL_MAP_INVALIDATE_RANGE_BIT*/, InvalidateAll   = 0x0008/*GL_MAP_INVALIDATE_BUFFER_BIT*/,
    ExplicitFlush   = 0x0010/*GL_MAP_FLUSH_EXPLICIT_BIT*/,   UnSynchronize   = 0x0020/*GL_MAP_UNSYNCHRONIZED_BIT*/,
    PrepareMask = MapRead | MapWrite | PersistentMap | CoherentMap | DynamicStorage  | ClientStorage,
    RangeMask   = MapRead | MapWrite | PersistentMap | CoherentMap | InvalidateRange | InvalidateAll | ExplicitFlush | UnSynchronize
};
MAKE_ENUM_BITFIELD(MapFlag)

class oglMapPtr;
namespace detail
{

class OGLUAPI _oglMapPtr : public common::NonCopyable, public common::NonMovable
{
    friend class _oglBuffer;
    friend class oglMapPtr;
private:
    void* Pointer = nullptr;
    GLuint BufferID;
    size_t Size;
    _oglMapPtr(_oglBuffer& buf, const MapFlag flags);
public:
    ~_oglMapPtr();
    void* GetPtr() const { return Pointer; }
};

}

class oglMapPtr : public std::shared_ptr<detail::_oglMapPtr>
{
public:
    constexpr oglMapPtr() {}
    oglMapPtr(detail::_oglMapPtr* ptr) : std::shared_ptr<detail::_oglMapPtr>(ptr) {}
    oglMapPtr(const std::shared_ptr<detail::_oglMapPtr>& ptr) : std::shared_ptr<detail::_oglMapPtr>(ptr) {}
    operator void*() const { return (*this)->GetPtr(); }
    template<typename T>
    T* AsType() const { return reinterpret_cast<T*>((*this)->GetPtr()); }
};

namespace detail
{


class OGLUAPI _oglBuffer : public common::NonMovable, public oglCtxObject<true>
{
    friend class _oglMapPtr;
    friend class _oglProgram;
    friend class ::oclu::GLInterop;
protected:
    oglMapPtr PersistentPtr;
    size_t BufSize;
    GLuint BufferID;
    const BufferTypes BufferType;
    void bind() const noexcept;
    void unbind() const noexcept;
    _oglBuffer(const BufferTypes type) noexcept;
public:
    virtual ~_oglBuffer() noexcept;

    oglMapPtr Map(const MapFlag flags);
    oglMapPtr GetPersistentPtr() const { return PersistentPtr; }

    void Write(const void * const dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
    template<typename T>
    void Write(const T& cont, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "need contiguous container type");
        Write(Helper::Data(cont), Helper::EleSize * Helper::Count(cont), mode);
    }
};


class OGLUAPI _oglPixelBuffer : public _oglBuffer
{
    template<typename Base>
    friend struct _oglTexCommon;
public:
    _oglPixelBuffer() noexcept : _oglBuffer(BufferTypes::Pixel) { }
    virtual ~_oglPixelBuffer() noexcept override;
};


class OGLUAPI _oglArrayBuffer : public _oglBuffer
{
    friend class _oglVAO;
public:
    _oglArrayBuffer() noexcept : _oglBuffer(BufferTypes::Array) { }
    virtual ~_oglArrayBuffer() noexcept override;
};


class OGLUAPI _oglTextureBuffer : public _oglBuffer
{
    friend class _oglBufferTexture;
public:
    _oglTextureBuffer() noexcept;
    virtual ~_oglTextureBuffer() noexcept override;
};


class OGLUAPI _oglUniformBuffer : public _oglBuffer
{
    friend class UBOManager;
    friend class _oglProgram;
    friend class ProgDraw;
protected:
    static UBOManager& getUBOMan();
    void bind(const uint16_t pos) const;
public:
    _oglUniformBuffer(const size_t size) noexcept;
    virtual ~_oglUniformBuffer() noexcept override;
    size_t Size() const { return BufSize; };
};


class OGLUAPI _oglIndirectBuffer : public _oglBuffer
{
    friend class _oglVAO;
    friend struct ::oglu::DSAFuncs;
public:
    struct DrawElementsIndirectCommand
    {
        GLuint count;
        GLuint instanceCount;
        GLuint firstIndex;
        GLuint baseVertex;
        GLuint baseInstance;
    };
    struct DrawArraysIndirectCommand
    {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
    };
protected:
    std::variant<std::vector<DrawElementsIndirectCommand>, std::vector<DrawArraysIndirectCommand>> Commands;
    GLsizei Count = 0;
    bool IsIndexed() const;
public:
    _oglIndirectBuffer() noexcept;
    virtual ~_oglIndirectBuffer() noexcept override;
    ///<summary>Write indirect draw commands</summary>  
    ///<param name="offsets">offsets</param>
    ///<param name="sizes">sizes</param>
    ///<param name="isIndexed">Indexed commands or not</param>
    void WriteCommands(const std::vector<uint32_t>& offsets, const std::vector<uint32_t>& sizes, const bool isIndexed);
    ///<summary>Write indirect draw commands</summary>  
    ///<param name="offset">offset</param>
    ///<param name="size">size</param>
    ///<param name="isIndexed">Indexed commands or not</param>
    void WriteCommands(const uint32_t offset, const uint32_t size, const bool isIndexed);
    const std::vector<DrawElementsIndirectCommand>& GetElementCommands() const { return std::get<std::vector<DrawElementsIndirectCommand>>(Commands); }
    const std::vector<DrawArraysIndirectCommand>& GetArrayCommands() const { return std::get<std::vector<DrawArraysIndirectCommand>>(Commands); }
};


class OGLUAPI _oglElementBuffer : public _oglBuffer
{
    friend class _oglVAO;
protected:
    GLenum IndexType;
    uint8_t IndexSize;
    void SetSize(const uint8_t elesize);
public:
    _oglElementBuffer() noexcept;
    virtual ~_oglElementBuffer() noexcept override;
    ///<summary>Write index</summary>  
    ///<param name="cont">index container</param>
    ///<param name="mode">buffer write mode</param>
    template<typename T>
    void Write(const T& cont, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "need contiguous container type");
        static_assert(std::is_integral_v<typename Helper::EleType> && sizeof(typename Helper::EleType) <= 4, "input type should be of integeral type and no more than uint32_t");
        SetSize(Helper::EleSize);
        _oglBuffer::Write(Helper::Data(cont), Helper::EleSize * Helper::Count(cont), mode);
    }
    ///<summary>Write index</summary>  
    ///<param name="dat">index ptr</param>
    ///<param name="count">index count</param>
    ///<param name="mode">buffer write mode</param>
    template<class T>
    void Write(const T * const dat, const size_t count, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        static_assert(std::is_integral_v<T> && sizeof(T) <= 4, "input type should be of integeral type and no more than uint32_t");
        SetSize(sizeof(T));
        _oglBuffer::Write(dat, IndexSize*count, mode);
    }
    ///<summary>Compact and write index</summary>  
    ///<param name="cont">index container</param>
    ///<param name="mode">buffer write mode</param>
    template<typename T>
    void WriteCompact(const T& cont, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "need contiguous container type");
        static_assert(std::is_integral_v<typename Helper::EleType>, "input type should be of integeral type");
        
        const auto* ptr = Helper::Data(cont);
        const size_t count = Helper::Count(cont);

        auto [minptr, maxptr] = std::minmax_element(ptr, ptr + count);
        if (*minptr < 0)
            COMMON_THROW(common::BaseException, u"element buffer cannot appear negatve value");
        auto maxval = *maxptr;

        if (maxval <= UINT8_MAX)
        {
            if constexpr (sizeof(T) == 1)
                Write(ptr, count, mode);
            else
            {
                common::AlignedBuffer newdata(count * 1);
                common::copy::CopyLittleEndian(newdata.GetRawPtr<uint8_t>(), newdata.GetSize(), ptr, count);
                Write(newdata.GetRawPtr<uint8_t>(), count, mode);
            }
        }
        else if (maxval <= UINT16_MAX)
        {
            if constexpr(sizeof(T) == 2)
                Write(ptr, count, mode);
            else
            {
                common::AlignedBuffer newdata(count * 2);
                common::copy::CopyLittleEndian(newdata.GetRawPtr<uint16_t>(), newdata.GetSize(), ptr, count);
                Write(newdata.GetRawPtr<uint16_t>(), count, mode);
            }
        }
        else if (maxval <= UINT32_MAX)
        {
            if constexpr(sizeof(T) == 4)
                Write(ptr, count, mode);
            else
            {
                std::vector<uint32_t> newdat;
                newdat.reserve(count);
                const auto *cur = ptr, *end = ptr + count;
                while (cur != end)
                    newdat.push_back(static_cast<uint32_t>(*cur++));
                Write(newdat, mode);
            }
        }
        else
            COMMON_THROW(common::BaseException, u"input should be no more than uint32_t");
    }
};

}



using oglBuffer = Wrapper<detail::_oglBuffer>;
using oglPBO = Wrapper<detail::_oglPixelBuffer>;
using oglVBO = Wrapper<detail::_oglArrayBuffer>;
using oglTBO = Wrapper<detail::_oglTextureBuffer>;
using oglUBO = Wrapper<detail::_oglUniformBuffer>;
using oglEBO = Wrapper<detail::_oglElementBuffer>;
using oglIBO = Wrapper<detail::_oglIndirectBuffer>;


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif