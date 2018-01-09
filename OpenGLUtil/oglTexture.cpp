#include "oglRely.h"
#include "oglTexture.h"
#include "oglException.h"
#include "BindingManager.h"

namespace oglu::detail
{

_oglTexBase::_oglTexBase(const TextureType _type) noexcept : type(_type)
{
	glGenTextures(1, &textureID);
}

_oglTexBase::~_oglTexBase() noexcept
{
	glDeleteTextures(1, &textureID);
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
void _oglTexture::parseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept
{
    if (HAS_FIELD(dformat, ImageDataType::FLOAT_MASK))
        datatype = GL_FLOAT;
    else
        datatype = GL_UNSIGNED_BYTE;
    if(normalized)
        switch (REMOVE_MASK(dformat, { ImageDataType::FLOAT_MASK }))
        {
        case ImageDataType::GRAY:
            comptype = GL_RED; break;
        case ImageDataType::RA:
            comptype = GL_RG; break;
        case ImageDataType::RGB:
            comptype = GL_RGB; break;
        case ImageDataType::BGR:
            comptype = GL_BGR; break;
        case ImageDataType::RGBA:
            comptype = GL_RGBA; break;
        case ImageDataType::BGRA:
            comptype = GL_BGRA; break;
        }
    else
        switch (REMOVE_MASK(dformat, { ImageDataType::FLOAT_MASK }))
        {
        case ImageDataType::GRAY:
            comptype = GL_RED_INTEGER; break;
        case ImageDataType::RA:
            comptype = GL_RG_INTEGER; break;
        case ImageDataType::RGB:
            comptype = GL_RGB_INTEGER; break;
        case ImageDataType::BGR:
            comptype = GL_BGR_INTEGER; break;
        case ImageDataType::RGBA:
            comptype = GL_RGBA_INTEGER; break;
        case ImageDataType::BGRA:
            comptype = GL_BGRA_INTEGER; break;
        }
}
ImageDataType _oglTexture::convertFormat(const TextureDataFormat dformat) noexcept
{
    ImageDataType isFloat = HAS_FIELD(dformat, TextureDataFormat::FLOAT_MASK) ? ImageDataType::FLOAT_MASK : ImageDataType::EMPTY_MASK;
    switch (REMOVE_MASK(dformat, TextureDataFormat::TYPE_MASK, TextureDataFormat::NORMAL_MASK))
    {
    case TextureDataFormat::R8:
        return ImageDataType::RED | isFloat;
    case TextureDataFormat::RG8:
        return ImageDataType::RA | isFloat;
    case TextureDataFormat::RGB8:
        return ImageDataType::RGB | isFloat;
    case TextureDataFormat::RGBA8:
        return ImageDataType::RGBA | isFloat;
    case TextureDataFormat::BGR8:
        return ImageDataType::BGR | isFloat;
    case TextureDataFormat::BGRA8:
        return ImageDataType::BGRA | isFloat;
    }
    return isFloat;//fallback
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
	bind(0);
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_S, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_T, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MAG_FILTER, (GLint)filtertype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MIN_FILTER, (GLint)filtertype);
	unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void *data)
{
	if (w % 4)
		COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
	bind(0);
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, data);
	inFormat = iformat;
	width = w, height = h;
	//unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(0);
	buf->bind();
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, nullptr);
	inFormat = iformat;
	width = w, height = h;
	buf->unbind();
	//unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const xziar::img::Image& img, const bool normalized)
{
    if (img.Width % 4)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"each line's should be aligned to 4 pixels");
    bind(0);
    GLenum datatype, comptype;
    parseFormat(img.DataType, normalized, datatype, comptype);
    glTexImage2D((GLenum)type, 0, (GLint)iformat, img.Width, img.Height, 0, comptype, datatype, img.GetRawPtr());
    inFormat = iformat;
    width = img.Width, height = img.Height;
}

void _oglTexture::setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const void *data, const size_t size)
{
	bind(0);
	glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, (GLsizei)size, data);
	inFormat = iformat;
	width = w, height = h;
	//unbind();
}

void _oglTexture::setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const oglBuffer& buf, const GLsizei size)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(0);
	buf->bind();
	glCompressedTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, size, nullptr);
	inFormat = iformat;
	width = w, height = h;
	buf->unbind();
	//unbind();
}

optional<vector<uint8_t>> _oglTexture::getCompressedData()
{
	bind(0);
	GLint ret = GL_FALSE;
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED, &ret);
	if (ret == GL_FALSE)//non-compressed
		return {};
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &ret);
	optional<vector<uint8_t>> output(std::in_place, ret);
	glGetCompressedTexImage((GLenum)type, 0, (*output).data());
	return output;
}

vector<uint8_t> _oglTexture::getData(const TextureDataFormat dformat)
{
	bind(0);
	GLint w = 0, h = 0;
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_HEIGHT, &h);
	auto size = w * h * parseFormatSize(dformat);
	vector<uint8_t> output(size);
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glGetTexImage((GLenum)type, 0, comptype, datatype, output.data());
	return output;
}

Image _oglTexture::getImage(const TextureDataFormat dformat)
{
    bind(0);
    GLint w = 0, h = 0;
    glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv((GLenum)type, 0, GL_TEXTURE_HEIGHT, &h);
    Image image(convertFormat(dformat));
    image.SetSize(w, h);
    GLenum datatype, comptype;
    parseFormat(dformat, datatype, comptype);
    glGetTexImage((GLenum)type, 0, comptype, datatype, image.GetRawPtr());
    return image;
}

pair<uint32_t, uint32_t> _oglTexture::getSize() const
{
	return { width,height };
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

void _oglBufferTexture::setBuffer(const TextureDataFormat dformat, const oglTBO& tbo)
{
	bind(0);
	innerBuf = tbo;
	glTexBuffer(GL_TEXTURE_BUFFER, parseFormat(dformat), tbo->bufferID);
	//unbind();
}



}