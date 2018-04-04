#pragma once

#include "3DElement.hpp"

namespace oglu
{


inline void GenerateTanPoint(b3d::PointEx& pt0, b3d::PointEx& pt1, b3d::PointEx& pt2)
{
    const auto edge1 = pt1.pos - pt0.pos, edge2 = pt2.pos - pt0.pos;
    const auto du1 = pt1.tcoord.u - pt0.tcoord.u, dv1 = pt1.tcoord.v - pt0.tcoord.v,
        du2 = pt2.tcoord.u - pt0.tcoord.u, dv2 = pt2.tcoord.v - pt0.tcoord.v;
    const auto r = 1.0f / (du1 * dv2 - dv1 * du2);
    const b3d::Normal tangent = b3d::Vec3((edge1 * dv2 - edge2 * dv1) * r);
    pt0.tan += tangent;
    pt1.tan += tangent;
    pt2.tan += tangent;
}


}
