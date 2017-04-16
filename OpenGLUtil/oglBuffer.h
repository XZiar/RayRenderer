#pragma once
#include "oglRely.h"

namespace oglu
{

enum class BufferType : GLenum
{
	Array = GL_ARRAY_BUFFER, Element = GL_ELEMENT_ARRAY_BUFFER, Uniform = GL_UNIFORM_BUFFER,
	Pixel = GL_PIXEL_UNPACK_BUFFER, Texture = GL_TEXTURE_BUFFER,
};
enum class BufferWriteMode : GLenum
{
	StreamDraw = GL_STREAM_DRAW, StreamRead = GL_STREAM_READ, StreamCopy = GL_STREAM_COPY,
	StaticDraw = GL_STATIC_DRAW, StaticRead = GL_STATIC_READ, StaticCopy = GL_STATIC_COPY,
	DynamicDraw = GL_DYNAMIC_DRAW, DynamicRead = GL_DYNAMIC_READ, DynamicCopy = GL_DYNAMIC_COPY,
};
enum class IndexSize : GLenum { Byte = GL_UNSIGNED_BYTE, Short = GL_UNSIGNED_SHORT, Int = GL_UNSIGNED_INT };

namespace detail
{


class OGLUAPI _oglBuffer : public NonCopyable, public NonMovable
{
protected:
	friend class oclu::detail::_oclMem;
	friend class _oglTexture;
	friend class _oglVAO;
	friend class _oglProgram;
	const BufferType bufferType;
	GLuint bufferID = GL_INVALID_INDEX;
	void bind() const noexcept;
	void unbind() const noexcept;
public:
	_oglBuffer(const BufferType type) noexcept;
	~_oglBuffer() noexcept;

	void write(const void *dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
	template<class T>
	inline void write(const miniBLAS::vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		write(dat.data(), sizeof(T)*dat.size(), mode);
	}
	template<class T, size_t N>
	inline void write(T(&dat)[N], const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		write(dat, sizeof(dat), mode);
	}
};


class OGLUAPI _oglTextureBuffer : public _oglBuffer
{
protected:
	friend class oclu::detail::_oclMem;
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
	static UBOManager& getUBOMan();
	void bind(const uint8_t pos) const;
public:
	_oglUniformBuffer() noexcept;
	~_oglUniformBuffer() noexcept;
};


class OGLUAPI _oglElementBuffer : public _oglBuffer
{
protected:
	friend class _oglProgram;
	friend class _oglVAO;
	const IndexSize idxtype;
	const uint8_t idxsize;
public:
	_oglElementBuffer(const IndexSize idxsize_) noexcept;
	template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	OPResult<uint8_t> write(const miniBLAS::vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		if (sizeof(T) != idxsize)
			return OPResult<uint8_t>(false, L"Unmatch idx size", idxsize);
		_oglBuffer::write(dat.data(), idxsize*dat.size(), mode);
		return true;
	}
	template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	OPResult<uint8_t> write(const T *dat, const size_t count, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		if (sizeof(T) != idxsize)
			return OPResult<uint8_t>(false, L"Unmatch idx size", idxsize);
		_oglBuffer::write(dat, idxsize*count, mode);
		return true;
	}
};

}

using oglBuffer = Wrapper<detail::_oglBuffer>;
using oglTBO = Wrapper<detail::_oglTextureBuffer>;
using oglUBO = Wrapper<detail::_oglUniformBuffer>;
using oglEBO = Wrapper<detail::_oglElementBuffer>;


}
