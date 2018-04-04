#pragma once

#include "RenderCoreRely.h"

namespace rayr
{
using namespace common;
using namespace b3d;

struct RAYCOREAPI alignas(16) MaterialData : public AlignBase<16>
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
    Vec4 ambient, diffuse, specular, emission;
    float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
    /*void SetMtl(const uint8_t prop, const Vec3 &);
    void SetMtl(const Property prop, const float r, const float g, const float b)
    {
        SetMtl(uint8_t(prop), Vec3(r, g, b));
    }
    void SetMtl(const uint8_t prop, const float val);
    void SetMtl(const Property prop, const float val)
    {
        SetMtl(uint8_t(prop), val);
    }*/
};


class RAYCOREAPI alignas(16) Drawable : public AlignBase<16>, public NonCopyable
{
public:
    using Drawcall = oglu::detail::ProgDraw;
    Vec3 position = Vec3::zero(), rotation = Vec3::zero(), scale = Vec3::one();
    u16string name;
    static void releaseAll(const oglu::oglProgram& prog);
    virtual ~Drawable();
    /*prepare VAO with given Vertex Attribute Location
     *-param: VertPos,VertNorm,TexPos
     */
    virtual void prepareGL(const oglu::oglProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
    virtual void draw(Drawcall& drawcall) const;
    u16string getType() const;
    void Move(const float x, const float y, const float z)
    {
        position += Vec3(x, y, z);
    }
    void Move(const Vec3& offset)
    {
        position += offset;
    }
    void Rotate(const float x, const float y, const float z)
    {
        Rotate(Vec3(x, y, z));
    }
    void Rotate(const Vec3& angles)
    {
        rotation += angles;
        rotation.RepeatClampPos(Vec3::Vec3_2PI());
    }
protected:
    const std::type_index DrawableType;
    oglu::oglVAO defaultVAO;
    Drawable(const std::type_index type, const u16string& typeName);
    auto defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare());
    Drawcall& drawPosition(Drawcall& prog) const;
    void setVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const;
    const oglu::oglVAO& getVAO(const oglu::oglProgram& prog) const { return getVAO(prog.weakRef()); }
    const oglu::oglVAO& getVAO(const oglu::oglProgram::weak_type& weakProg) const;
    template<typename T>
    static std::type_index GetType(const T* const obj) { return std::type_index(typeid(obj)); }
};

}
