#pragma once

#define _USE_MATH_DEFINES 1
#include <cmath>
#include <string>
#include "miniBLAS.hpp"

#ifndef M_PI
#   define M_PI 3.14159265358979323846   // pi
#endif

namespace b3d
{
using std::string;
using miniBLAS::AlignBase;

template<typename T>
inline T ang2rad(T in)
{
	return static_cast<T>(in * M_PI / 180);
}

template<typename T>
inline T rad2ang(T in)
{
	return static_cast<T>(in * 180 / M_PI);
}

class Coord2D
{
public:
	float u, v;
	Coord2D() { u = v = 0.0f; };
	template<class T>
	Coord2D(const T& u_, const T& v_) :u(static_cast<float>(u_)), v(static_cast<float>(v_)) { };

	Coord2D operator+(const Coord2D &c) const
	{
		return Coord2D(u + c.u, v + c.v);
	}
	Coord2D operator*(const float &n) const
	{
		return Coord2D(u*n, v*n);
	}
	Coord2D repos(const float scalex, const float scaley, const float offsetx, const float offsety)
	{
		return Coord2D(u*scalex + offsetx, v*scaley + offsety);
	}
	operator float*() { return &u; };
};


class alignas(16) Vec3 : public miniBLAS::Vec3
{
public:
	using miniBLAS::Vec3::Vec3;
	Vec3() : miniBLAS::Vec3(true) { }
	Vec3(const miniBLAS::Vec3& v) { *(miniBLAS::Vec3*)this = v; }
};


class alignas(16) Vec4 : public miniBLAS::Vec4
{
public:
	using miniBLAS::Vec4::Vec4;
	Vec4() : miniBLAS::Vec4(true) { }
	Vec4(const miniBLAS::Vec4& v) { *(miniBLAS::Vec4*)this = v; }
	Vec4(const Vec3& v) :miniBLAS::Vec4(v, true) { }

	operator Vec3& () { return *(Vec3*)this; }
	operator const Vec3& () const { return *(const Vec3*)this; }
};


class alignas(16) Normal : public Vec3
{
public:
	Normal() : Vec3() { };
	Normal(const Vec3& v)
	{
		*(Vec3*)this = v.normalize();
	}
	template<class T>
	Normal(const T& ix, const T& iy, const T& iz) :Vec3(ix, iy, iz) { normalized(); };

	Normal& operator=(const Vec3& v)
	{
		*(Vec3*)this = v.normalize();
		return *this;
	}
};


class alignas(32) Mat3x3 : public miniBLAS::Mat3x3
{
public:
	using miniBLAS::Mat3x3::element;
	using miniBLAS::Mat3x3::Mat3x3;
	Mat3x3() :miniBLAS::Mat3x3() { }
	Mat3x3(const miniBLAS::Mat3x3& m) :miniBLAS::Mat3x3(m) { }
	//Vec4's xyz define axis, w define angle(in degree)
	static Mat3x3 RotateMat(const Vec4& rv)
	{
		const auto sqr = rv * rv;
		const float rad = ang2rad(rv.w);
		const float sinx = std::sin(rad), cosx = std::cos(rad);
		const float OMcosx = 1 - cosx;
		/*  (1-cos)x^2+cos  xy(1-cos)-zsin  xz(1-cos)+ysin
		 *  xy(1-cos)+zsin  (1-cos)y^2+cos  yz(1-cos)-xsin
		 *  xz(1-cos)-ysin  yz(1-cos)+xsin  (1-cos)z^2+cos
		 **/
		return Mat3x3(
			Vec3(sqr.x*OMcosx + cosx, rv.x*rv.y*OMcosx - rv.z*sinx, rv.x*rv.z*OMcosx + rv.y*sinx),
			Vec3(rv.x*rv.y*OMcosx + rv.z*sinx, sqr.y*OMcosx + cosx, rv.y*rv.z*OMcosx - rv.x*sinx),
			Vec3(rv.x*rv.z*OMcosx - rv.y*sinx, rv.y*rv.z*OMcosx + rv.x*sinx, sqr.z*OMcosx + cosx));
		/*  Rotate x-axis
		 *  1     0      0
		 *  0  cosx  -sinx
		 *  0  sinx   cosx
		 *  Rotate y-axis
		 *   cosy  0  siny
		 *      0  1     0
		 *  -siny  0  cosy
		 *  Rotate z-axis
		 *  cosz  -sinz  0
		 *  sinz   cosz  0
		 *     0      0  1
		 **/
	}
	static Mat3x3 ScaleMat(const Vec3& sv)
	{
		auto sMat = Mat3x3::zero();
		sMat.x.x = sv.x, sMat.y.y = sv.y, sMat.z.z = sv.z;
		return sMat;
	}
};


class alignas(32) Mat4x4 : public miniBLAS::Mat4x4
{
public:
	using miniBLAS::Mat4x4::element;
	using miniBLAS::Mat4x4::Mat4x4;
	Mat4x4() :miniBLAS::Mat4x4() { }
	Mat4x4(const miniBLAS::Mat3x3& m) :miniBLAS::Mat4x4(m, true) { }
	Mat4x4(const miniBLAS::Mat4x4& m) :miniBLAS::Mat4x4(m) { }
	/*pure translate(translata DOT identity-rotate)*/
	static Mat4x4 TranslateMat(const Vec3& tv)
	{
		static Mat4x4 idmat = miniBLAS::Mat4x4::identity();
		return TranslateMat(idmat, tv);
	}
	/*translate DOT rotate*/
	static Mat4x4 TranslateMat(const Mat4x4& rMat, const Vec3& tv)
	{
		Mat4x4 tMat(rMat);
		tMat.x.w = tv.x, tMat.y.w = tv.y, tMat.z.w = tv.z;
		return tMat;
	}
	/*rotate DOT translate*/
	static Mat4x4 TranslateMat(const Vec3& tv, const Mat3x3& rMat)
	{
		const auto rtv = rMat * tv;
		Mat4x4 tMat(rMat);
		tMat.x.w = rtv.x, tMat.y.w = rtv.y, tMat.z.w = rtv.z;
		return tMat;
	}
};


inline float mod(const float &l, const float &r)
{
	float t, e;
	modf(l / r, &t);
	e = t*r;
	return l - e;
}

class alignas(16) Point : public AlignBase<>
{
public:
	Vec3 pos;
	Normal norm;
	union
	{
		Coord2D tcoord;
		Vec3 tcoord3;
	};

	Point() { };
	Point(const Vec3 &v, const Normal &n, const Coord2D &t) : pos(v), norm(n), tcoord(t) { };
	Point(const Vec3 &v, const Normal &n, const Vec3 &t3) : pos(v), norm(n), tcoord3(t3) { };
};

struct alignas(32) Triangle : public AlignBase<>
{
public:
	Vec3 points[3];
	Normal norms[3];
	Coord2D tcoords[3];
	float dummy[2];

	Triangle() { };
	Triangle(const Vec3& va, const Vec3& vb, const Vec3& vc) : points{ va, vb, vc } { }
	Triangle(const Vec3& va, const Normal& na, const Vec3& vb, const Normal& nb, const Vec3& vc, const Normal& nc)
		: points{ va, vb, vc }, norms{ na, nb, nc } { }
	Triangle(const Vec3& va, const Normal& na, const Coord2D& ta, const Vec3& vb, const Normal& nb, const Coord2D& tb, const Vec3& vc, const Normal& nc, const Coord2D& tc)
		: points{ va, vb, vc }, norms{ na, nb, nc }, tcoords{ ta, tb, tc }
	{
	}
};


class alignas(16) Light : public AlignBase<>
{
public:
	enum class Type : uint8_t
	{
		Parallel = 0x1, Point = 0x2, Spot = 0x3
	};
	enum class Property : uint8_t
	{
		Position = 0x1,
		Ambient = 0x2,
		Diffuse = 0x4,
		Specular = 0x8,
		Atten = 0x10
	};
public:
	Vec3 position,
		ambient,
		diffuse,
		specular,
		attenuation;
	float coang, exponent;//for spot light
	float rangy, rangz, rdis,
		angy, angz, dis;
	int type;
	bool bLight;
public:
	Light(const Type type);
	bool turn();
	void move(const float dangy, const float dangz, const float ddis);
	void SetProperty(const uint8_t prop, const float r, const float g, const float b, const float a = 1.0f);
	void SetProperty(const Property prop, const float r, const float g, const float b, const float a = 1.0f)
	{
		SetProperty(uint8_t(prop), r, g, b, a);
	}
	void SetLumi(const float lum);
};

class alignas(16) Material : public AlignBase<>
{
public:
	enum class Property : uint8_t
	{
		Ambient = 0x1,
		Diffuse = 0x2,
		Specular = 0x4,
		Emission = 0x8,
		Shiness = 0x10,
		Reflect = 0x20,
		Refract = 0x40,
		RefractRate = 0x80
	};
private:
	Vec3 ambient,
		diffuse,
		specular,
		emission;
	float /*高光权重*/shiness,
		/*反射比率*/reflect,
		/*折射比率*/refract,
		/*折射率*/rfr;
public:
	string name;
	Material();
	~Material();
	void SetMtl(const uint8_t prop, const Vec3 &);
	void SetMtl(const Property prop, const float r, const float g, const float b)
	{
		SetMtl(uint8_t(prop), Vec3(r, g, b));
	}
	void SetMtl(const uint8_t prop, const float val);
	void SetMtl(const Property prop, const float val)
	{
		SetMtl(uint8_t(prop), val);
	}
};

class alignas(32) Camera : public AlignBase<>
{
protected:
	void rotate(const Mat3x3& rMat)
	{
		u = rMat * u;
		v = rMat * v;
		n = rMat * n;
	}
public:
	union
	{
		Mat3x3 camMat;
		struct
		{
			Normal u, v, n;//to right,up,toward
		};
	};
	Vec3 position;
	int width, height;
	float fovy, aspect, zNear, zFar;
	Camera(int w = 1120, int h = 630)
	{
		width = w, height = h;
		aspect = (float)w / h;
		fovy = 45.0f, zNear = 1.0f, zFar = 100.0f;

		position = Vec3(0, 0, 10);
		u = Vec3(1, 0, 0);
		v = Vec3(0, 1, 0);
		n = Vec3(0, 0, -1);
	}
	void resize(const int w, const int h)
	{
		width = w, height = h;
		aspect = (float)w / h;
	}
	void move(const float &x, const float &y, const float &z)
	{
		position += camMat * Vec3(x, y, z);
	}
	//rotate along x-axis
	void pitch(const float angx)
	{
		rotate(Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, angx)));
	}
	//rotate along y-axis
	void yaw(const float angy)
	{
		rotate(Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, angy)));
	}
	//rotate along z-axis
	void roll(const float angz)
	{
		rotate(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, angz)));
	}
};


}