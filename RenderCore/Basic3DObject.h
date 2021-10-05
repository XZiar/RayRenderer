#pragma once

#include "RenderElement.h"

namespace dizz
{

class alignas(16) Pyramid : public Drawable
{
protected:
    float Sidelen;
    oglu::oglVBO vbo;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"dizz#Drawable#Pyramid"sv;
    }
public:
    static constexpr auto TYPENAME = u"Pyramid";
    Pyramid(const float len);
    ~Pyramid() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string, string>& translator = std::map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Drawable#Pyramid")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};

class alignas(16) Sphere : public Drawable
{
protected:
    float Radius;
    oglu::oglVBO vbo;
    oglu::oglEBO ebo;
    uint32_t ptcount;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"dizz#Drawable#Sphere"sv;
    }
public:
    static constexpr auto TYPENAME = u"Sphere";
    Sphere(const float r);
    ~Sphere() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string, string>& translator = std::map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Drawable#Sphere")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};


class alignas(16) Box : public Drawable
{
protected:
    b3d::Vec3 Size;
    oglu::oglVBO vbo;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"dizz#Drawable#Box"sv;
    }
public:
    static constexpr auto TYPENAME = u"Box";
    Box(const float len) : Box(len, len, len) { };
    Box(const float length, const float height, const float width);
    ~Box() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string, string>& translator = std::map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Drawable#Box")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};


class alignas(16) Plane : public Drawable
{
protected:
    oglu::oglVBO vbo;
    float SideLen, TexRepeat;
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"dizz#Drawable#Plane"sv;
    }
public:
    static constexpr auto TYPENAME = u"Plane";
    Plane(const float len = 500.0f, const float texRepeat = 1.0f);
    ~Plane() override { }
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string, string>& translator = std::map<string, string>()) override;
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Drawable#Plane")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};

}
