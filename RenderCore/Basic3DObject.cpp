#include "RenderCoreRely.h"
#include "Basic3DObject.h"

namespace rayr
{
using b3d::PI_float;


Pyramid::Pyramid(const float len) : Drawable(this, TYPENAME), sidelen(len)
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
        pt.pos *= sidelen;
    vbo.reset();
    vbo->Write(pts);
}

void Pyramid::PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 12);
    SetVAO(prog, vao);
}

ejson::JObject Pyramid::Serialize(SerializeUtil& context) const
{
    auto jself = Drawable::Serialize(context);
    jself.EJOBJECT_ADD(sidelen);
    return jself;
}
void Pyramid::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_DESERIALIZER(Pyramid)
{
    auto ret = new Pyramid(object.Get<float>("sidelen"));
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Pyramid)


vector<uint16_t> Sphere::CreateSphere(vectorEx<Point>& pts, const float radius, const uint16_t rings /*= 80*/, const uint16_t sectors /*= 80*/)
{
    const float rstep = 1.0f / (rings - 1);
    const float sstep = 1.0f / (sectors - 1);
    pts.clear();
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
            indexs.push_back(idx0);
            indexs.push_back(idx0 + sectors);
            indexs.push_back(idx0 + 1);
            indexs.push_back(idx0 + 1);
            indexs.push_back(idx0 + sectors);
            indexs.push_back(idx0 + sectors + 1);
        }
    }
    return indexs;
}

Sphere::Sphere(const float r) : Drawable(this, TYPENAME), radius(r), radius_sqr(r*r)
{
    vectorEx<Point> pts;
    auto indexs = CreateSphere(pts, radius);
    vbo.reset();
    vbo->Write(pts);
    ebo.reset();
    ebo->WriteCompact(indexs);
    ptcount = static_cast<uint32_t>(indexs.size());
}

void Sphere::PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)//bind vertex attribute
        .SetIndex(ebo)//index draw
        .SetDrawSize(0, ptcount);
    SetVAO(prog, vao);
}

ejson::JObject Sphere::Serialize(SerializeUtil& context) const
{
    auto jself = Drawable::Serialize(context);
    jself.EJOBJECT_ADD(radius);
    return jself;
}
void Sphere::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_DESERIALIZER(Sphere)
{
    auto ret = new Sphere(object.Get<float>("radius"));
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Sphere)


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
    { { +0.5f,+0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
    { { +0.5f,-0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },//v2
    { { +0.5f,-0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
    { { +0.5f,+0.5f,+0.5f },{ +1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },//v3
    { { +0.5f,-0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },//v6
    { { +0.5f,+0.5f,-0.5f },{ +1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 0.0f } },//v7

    { { -0.5f,+0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
    { { -0.5f,-0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },//v5
    { { -0.5f,-0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
    { { -0.5f,+0.5f,-0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 1.0f } },//v4
    { { -0.5f,-0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 1.0f } },//v1
    { { -0.5f,+0.5f,+0.5f },{ -1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } },//v0

    { { -0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
    { { -0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 0.0f, 2.0f } },//v0
    { { +0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
    { { -0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 0.0f, 1.0f, 2.0f } },//v4
    { { +0.5f,+0.5f,+0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 0.0f, 2.0f } },//v3
    { { +0.5f,+0.5f,-0.5f },{ 0.0f, +1.0f, 0.0f },{ 1.0f, 1.0f, 2.0f } },//v7

    { { -0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
    { { -0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 3.0f } },//v5
    { { +0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
    { { -0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 3.0f } },//v1
    { { +0.5f,-0.5f,-0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.0f, 3.0f } },//v6
    { { +0.5f,-0.5f,+0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 1.0f, 3.0f } },//v2

    { { -0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
    { { -0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 0.0f, 4.0f } },//v1
    { { +0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
    { { -0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 0.0f, 1.0f, 4.0f } },//v0
    { { +0.5f,-0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 0.0f, 4.0f } },//v2
    { { +0.5f,+0.5f,+0.5f },{ 0.0f, 0.0f, +1.0f },{ 1.0f, 1.0f, 4.0f } },//v3

    { { +0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
    { { +0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 0.0f, 5.0f } },//v6
    { { -0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
    { { +0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 1.0f, 5.0f } },//v7
    { { -0.5f,-0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 5.0f } },//v5
    { { -0.5f,+0.5f,-0.5f },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 1.0f, 5.0f } },//v4
};

Box::Box(const float length, const float height, const float width) : Drawable(this, TYPENAME)
{
    size = Vec3(length, height, width);
    vector<Point> pts;
    pts.assign(BoxBasePts, BoxBasePts + 36);
    for (auto& pt : pts)
        pt.pos *= size;
    vbo.reset();
    vbo->Write(pts);
}

void Box::PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 36);
    SetVAO(prog, vao);
}

ejson::JObject Box::Serialize(SerializeUtil& context) const
{
    auto jself = Drawable::Serialize(context);
    jself.Add("size", detail::ToJArray(context, size));
    return jself;
}
void Box::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_DESERIALIZER(Box)
{
    b3d::Vec3 size;
    detail::FromJArray(object.GetArray("size"), size);
    auto ret = new Box(size.x, size.y, size.z);
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Box)


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
    vbo.reset();
    vbo->Write(pts, sizeof(pts));
}

void Plane::PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator)
{
    oglu::oglVAO vao(oglu::VAODrawMode::Triangles);
    DefaultBind(prog, vao, vbo)
        .SetDrawSize(0, 6);
    SetVAO(prog, vao);
}

ejson::JObject Plane::Serialize(SerializeUtil& context) const
{
    auto jself = Drawable::Serialize(context);
    jself.EJOBJECT_ADD(SideLen).EJOBJECT_ADD(TexRepeat);
    return jself;
}
void Plane::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
    Drawable::Deserialize(context, object);
}

RESPAK_DESERIALIZER(Plane)
{
    auto ret = new Plane(object.Get<float>("SideLen"), object.Get<float>("TexRepeat"));
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Plane)

}
