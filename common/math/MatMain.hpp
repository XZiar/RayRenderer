

namespace mat
{

template<typename E, size_t N, typename V, typename T>
struct COMMON_EMPTY_BASES FuncBasics : public MatBasic<E, N, V, T>,
    public FuncSAddSubMulDiv<E, N, V, T>, public FuncVAddSubMulDiv<E, N, V, T>, public FuncMAddSubMulDiv<E, N, V, T>,
    public shared::FuncSelfCalc<T>, public FuncMinMax<N, T>
{ };


}

struct COMMON_EMPTY_BASES alignas(32) Mat3 : public rule::ElementBasic<float, 9, 16>, public mat::Mat4x4Base<float, Vec3>,
    public mat::FuncBasics<float, 3, Vec3, Mat3>, public mat::FuncNegative<3, Mat3>, public mat::FuncFPMath<3, Mat3>
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
    /*template<typename Arg, typename... Args>
    constexpr Mat3(Arg&& arg, Args&&... args) noexcept :
        Mat3(rule::Concater<Mat3>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}*/
private:
    constexpr Mat3(const VecType& x, const VecType& y, const VecType& z, const VecType& w) noexcept : Mat4x4Base(x, y, z, w) { }
};

struct COMMON_EMPTY_BASES alignas(32) Mat4 : public rule::ElementBasic<float, 16, 16>, public mat::Mat4x4Base<float, Vec4>,
    public mat::FuncBasics<float, 4, Vec4, Mat4>, public mat::FuncNegative<4, Mat4>, public mat::FuncFPMath<4, Mat4>
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
    /*template<typename Arg, typename... Args>
    constexpr Mat3(Arg&& arg, Args&&... args) noexcept :
        Mat3(rule::Concater<Mat3>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}*/
};
