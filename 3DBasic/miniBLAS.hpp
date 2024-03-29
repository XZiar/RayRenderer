#pragma once

#ifndef OLD_BLAS
#   error miniBLAS deprecated
#endif

#include "miniBLAS/BLASrely.h"

#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wnested-anon-types"
#   pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#elif COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201)
#endif
#include "miniBLAS/Vec3.hpp"
#include "miniBLAS/Vec4.hpp"
#include "miniBLAS/VecI4.hpp"
#include "miniBLAS/Mat3x3.hpp"
#include "miniBLAS/Mat4x4.hpp"
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


namespace miniBLAS
{

template<class T1, class T2>
auto dot(const T1& left, const T2& right)
{
	return left.dot(right);
}

}
