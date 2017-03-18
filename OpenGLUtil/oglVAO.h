#pragma once
#include "oglRely.h"
#include "oglBuffer.h"

namespace oglu
{


enum class VAODrawMode : GLenum
{
	Triangles = GL_TRIANGLES
};


namespace inner
{


class OGLUAPI _oglVAO : public NonCopyable, public NonMovable
{
protected:
	friend class _oglProgram;
	enum class DrawMethod { UnPrepared, Array, Arrays, Index, Indexs };
	class OGLUAPI VAOPrep : public NonCopyable
	{
	private:
		friend class _oglVAO;
		_oglVAO& vao;
		VAOPrep(_oglVAO& vao_) noexcept;
	public:
		void end() noexcept;
		/*-param  vbo buffer, vertex attr index, stride(number), size(number), offset(byte)
		*Each group contains (stride) byte, (size) float are taken as an element, 1st element is at (offset) byte*/
		VAOPrep& set(const oglBuffer& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset);
		/*vbo's inner data is assumed to be Point, automatic set VertPos,VertNorm,TexPos*/
		VAOPrep& set(const oglBuffer& vbo, const GLint(&attridx)[3], const GLint offset);
		VAOPrep& setIndex(const oglEBO& ebo);
	};
	VAODrawMode vaoMode;
	DrawMethod drawMethod = DrawMethod::UnPrepared;
	GLuint vaoID = GL_INVALID_INDEX;
	oglEBO index;
	//IndexSize indexType;
	//uint8_t indexSizeof;
	vector<GLsizei> sizes;
	vector<void*> poffsets;
	vector<GLint> ioffsets;
	vector<uint32_t> offsets;
	//uint32_t offset, size;
	void bind() const noexcept;
	static void unbind() noexcept;
	void initSize();
public:
	_oglVAO(const VAODrawMode) noexcept;
	~_oglVAO() noexcept;

	VAOPrep prepare() noexcept;
	void setDrawSize(const uint32_t offset_, const uint32_t size_);
	void setDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_);
	void draw(const uint32_t size, const uint32_t offset = 0) const noexcept;
	void draw() const noexcept;
};


}
using oglVAO = Wrapper<inner::_oglVAO>;


}