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


}
