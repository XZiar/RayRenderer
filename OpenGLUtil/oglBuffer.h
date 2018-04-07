#pragma once
#include "oglRely.h"

namespace oglu
{

enum class BufferType : GLenum
{
    Array = GL_ARRAY_BUFFER, Element = GL_ELEMENT_ARRAY_BUFFER, Uniform = GL_UNIFORM_BUFFER, ShaderStorage = GL_SHADER_STORAGE_BUFFER,
    Pixel = GL_PIXEL_UNPACK_BUFFER, Texture = GL_TEXTURE_BUFFER, Indirect = GL_DISPATCH_INDIRECT_BUFFER
};
enum class BufferWriteMode : GLenum
{
    StreamDraw = GL_STREAM_DRAW, StreamRead = GL_STREAM_READ, StreamCopy = GL_STREAM_COPY,
    StaticDraw = GL_STATIC_DRAW, StaticRead = GL_STATIC_READ, StaticCopy = GL_STATIC_COPY,
    DynamicDraw = GL_DYNAMIC_DRAW, DynamicRead = GL_DYNAMIC_READ, DynamicCopy = GL_DYNAMIC_COPY,
};
enum class BufferFlags : GLenum
{
    MapRead = GL_MAP_READ_BIT, MapWrite = GL_MAP_WRITE_BIT, PersistentMap = GL_MAP_PERSISTENT_BIT, CoherentMap = GL_MAP_COHERENT_BIT,
    DynamicStorage = GL_DYNAMIC_STORAGE_BIT, ClientStorage = GL_CLIENT_STORAGE_BIT,
    ExplicitFlush = GL_MAP_FLUSH_EXPLICIT_BIT, UnSynchronize = GL_MAP_UNSYNCHRONIZED_BIT,
    InvalidateRange = GL_MAP_INVALIDATE_RANGE_BIT, InvalidateAll = GL_MAP_INVALIDATE_BUFFER_BIT,
    PrepareMask = MapRead | MapWrite | PersistentMap | CoherentMap | DynamicStorage | ClientStorage,
    RangeMask = MapRead | MapWrite | PersistentMap | CoherentMap | InvalidateRange | InvalidateAll | ExplicitFlush | UnSynchronize
};
MAKE_ENUM_BITFIELD(BufferFlags)

namespace detail
{


class OGLUAPI _oglBuffer : public NonCopyable
{
protected:
    friend class oclu::detail::_oclGLBuffer;
    friend class _oglTexture;
    friend class _oglVAO;
    friend class _oglProgram;
    void *MappedPtr = nullptr;
    size_t BufSize;
    const BufferType BufType;
    BufferFlags BufFlag;
    GLuint bufferID = GL_INVALID_INDEX;
    void bind() const noexcept;
    void unbind() const noexcept;
public:
    _oglBuffer(const BufferType type) noexcept;
    ~_oglBuffer() noexcept;

    void PersistentMap(const size_t size, const BufferFlags flags);

    void Write(const void * const dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
    template<class T, class A>
    inline void Write(const vector<T, A>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        Write(dat.data(), sizeof(T)*dat.size(), mode);
    }
    template<class T, size_t N>
    inline void Write(T(&dat)[N], const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        Write(dat, sizeof(dat), mode);
    }
};


class OGLUAPI _oglTextureBuffer : public _oglBuffer
{
protected:
    friend class _oglBufferTexture;
    friend class _oglProgram;
public:
    _oglTextureBuffer() noexcept;
};


class OGLUAPI _oglUniformBuffer : public _oglBuffer
{
protected:
    friend class UBOManager;
    friend class _oglProgram;
    friend class ProgState;
    friend class ProgDraw;
    static UBOManager& getUBOMan();
    void bind(const uint16_t pos) const;
public:
    _oglUniformBuffer(const size_t size) noexcept;
    ~_oglUniformBuffer() noexcept;
    size_t Size() const { return BufSize; };
};


class OGLUAPI _oglElementBuffer : public _oglBuffer
{
protected:
    friend class _oglVAO;
    GLenum IndexType;
    uint8_t IndexSize;
    void SetSize(const uint8_t elesize)
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
public:
    _oglElementBuffer() noexcept;
    ///<summary>Write index</summary>  
    ///<param name="dat">index container of [std::vector]</param>
    ///<param name="mode">buffer write mode</param>
    template<class T, class A>
    void Write(const vector<T, A>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        static_assert(std::is_integral_v<T> && sizeof(T) <= 4, "input type should be of integeral type and no more than uint32_t");
        SetSize(sizeof(T));
        _oglBuffer::Write(dat.data(), IndexSize*dat.size(), mode);
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
    ///<param name="dat">index container of [std::vector]</param>
    ///<param name="mode">buffer write mode</param>
    template<class T, class A>
    void WriteCompact(const vector<T, A>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
    {
        static_assert(std::is_integral_v<T>, "input type should be of integeral type and no more than uint32_t");
        auto res = std::minmax_element(dat.begin(), dat.end());
        if (*res.first < 0)
            COMMON_THROW(BaseException, L"element buffer cannot appear negatve value");
        auto maxval = *res.second;
        if (maxval <= UINT8_MAX)
        {
            if constexpr(sizeof(T) == 1)
                Write(dat, mode);
            else
            {
                common::AlignedBuffer<32> newdata(dat.size());
                common::copy::CopyLittleEndian(newdata.GetRawPtr<uint8_t>(), newdata.GetSize(), dat.data(), dat.size());
                Write(newdata.GetRawPtr<uint8_t>(), dat.size(), mode);
            }
        }
        else if (maxval <= UINT16_MAX)
        {
            if (sizeof(T) == 2)
                Write(dat, mode);
            else
            {
                common::AlignedBuffer<32> newdata(dat.size()*sizeof(uint16_t));
                common::copy::CopyLittleEndian(newdata.GetRawPtr<uint16_t>(), newdata.GetSize(), dat.data(), dat.size());
                Write(newdata.GetRawPtr<uint16_t>(), dat.size(), mode);
            }
        }
        else if (maxval <= UINT32_MAX)
        {
            if (sizeof(T) == 4)
                Write(dat, mode);
            else
            {
                vector<uint32_t> newdat;
                newdat.reserve(dat.size());
                for (const auto idx : dat)
                    newdat.push_back((uint32_t)idx);
                //std::copy(dat.begin(), dat.end(), std::back_inserter(newdat));
                Write(newdat, mode);
            }
        }
        else
            COMMON_THROW(BaseException, L"Too much element held for element buffer");
    }
};

}

using oglBuffer = Wrapper<detail::_oglBuffer>;
using oglTBO = Wrapper<detail::_oglTextureBuffer>;
using oglUBO = Wrapper<detail::_oglUniformBuffer>;
using oglEBO = Wrapper<detail::_oglElementBuffer>;


}
