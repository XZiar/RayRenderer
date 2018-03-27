#pragma once

#include "RenderCoreRely.h"

namespace rayr
{
using namespace common;
using namespace b3d;

struct RAYCOREAPI alignas(16) Material : public AlignBase<16>
{
public:
    Vec4 ambient, diffuse, specular, emission;
    float shiness, reflect, refract, rfr;//高光权重，反射比率，折射比率，折射率
};


class RAYCOREAPI alignas(16) Drawable : public AlignBase<16>, public NonCopyable
{
public:
    Vec3 position = Vec3::zero(), rotation = Vec3::zero(), scale = Vec3::one();
    u16string name;
    static void releaseAll(const oglu::oglProgram& prog);
    virtual ~Drawable();
    /*prepare VAO with given Vertex Attribute Location
     *-param: VertPos,VertNorm,TexPos
     */
    virtual void prepareGL(const oglu::oglProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
    virtual void draw(oglu::oglProgram& prog) const;
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
        rotation += Vec3(x, y, z);
    }
    void Rotate(const Vec3& angles)
    {
        rotation += angles;
    }
protected:
    uint32_t drawableID;
    oglu::oglVAO defaultVAO;
    Drawable(const u16string& typeName);
    auto defaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglBuffer& vbo) -> decltype(vao->prepare());
    auto drawPosition(oglu::oglProgram& prog) const -> decltype(prog->draw());
    void setVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const;
    const oglu::oglVAO& getVAO(const oglu::oglProgram& prog) const;
};

}
