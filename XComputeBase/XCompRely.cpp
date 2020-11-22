#include "XCompRely.h"
#include "common/StrParsePack.hpp"

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace xcomp
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;


std::pair<VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept
{
#define CASE(str, dtype, bit, n, least) \
    HashCase(type, str) return { {common::simd::VecDataInfo::DataTypes::dtype, bit, n, 0}, least };
#define CASEV(pfx, type, bit, least) \
    CASE(U"" STRINGIZE(pfx) "",    type, bit, 1,  least) \
    CASE(U"" STRINGIZE(pfx) "v2",  type, bit, 2,  least) \
    CASE(U"" STRINGIZE(pfx) "v3",  type, bit, 3,  least) \
    CASE(U"" STRINGIZE(pfx) "v4",  type, bit, 4,  least) \
    CASE(U"" STRINGIZE(pfx) "v8",  type, bit, 8,  least) \
    CASE(U"" STRINGIZE(pfx) "v16", type, bit, 16, least) \

#define CASE2(tstr, type, bit)                  \
    CASEV(PPCAT(tstr, bit),  type, bit, false)  \
    CASEV(PPCAT(tstr, bit+), type, bit, true)   \

    switch (common::DJBHash::HashC(type))
    {
    CASE2(u, Unsigned, 8)
    CASE2(u, Unsigned, 16)
    CASE2(u, Unsigned, 32)
    CASE2(u, Unsigned, 64)
    CASE2(i, Signed,   8)
    CASE2(i, Signed,   16)
    CASE2(i, Signed,   32)
    CASE2(i, Signed,   64)
    CASE2(f, Float,    16)
    CASE2(f, Float,    32)
    CASE2(f, Float,    64)
    default: break;
    }
    return { {VecDataInfo::DataTypes::Unsigned, 0, 0, 0}, false };

#undef CASE2
#undef CASEV
#undef CASE
}

std::u32string_view StringifyVDataType(const VecDataInfo vtype) noexcept
{
#define CASE(s, type, bit, n) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::type, bit, n, 0}): return PPCAT(PPCAT(U,s),sv);
#define CASEV(pfx, type, bit) \
    CASE(STRINGIZE(pfx),             type, bit, 1)  \
    CASE(STRINGIZE(PPCAT(pfx, v2)),  type, bit, 2)  \
    CASE(STRINGIZE(PPCAT(pfx, v3)),  type, bit, 3)  \
    CASE(STRINGIZE(PPCAT(pfx, v4)),  type, bit, 4)  \
    CASE(STRINGIZE(PPCAT(pfx, v8)),  type, bit, 8)  \
    CASE(STRINGIZE(PPCAT(pfx, v16)), type, bit, 16) \

    switch (static_cast<uint32_t>(vtype))
    {
    CASEV(u8,  Unsigned, 8)
    CASEV(u16, Unsigned, 16)
    CASEV(u32, Unsigned, 32)
    CASEV(u64, Unsigned, 64)
    CASEV(i8,  Signed,   8)
    CASEV(i16, Signed,   16)
    CASEV(i32, Signed,   32)
    CASEV(i64, Signed,   64)
    CASEV(f16, Float,    16)
    CASEV(f32, Float,    32)
    CASEV(f64, Float,    64)
    default: return U"Error"sv;
    }

#undef CASEV
#undef CASE
}


}