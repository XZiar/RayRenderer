#pragma once

#include "oglRely.h"
#include "3DElement.hpp"
#include "oglVAO.h"

namespace oglu
{


class alignas(16) Point
{
public:
    b3d::Vec3 pos;
    b3d::Normal norm;
    union
    {
        b3d::Coord2D tcoord;
        b3d::Vec3 tcoord3;
    };
    using ComponentType = std::tuple<
        VAComponent<float, false, 3, 0>,
        VAComponent<float, false, 3, 16>,
        VAComponent<float, false, 3, 32>>;

    Point() noexcept { };
    Point(const b3d::Vec3 &v, const b3d::Normal &n, const b3d::Coord2D &t) noexcept : pos(v), norm(n), tcoord(t) { };
    Point(const b3d::Vec3 &v, const b3d::Normal &n, const b3d::Vec3 &t3) noexcept : pos(v), norm(n), tcoord3(t3) { };
};
class alignas(16) PointEx : public Point
{
public:
    b3d::Vec4 tan;
    using ComponentType = std::tuple<
        VAComponent<float, false, 3, 0>,
        VAComponent<float, false, 3, 16>,
        VAComponent<float, false, 3, 32>,
        VAComponent<float, false, 4, 48>>;

    PointEx() noexcept { };
    PointEx(const b3d::Vec3 &v, const b3d::Normal &n, const b3d::Coord2D &t, const b3d::Vec4 &tanNorm = b3d::Vec4::zero()) noexcept : Point(v, n, t), tan(tanNorm) { };
    PointEx(const b3d::Vec3 &v, const b3d::Normal &n, const b3d::Vec3 &t3, const b3d::Vec4 &tanNorm = b3d::Vec4::zero()) noexcept : Point(v, n, t3), tan(tanNorm) { };
};

struct alignas(32) Triangle
{
public:
    b3d::Vec3 points[3];
    b3d::Normal norms[3];
    b3d::Coord2D tcoords[3];
    float dummy[2] = { 0.f,0.f };

    Triangle() noexcept { };
    Triangle(const b3d::Vec3& va, const b3d::Vec3& vb, const b3d::Vec3& vc) noexcept : points{ va, vb, vc } { }
    Triangle(const b3d::Vec3& va, const b3d::Normal& na, const b3d::Vec3& vb, const b3d::Normal& nb, const b3d::Vec3& vc, const b3d::Normal& nc) noexcept
        : points{ va, vb, vc }, norms{ na, nb, nc } { }
    Triangle(const b3d::Vec3& va, const b3d::Normal& na, const b3d::Coord2D& ta, const b3d::Vec3& vb, const b3d::Normal& nb, const b3d::Coord2D& tb, const b3d::Vec3& vc, const b3d::Normal& nc, const b3d::Coord2D& tc) noexcept
        : points{ va, vb, vc }, norms{ na, nb, nc }, tcoords{ ta, tb, tc }
    {
    }
};

inline bool CheckInvertNormal(const Point& pt0, const Point& pt1, const Point& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = edge1.cross(edge2);
    if (norm.dot(pt0.norm) < 0) return true;
    if (norm.dot(pt1.norm) < 0) return true;
    if (norm.dot(pt2.norm) < 0) return true;
    return false;
}

inline void FixInvertNormal(Point& pt0, Point& pt1, Point& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = edge1.cross(edge2);
    if (norm.dot(pt0.norm) < 0) pt0.norm.negatived();
    if (norm.dot(pt1.norm) < 0) pt1.norm.negatived();
    if (norm.dot(pt2.norm) < 0) pt2.norm.negatived();
}

inline void GenerateTanPoint(PointEx& pt0, PointEx& pt1, PointEx& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto du1 = pt1.tcoord.u - pt0.tcoord.u, dv1 = pt1.tcoord.v - pt0.tcoord.v,
        du2 = pt2.tcoord.u - pt0.tcoord.u, dv2 = pt2.tcoord.v - pt0.tcoord.v;
    const auto r = 1.0f / (du1 * dv2 - dv1 * du2);
    const b3d::Normal tangent = b3d::Vec3((edge1 * dv2 - edge2 * dv1) * r);
    const auto bitangent_ = b3d::Vec3(edge2 * du1 - edge1 * du2);
    for (PointEx *pt : { &pt0, &pt1, &pt2 })
    {
        const b3d::Vec3 bitan = tangent.cross(pt->norm);
        const auto newtan = b3d::Normal(tangent - pt->norm * pt->norm.dot(tangent));
        pt->tan += b3d::Vec4(newtan, bitangent_.dot(bitan) * r > 0 ? 1.0f : -1.0f);
    }
}


struct PointExCompressed
{
public:
    using ComponentType = std::tuple<
        VARawComponent<VAValType::I16,   true, false, 3, 0>,
        VARawComponent<VAValType::I16,   true, false, 3, 16>,
        VARawComponent<VAValType::I10_2, true, false, 4, 32>,
        VARawComponent<VAValType::I10_2, true, false, 4, 48>>;
    int16_t Pos[4];
    int16_t TCoord[2];
    uint32_t Norm;
    uint32_t Tan;
    static uint32_t CompressVec4(const b3d::Vec4& v)
    {
        const auto int9V = v * 512.f;
        const auto x = ((int32_t)int9V.x) & 0x3ff, y = ((int32_t)int9V.y) & 0x3ff, z = ((int32_t)int9V.x) & 0x3ff;
        const bool w = int9V.w >= 0.f;
        return x | (y << 10) | (z << 20) | (w ? 0x00000000u : 0xc0000000u);
    }
    PointExCompressed(const PointEx& pt) noexcept
    {
        const auto int16Pos = pt.pos * INT16_MAX;
        Pos[0] = static_cast<int16_t>(int16Pos.x), Pos[1] = static_cast<int16_t>(int16Pos.y), Pos[2] = static_cast<int16_t>(int16Pos.z);
        const auto int16Tcoord = pt.tcoord * INT16_MAX;
        TCoord[0] = static_cast<int16_t>(int16Tcoord.u), TCoord[1] = static_cast<int16_t>(int16Tcoord.v);
        Norm = CompressVec4(b3d::Vec4(pt.norm, 1.f));
        Tan = CompressVec4(b3d::Vec4(b3d::Normal(pt.tan), pt.tan.w));
    }
};


}
