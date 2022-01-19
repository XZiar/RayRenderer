#pragma once

#include "RenderCoreRely.h"
#include "Material.h"

namespace dizz
{


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI Drawable : public common::NonCopyable,
    public xziar::respak::Serializable, public common::Controllable
{
public:
    class Drawcall
    {
    public:
        oglu::ProgDraw Drawer;
        mbase::Mat4 ProjMat;
        mbase::Mat4 ViewMat;
        mbase::Mat4 PVMat;
        Drawcall(const oglu::oglDrawProgram& prog, const mbase::Mat4& projMat, const mbase::Mat4& viewMat)
            : Drawer(prog->Draw()), ProjMat(projMat), ViewMat(viewMat), PVMat(projMat * viewMat)
        { }
    };
    mbase::Vec3 Position = mbase::Vec3::Zeros(), Rotation = mbase::Vec3::Zeros(), Scale = mbase::Vec3::Ones();
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
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string,string>& translator = std::map<string, string>()) = 0;
    virtual void Draw(Drawcall& drawcall) const;

    u16string GetType() const;
    const boost::uuids::uuid& GetUid() const { return Uid; };

    void Move(const float x, const float y, const float z)
    {
        Position += mbase::Vec3(x, y, z);
    }
    void Move(const mbase::Vec3& offset)
    {
        Position += offset;
    }
    void Rotate(const mbase::Vec3& angles);
    void Rotate(const float x, const float y, const float z)
    {
        Rotate({ x, y, z });
    }
    void PrepareMaterial();
    void AssignMaterial();

    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
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
    auto DefaultBind(const oglu::oglDrawProgram& prog, oglu::oglVAO& vao, const oglu::oglVBO& vbo) -> decltype(vao->Prepare(prog));
    oglu::ProgDraw& DrawPosition(Drawcall& prog) const;
    ///<summary>Assign VAO into prog-sensative std::map</summary>  
    ///<param name="prog">target shader program</param>
    ///<param name="vao">saved VAO</param>
    void SetVAO(const oglu::oglDrawProgram& prog, const oglu::oglVAO& vao) const;
    const oglu::oglVAO& GetVAO(const oglu::oglDrawProgram& prog) const { return GetVAO(std::weak_ptr<oglu::oglDrawProgram_>(prog)); }
    const oglu::oglVAO& GetVAO(const Drawcall& drawcall) const { return GetVAO(drawcall.Drawer.GetProg()); }
    const oglu::oglVAO& GetVAO(const oglu::oglDrawProgram::weak_type& weakProg) const;
    template<typename T>
    static std::type_index GetType(const T* const obj) { return std::type_index(typeid(obj)); }
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}
