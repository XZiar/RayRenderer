#include "oglRely.h"
#include "oglTexture.h"
#include "BindingManager.h"

namespace oglu::detail
{


_oglTexBase::_oglTexBase(const TextureType _type) noexcept : type(_type), defPos(getDefaultPos())
{
	glGenTextures(1, &textureID);
}

_oglTexBase::~_oglTexBase() noexcept
{
	glDeleteTextures(1, &textureID);
}

uint8_t _oglTexBase::getDefaultPos() noexcept
{
	GLint maxtexs;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxtexs);
	return (uint8_t)(maxtexs > 255 ? 255 : maxtexs - 1);
}

void _oglTexBase::bind(const uint8_t pos) const noexcept
{
	glActiveTexture(GL_TEXTURE0 + pos);
	glBindTexture((GLenum)type, textureID);
}

void _oglTexBase::unbind() const noexcept
{
	glBindTexture((GLenum)type, 0);
}


TextureManager& _oglTexture::getTexMan() noexcept
{
	static thread_local TextureManager texMan;
	return texMan;
}

void _oglTexture::parseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept
{
	switch ((uint8_t)dformat & 0x0f)
	{
	case 0x0:
		datatype = GL_UNSIGNED_BYTE; break;
	case 0x1:
		datatype = GL_BYTE; break;
	case 0x2:
		datatype = GL_UNSIGNED_SHORT; break;
	case 0x3:
		datatype = GL_SHORT; break;
	case 0x4:
		datatype = GL_UNSIGNED_INT; break;
	case 0x5:
		datatype = GL_INT; break;
	case 0x6:
		datatype = GL_HALF_FLOAT; break;
	case 0x7:
		datatype = GL_FLOAT; break;
	default:
		return;
	}
	switch ((uint8_t)dformat & 0xf0)
	{
	case 0x00:
		comptype = GL_RED; break;
	case 0x10:
		comptype = GL_RG; break;
	case 0x20:
		comptype = GL_RGB; break;
	case 0x30:
		comptype = GL_BGR; break;
	case 0x40:
		comptype = GL_RGBA; break;
	case 0x50:
		comptype = GL_BGRA; break;
	case 0x80:
		comptype = GL_RED_INTEGER; break;
	case 0x90:
		comptype = GL_RG_INTEGER; break;
	case 0xa0:
		comptype = GL_RGB_INTEGER; break;
	case 0xb0:
		comptype = GL_BGR_INTEGER; break;
	case 0xc0:
		comptype = GL_RGBA_INTEGER; break;
	case 0xd0:
		comptype = GL_BGRA_INTEGER; break;
	}
}


size_t _oglTexture::parseFormatSize(const TextureDataFormat dformat) noexcept
{
	size_t size = 0;
	switch ((uint8_t)dformat & 0x0f)
	{
	case 0x0:
	case 0x1:
		size = 8; break;
	case 0x2:
	case 0x3:
	case 0x6:
		size = 16; break;
	case 0x4:
	case 0x5:
	case 0x7:
		size = 32; break;
	default:
		return 0;
	}
	switch ((uint8_t)dformat & 0x70)
	{
	case 0x00:
		size *= 1; break;
	case 0x10:
		size *= 2; break;
	case 0x20:
	case 0x30:
		size *= 3; break;
	case 0x40:
	case 0x50:
		size *= 4; break;
	default:
		return 0;
	}
	return size / 8;
}

_oglTexture::_oglTexture(const TextureType _type) noexcept : _oglTexBase(_type)
{
	glGenTextures(1, &textureID);
}

_oglTexture::~_oglTexture() noexcept
{
	//force unbind texture, since texID may be reused after releasaed
	getTexMan().forcePop(textureID);
}

void _oglTexture::setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype)
{
	bind(defPos);
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_S, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_T, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MAG_FILTER, (GLint)filtertype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MIN_FILTER, (GLint)filtertype);
	unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void *data)
{
	bind(defPos);
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, data);
	inFormat = iformat;
	//unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(defPos);
	buf->bind();
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, nullptr);
	inFormat = iformat;
	buf->unbind();
	//unbind();
}

void _oglTexture::setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const void *data, const size_t size)
{
	bind(defPos);
	glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, (GLsizei)size, data);
	inFormat = iformat;
	//unbind();
}

void _oglTexture::setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const oglBuffer& buf, const GLsizei size)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(defPos);
	buf->bind();
	glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, size, nullptr);
	inFormat = iformat;
	buf->unbind();
	//unbind();
}

OPResult<> _oglTexture::getCompressedData(vector<uint8_t>& output)
{
	bind(defPos);
	GLint ret = GL_FALSE;
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED, &ret);
	if (ret == GL_FALSE)//non-compressed
		return false;
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &ret);
	output.resize(ret);
	glGetCompressedTexImage((GLenum)type, 0, output.data());
	return true;
}

OPResult<> _oglTexture::getData(vector<uint8_t>& output, const TextureDataFormat dformat)
{
	bind(defPos);
	GLint w = 0, h = 0;
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_HEIGHT, &h);
	output.resize(parseFormatSize(dformat) * w * h);
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glGetTexImage((GLenum)type, 0, comptype, datatype, output.data());
	return true;
}



GLenum _oglBufferTexture::parseFormat(const TextureDataFormat dformat) noexcept
{
	switch ((uint8_t)dformat & 0x7f)
	{
	case 0x00:
		return GL_R8;
	case 0x10:
		return GL_RG8;
	case 0x20:
	case 0x30:
		return GL_RGB8;
	case 0x40:
	case 0x50:
		return GL_RGBA8;
	case 0x07:
		return GL_R32F;
	case 0x17:
		return GL_RG32F;
	case 0x27:
	case 0x37:
		return GL_RGB32F;
	case 0x47:
	case 0x57:
		return GL_RGBA32F;
	}
	return GL_INVALID_ENUM;
}

_oglBufferTexture::_oglBufferTexture() noexcept : _oglTexBase(TextureType::TexBuf)
{
}

OPResult<> _oglBufferTexture::setBuffer(const TextureDataFormat dformat, const oglTBO& tbo)
{
	bind(defPos);
	innerBuf = tbo;
	glTexBuffer(GL_TEXTURE_BUFFER, parseFormat(dformat), tbo->bufferID);
	//unbind();
	return true;
}



}