#pragma once

#include "miniBLAS/BLASrely.h"

#include "miniBLAS/Vec3.hpp"
#include "miniBLAS/Vec4.hpp"
#include "miniBLAS/VecI4.hpp"
#include "miniBLAS/Mat3x3.hpp"
#include "miniBLAS/Mat4x4.hpp"


namespace miniBLAS
{

template<class T1, class T2, class T3>
T3 dot(const T1& left, const T2& right)
{
	return left.dot(right);
}

}
