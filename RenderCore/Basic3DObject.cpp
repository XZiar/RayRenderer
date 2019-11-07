#include "RenderCorePch.h"
#include "Basic3DObject.h"

namespace rayr
{
using std::set;
using std::map;
using std::vector;
using b3d::Vec3;
using b3d::Normal;
using b3d::Coord2D;
using b3d::PI_float;
using oglu::Point;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


Pyramid::Pyramid(const float len) : Drawable(this, TYPENAME), Sidelen(len)
{
    constexpr double sqrt3 = 1.73205080757;
    constexpr float sqrt3_2 = float(sqrt3 / 2);
    constexpr float sqrt3_3 = float(sqrt3 / 3);
    constexpr float sqrt3_6 = float(sqrt3 / 6);
    Point pts[] =
    { 
        { {  0.0f, sqrt3_2,  0.0f },{ -0.86326f,0.33227f,0.37998f },{ 0.0f, 0.0f } },
        { { -0.5f, 0.0f, -sqrt3_6 },{ -0.86326f,0.33227f,0.37998f },{ 2.0f, 0.0f } },
        { {  0.0f, 0.0f,  sqrt3_3 },{ -0.86326f,0.33227f,0.37998f },{ 0.0f, 2.0f } },
        { {  0.0f, sqrt3_2,  0.0f },{  0.86326f,0.33227f,0.37998f },{ 0.0f, 0.0f } },
        { {  0.0f, 0.0f,  sqrt3_3 },{  0.86326f,0.33227f,0.37998f },{ 0.0f, 2.0f } },
        { {  0.5f, 0.0f, -sqrt3_6 },{  0.86326f,0.33227f,0.37998f },{ 2.0f, 2.0f } },
        { {  0.0f, sqrt3_2,  0.0f },{  0.0f, 0.316228f, -0.94868f },{ 0.0f, 0.0f } },
        { {  0.5f, 0.0f, -sqrt3_6 },{  0.0f, 0.316228f, -0.94868f },{ 2.0f, 2.0f } },
        { { -0.5f, 0.0f, -sqrt3_6 },{  0.0f, 0.316228f, -0.94868f },{ 2.0f, 0.0f } },
        { {  0.0f, 0.0f,  sqrt3_3 },{  0.0f, -1.0f, 0.0f },{ 0.0f, 2.0f } },
        { { -0.5f, 0.0f, -sqrt3_6 },{  0.0f, -1.0f, 0.0f },{ 2.0f, 0.0f } },
        { {  0.5f, 0.0f, -sqrt3_6 },{  0.0f, -1.0f, 0.0f },{ 2.0f, 2.0f } }
    };
    for (auto& pt : pts)
        pt.pos *= Sidelen;
    vbo = oglu::oglArrayBuffer_::Create();
    vbo->WriteSpan(pts);
}

void Pyramid::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>&)
{
    auto vao = oglu::oglVAO_::Create(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 12);
    SetVAO(prog, vao);
}

void Pyramid::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    Drawable::Serialize(context, jself);
    jself.Add<detail::JsonConv>(EJ_FIELD(Sidelen));
}
void Pyramid::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Pyramid, float)
{
    return std::tuple(object.Get<float>("Sidelen"));
}


static std::pair<vector<Point>, vector<uint16_t>> CreateSphere(const float radius, const uint16_t rings = 80, const uint16_t sectors = 80)
{
    const float rstep = 1.0f / (rings - 1);
    const float sstep = 1.0f / (sectors - 1);
    std::vector<Point> pts;
    pts.reserve(rings*sectors);
    uint16_t rcnt = rings, scnt = sectors;
    for (float r = 0; rcnt--; r += rstep)
    {
        scnt = sectors;
        for (float s = 0; scnt--; s += sstep)
        {
            const auto x = cos(2 * PI_float * s) * sin(PI_float * r);
            const auto y = sin(PI_float * r - PI_float / 2);
            const auto z = sin(2 * PI_float * s) * sin(PI_float * r);
            Normal norm(x, y, z);
            Coord2D texc(s, r);
            Point pt(norm * radius, norm, texc);
            pts.push_back(pt);
        }
    }
    vector<uint16_t> indexs;
    indexs.reserve(rings*sectors * 6);
    for (uint16_t r = 0; r < rings - 1; r++)
    {
        for (uint16_t s = 0; s < sectors - 1; s++)
        {
            const auto idx0 = r * sectors + s;
            indexs.push_back(static_cast<uint16_t>(idx0));
            indexs.push_back(static_cast<uint16_t>(idx0 + sectors));
            indexs.push_back(static_cast<uint16_t>(idx0 + 1));
            indexs.push_back(static_cast<uint16_t>(idx0 + 1));
            indexs.push_back(static_cast<uint16_t>(idx0 + sectors));
            indexs.push_back(static_cast<uint16_t>(idx0 + sectors + 1));
        }
    }
    return { pts,indexs };
}

Sphere::Sphere(const float r) : Drawable(this, TYPENAME), Radius(r)
{
    const auto [pts,indexs] = CreateSphere(Radius);
    vbo = oglu::oglArrayBuffer_::Create();
    vbo->WriteSpan(pts);
    ebo = oglu::oglElementBuffer_::Create();
    ebo->WriteCompact(indexs);
    ptcount = static_cast<uint32_t>(indexs.size());
}

void Sphere::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>&)
{
    auto vao = oglu::oglVAO_::Create(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)//bind vertex attribute
        .SetIndex(ebo)//index draw
        .SetDrawSize(0, ptcount);
    SetVAO(prog, vao);
}

void Sphere::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    Drawable::Serialize(context, jself);
    jself.Add(EJ_FIELD(Radius));
}
void Sphere::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Sphere, float)
{
    return std::tuple(object.Get<float>("Radius"));
}


/*
 *      v4-------v7
 *      /:       /|
 *     / :      / |
 *   v0--:----v3  |
 *    | v5-----|-v6
 *    | /      | /
 *    |/       |/
 *   v1-------v2
 *
 **/
const Point BoxBasePts[] = 
{ 
    { { +0.5f, +0.5f, +0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
    { { +0.5f, -0.5f, +0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },//v2
    { { +0.5f, -0.5f, -0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
    { { +0.5f, +0.5f, +0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
    { { +0.5f, -0.5f, -0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
    { { +0.5f, +0.5f, -0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 0.0f } },//v7

    { { -0.5f, +0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
    { { -0.5f, -0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },//v5
    { { -0.5f, -0.5f, +0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
    { { -0.5f, +0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
    { { -0.5f, -0.5f, +0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
    { { -0.5f, +0.5f, +0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } },//v0

    { { -0.5f, +0.5f, -0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
    { { -0.5f, +0.5f, +0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 0.0f, 2.0f } },//v0
    { { +0.5f, +0.5f, +0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
    { { -0.5f, +0.5f, -0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
    { { +0.5f, +0.5f, +0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
    { { +0.5f, +0.5f, -0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 1.0f, 2.0f } },//v7

    { { -0.5f, -0.5f, +0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
    { { -0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 3.0f } },//v5
    { { +0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
    { { -0.5f, -0.5f, +0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
    { { +0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
    { { +0.5f, -0.5f, +0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 1.0f, 3.0f } },//v2

    { { -0.5f, +0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
    { { -0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 0.0f, 4.0f } },//v1
    { { +0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
    { { -0.5f, +0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
    { { +0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
    { { +0.5f, +0.5f, +0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 1.0f, 4.0f } },//v3

    { { +0.5f, +0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
    { { +0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 0.0f, 5.0f } },//v6
    { { -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
    { { +0.5f, +0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
    { { -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
    { { -0.5f, +0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 1.0f, 5.0f } },//v4
};

Box::Box(const float length, const float height, const float width) : Drawable(this, TYPENAME)
{
    Size = Vec3(length, height, width);
    vector<Point> pts;
    pts.assign(BoxBasePts, BoxBasePts + 36);
    for (auto& pt : pts)
        pt.pos *= Size;
    vbo = oglu::oglArrayBuffer_::Create();
    vbo->WriteSpan(pts);
}

void Box::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>&)
{
    auto vao = oglu::oglVAO_::Create(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 36);
    SetVAO(prog, vao);
}

void Box::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    Drawable::Serialize(context, jself);
    jself.Add<detail::JsonConv>("Size", Size);
}
void Box::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Box, float, float, float)
{
    b3d::Vec3 Size;
    detail::FromJArray(object.GetArray("Size"), Size);
    return std::tuple(Size.x, Size.y, Size.z);
}


Plane::Plane(const float len, const float texRepeat) : Drawable(this, TYPENAME), SideLen(len), TexRepeat(texRepeat)
{
    const Point pts[] =
    {
        { { -len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, texRepeat } },//v4
        { { -len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 0.0f } },//v0
        { { +len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, 0.0f } },//v3
        { { -len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ 0.0f, texRepeat } },//v4
        { { +len,0.0f,+len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, 0.0f } },//v3
        { { +len,0.0f,-len },{ 0.0f, +1.0f, 0.0f },{ texRepeat, texRepeat } },//v7
    };
    vbo = oglu::oglArrayBuffer_::Create();
    vbo->WriteSpan(pts);
}

void Plane::PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>&)
{
    auto vao = oglu::oglVAO_::Create(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 6);
    SetVAO(prog, vao);
}

void Plane::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    Drawable::Serialize(context, jself);
    jself.Add(EJ_FIELD(SideLen))
         .Add(EJ_FIELD(TexRepeat));
}
void Plane::Deserialize(DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_IMPL_COMP_DESERIALIZE(Plane, float, float)
{
    return std::tuple(object.Get<float>("SideLen"), object.Get<float>("TexRepeat"));
}

}
