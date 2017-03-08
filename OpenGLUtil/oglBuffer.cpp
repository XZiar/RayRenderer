#include "oglBuffer.h"
#include "BindingManager.h"

namespace oglu::inner
{

_oglBuffer::_oglBuffer(const BufferType _type) :bufferType(_type)
{
	glGenBuffers(1, &bufferID);
}

_oglBuffer::~_oglBuffer()
{
	if (bufferID != GL_INVALID_INDEX)
		glDeleteBuffers(1, &bufferID);
}

void _oglBuffer::bind() const
{
	glBindBuffer((GLenum)bufferType, bufferID);
}

void _oglBuffer::unbind() const
{
	glBindBuffer((GLenum)bufferType, 0);
}

void _oglBuffer::write(const void *dat, const size_t size, const BufferWriteMode mode)
{
	bind();
	glBufferData((GLenum)bufferType, size, dat, (GLenum)mode);
	if(bufferType == BufferType::Pixel)//in case of invalid operation
		unbind();
}

_oglUniformBuffer::_oglUniformBuffer() : _oglBuffer(BufferType::Uniform)
{
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


_oglElementBuffer::_oglElementBuffer(const IndexSize idxsize_) : _oglBuffer(BufferType::Element),
	idxtype(idxsize_), idxsize(idxsize_ == IndexSize::Byte ? 1 : (idxsize_ == IndexSize::Short ? 2 : 4))
{
}

}