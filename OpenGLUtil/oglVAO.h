#pragma once
#include "oglRely.h"
#include "oglBuffer.h"
#include "3DElement.hpp"

namespace oglu
{

enum class VAODrawMode : GLenum
{
    Triangles = GL_TRIANGLES
};


namespace detail
{
class _oglDrawProgram;


class OGLUAPI _oglVAO : public NonMovable, public oglCtxObject<false>
{
protected:
    friend class _oglProgram;
    enum class DrawMethod : uint8_t { UnPrepared, Array, Arrays, Index, Indexs, IndirectArrays, IndirectIndexes };
    std::variant<std::monostate, GLsizei, std::vector<GLsizei>> Count;
    std::variant<std::monostate, const void*, GLint, vector<const void*>, vector<GLint>> Offsets;
    oglEBO IndexBuffer;
    oglIBO IndirectBuffer;
    GLuint VAOId = GL_INVALID_INDEX;
    VAODrawMode DrawMode;
    DrawMethod Method = DrawMethod::UnPrepared;
    void bind() const noexcept;
    static void unbind() noexcept;
public:
    class OGLUAPI VAOPrep : public NonCopyable
    {
        friend class _oglVAO;
    private:
        template<typename Val> constexpr GLenum ParseType() 
        {
            if constexpr(std::is_same_v<Val, float>) return GL_FLOAT;
            else if constexpr (std::is_same_v<Val, b3d::half>) return GL_HALF_FLOAT;
            else if constexpr(std::is_same_v<Val, uint32_t>) return GL_UNSIGNED_INT;
            else if constexpr(std::is_same_v<Val, int32_t>) return GL_INT;
            else static_assert(common::AlwaysTrue<Val>(), "unsupported type");
        }
        _oglVAO& vao;
        bool isEmpty;
        VAOPrep(_oglVAO& vao_) noexcept;
        VAOPrep& SetInteger(const GLenum valType, const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor);
        VAOPrep& SetFloat(const GLenum valType, const bool isInteger, const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor);
    public:
        VAOPrep(VAOPrep&& other) : vao(other.vao), isEmpty(other.isEmpty) { other.isEmpty = true; }
        ~VAOPrep() noexcept { End(); }
        void End() noexcept;
        ///<summary>Set single Vertex Attribute(integer)</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="attridx">vertex attribute index</param>
        ///<param name="stride">size(byte) of a group of data</param>
        ///<param name="size">count of {ValueType} taken as an element</param>
        ///<param name="offset">offset(byte) of the 1st elements</param>
        ///<param name="divisor">increase attri index foreach {x} instance</param>
        template<typename Val = uint32_t>
        VAOPrep& SetInteger(const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor = 0)
        {
            static_assert(std::is_integral_v<Val>, "Only integral types are allowed when using SetInteger.");
            return SetInteger(ParseType<Val>(), vbo, attridx, stride, size, offset, divisor);
        }
        ///<summary>Set single Vertex Attribute(float)</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="attridx">vertex attribute index</param>
        ///<param name="stride">size(byte) of a group of data</param>
        ///<param name="size">count of {ValueType} taken as an element</param>
        ///<param name="offset">offset(byte) of the 1st elements</param>
        ///<param name="divisor">increase attri index foreach {x} instance</param>
        template<typename Val = float>
        VAOPrep& SetFloat(const oglVBO& vbo, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor = 0)
        {
            return SetFloat(ParseType<Val>(), std::is_integral_v<Val>, vbo, attridx, stride, size, offset, divisor);
        }
        ///<summary>Set Vertex Attribute [VertexPos, VertexNormal, TexCoord]</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array, data should be [b3d::Point]</param>
        ///<param name="attridx">vertex attribute index x3</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        VAOPrep& SetPoints(const oglVBO& vbo, const GLint(&attridx)[3], const GLint offset);
        ///<summary>Set Vertex Attribute [VertexPos, VertexNormal, TexCoord, VertexTangent]</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array, data should be [b3d::PointEx]</param>
        ///<param name="attridx">vertex attribute index x4</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        VAOPrep& SetPointExs(const oglVBO& vbo, const GLint(&attridx)[4], const GLint offset);
        ///<summary>Set Indexed buffer</summary>  
        ///<param name="ebo">element buffer</param>
        VAOPrep& SetIndex(const oglEBO& ebo);
        ///<summary>Set draw size(using Draw[Array/Element])</summary>  
        ///<param name="offset">offset</param>
        ///<param name="size">size</param>
        VAOPrep& SetDrawSize(const uint32_t offset, const uint32_t size);
        ///<summary>Set draw size(using MultyDraw[Arrays/Elements])</summary>  
        ///<param name="offsets">offsets</param>
        ///<param name="sizes">sizes</param>
        VAOPrep& SetDrawSize(const vector<uint32_t>& offsets, const vector<uint32_t>& sizes);
        ///<summary>Set Indirect buffer</summary>  
        ///<param name="ibo">indirect buffer</param>
        VAOPrep& SetDrawSize(const oglIBO& ibo);
        ///<summary>Set DrawId</summary>  
        ///<param name="prog">drawProgram</param>
        VAOPrep& SetDrawId(const Wrapper<_oglDrawProgram>& prog);
    };
    _oglVAO(const VAODrawMode) noexcept;
    ~_oglVAO() noexcept;

    VAOPrep Prepare() noexcept;
    void Draw(const uint32_t size, const uint32_t offset = 0) const noexcept;
    void Draw() const noexcept;

    void Test() const noexcept;
};


}
using oglVAO = Wrapper<detail::_oglVAO>;


}