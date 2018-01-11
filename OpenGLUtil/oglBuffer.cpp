#include "oglRely.h"
#include "oglBuffer.h"
#include "BindingManager.h"

namespace oglu::detail
{

_oglBuffer::_oglBuffer(const BufferType _type) noexcept :bufferType(_type)
{
	glGenBuffers(1, &bufferID);
}

_oglBuffer::~_oglBuffer() noexcept
{
	if (bufferID != GL_INVALID_INDEX)
		glDeleteBuffers(1, &bufferID);
}

void _oglBuffer::bind() const noexcept
{
	glBindBuffer((GLenum)bufferType, bufferID);
}

void _oglBuffer::unbind() const noexcept
{
	glBindBuffer((GLenum)bufferType, 0);
}

void _oglBuffer::write(const void * const dat, const size_t size, const BufferWriteMode mode)
{
	bind();
	glBufferData((GLenum)bufferType, size, dat, (GLenum)mode);
	if(bufferType == BufferType::Pixel)//in case of invalid operation
		unbind();
}


_oglTextureBuffer::_oglTextureBuffer() noexcept : _oglBuffer(BufferType::Uniform)
{
}


_oglUniformBuffer::_oglUniformBuffer(const size_t size_) noexcept : _oglBuffer(BufferType::Uniform), size(size_)
{
	vector<uint8_t> empty(size);
	write(empty, BufferWriteMode::StreamDraw);
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
	static thread_local UBOManager uboMan;
	return uboMan;
}

void _oglUniformBuffer::bind(const uint8_t pos) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, pos, bufferID);
}


_oglElementBuffer::_oglElementBuffer() noexcept : _oglBuffer(BufferType::Element)
{
}

}