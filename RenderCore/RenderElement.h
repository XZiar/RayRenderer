#pragma once

#include "RenderCoreRely.h"
#include "Material.h"

namespace rayr
{
using namespace common;
using namespace b3d;


class RAYCOREAPI alignas(16) Drawable : public AlignBase<16>, public NonCopyable
{
public:
    using Drawcall = oglu::detail::ProgDraw;
    Vec3 position = Vec3::zero(), rotation = Vec3::zero(), scale = Vec3::one();
    MultiMaterialHolder MaterialHolder;
    u16string Name;
    bool ShouldRender = true;

    ///<summary>Release all related VAO for the shader-program</summary>  
    ///<param name="prog">shader program</param>
    static void ReleaseAll(const oglu::oglProgram& prog);
    
    virtual ~Drawable();
    ///<summary>Prepare for specific shader program(Generate VAO)</summary>  
    ///<param name="prog">shader program</param>
    ///<param name="translator">mapping from common resource name to program's resource name</param>
    virtual void PrepareGL(const oglu::oglProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
    virtual void Draw(Drawcall& drawcall) const;

    u16string GetType() const;
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
    void AssignMaterial();
private:
    Drawable(const std::type_index type, const u16string& typeName);
protected:
    const std::type_index DrawableType;
    oglu::oglVAO EmptyVAO;
    oglu::oglUBO MaterialUBO;
    vector<byte> MaterialBuf;

    template<typename T>
    Drawable(const T * const childThis, const u16string& typeName) : Drawable(std::type_index(typeid(childThis)), typeName) { }
    void PrepareMaterial(const bool defaultAssign = true);
    auto DefaultBind(const oglu::oglProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare());
    Drawcall& DrawPosition(Drawcall& prog) const;
    ///<summary>Assign VAO into prog-sensative map</summary>  
    ///<param name="prog">target shader program</param>
    ///<param name="vao">saved VAO</param>
    void SetVAO(const oglu::oglProgram& prog, const oglu::oglVAO& vao) const;
    const oglu::oglVAO& GetVAO(const oglu::oglProgram& prog) const { return GetVAO(prog.weakRef()); }
    const oglu::oglVAO& GetVAO(const Drawcall& drawcall) const { return GetVAO(drawcall.GetProg()); }
    const oglu::oglVAO& GetVAO(const oglu::oglProgram::weak_type& weakProg) const;
    template<typename T>
    static std::type_index GetType(const T* const obj) { return std::type_index(typeid(obj)); }
    static oglu::oglVBO GetDrawIdVBO();
};

}
