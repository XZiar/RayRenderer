#pragma once
#include "oglRely.h"
#include "oglBuffer.h"
#include "3DElement.hpp"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu
{

enum class VAODrawMode : GLenum
{
    Triangles = 0x0004/*GL_TRIANGLES*/
};

template<GLenum ValType_, bool IsNormalize_, bool AsInteger_, uint8_t Size_, size_t Offset_>
struct VARawComponent
{
    static constexpr auto ValType = ValType_;
    static constexpr auto IsNormalize = IsNormalize_;
    static constexpr auto AsInteger = AsInteger_;
    static constexpr auto Size = Size_;
    static constexpr auto Offset = Offset_;
    static_assert(Size > 0 && Size <= 4, "Size should be [1~4]");
    static_assert(ValType != GL_INT_2_10_10_10_REV || Size != 4, "GL_INT_2_10_10_10_REV only accept size of 4");
    static_assert(ValType != GL_UNSIGNED_INT_2_10_10_10_REV || Size != 4, "GL_UNSIGNED_INT_2_10_10_10_REV only accept size of 4");
    static_assert(ValType != GL_UNSIGNED_INT_10F_11F_11F_REV || Size != 3, "GL_UNSIGNED_INT_10F_11F_11F_REV  only accept size of 3");
};
template<typename Val>
inline constexpr GLenum ParseType()
{
    if constexpr (std::is_same_v<Val, float>) return GL_FLOAT;
    else if constexpr (std::is_same_v<Val, b3d::half>) return GL_HALF_FLOAT;
    else if constexpr (std::is_same_v<Val, uint32_t>) return GL_UNSIGNED_INT;
    else if constexpr (std::is_same_v<Val, int32_t>) return GL_INT;
    else if constexpr (std::is_same_v<Val, uint16_t>) return GL_UNSIGNED_SHORT;
    else if constexpr (std::is_same_v<Val, int16_t>) return GL_SHORT;
    else if constexpr (std::is_same_v<Val, uint8_t>) return GL_UNSIGNED_BYTE;
    else if constexpr (std::is_same_v<Val, int8_t>) return GL_BYTE;
    else static_assert(common::AlwaysTrue<Val>(), "unsupported type");
}
template<typename T, bool IsNormalize_, uint8_t Size_, size_t Offset_>
struct VAComponent : public VARawComponent<ParseType<T>(), IsNormalize_, IsNormalize_ ? false : std::is_integral_v<T>, Size_, Offset_>
{
    static_assert(std::is_integral_v<T> || !IsNormalize_, "Float type cannot be set as Integer");
};

namespace detail
{
class _oglDrawProgram;
template<typename T>
struct IsVAComp : public std::false_type {};
template <GLenum ValType_, bool IsNormalize_, bool AsInteger_, uint8_t Size_, size_t Offset_>
struct IsVAComp<VARawComponent<ValType_, IsNormalize_, AsInteger_, Size_, Offset_>> : public std::true_type {};
template<typename T, bool IsNormalize_, uint8_t Size_, size_t Offset_>
struct IsVAComp<VAComponent<T, IsNormalize_, Size_, Offset_>> : public std::true_type {};

class OGLUAPI _oglVAO : public NonMovable, public oglCtxObject<false>
{
protected:
    friend class _oglProgram;
    enum class DrawMethod : uint8_t { UnPrepared, Array, Arrays, Index, Indexs, IndirectArrays, IndirectIndexes };
    std::variant<std::monostate, GLsizei, std::vector<GLsizei>> Count;
    std::variant<std::monostate, const void*, GLint, vector<const void*>, vector<GLint>> Offsets;
    oglEBO IndexBuffer;
    oglIBO IndirectBuffer;
    GLuint VAOId;
    VAODrawMode DrawMode;
    DrawMethod Method = DrawMethod::UnPrepared;
    void bind() const noexcept;
    static void unbind() noexcept;
public:
    class OGLUAPI VAOPrep : public NonCopyable
    {
        friend class _oglVAO;
    private:
        _oglVAO& vao;
        bool isEmpty;
        VAOPrep(_oglVAO& vao_) noexcept;
        void SetInteger(const GLenum valType, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor);
        void SetFloat(const GLenum valType, const bool isNormalize, const GLint attridx, const uint16_t stride, const uint8_t size, const GLint offset, GLuint divisor);
        template<typename T>
        void SetAttrib(const uint16_t eleSize, const GLint offset, const GLint attridx)
        {
            static_assert(IsVAComp<T>::value, "Attribe descriptor should be VARawComponent or VAComponent");
            if constexpr(T::AsInteger)
                SetInteger(T::ValType, attridx, eleSize, T::Size, offset + T::Offset, 0);
            else
                SetFloat(T::ValType, T::IsNormalize, attridx, eleSize, T::Size, offset + T::Offset, 0);
        }
        template<typename Tuple, size_t N, std::size_t... Indexes>
        void SetAttribs(const uint16_t eleSize, const GLint offset, const GLint(&attridx)[N], std::index_sequence<Indexes...>)
        {
            (SetAttrib<std::tuple_element_t<Indexes, Tuple>>(eleSize, offset, attridx[Indexes]), ...);
        }
    public:
        VAOPrep(VAOPrep&& other) noexcept : vao(other.vao), isEmpty(other.isEmpty) { other.isEmpty = true; }
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
            vao.CheckCurrent();
            vbo->bind();
            SetInteger(ParseType<Val>(), attridx, stride, size, offset, divisor);
            return *this;
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
            vao.CheckCurrent();
            vbo->bind();
            SetFloat(ParseType<Val>(), std::is_integral_v<Val>, attridx, stride, size, offset, divisor);
            return *this;
        }
        ///<summary>Set multiple Vertex Attributed</summary>  
        ///<typeparam name="T">Data Type, should contain ComponentType member type as attributes mapping</param>
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        ///<param name="attridx">vertex attribute index</param>
        template<typename T, size_t N, typename C = typename T::ComponentType>
        VAOPrep& SetAttribs(const oglVBO& vbo, const GLint offset, const GLint(&attridx)[N])
        {
            static_assert(common::is_specialization<C, std::tuple>::value, "ComponentType should be tuple of VAComponent");
            static_assert(std::tuple_size_v<C> == N, "attrib index size mismatch with component count");
            vao.CheckCurrent();
            vbo->bind();
            SetAttribs<C>(static_cast<uint16_t>(sizeof(T)), offset, attridx, std::make_index_sequence<N>{});
            return *this;
        }
        ///<summary>Set DrawId</summary>  
        ///<param name="attridx">drawID attribute index</param>
        VAOPrep& SetDrawId(const GLint attridx);
        ///<summary>Set DrawId</summary>  
        ///<param name="prog">drawProgram</param>
        VAOPrep& SetDrawId(const Wrapper<_oglDrawProgram>& prog);
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
        VAOPrep& SetDrawSize(const oglIBO& ibo, GLint offset = 0, GLsizei size = 0);
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

#if COMPILER_MSVC
#   pragma warning(pop)
#endif