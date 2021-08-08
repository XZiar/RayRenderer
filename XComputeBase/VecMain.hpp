
namespace vec
{

template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES FuncAllAdd : public FuncSAdd<E, N, T, T>, public FuncVAdd<N, T, T, T>, public FuncSelfAdd<T>
{ };


template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES FuncAllSub : public FuncSSub<E, N, T, T>, public FuncVSub<N, T, T, T>, public FuncSelfSub<T>
{ };


template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES FuncAllMulDiv : public FuncSMulDiv<E, N, T, T>, public FuncVMulDiv<N, T, T, T>, public FuncSelfMulDiv<T>
{ };

}

struct COMMON_EMPTY_BASES alignas(16) Vec3 : public vec::Vec4Base<float>, public vec::VecBasic<float, 3, Vec3>,
    public vec::FuncAllAdd<float, 3, Vec3>, public vec::FuncAllSub<float, 3, Vec3>, public vec::FuncAllMulDiv<float, 3, Vec3>,
    public vec::FuncDot<float, 3, Vec3, true>, public vec::FuncCross<Vec3>, public vec::FuncMinMax<3, Vec3>, 
    public vec::FuncFPMath<3, Vec3>, public vec::FuncNegative<3, Vec3>
{
    using EleType = float;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::Vec4Base;
    constexpr Vec3(float x, float y, float z) noexcept : Vec4Base(x, y, z, 0.f) { }
    constexpr Vec3(float all) noexcept : Vec4Base(all, all, all, 0.f) { }
};


struct COMMON_EMPTY_BASES alignas(16) Vec4 : public vec::Vec4Base<float>, public vec::VecBasic<float, 4, Vec4>,
    public vec::FuncAllAdd<float, 4, Vec4>, public vec::FuncAllSub<float, 4, Vec4>, public vec::FuncAllMulDiv<float, 4, Vec4>,
    public vec::FuncDot<float, 4, Vec4, true>, public vec::FuncMinMax<4, Vec4>, 
    public vec::FuncFPMath<4, Vec4>, public vec::FuncNegative<4, Vec4>
{
    using EleType = float;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
    using Vec4Base::Vec4Base;
    constexpr Vec4(float x, float y, float z, float w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr Vec4(float all) noexcept : Vec4Base(all, all, all, all) { }
    constexpr Vec4(Vec3 xyz, float w) noexcept : Vec4Base(xyz.X, xyz.Y, xyz.Z, w) { }
};


struct COMMON_EMPTY_BASES alignas(16) IVec4 : public vec::Vec4Base<int32_t>, public vec::VecBasic<int32_t, 3, IVec4>,
    public vec::FuncAllAdd<int32_t, 4, IVec4>, public vec::FuncAllSub<int32_t, 4, IVec4>, public vec::FuncAllMulDiv<int32_t, 4, IVec4>,
    public vec::FuncDot<int32_t, 4, IVec4, true>, public vec::FuncMinMax<4, IVec4>, 
    public vec::FuncNegative<4, IVec4>
{
    using EleType = int32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
    using Vec4Base::Vec4Base;
    constexpr IVec4(int32_t x, int32_t y, int32_t z, int32_t w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr IVec4(int32_t all) noexcept : Vec4Base(all, all, all, all) { }
};


struct COMMON_EMPTY_BASES alignas(16) UVec4 : public vec::Vec4Base<uint32_t>, public vec::VecBasic<uint32_t, 4, UVec4>,
    public vec::FuncAllAdd<uint32_t, 4, UVec4>, public vec::FuncAllSub<uint32_t, 4, UVec4>, public vec::FuncAllMulDiv<uint32_t, 4, UVec4>,
    public vec::FuncDot<uint32_t, 4, UVec4, true>, public vec::FuncMinMax<4, UVec4>
{
    using EleType = uint32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
    using Vec4Base::Vec4Base;
    constexpr UVec4(uint32_t x, uint32_t y, uint32_t z, uint32_t w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr UVec4(uint32_t all) noexcept : Vec4Base(all, all, all, all) { }
};

