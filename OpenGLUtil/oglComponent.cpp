#include "oglComponent.h"
#include "BindingManager.h"

namespace oglu::inner
{

string _oglShader::loadFromFile(FILE * fp)
{
	fseek(fp, 0, SEEK_END);
	const size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *dat = new char[fsize + 1];
	fread(dat, fsize, 1, fp);
	dat[fsize] = '\0';
	string txt(dat);
	delete[] dat;

	return txt;
}

_oglShader::_oglShader(const ShaderType type, const string & txt) : shaderType(type), src(txt)
{
	auto ptr = txt.c_str();
	shaderID = glCreateShader(GLenum(type));
	glShaderSource(shaderID, 1, &ptr, NULL);
}

_oglShader::~_oglShader()
{
	if (shaderID != GL_INVALID_INDEX)
		glDeleteShader(shaderID);
}

OPResult<> _oglShader::compile()
{
	glCompileShader(shaderID);

	GLint result;
	char logstr[4096] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(logstr), NULL, logstr);
		return OPResult<>(false, logstr);
	}
	return true;
}


TextureManager& _oglTexture::getTexMan()
{
	static thread_local TextureManager texMan;
	return texMan;
}

uint8_t _oglTexture::getDefaultPos()
{
	GLint maxtexs;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxtexs);
	return (uint8_t)(maxtexs > 255 ? 255 : maxtexs - 1);
}

void _oglTexture::parseFormat(const TextureDataFormat dformat, GLenum& datatype, GLenum& comptype)
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

GLenum _oglTexture::parseFormat(const TextureDataFormat dformat)
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
}

void _oglTexture::bind(const uint8_t pos) const
{
	glActiveTexture(GL_TEXTURE0 + pos);
	glBindTexture((GLenum)type, textureID);
}

void _oglTexture::unbind() const
{
	glBindTexture((GLenum)type, 0);
}

_oglTexture::_oglTexture(const TextureType _type) : type(_type), defPos(getDefaultPos())
{
	glGenTextures(1, &textureID);
}

_oglTexture::~_oglTexture()
{
	//force unbind texture, since texID may be reused after releasaed
	getTexMan().forcePop(textureID);
	glDeleteTextures(1, &textureID);
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

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const void * data)
{
	bind(defPos);
	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, data);
	//unbind();
}

void _oglTexture::setData(const TextureInnerFormat iformat, const TextureDataFormat dformat, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(defPos);
	buf->bind();

	GLenum datatype, comptype;
	parseFormat(dformat, datatype, comptype);
	glTexImage2D((GLenum)type, 0, (GLint)iformat, w, h, 0, comptype, datatype, NULL);

	//unbind();
}


OPResult<> _oglTexture::setBuffer(const TextureDataFormat dformat, const oglBuffer& tbo)
{
	if (type != TextureType::TexBuf)
		return OPResult<>(false, L"Texture is not TextureBuffer");
	if(tbo->bufferType != BufferType::Texture)
		return OPResult<>(false, L"Buffer is not TextureBuffer");
	bind(defPos);
	glTexBuffer(GL_TEXTURE_BUFFER, parseFormat(dformat), tbo->bufferID);
	innerBuf = tbo;
	//unbind();
	return true;
}


_oglVAO::VAOPrep::VAOPrep(_oglVAO& vao_) :vao(vao_)
{
	vao.bind();
}

void _oglVAO::VAOPrep::end()
{
	vao.unbind();
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::set(const oglBuffer& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset)
{
	assert(vbo->bufferType == BufferType::Array);
	if (attridx != GL_INVALID_INDEX)
	{
		vbo->bind();
		glEnableVertexAttribArray(attridx);//vertex attr index
		glVertexAttribPointer(attridx, size, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	}
	return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::set(const oglBuffer& vbo, const GLint(&attridx)[3], const GLint offset)
{
	assert(vbo->bufferType == BufferType::Array);
	vbo->bind();
	if (attridx[0] != GL_INVALID_INDEX)
	{
		glEnableVertexAttribArray(attridx[0]);//VertPos
		glVertexAttribPointer(attridx[0], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)offset);
	}
	if (attridx[1] != GL_INVALID_INDEX)
	{
		glEnableVertexAttribArray(attridx[1]);//VertNorm
		glVertexAttribPointer(attridx[1], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)(offset + 16));
	}
	if (attridx[2] != GL_INVALID_INDEX)
	{
		glEnableVertexAttribArray(attridx[2]);//TexPos
		glVertexAttribPointer(attridx[2], 3, GL_FLOAT, GL_FALSE, sizeof(b3d::Point), (void*)(offset + 32));
	}
	return *this;
}

_oglVAO::VAOPrep& _oglVAO::VAOPrep::setIndex(const oglEBO& ebo)
{
	ebo->bind();
	vao.index = ebo;
	vao.initSize();
	return *this;
}


void _oglVAO::bind() const
{
	glBindVertexArray(vaoID);
}

void _oglVAO::unbind()
{
	glBindVertexArray(0);
}

void _oglVAO::initSize()
{
	if (index)
	{
		drawMethod = sizes.size() > 1 ? DrawMethod::Indexs : DrawMethod::Index;
		poffsets.clear();
		for (const auto& off : offsets)
			poffsets.push_back((void*)(off * index->idxsize));
	}
	else
	{
		drawMethod = sizes.size() > 1 ? DrawMethod::Arrays : DrawMethod::Array;
		ioffsets.clear();
		for (const auto& off : offsets)
			ioffsets.push_back((GLint)off);
	}
}

_oglVAO::_oglVAO(const VAODrawMode _mode) :vaoMode(_mode)
{
	glGenVertexArrays(1, &vaoID);
}

_oglVAO::~_oglVAO()
{
	glDeleteVertexArrays(1, &vaoID);
}

_oglVAO::VAOPrep _oglVAO::prepare()
{
	return VAOPrep(*this);
}

void _oglVAO::setDrawSize(const uint32_t offset_, const uint32_t size_)
{
	sizes = { (GLsizei)size_ };
	offsets = { offset_ };
	initSize();
}

void _oglVAO::setDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_)
{
	offsets = offset_;
	sizes.clear();
	for (const auto& s : size_)
		sizes.push_back((GLsizei)s);
	initSize();
}

void _oglVAO::draw(const uint32_t size, const uint32_t offset) const
{
	bind();
	if (index)
		glDrawElements((GLenum)vaoMode, size, (GLenum)index->idxtype, (void*)(offset * index->idxsize));
	else
		glDrawArrays((GLenum)vaoMode, offset, size);
}

void _oglVAO::draw() const
{
	bind();
	switch (drawMethod)
	{
	case DrawMethod::Array:
		glDrawArrays((GLenum)vaoMode, ioffsets[0], sizes[0]);
		break;
	case DrawMethod::Index:
		glDrawElements((GLenum)vaoMode, sizes[0], (GLenum)index->idxtype, poffsets[0]);
		break;
	case DrawMethod::Arrays:
		glMultiDrawArrays((GLenum)vaoMode, ioffsets.data(), sizes.data(), (GLsizei)sizes.size());
		break;
	case DrawMethod::Indexs:
		glMultiDrawElements((GLenum)vaoMode, sizes.data(), (GLenum)index->idxtype, poffsets.data(), (GLsizei)sizes.size());
		break;
	}
}

}
