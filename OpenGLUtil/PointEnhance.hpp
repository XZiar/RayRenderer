#pragma once

#include "3DElement.hpp"

namespace oglu
{


inline bool CheckInvertNormal(const b3d::Point& pt0, const b3d::Point& pt1, const b3d::Point& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = edge1.cross(edge2);
    if (norm.dot(pt0.norm) < 0) return true;
    if (norm.dot(pt1.norm) < 0) return true;
    if (norm.dot(pt2.norm) < 0) return true;
    return false;
}

inline void FixInvertNormal(b3d::Point& pt0, b3d::Point& pt1, b3d::Point& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto norm = edge1.cross(edge2);
    if (norm.dot(pt0.norm) < 0) pt0.norm.negatived();
    if (norm.dot(pt1.norm) < 0) pt1.norm.negatived();
    if (norm.dot(pt2.norm) < 0) pt2.norm.negatived();
}

inline void GenerateTanPoint(b3d::PointEx& pt0, b3d::PointEx& pt1, b3d::PointEx& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto du1 = pt1.tcoord.u - pt0.tcoord.u, dv1 = pt1.tcoord.v - pt0.tcoord.v,
        du2 = pt2.tcoord.u - pt0.tcoord.u, dv2 = pt2.tcoord.v - pt0.tcoord.v;
    const auto r = 1.0f / (du1 * dv2 - dv1 * du2);
    const b3d::Normal tangent = b3d::Vec3((edge1 * dv2 - edge2 * dv1) * r);
    const auto bitangent_ = b3d::Vec3(edge2 * du1 - edge1 * du2);
    for (b3d::PointEx *pt : { &pt0, &pt1, &pt2 })
    {
        const b3d::Vec3 bitan = tangent.cross(pt->norm);
        const auto newtan = b3d::Normal(tangent - pt->norm * pt->norm.dot(tangent));
        pt->tan += b3d::Vec4(newtan, bitangent_.dot(bitan) * r > 0 ? 1.0f : -1.0f);
    }
}


struct PointExCompressed
{
public:
    int16_t Pos[4];
    int16_t TCoord[2];
    uint32_t Norm;
    uint32_t Tan;
    static uint32_t CompressVec4(const miniBLAS::Vec4& v)
    {
        const auto int9V = v * 512.f;
        const auto x = ((int32_t)int9V.x) & 0x3ff, y = ((int32_t)int9V.y) & 0x3ff, z = ((int32_t)int9V.x) & 0x3ff;
        const bool w = int9V.w >= 0.f;
        return x + (y << 10) + (x << 10) + w ? 0x00000000u : 0xc0000000u;
    }
    PointExCompressed(const b3d::PointEx& pt) noexcept
    {
        const auto int16Pos = pt.pos * INT16_MAX;
        Pos[0] = static_cast<int16_t>(int16Pos.x), Pos[1] = static_cast<int16_t>(int16Pos.y), Pos[2] = static_cast<int16_t>(int16Pos.z);
        const auto int16Tcoord = pt.tcoord * INT16_MAX;
        TCoord[0] = static_cast<int16_t>(int16Tcoord.u), TCoord[1] = static_cast<int16_t>(int16Tcoord.v);
        Norm = CompressVec4(b3d::Vec4(pt.norm, 1.f));
        Tan = CompressVec4(b3d::Vec4(b3d::Normal(pt.norm), pt.norm.w));
    }
};


}
