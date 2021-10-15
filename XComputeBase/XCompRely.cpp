#include "XCompRely.h"
#include "common/StaticLookup.hpp"

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )



namespace xcomp
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;


#define GENV(dtype, bit, n, least) VTypeInfo{ common::simd::VecDataInfo::DataTypes::dtype, n, 0, bit, least ? VTypeInfo::TypeFlags::MinBits : VTypeInfo::TypeFlags::Empty }
#define PERPFX(pfx, type, bit, least) \
    { STRINGIZE(pfx) "",    GENV(type, bit, 1,  least) }, \
    { STRINGIZE(pfx) "v2",  GENV(type, bit, 2,  least) }, \
    { STRINGIZE(pfx) "v3",  GENV(type, bit, 3,  least) }, \
    { STRINGIZE(pfx) "v4",  GENV(type, bit, 4,  least) }, \
    { STRINGIZE(pfx) "v8",  GENV(type, bit, 8,  least) }, \
    { STRINGIZE(pfx) "v16", GENV(type, bit, 16, least) }

#define MIN2(tstr, type, bit)                    \
    PERPFX(PPCAT(tstr, bit),  type, bit, false), \
    PERPFX(PPCAT(tstr, bit+), type, bit, true)

static constexpr auto S2TMapping = BuildStaticLookup(common::str::ShortStrVal<8>, VTypeInfo,
    MIN2(u, Unsigned, 8),
    MIN2(u, Unsigned, 16),
    MIN2(u, Unsigned, 32),
    MIN2(u, Unsigned, 64),
    MIN2(i, Signed,   8),
    MIN2(i, Signed,   16),
    MIN2(i, Signed,   32),
    MIN2(i, Signed,   64),
    MIN2(f, Float,    16),
    MIN2(f, Float,    32),
    MIN2(f, Float,    64));

#undef MIN2
#undef PERPFX
#undef GENV

VTypeInfo ParseVDataType(const std::u32string_view type) noexcept
{
    const auto ret = S2TMapping(type);
    if (ret.has_value())
        return ret.value();
    return {};
}


#define GENV(type, flag, bit, n) static_cast<uint32_t>(VTypeInfo{ VecDataInfo::DataTypes::type, n, 0, bit, VTypeInfo::TypeFlags::flag })
#define MIN2(s, type, bit, n) \
    { GENV(type, Empty,   bit, n), U"" STRINGIZE(s) ""sv }, { GENV(type, MinBits, bit, n), U"" STRINGIZE(s) "+"sv }
#define PERPFX(pfx, type, bit) \
    MIN2(pfx,             type, bit, 1), \
    MIN2(PPCAT(pfx, v2),  type, bit, 2), \
    MIN2(PPCAT(pfx, v3),  type, bit, 3), \
    MIN2(PPCAT(pfx, v4),  type, bit, 4), \
    MIN2(PPCAT(pfx, v8),  type, bit, 8), \
    MIN2(PPCAT(pfx, v16), type, bit, 16)

static constexpr auto T2SMapping = BuildStaticLookup(uint32_t, std::u32string_view,
    PERPFX(u8,  Unsigned, 8),
    PERPFX(u16, Unsigned, 16),
    PERPFX(u32, Unsigned, 32),
    PERPFX(u64, Unsigned, 64),
    PERPFX(i8,  Signed,   8),
    PERPFX(i16, Signed,   16),
    PERPFX(i32, Signed,   32),
    PERPFX(i64, Signed,   64),
    PERPFX(f16, Float,    16),
    PERPFX(f32, Float,    32),
    PERPFX(f64, Float,    64));

#undef PERPFX
#undef MIN2
#undef GENV
std::u32string_view StringifyVDataType(const VTypeInfo vtype) noexcept
{
    if (vtype.IsCustomType())
        return U"Custom"sv;
    return T2SMapping(vtype).value_or(U"Error"sv);
}


struct RangeHolder::Range
{
    std::shared_ptr<const RangeHolder> Host;
    std::shared_ptr<Range> Previous;
    Range(std::shared_ptr<const RangeHolder> host) noexcept :
        Host(std::move(host)), Previous(Host->CurrentRange.lock())
    { }
    ~Range()
    {
        Host->EndRange();
    }
};

RangeHolder::~RangeHolder() {}
std::shared_ptr<void> RangeHolder::DeclareRange(std::u16string_view msg)
{
    auto range = std::make_shared<Range>(BeginRange(msg));
    CurrentRange = range;
    return range;
}


}