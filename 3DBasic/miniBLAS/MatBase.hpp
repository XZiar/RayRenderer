#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

/*4x4matrix contains 4 row of 4elements-vector, align to 32bytes for AVX*/
static constexpr uint32_t SQMat4Align = 32;
template<typename T1, typename T2, typename T3>
class alignas(32) SQMat4Base : public AlignBase<SQMat4Align>
{
	static_assert(sizeof(T1) == 4, "only 4-byte length type(for a element) allowed");
	static_assert(sizeof(T2) == 16, "only 16-byte length type(for a row) allowed");
	static_assert(sizeof(T3) == 32, "T2 should be twice of T");
protected:
	union
	{
		float float_ele[16];
		int int_ele[16];
		T1 element[16];
		__m128 float_row[4];
		__m128i int_row[4];
		T2 row[4];
		__m256 float_row2[2];
		__m256i int_row2[2];
		T3 row2[2];
		struct
		{
			T2 x, y, z, w;
		};
		struct
		{
			__m128 float_x, float_y, float_z, float_w;
		};
		struct
		{
			__m128i int_x, int_y, int_z, int_w;
		};
		struct
		{
			T3 xy, zw;
		};
		struct
		{
			__m256 float_xy, float_zw;
		};
		struct
		{
			__m256i int_xy, int_zw;
		};
	};

	SQMat4Base() { };
	SQMat4Base(const T2& x_) :x(x_) { };
	SQMat4Base(const T2& x_, const T2& y_) :x(x_), y(y_) { };
	SQMat4Base(const T2& x_, const T2& y_, const T2& z_) :x(x_), y(y_), z(z_) { };
	SQMat4Base(const T2& x_, const T2& y_, const T2& z_, const T2& w_) :x(x_), y(y_), z(z_), w(w_) { };
	SQMat4Base(const T3& xy_) :xy(xy_) { };
	SQMat4Base(const T3& xy_, const T2& z_) :xy(xy_), z(z_) { };
	SQMat4Base(const T3& xy_, const T3& zw_) :xy(xy_), zw(zw_) { };
public:
	T2& operator[](const int rowidx)
	{
		return row[rowidx];
	};
	const T2& operator[](const int rowidx) const
	{
		return row[rowidx];
	};
	T1& operator()(const int rowidx, const int colidx)
	{
		return element[rowidx * 4 + colidx];
	}
	const T1& operator()(const int rowidx, const int colidx) const
	{
		return element[rowidx * 4 + colidx];
	}
};

using __m128x3 = __m128[3];
using __m128x4 = __m128[4];
using __m256x2 = __m256[2];

class Mat3x3;
class Mat4x4;

}