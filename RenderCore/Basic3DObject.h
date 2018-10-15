#pragma once

#include "RenderElement.h"

namespace rayr
{

class alignas(16) Pyramid : public Drawable
{
protected:
    float sidelen;
    oglu::oglVBO vbo;
public:
    static constexpr auto TYPENAME = u"Pyramid";
    Pyramid(const float len);
    ~Pyramid() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Drawable#Pyramid")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};

class alignas(16) Sphere : public Drawable
{
protected:
    float radius, radius_sqr;
    oglu::oglVBO vbo;
    oglu::oglEBO ebo;
    uint32_t ptcount;
public:
    static constexpr auto TYPENAME = u"Sphere";
    Sphere(const float r);
    ~Sphere() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Drawable#Sphere")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};


class alignas(16) Box : public Drawable
{
protected:
    Vec3 size;
    oglu::oglVBO vbo;
public:
    static constexpr auto TYPENAME = u"Box";
    Box(const float len) : Box(len, len, len) { };
    Box(const float length, const float height, const float width);
    ~Box() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Drawable#Box")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};


class alignas(16) Plane : public Drawable
{
protected:
    oglu::oglVBO vbo;
    float SideLen, TexRepeat;
public:
    static constexpr auto TYPENAME = u"Plane";
    Plane(const float len = 500.0f, const float texRepeat = 1.0f);
    ~Plane() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Drawable#Plane")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};

}
