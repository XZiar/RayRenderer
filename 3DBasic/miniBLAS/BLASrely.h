#pragma once

#include "common/CommonRely.hpp"
#include "common/SIMD.hpp"
#include <cstdint>
#include <type_traits>
#include <cmath>


#if defined(_WIN32) && defined(__SSE2__) && !defined(_MANAGED) && !defined(_M_CEE)
#   define VECCALL __vectorcall
#else
#   define VECCALL 
#endif

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace miniBLAS
{


inline constexpr auto miniBLAS_intrin()
{
	return STRINGIZE(COMMON_SIMD_INTRIN);
}


}