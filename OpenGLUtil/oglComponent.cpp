#include "oglComponent.h"
#include "BindingManager.h"

namespace oglu::inner
{

string _oglShader::loadFromFile(FILE * fp)
{
	fseek(fp, 0, SEEK_END);
	const auto fsize = ftell(fp);
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

void _oglTexture::parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype)
{
	switch (format)
	{
	case TextureFormat::RGB:
		intertype = GL_RGB;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGB;
		break;
	case TextureFormat::RGBA:
		intertype = GL_RGBA;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGBA;
		break;
	case TextureFormat::RGBf:
		intertype = GL_RGB32F;
		datatype = GL_FLOAT;
		comptype = GL_RGB;
		break;
	case TextureFormat::RGBAf:
		intertype = GL_RGBA32F;
		datatype = GL_FLOAT;
		comptype = GL_RGBA;
		break;
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

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void * data)
{
	bind(defPos);
	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, data);
	//unbind();
}

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	assert(buf->bufferType == BufferType::Pixel);
	bind(defPos);
	buf->bind();

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, NULL);

	//unbind();
}


OPResult<> _oglTexture::setBuffer(const TextureFormat format, const oglBuffer& tbo)
{
	if (type != TextureType::TexBuf)
		return OPResult<>(false, L"Texture is not TextureBuffer");
	if(tbo->bufferType != BufferType::Texture)
		return OPResult<>(false, L"Buffer is not TextureBuffer");
	bind(defPos);
	glTexBuffer(GL_TEXTURE_BUFFER, (GLenum)format, tbo->bufferID);
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

_oglVAO::VAOPrep& _oglVAO::VAOPrep::setIndex(const oglBuffer& ebo, const IndexSize idxSize)
{
	assert(ebo->bufferType == BufferType::Element);
	ebo->bind();
	vao.indexType = idxSize;
	vao.indexSizeof = (idxSize == IndexSize::Byte ? 1 : (idxSize == IndexSize::Short ? 2 : 4));
	vao.index = ebo;
	vao.initSize();
	return *this;
}


void _oglVAO::bind() const
{
	glBindVertexArray(vaoID);
}

void _oglVAO::unbind() const
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
			poffsets.push_back((void*)(off * indexSizeof));
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
		glDrawElements((GLenum)vaoMode, size, (GLenum)indexType, (void*)(offset * indexSizeof));
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
		glDrawElements((GLenum)vaoMode, sizes[0], (GLenum)indexType, poffsets[0]);
		break;
	case DrawMethod::Arrays:
		glMultiDrawArrays((GLenum)vaoMode, ioffsets.data(), sizes.data(), (GLsizei)sizes.size());
		break;
	case DrawMethod::Indexs:
		glMultiDrawElements((GLenum)vaoMode, sizes.data(), (GLenum)indexType, poffsets.data(), (GLsizei)sizes.size());
		break;
	}
}

}
