#pragma once
#include "oglRely.h"
#include "oglBuffer.h"

namespace oglu
{


enum class VAODrawMode : GLenum
{
    Triangles = GL_TRIANGLES
};


namespace detail
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
        bool isEmpty;
        VAOPrep(_oglVAO& vao_) noexcept;
    public:
        VAOPrep(VAOPrep&& other) : vao(other.vao), isEmpty(other.isEmpty) { other.isEmpty = true; }
        ~VAOPrep() noexcept { End(); }
        void End() noexcept;
        ///<summary>Set single Vertex Attribute</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="attridx">vertex attribute index</param>
        ///<param name="stride">size(byte) of a group of data</param>
        ///<param name="size">count of float taken as an element</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        VAOPrep& Set(const oglBuffer& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset);
        ///<summary>Set Vertex Attribute [VertexPos, VertexNormal, TexCoord]</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array, data should be [b3d::Point]</param>
        ///<param name="attridx">vertex attribute index x3</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        VAOPrep& Set(const oglBuffer& vbo, const GLint(&attridx)[3], const GLint offset);
        ///<summary>Set Vertex Attribute [VertexPos, VertexNormal, TexCoord, VertexTangent]</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array, data should be [b3d::PointEx]</param>
        ///<param name="attridx">vertex attribute index x3</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        VAOPrep& Set(const oglBuffer& vbo, const GLint(&attridx)[4], const GLint offset);
        ///<summary>Set Indexed buffer</summary>  
        ///<param name="ebo">element buffer</param>
        VAOPrep& SetIndex(const oglEBO& ebo);
    };
    VAODrawMode vaoMode;
    DrawMethod drawMethod = DrawMethod::UnPrepared;
    GLuint vaoID = GL_INVALID_INDEX;
    oglEBO IndexBuffer;
    vector<GLsizei> sizes;
    vector<void*> poffsets;
    vector<GLint> ioffsets;
    vector<uint32_t> offsets;
    void bind() const noexcept;
    static void unbind() noexcept;
    void InitSize();
public:
    _oglVAO(const VAODrawMode) noexcept;
    ~_oglVAO() noexcept;

    VAOPrep Prepare() noexcept;
    ///<summary>Set draw size(using DrawElement)</summary>  
    ///<param name="offset_">offset</param>
    ///<param name="size_">size</param>
    void SetDrawSize(const uint32_t offset_, const uint32_t size_);
    ///<summary>Set draw size(using MultyDrawElement)</summary>  
    ///<param name="offset_">offsets</param>
    ///<param name="size_">sizes</param>
    void SetDrawSize(const vector<uint32_t> offset_, const vector<uint32_t> size_);
    void Draw(const uint32_t size, const uint32_t offset = 0) const noexcept;
    void Draw() const noexcept;
};


}
using oglVAO = Wrapper<detail::_oglVAO>;


}