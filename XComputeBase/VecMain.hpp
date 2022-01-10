
namespace vec
{

template<typename E, size_t N, typename T>
struct FuncDot : public FuncDotBase<E, N, T>
{
    forceinline constexpr E LengthSqr() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return Dot(self, self);
    }
    forceinline constexpr E Length() const noexcept
    {
        return std::sqrt(LengthSqr());
    }
    forceinline constexpr T Normalize() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return self / Length();
    }
};

template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES FuncBasics : public VecBasic<E, N, T>,
    public FuncSAddSubMulDiv<E, N, T>, public FuncVAddSubMulDiv<E, N, T>, public shared::FuncSelfCalc<      T>,
    public FuncDot<E, N, T>, public FuncMinMax<   N, T>
{ };

}


#ifdef ENABLE_VEC2
struct COMMON_EMPTY_BASES alignas(8) Vec2 : public rule::ElementBasic<float, 2, 2>, public vec::Vec2Base<float>, 
    public vec::FuncBasics<float, 2, Vec2>, public vec::FuncNegative<2, Vec2>, public vec::FuncFPMath<2, Vec2>
{
    using EleType = float;
    using Vec2Base::X;
    using Vec2Base::Y;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec2Base::Vec2Base;
    constexpr Vec2(EleType x, EleType y) noexcept : Vec2Base(x, y) { }
    constexpr Vec2(EleType all) noexcept : Vec2Base(all, all) { }
    template<typename Arg, typename... Args>
    constexpr Vec2(Arg&& arg, Args&&... args) noexcept :
        Vec2(rule::Concater<Vec2>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

struct COMMON_EMPTY_BASES alignas(8) IVec2 : public rule::ElementBasic<int32_t, 2, 2>, public vec::Vec2Base<int32_t>, 
    public vec::FuncBasics<int32_t, 2, IVec2>, public vec::FuncNegative<2, IVec2>
{
    using EleType = int32_t;
    using Vec2Base::X;
    using Vec2Base::Y;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec2Base::Vec2Base;
    constexpr IVec2(EleType x, EleType y) noexcept : Vec2Base(x, y) { }
    constexpr IVec2(EleType all) noexcept : Vec2Base(all, all) { }
    template<typename Arg, typename... Args>
    constexpr IVec2(Arg&& arg, Args&&... args) noexcept :
        IVec2(rule::Concater<Vec2>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

struct COMMON_EMPTY_BASES alignas(8) UVec2 : public rule::ElementBasic<uint32_t, 2, 2>, public vec::Vec2Base<uint32_t>,
    public vec::FuncBasics<uint32_t, 2, UVec2>
{
    using EleType = uint32_t;
    using Vec2Base::X;
    using Vec2Base::Y;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec2Base::Vec2Base;
    constexpr UVec2(EleType x, EleType y) noexcept : Vec2Base(x, y) { }
    constexpr UVec2(EleType all) noexcept : Vec2Base(all, all) { }
    template<typename Arg, typename... Args>
    constexpr UVec2(Arg&& arg, Args&&... args) noexcept :
        UVec2(rule::Concater<Vec2>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};
#endif

struct COMMON_EMPTY_BASES alignas(16) Vec3 : public rule::ElementBasic<float, 3, 4>, public vec::Vec4Base<float>, 
    public vec::FuncBasics<float, 3, Vec3>, public vec::FuncNegative<3, Vec3>, public vec::FuncFPMath<3, Vec3>,
    public vec::FuncCross<Vec3>
{
    using EleType = float;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr Vec3(EleType x, EleType y, EleType z) noexcept : Vec4Base(x, y, z, 0.f) { }
    constexpr Vec3(EleType all) noexcept : Vec4Base(all, all, all, 0.f) { }
    template<typename Arg, typename... Args>
    constexpr Vec3(Arg&& arg, Args&&... args) noexcept :
        Vec3(rule::Concater<Vec3>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

struct COMMON_EMPTY_BASES alignas(16) IVec3 : public rule::ElementBasic<int32_t, 3, 4>, public vec::Vec4Base<int32_t>,
    public vec::FuncBasics<int32_t, 3, IVec3>, public vec::FuncNegative<3, IVec3>
{
    using EleType = int32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr IVec3(EleType x, EleType y, EleType z) noexcept : Vec4Base(x, y, z, 0) { }
    constexpr IVec3(EleType all) noexcept : Vec4Base(all, all, all, 0) { }
    template<typename Arg, typename... Args>
    constexpr IVec3(Arg&& arg, Args&&... args) noexcept :
        IVec3(rule::Concater<IVec3>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

struct COMMON_EMPTY_BASES alignas(16) UVec3 : public rule::ElementBasic<uint32_t, 3, 4>, public vec::Vec4Base<uint32_t>,
    public vec::FuncBasics<uint32_t, 3, UVec3>
{
    using EleType = uint32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr UVec3(EleType x, EleType y, EleType z) noexcept : Vec4Base(x, y, z, 0) { }
    constexpr UVec3(EleType all) noexcept : Vec4Base(all, all, all, 0) { }
    template<typename Arg, typename... Args>
    constexpr UVec3(Arg&& arg, Args&&... args) noexcept :
        UVec3(rule::Concater<IVec3>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

struct Normal : public Vec3
{
    constexpr Normal() noexcept : Vec3() {}
    constexpr Normal(const Vec3& v) noexcept : Vec3(v.Normalize()) { }
    constexpr Normal(float x, float y, float z) noexcept : Vec3(x, y, z) 
    {
        Normalize();
    }
    constexpr Normal& operator=(const Vec3& v) noexcept 
    {
        *static_cast<Vec3*>(this) = v.Normalize();
        return *this;
    }
};


struct COMMON_EMPTY_BASES alignas(16) Vec4 : public rule::ElementBasic<float, 4, 4>, public vec::Vec4Base<float>, 
    public vec::FuncBasics<float, 4, Vec4>, public vec::FuncNegative<4, Vec4>, public vec::FuncFPMath<4, Vec4>
{
    using EleType = float;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr Vec4(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr Vec4(EleType all) noexcept : Vec4Base(all, all, all, all) { }
    template<typename Arg, typename... Args>
    constexpr Vec4(Arg&& arg, Args&&... args) noexcept :
        Vec4(rule::Concater<Vec4>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};


struct COMMON_EMPTY_BASES alignas(16) IVec4 : public rule::ElementBasic<int32_t, 4, 4>, public vec::Vec4Base<int32_t>, 
    public vec::FuncBasics<int32_t, 4, IVec4>, public vec::FuncNegative<4, IVec4>
{
    using EleType = int32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr IVec4(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr IVec4(EleType all) noexcept : Vec4Base(all, all, all, all) { }
    template<typename Arg, typename... Args>
    constexpr IVec4(Arg&& arg, Args&&... args) noexcept :
        IVec4(rule::Concater<IVec4>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};


struct COMMON_EMPTY_BASES alignas(16) UVec4 : public rule::ElementBasic<uint32_t, 4, 4>, public vec::Vec4Base<uint32_t>, 
    public vec::FuncBasics<uint32_t, 4, UVec4>
{
    using EleType = uint32_t;
    using Vec4Base::X;
    using Vec4Base::Y;
    using Vec4Base::Z;
    using Vec4Base::W;
#ifdef VEC_EXTRA_DEF
    VEC_EXTRA_DEF
#endif
    using Vec4Base::Vec4Base;
    constexpr UVec4(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
    constexpr UVec4(EleType all) noexcept : Vec4Base(all, all, all, all) { }
    template<typename Arg, typename... Args>
    constexpr UVec4(Arg&& arg, Args&&... args) noexcept :
        UVec4(rule::Concater<UVec4>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}
};

