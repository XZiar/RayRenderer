#include "oglRely.h"
#include "oglVAO.h"

namespace oglu::inner
{


_oglVAO::VAOPrep::VAOPrep(_oglVAO& vao_) noexcept :vao(vao_)
{
	vao.bind();
}

void _oglVAO::VAOPrep::end() noexcept
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


void _oglVAO::bind() const noexcept
{
	glBindVertexArray(vaoID);
}

void _oglVAO::unbind() noexcept
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

_oglVAO::_oglVAO(const VAODrawMode _mode) noexcept :vaoMode(_mode)
{
	glGenVertexArrays(1, &vaoID);
}

_oglVAO::~_oglVAO() noexcept
{
	glDeleteVertexArrays(1, &vaoID);
}

_oglVAO::VAOPrep _oglVAO::prepare() noexcept
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

void _oglVAO::draw(const uint32_t size, const uint32_t offset) const noexcept
{
	bind();
	if (index)
		glDrawElements((GLenum)vaoMode, size, (GLenum)index->idxtype, (void*)(offset * index->idxsize));
	else
		glDrawArrays((GLenum)vaoMode, offset, size);
}

void _oglVAO::draw() const noexcept
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
