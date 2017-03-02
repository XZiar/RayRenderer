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
class OGLUAPI _oglBuffer : public NonCopyable, public NonMovable
{
protected:
	friend class oclu::_oclMem;
	friend class _oglTexture;
	friend class _oglVAO;
	friend class _oglProgram;
	const BufferType bufferType;
	GLuint bufferID = GL_INVALID_INDEX;
	void bind() const;
	void unbind() const;
public:
	_oglBuffer(const BufferType type);
	~_oglBuffer();

	void write(const void *dat, const size_t size, const BufferWriteMode mode = BufferWriteMode::StaticDraw);
	template<class T>
	void write(const miniBLAS::vector<T>& dat, const BufferWriteMode mode = BufferWriteMode::StaticDraw)
	{
		write(dat.data(), sizeof(T)*dat.size(), mode);
	}
};
using oglBuffer = Wrapper<_oglBuffer, false>;


class OGLUAPI _oglUniformBuffer : public _oglBuffer
{
protected:
	friend class UBOManager;
	friend class _oglProgram;
	static UBOManager& getUBOMan();
	void bind(const uint8_t pos) const;
public:
	_oglUniformBuffer();
	~_oglUniformBuffer();
};
using oglUBO = Wrapper<_oglUniformBuffer, false>;


}
