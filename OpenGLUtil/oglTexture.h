#pragma once
#include "oglRely.h"
#include "oglBuffer.h"

namespace oglu
{


enum class TextureType : GLenum { Tex2D = GL_TEXTURE_2D, TexBuf = GL_TEXTURE_BUFFER, };

//upper for format, lower for type
enum class TextureDataFormat : uint8_t
{
    FLOAT_MASK = 0x07, NORMAL_MASK = 0x80, TYPE_MASK = 0x0f, FORMAT_MASK = 0xf0,
    //normalized integer(uint8)
	R8 = 0x00, RG8 = 0x10, RGB8 = 0x20, BGR8 = 0x30, RGBA8 = 0x40, BGRA8 = 0x50,
	//non-normalized integer(uint8)
	R8U = 0x80, RG8U = 0x90, RGB8U = 0xa0, BGR8U = 0xb0, RGBA8U = 0xc0, BGRA8U = 0xd0,
	//float(FP32)
	Rf = 0x07, RGf = 0x17, RGBf = 0x27, BGRf = 0x37, RGBAf = 0x47, BGRAf = 0x57,
};
MAKE_ENUM_BITFIELD(TextureDataFormat)
enum class TextureInnerFormat : GLint
{
	//normalized integer->as float[0,1]
	R8 = GL_R8, RG8 = GL_RG8, RGB8 = GL_RGB8, RGBA8 = GL_RGBA8,
	//non-normalized integer(uint8)
	R8U = GL_R8UI, RG8U = GL_RG8UI, RGB8U = GL_RGB8UI, RGBA8U = GL_RGBA8UI,
	//float(FP32)
	Rf = GL_R32F, RGf = GL_RG32F, RGBf = GL_RGB32F, RGBAf = GL_RGBA32F,
	//compressed(S3TC/DXT1,S3TC/DXT3,S3TC/DXT5,RGTC,BPTC
	BC1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT, BC1A = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, BC2 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	BC3 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, BC4 = GL_COMPRESSED_RED_RGTC1, BC5 = GL_COMPRESSED_RG_RGTC2,
	BC6H = GL_COMPRESSED_RGBA_BPTC_UNORM, BC7 = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
};

enum class TextureFilterVal : GLint { Linear = GL_LINEAR, Nearest = GL_NEAREST, };

enum class TextureWrapVal : GLint { Repeat = GL_REPEAT, Clamp = GL_CLAMP, };


namespace detail
{
using xziar::img::ImageDataType;
using xziar::img::Image;

class OGLUAPI _oglTexBase : public NonCopyable, public NonMovable
{
protected:
	friend class TextureManager;
	friend class _oglProgram;
	friend class ::oclu::detail::_oclGLBuffer;
	const TextureType type;
	GLuint textureID = GL_INVALID_INDEX;
	_oglTexBase(const TextureType _type) noexcept;
	void bind(const uint16_t pos) const noexcept;
	void unbind() const noexcept;
public:
	~_oglTexBase() noexcept;
};

class OGLUAPI _oglTexture : public _oglTexBase
{
protected:
	friend class TextureManager;
	friend class _oglProgram;
	friend class ::oclu::detail::_oclGLBuffer;
	TextureInnerFormat inFormat;
	uint32_t width, height;
	static TextureManager& getTexMan() noexcept;
    static void parseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype) noexcept;
    static void parseFormat(const ImageDataType dformat, const bool normalized, GLenum& datatype, GLenum& comptype) noexcept;
    static ImageDataType convertFormat(const TextureDataFormat dformat) noexcept;
	static size_t parseFormatSize(const TextureDataFormat dformat) noexcept;
public:
	_oglTexture(const TextureType _type) noexcept;
	~_oglTexture() noexcept;
	std::pair<uint32_t, uint32_t> getSize() const;
	void setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype);
	template<class T, class A>
	void setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const vector<T, A>& data)
	{
		setData(iformat, dformat, w, h, data.data());
	}
	void setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void *data);
	void setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglBuffer& buf);
    void setData(const TextureInnerFormat iformat, const Image& img, const bool normalized = true);
	void setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const void *data, const size_t size);
	template<class T, class A>
	void setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const vector<T, A>& data)
	{
		setCompressedData(iformat, w, h, data.data(), data.size() * sizeof(T));
	}
	void setCompressedData(const TextureInnerFormat iformat, const GLsizei w, const GLsizei h, const oglBuffer& buf, const GLsizei size);
	optional<vector<uint8_t>> getCompressedData();
    vector<uint8_t> getData(const TextureDataFormat dformat);
    Image getImage(const TextureDataFormat dformat);
};


class OGLUAPI _oglBufferTexture : public _oglTexBase
{
protected:
	friend class _oglProgram;
	oglTBO innerBuf;
	static GLenum parseFormat(const TextureDataFormat dformat) noexcept;
public:
	_oglBufferTexture() noexcept;
	void setBuffer(const TextureDataFormat dformat, const oglTBO& tbo);
};


}
using oglTexture = Wrapper<detail::_oglTexture>;
using oglBufTex = Wrapper<detail::_oglBufferTexture>;


}