#pragma once

#include "oglRely.h"
#include "oglVAO.h"

namespace oglu
{


class alignas(16) Point
{
public:
    mbase::Vec3 pos;
    mbase::Normal norm;
    union
    {
        mbase::Vec2 tcoord;
        mbase::Vec3 tcoord3;
    };
    using ComponentType = std::tuple<
        VAComponent<float, false, 3, 0>,
        VAComponent<float, false, 3, 16>,
        VAComponent<float, false, 3, 32>>;

    Point() noexcept { };
    Point(const mbase::Vec3 &v, const mbase::Normal &n, const mbase::Vec2 &t ) noexcept : pos(v), norm(n), tcoord(t) { };
    Point(const mbase::Vec3 &v, const mbase::Normal &n, const mbase::Vec3 &t3) noexcept : pos(v), norm(n), tcoord3(t3) { };
};
class alignas(16) PointEx : public Point
{
public:
    mbase::Vec4 tan;
    using ComponentType = std::tuple<
        VAComponent<float, false, 3, 0>,
        VAComponent<float, false, 3, 16>,
        VAComponent<float, false, 3, 32>,
        VAComponent<float, false, 4, 48>>;

    PointEx() noexcept { };
    PointEx(const mbase::Vec3 &v, const mbase::Normal &n, const mbase::Vec2 &t,  const mbase::Vec4 &tanNorm = mbase::Vec4::Zeros()) noexcept : Point(v, n, t),  tan(tanNorm) { };
    PointEx(const mbase::Vec3 &v, const mbase::Normal &n, const mbase::Vec3 &t3, const mbase::Vec4 &tanNorm = mbase::Vec4::Zeros()) noexcept : Point(v, n, t3), tan(tanNorm) { };
};

struct alignas(32) Triangle
{
public:
    mbase::Vec3 points[3];
    mbase::Normal norms[3];
    mbase::Vec2 tcoords[3];
    float dummy[2] = { 0.f,0.f };

    Triangle() noexcept { };
    Triangle(const mbase::Vec3& va, const mbase::Vec3& vb, const mbase::Vec3& vc) noexcept : points{ va, vb, vc } { }
    Triangle(const mbase::Vec3& va, const mbase::Normal& na, const mbase::Vec3& vb, const mbase::Normal& nb, const mbase::Vec3& vc, const mbase::Normal& nc) noexcept
        : points{ va, vb, vc }, norms{ na, nb, nc } { }
    Triangle(const mbase::Vec3& va, const mbase::Normal& na, const mbase::Vec2& ta, const mbase::Vec3& vb, const mbase::Normal& nb, const mbase::Vec2& tb, const mbase::Vec3& vc, const mbase::Normal& nc, const mbase::Vec2& tc) noexcept
        : points{ va, vb, vc }, norms{ na, nb, nc }, tcoords{ ta, tb, tc }
    {
    }
};

inline bool CheckInvertNormal(const Point& pt0, const Point& pt1, const Point& pt2) noexcept
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = Cross(edge1, edge2);
    if (Dot(norm, pt0.norm) < 0) return true;
    if (Dot(norm, pt1.norm) < 0) return true;
    if (Dot(norm, pt2.norm) < 0) return true;
    return false;
}

inline void FixInvertNormal(Point& pt0, Point& pt1, Point& pt2) noexcept
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = Cross(edge1, edge2);
    if (Dot(norm, pt0.norm) < 0) pt0.norm = pt0.norm.Negative();
    if (Dot(norm, pt1.norm) < 0) pt1.norm = pt1.norm.Negative();
    if (Dot(norm, pt2.norm) < 0) pt2.norm = pt2.norm.Negative();
}

inline void GenerateTanPoint(PointEx& pt0, PointEx& pt1, PointEx& pt2) noexcept
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto du1 = pt1.tcoord.X - pt0.tcoord.X, dv1 = pt1.tcoord.Y - pt0.tcoord.Y,
        du2 = pt2.tcoord.X - pt0.tcoord.X, dv2 = pt2.tcoord.Y - pt0.tcoord.Y;
    const auto r = 1.0f / (du1 * dv2 - dv1 * du2);
    const mbase::Normal tangent = mbase::Vec3((edge1 * dv2 - edge2 * dv1) * r);
    const auto bitangent_ = mbase::Vec3(edge2 * du1 - edge1 * du2);
    for (PointEx *pt : { &pt0, &pt1, &pt2 })
    {
        const mbase::Vec3 bitan = Cross(tangent, pt->norm);
        const auto newtan = mbase::Normal(tangent - pt->norm * Dot(pt->norm, tangent));
        pt->tan += mbase::Vec4(newtan, Dot(bitangent_, bitan) * r > 0 ? 1.0f : -1.0f);
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
    [[nodiscard]] static uint32_t CompressVec4(const mbase::Vec4& v) noexcept
    {
        const auto int9V = v * 512.f;
        const auto x = ((int32_t)int9V.X) & 0x3ff, y = ((int32_t)int9V.Y) & 0x3ff, z = ((int32_t)int9V.Z) & 0x3ff;
        const bool w = int9V.W >= 0.f;
        return x | (y << 10) | (z << 20) | (w ? 0x00000000u : 0xc0000000u);
    }
    PointExCompressed(const PointEx& pt) noexcept
    {
        const auto int16Pos = pt.pos * INT16_MAX;
        Pos[0] = static_cast<int16_t>(int16Pos.X), Pos[1] = static_cast<int16_t>(int16Pos.Y), Pos[2] = static_cast<int16_t>(int16Pos.Z);
        const auto int16Tcoord = pt.tcoord * INT16_MAX;
        TCoord[0] = static_cast<int16_t>(int16Tcoord.X), TCoord[1] = static_cast<int16_t>(int16Tcoord.Y);
        Norm = CompressVec4(mbase::Vec4(pt.norm, 1.f));
        Tan = CompressVec4(mbase::Vec4(pt.tan.As<mbase::Vec3>().Normalize(), pt.tan.W));
    }
};


}
