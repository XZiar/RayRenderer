
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

template<typename E, size_t N, typename T>
struct COMMON_EMPTY_BASES FuncBasics : public VecBasic<E, N, T>, public EleBasics<E, N, T>,
    public FuncSAddSubMulDiv<E, N, T>, public FuncVAddSubMulDiv<E, N, T>, public shared::FuncSelfCalc<      T>,
    public FuncDot<E, N, T>, public FuncMinMax<   N, T>
{ };

}

#define VEC_CC_TP(ele, type, base, n)                                                                           \
template<typename Arg, typename... Args, typename = std::enable_if_t<!(sizeof...(Args) == 0 &&                  \
    (shared::DecayMatchAny<Arg, base> || shared::DecayInheritAny<Arg, shared::vec::VecType<ele, n>, type>))>>   \
explicit constexpr type(Arg&& arg, Args&&... args) noexcept :                                                   \
        type(shared::vec::Concater<type>::Concat(std::forward<Arg>(arg), std::forward<Args>(args)...)) {}       \
template<typename Arg, typename = std::enable_if_t<std::is_base_of_v<shared::vec::VecType<ele, n>, Arg> &&      \
    !shared::DecayMatchAny<Arg, type>>>                                                                         \
explicit type(const Arg& arg) noexcept : type(*reinterpret_cast<const type*>(&arg)) {}

#ifdef ENABLE_VEC2
struct COMMON_EMPTY_BASES alignas(8) Vec2 : public shared::vec::VecType<float, 2>, public vec::Vec2Base<float>,
    public vec::FuncBasics<float, 2, Vec2>
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
    VEC_CC_TP(float, Vec2, Vec2Base, 2)
    forceinline static constexpr Vec2 LoadAll(const EleType* ptr) noexcept { return Vec2Base::LoadAll(ptr); }
private:
    forceinline constexpr Vec2(const Vec2Base& base) noexcept : Vec2Base(base) {}
};

struct COMMON_EMPTY_BASES alignas(8) IVec2 : public shared::vec::VecType<int32_t, 2>, public vec::Vec2Base<int32_t>,
    public vec::FuncBasics<int32_t, 2, IVec2>
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
    VEC_CC_TP(int32_t, IVec2, Vec2Base, 2)
    forceinline static constexpr IVec2 LoadAll(const EleType* ptr) noexcept { return Vec2Base::LoadAll(ptr); }
private:
    forceinline constexpr IVec2(const Vec2Base& base) noexcept : Vec2Base(base) {}
};

struct COMMON_EMPTY_BASES alignas(8) UVec2 : public shared::vec::VecType<uint32_t, 2>, public vec::Vec2Base<uint32_t>,
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
    VEC_CC_TP(uint32_t, UVec2, Vec2Base, 2)
    forceinline static constexpr UVec2 LoadAll(const EleType* ptr) noexcept { return Vec2Base::LoadAll(ptr); }
private:
    forceinline constexpr UVec2(const Vec2Base& base) noexcept : Vec2Base(base) {}
};
#endif

struct COMMON_EMPTY_BASES alignas(16) Vec3 : public shared::vec::VecType<float, 3>, public vec::Vec4Base<float>,
    public vec::FuncBasics<float, 3, Vec3>, public vec::FuncCross<Vec3>
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
    VEC_CC_TP(float, Vec3, Vec4Base, 3)
    forceinline static constexpr Vec3 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr Vec3(const Vec4Base& base) noexcept : Vec4Base(base) {}
    constexpr Vec3(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
};

struct COMMON_EMPTY_BASES alignas(16) IVec3 : public shared::vec::VecType<int32_t, 3>, public vec::Vec4Base<int32_t>,
    public vec::FuncBasics<int32_t, 3, IVec3>
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
    VEC_CC_TP(int32_t, IVec3, Vec4Base, 3)
    forceinline static constexpr IVec3 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr IVec3(const Vec4Base& base) noexcept : Vec4Base(base) {}
    constexpr IVec3(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
};

struct COMMON_EMPTY_BASES alignas(16) UVec3 : public shared::vec::VecType<uint32_t, 3>, public vec::Vec4Base<uint32_t>,
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
    VEC_CC_TP(uint32_t, UVec3, Vec4Base, 3)
    forceinline static constexpr UVec3 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr UVec3(const Vec4Base& base) noexcept : Vec4Base(base) {}
    constexpr UVec3(EleType x, EleType y, EleType z, EleType w) noexcept : Vec4Base(x, y, z, w) { }
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


struct COMMON_EMPTY_BASES alignas(16) Vec4 : public shared::vec::VecType<float, 4>, public vec::Vec4Base<float>,
    public vec::FuncBasics<float, 4, Vec4>
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
    VEC_CC_TP(float, Vec4, Vec4Base, 4)
    forceinline static constexpr Vec4 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr Vec4(const Vec4Base& base) noexcept : Vec4Base(base) {}
};


struct COMMON_EMPTY_BASES alignas(16) IVec4 : public shared::vec::VecType<int32_t, 4>, public vec::Vec4Base<int32_t>,
    public vec::FuncBasics<int32_t, 4, IVec4>
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
    VEC_CC_TP(int32_t, IVec4, Vec4Base, 4)
    forceinline static constexpr IVec4 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr IVec4(const Vec4Base& base) noexcept : Vec4Base(base) {}
};


struct COMMON_EMPTY_BASES alignas(16) UVec4 : public shared::vec::VecType<uint32_t, 4>, public vec::Vec4Base<uint32_t>,
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
    VEC_CC_TP(uint32_t, UVec4, Vec4Base, 4)
    forceinline static constexpr UVec4 LoadAll(const EleType* ptr) noexcept { return Vec4Base::LoadAll(ptr); }
private:
    forceinline constexpr UVec4(const Vec4Base& base) noexcept : Vec4Base(base) {}
};

#undef VEC_CC_TP
