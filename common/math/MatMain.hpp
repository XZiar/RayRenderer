

namespace mat
{

template<typename E, size_t N, typename T, bool IsFP = std::is_floating_point_v<E>, bool IsSigned = std::is_signed_v<E>>
struct COMMON_EMPTY_BASES EleBasics;
template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES EleBasics<E, N, T, true, true> : public FuncNegative<N, T>, public FuncFPMath<N, T>
{};
template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES EleBasics<E, N, T, false, true> : public FuncNegative<N, T>
{};
template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES EleBasics<E, N, T, false, false>
{};

template<typename E, size_t N, typename V, typename T>
struct COMMON_EMPTY_BASES FuncBasics : public MatBasic<E, N, V, T>, public EleBasics<E, N, T>,
    public FuncSAddSubMulDiv<E, N, V, T>, public FuncVAddSubMulDiv<E, N, V, T>, public FuncMAddSubMulDiv<E, N, V, T>,
    public shared::FuncSelfCalc<T>, public FuncMinMax<N, T>
{ };

}

#define MAT_CC_TP(ele, type, n) template<typename Arg, typename =               \
    std::enable_if_t<std::is_base_of_v<shared::mat::MatType<ele, n>, Arg> &&    \
    !shared::DecayMatchAny<Arg, type>>>                                         \
explicit type(const Arg& arg) noexcept : type(*reinterpret_cast<const type*>(&arg)) {}

struct COMMON_EMPTY_BASES alignas(32) Mat3 : public shared::mat::MatType<float, 9>,
    public mat::Mat4x4Base<float, Vec3>, public mat::FuncBasics<float, 3, Vec3, Mat3>
{
    using EleType = float;
    using VecType = Vec3;
    using Mat4x4Base::X;
    using Mat4x4Base::Y;
    using Mat4x4Base::Z;
#ifdef MAT_EXTRA_DEF
    MAT_EXTRA_DEF
#endif
    using Mat4x4Base::Mat4x4Base;
    constexpr Mat3(const VecType& x, const VecType& y, const VecType& z) noexcept : Mat4x4Base(x, y, z, {}) { }
    constexpr Mat3(const VecType& all) noexcept : Mat4x4Base(all, all, all, {}) { }
    MAT_CC_TP(float, Mat3, 9)
    forceinline static constexpr Mat3 LoadAll(const EleType* ptr) noexcept { return Mat4x4Base::LoadAll(ptr); }
private:
    forceinline constexpr Mat3(const Mat4x4Base& base) noexcept : Mat4x4Base(base) {}
    constexpr Mat3(const VecType& x, const VecType& y, const VecType& z, const VecType& w) noexcept : Mat4x4Base(x, y, z, w) { }
};

struct COMMON_EMPTY_BASES alignas(32) Mat4 : public shared::mat::MatType<float, 16>,
    public mat::Mat4x4Base<float, Vec4>, public mat::FuncBasics<float, 4, Vec4, Mat4>
{
    using EleType = float;
    using VecType = Vec4;
    using Mat4x4Base::X;
    using Mat4x4Base::Y;
    using Mat4x4Base::Z;
    using Mat4x4Base::W;
#ifdef MAT_EXTRA_DEF
    MAT_EXTRA_DEF
#endif
    using Mat4x4Base::Mat4x4Base;
    constexpr Mat4(const VecType& x, const VecType& y, const VecType& z, const VecType& w) noexcept : Mat4x4Base(x, y, z, w) { }
    constexpr Mat4(const VecType& all) noexcept : Mat4x4Base(all, all, all, all) { }
    MAT_CC_TP(float, Mat4, 16)
    forceinline static constexpr Mat4 LoadAll(const EleType* ptr) noexcept { return Mat4x4Base::LoadAll(ptr); }
private:
    forceinline constexpr Mat4(const Mat4x4Base& base) noexcept : Mat4x4Base(base) {}
};

#undef MAT_CC_TP
