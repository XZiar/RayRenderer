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
class oglDrawProgram_;

class oglVAO_;
using oglVAO = std::shared_ptr<oglVAO_>;


enum class VAODrawMode : GLenum
{
    Triangles = 0x0004/*GL_TRIANGLES*/
};

enum class VAValType : uint8_t { Double, Float, Half, U32, I32, U16, I16, U8, I8, U10_2, I10_2, UF11_10 };

template<VAValType ValType_, bool IsNormalize_, bool AsInteger_, uint8_t Size_, size_t Offset_>
struct VARawComponent
{
    static constexpr auto ValType = ValType_;
    static constexpr auto IsNormalize = IsNormalize_;
    static constexpr auto AsInteger = AsInteger_;
    static constexpr auto Size = Size_;
    static constexpr auto Offset = Offset_;
    static_assert(Size > 0 && Size <= 4, "Size should be [1~4]");
    static_assert(ValType != VAValType::I10_2   || Size != 4, "GL_INT_2_10_10_10_REV only accept size of 4");
    static_assert(ValType != VAValType::U10_2   || Size != 4, "GL_UNSIGNED_INT_2_10_10_10_REV only accept size of 4");
    static_assert(ValType != VAValType::UF11_10 || Size != 3, "GL_UNSIGNED_INT_10F_11F_11F_REV only accept size of 3");

    //static_assert(ValType != GL_INT_2_10_10_10_REV || Size != 4, "GL_INT_2_10_10_10_REV only accept size of 4");
    //static_assert(ValType != GL_UNSIGNED_INT_2_10_10_10_REV || Size != 4, "GL_UNSIGNED_INT_2_10_10_10_REV only accept size of 4");
    //static_assert(ValType != GL_UNSIGNED_INT_10F_11F_11F_REV || Size != 3, "GL_UNSIGNED_INT_10F_11F_11F_REV  only accept size of 3");
};
template<typename Val>
inline constexpr VAValType ParseType()
{
         if constexpr (std::is_same_v<Val, float>)      return VAValType::Float;
    else if constexpr (std::is_same_v<Val, b3d::half>)  return VAValType::Half;
    else if constexpr (std::is_same_v<Val, uint32_t>)   return VAValType::U32;
    else if constexpr (std::is_same_v<Val, int32_t>)    return VAValType::I32;
    else if constexpr (std::is_same_v<Val, uint16_t>)   return VAValType::U16;
    else if constexpr (std::is_same_v<Val, int16_t>)    return VAValType::I16;
    else if constexpr (std::is_same_v<Val, uint8_t>)    return VAValType::U8;
    else if constexpr (std::is_same_v<Val, int8_t>)     return VAValType::I8;
    else static_assert(!common::AlwaysTrue<Val>, "unsupported type");
}
template<typename T, bool IsNormalize_, uint8_t Size_, size_t Offset_>
struct VAComponent : public VARawComponent<ParseType<T>(), IsNormalize_, IsNormalize_ ? false : std::is_integral_v<T>, Size_, Offset_>
{
    static_assert(std::is_integral_v<T> || !IsNormalize_, "Float type cannot be set as Integer");
};

namespace detail
{
template<typename T>
struct IsVAComp : public std::false_type {};
template <VAValType ValType_, bool IsNormalize_, bool AsInteger_, uint8_t Size_, size_t Offset_>
struct IsVAComp<VARawComponent<ValType_, IsNormalize_, AsInteger_, Size_, Offset_>> : public std::true_type {};
template<typename T, bool IsNormalize_, uint8_t Size_, size_t Offset_>
struct IsVAComp<VAComponent<T, IsNormalize_, Size_, Offset_>> : public std::true_type {};
}


class OGLUAPI oglVAO_ : 
    public common::NonMovable, 
    public detail::oglCtxObject<false>  // GL Container objects
{
    friend class oglProgram_;
private:
    MAKE_ENABLER();
    enum class DrawMethod : uint8_t { UnPrepared, Array, Arrays, Index, Indexs, IndirectArrays, IndirectIndexes };
    std::u16string Name;
    std::variant<std::monostate, GLsizei, std::vector<GLsizei>> Count;
    std::variant<std::monostate, const void*, GLint, std::vector<const void*>, std::vector<GLint>> Offsets;
    oglEBO IndexBuffer;
    oglIBO IndirectBuffer;
    GLuint VAOId;
    VAODrawMode DrawMode;
    DrawMethod Method = DrawMethod::UnPrepared;
    oglVAO_(const VAODrawMode) noexcept;
    void bind() const noexcept;
    static void unbind() noexcept;
public:
    class OGLUAPI VAOPrep : public common::NonCopyable
    {
        friend class oglVAO_;
    private:
        oglVAO_* VAO;
        const oglDrawProgram_& Prog;
        VAOPrep(oglVAO_* vao, const oglDrawProgram_& prog) noexcept;
        GLint GetLoc(const std::string_view name) const noexcept;
        void GetLocs(const common::span<const std::string_view> names, const common::span<GLint> idxs) const noexcept;
        void SetInteger(const VAValType valType, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor);
        void SetFloat(const VAValType valType, const bool isNormalize, const GLint attridx, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor);
        template<typename T>
        void SetAttrib(const uint16_t eleSize, const size_t offset, const GLint attridx)
        {
            static_assert(detail::IsVAComp<T>::value, "Attribe descriptor should be VARawComponent or VAComponent");
            if constexpr(T::AsInteger)
                SetInteger(T::ValType, attridx, eleSize, T::Size, offset + T::Offset, 0);
            else
                SetFloat(T::ValType, T::IsNormalize, attridx, eleSize, T::Size, offset + T::Offset, 0);
        }
        template<typename Tuple, size_t N, std::size_t... Indexes>
        void SetAttribs(const uint16_t eleSize, const size_t offset, const GLint(&attridx)[N], std::index_sequence<Indexes...>)
        {
            (SetAttrib<std::tuple_element_t<Indexes, Tuple>>(eleSize, offset, attridx[Indexes]), ...);
        }
    public:
        VAOPrep(VAOPrep&& other) noexcept : VAO(other.VAO), Prog(other.Prog) { other.VAO = nullptr; }
        ~VAOPrep();
        ///<summary>Set single Vertex Attribute(integer)</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="attr">vertex attribute index or name</param>
        ///<param name="stride">size(byte) of a group of data</param>
        ///<param name="size">count of {ValueType} taken as an element</param>
        ///<param name="offset">offset(byte) of the 1st elements</param>
        ///<param name="divisor">increase attri index foreach {x} instance</param>
        template<typename Attr, typename Val = uint32_t>
        VAOPrep& SetInteger(const oglVBO& vbo, const Attr& attr, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor = 0)
        {
            static_assert(std::is_integral_v<Val>, "Only integral types are allowed when using SetInteger.");
            if constexpr (std::is_same_v<Attr, GLint>)
            {
                VAO->CheckCurrent();
                vbo->bind();
                SetInteger(ParseType<Val>(), attr, stride, size, offset, divisor);
                return *this;
            }
            else
                return SetInteger(vbo, GetLoc(attr), stride, size, offset, divisor);
        }
        ///<summary>Set single Vertex Attribute(float)</summary>  
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="attr">vertex attribute index or name</param>
        ///<param name="stride">size(byte) of a group of data</param>
        ///<param name="size">count of {ValueType} taken as an element</param>
        ///<param name="offset">offset(byte) of the 1st elements</param>
        ///<param name="divisor">increase attri index foreach {x} instance</param>
        template<typename Attr, typename Val = float>
        VAOPrep& SetFloat(const oglVBO& vbo, const Attr& attr, const uint16_t stride, const uint8_t size, const size_t offset, GLuint divisor = 0)
        {
            if constexpr (std::is_same_v<Attr, GLint>)
            {
                VAO->CheckCurrent();
                vbo->bind();
                SetFloat(ParseType<Val>(), std::is_integral_v<Val>, attr, stride, size, offset, divisor);
                return *this;
            }
            else
                return SetFloat(vbo, GetLoc(attr), stride, size, offset, divisor);
        }
        ///<summary>Set multiple Vertex Attributed</summary>  
        ///<typeparam name="T">Data Type, should contain ComponentType member type as attributes mapping</typeparam>
        ///<param name="vbo">vertex attribute datasource, must be array</param>
        ///<param name="offset">offset(byte) od the 1st elements</param>
        ///<param name="attr">vertex attribute indexes or names</param>
        template<typename T, typename Attr, size_t N, typename C = typename T::ComponentType>
        VAOPrep& SetAttribs(const oglVBO& vbo, const size_t offset, const Attr(&attr)[N])
        {
            static_assert(common::is_specialization<C, std::tuple>::value, "ComponentType should be tuple of VAComponent");
            static_assert(std::tuple_size_v<C> == N, "attrib index size mismatch with component count");
            VAO->CheckCurrent();
            vbo->bind();
            if constexpr (std::is_same_v<Attr, GLint>)
            {
                SetAttribs<C>(static_cast<uint16_t>(sizeof(T)), offset, attr, std::make_index_sequence<N>{});
            }
            else
            {
                GLint attrIdx[N] = { GLInvalidIndex };
                GetLocs(attr, attrIdx);
                SetAttribs<C>(static_cast<uint16_t>(sizeof(T)), offset, attrIdx, std::make_index_sequence<N>{});
            }
            return *this;
        }
        ///<summary>Set DrawId</summary>  
        ///<param name="attridx">drawID attribute index</param>
        VAOPrep& SetDrawId(const GLint attridx);
        ///<summary>Set DrawId</summary>  
        VAOPrep& SetDrawId();
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
        VAOPrep& SetDrawSizes(const std::vector<uint32_t>& offsets, const std::vector<uint32_t>& sizes);
        ///<summary>Set Indirect buffer</summary>  
        ///<param name="ibo">indirect buffer</param>
        VAOPrep& SetDrawSizeFrom(const oglIBO& ibo, GLint offset = 0, GLsizei size = 0);
    };
    ~oglVAO_() noexcept;

    ///<summary>Prepare VAO</summary>  
    ///<param name="prog">drawProgram</param>
    [[nodiscard]] VAOPrep Prepare(const std::shared_ptr<oglDrawProgram_>& prog) noexcept;
    void SetName(std::u16string name) noexcept;
    void RangeDraw(const uint32_t size, const uint32_t offset = 0) const noexcept;
    void Draw() const noexcept;
    void InstanceDraw(const uint32_t count, const uint32_t base = 0) const noexcept;
    [[nodiscard]] std::u16string_view GetName() const noexcept { return Name; }

    void Test() const noexcept;

    [[nodiscard]] static oglVAO Create(const VAODrawMode);
};




}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif