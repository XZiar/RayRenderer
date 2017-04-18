#pragma once

#include "BLASrely.h"

namespace miniBLAS
{

/*a vector contains 4 element(int32 or float), align to 32 boundary for AVX*/
static constexpr uint32_t Vec4Align = 16;
template<typename T>
class alignas(Vec4Align) Vec4Base : public common::AlignBase<Vec4Base<T>>
{
private:
	static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
	union
	{
		T data[4];
		__m128 float_dat;
		__m128i int_dat;
		struct
		{
			T x, y, z, w;
		};
		struct
		{
			float float_x, float_y, float_z, float_w;
		};
		struct
		{
			int32_t int_x, int_y, int_z, int_w;
		};
	};

	Vec4Base() { };
	Vec4Base(const T x_) :x(x_) { };
	Vec4Base(const T x_, const T y_) :x(x_), y(y_) { };
	Vec4Base(const T x_, const T y_, const T z_) :x(x_), y(y_), z(z_) { };
	Vec4Base(const T x_, const T y_, const T z_, const T w_) :x(x_), y(y_), z(z_), w(w_) { };
public:
	
	T& operator[](int idx)
	{
		return data[idx];
	}
	T const& operator[](int idx) const
	{
		return data[idx];
	}

};


class alignas(Vec4Align) Vec3;
class alignas(Vec4Align) Vec4;

}

