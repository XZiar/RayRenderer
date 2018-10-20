#pragma once

#include "RenderCoreRely.h"
#include "Material.h"

namespace rayr
{
using namespace common;
using namespace b3d;


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class RAYCOREAPI Drawable : public AlignBase<16>, public NonCopyable, public xziar::respak::Serializable, public Controllable
{
public:
    using Drawcall = oglu::detail::ProgDraw;
    Vec3 Position = Vec3::zero(), Rotation = Vec3::zero(), Scale = Vec3::one();
    MultiMaterialHolder MaterialHolder;
    u16string Name;
    bool ShouldRender = true;

    ///<summary>Release all related VAO for the shader-program</summary>  
    ///<param name="prog">shader program</param>
    static void ReleaseAll(const oglu::oglDrawProgram& prog);
    
    virtual ~Drawable();
    ///<summary>Prepare for specific shader program(Generate VAO)</summary>  
    ///<param name="prog">shader program</param>
    ///<param name="translator">mapping from common resource name to program's resource name</param>
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string,string>& translator = map<string, string>()) = 0;
    virtual void Draw(Drawcall& drawcall) const;

    u16string GetType() const;
    const boost::uuids::uuid& GetUid() const { return Uid; };

    void Move(const float x, const float y, const float z)
    {
        Position += Vec3(x, y, z);
    }
    void Move(const Vec3& offset)
    {
        Position += offset;
    }
    void Rotate(const float x, const float y, const float z)
    {
        Rotate(Vec3(x, y, z));
    }
    void Rotate(const Vec3& angles)
    {
        Rotation += angles;
        Rotation.RepeatClampPos(Vec3::Vec3_2PI());
    }
    void PrepareMaterial();
    void AssignMaterial();

    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
private:
    boost::uuids::uuid Uid;
    Drawable(const std::type_index type, const u16string& typeName);
protected:
    const std::type_index DrawableType;
    oglu::oglVAO EmptyVAO;
    oglu::oglUBO MaterialUBO;

    template<typename T>
    Drawable(const T * const childThis, const u16string& typeName) : Drawable(std::type_index(typeid(childThis)), typeName) { }

    void RegistControllable();
    virtual MultiMaterialHolder OnPrepareMaterial() const;
    auto DefaultBind(const oglu::oglDrawProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare());
    Drawcall& DrawPosition(Drawcall& prog) const;
    ///<summary>Assign VAO into prog-sensative map</summary>  
    ///<param name="prog">target shader program</param>
    ///<param name="vao">saved VAO</param>
    void SetVAO(const oglu::oglDrawProgram& prog, const oglu::oglVAO& vao) const;
    const oglu::oglVAO& GetVAO(const oglu::oglDrawProgram& prog) const { return GetVAO(prog.weakRef()); }
    const oglu::oglVAO& GetVAO(const Drawcall& drawcall) const { return GetVAO(drawcall.GetProg()); }
    const oglu::oglVAO& GetVAO(const oglu::oglDrawProgram::weak_type& weakProg) const;
    template<typename T>
    static std::type_index GetType(const T* const obj) { return std::type_index(typeid(obj)); }
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}
