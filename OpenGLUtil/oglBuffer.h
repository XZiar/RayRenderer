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

namespace detail
{


class OGLUAPI _oglBuffer : public NonCopyable
{
protected:
	friend class oclu::detail::_oclGLBuffer;
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
	template<class T, class A>
	inline void write(const vector<T, A>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
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
	const size_t size;
	_oglUniformBuffer(const size_t size_) noexcept;
	~_oglUniformBuffer() noexcept;
};


class OGLUAPI _oglElementBuffer : public _oglBuffer
{
protected:
	friend class _oglVAO;
	GLenum idxtype;
	uint8_t idxsize;
	void setSize(uint8_t elesize)
	{
		switch (idxsize = elesize)
		{
		case 1:
			idxtype = GL_UNSIGNED_BYTE; return;
		case 2:
			idxtype = GL_UNSIGNED_SHORT; return;
		case 3:
			idxtype = GL_UNSIGNED_INT; return;
		}
	}
public:
	_oglElementBuffer() noexcept;
	template<class T, class = typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= 4>::type>
	void write(const vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		setSize(sizeof(T));
		_oglBuffer::write(dat.data(), idxsize*dat.size(), mode);
	}
	template<class T, class = typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= 4>::type>
	void write(const T *dat, const size_t count, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		setSize(sizeof(T));
		_oglBuffer::write(dat, idxsize*count, mode);
	}
#pragma warning(disable:4244)
	template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	void writeCompat(const vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		auto res = std::minmax_element(dat.begin(), dat.end());
		if (*res.first < 0)
			COMMON_THROW(BaseException, L"element buffer cannot appear negatve value");
		auto maxval = *res.second;
		if (maxval <= UINT8_MAX)
		{
			if (sizeof(T) == 1)
				write(dat, mode);
			else
			{
				vector<uint8_t> newdat;
				newdat.reserve(dat.size());
				std::copy(dat.begin(), dat.end(), std::back_inserter(newdat));
				write(newdat, mode);
			}
		}
		else if (maxval <= UINT16_MAX)
		{
			if (sizeof(T) == 2)
				write(dat, mode);
			else
			{
				vector<uint16_t> newdat;
				newdat.reserve(dat.size());
				std::copy(dat.begin(), dat.end(), std::back_inserter(newdat));
				write(newdat, mode);
			}
		}
		else if (maxval <= UINT32_MAX)
		{
			if (sizeof(T) == 4)
				write(dat, mode);
			else
			{
				vector<uint32_t> newdat;
				newdat.reserve(dat.size());
				std::copy(dat.begin(), dat.end(), std::back_inserter(newdat));
				write(newdat, mode);
			}
		}
		else
			COMMON_THROW(BaseException, L"Too much element held for element buffer");
	}
#pragma warning(default:4244)
};

}

using oglBuffer = Wrapper<detail::_oglBuffer>;
using oglTBO = Wrapper<detail::_oglTextureBuffer>;
using oglUBO = Wrapper<detail::_oglUniformBuffer>;
using oglEBO = Wrapper<detail::_oglElementBuffer>;


}
