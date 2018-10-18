#pragma once

#include "RenderElement.h"
#include "Material.h"
#include "common/AsyncExecutor/AsyncAgent.h"
#include "Model/ModelMesh.h"

namespace rayr
{

class alignas(16) Model : public Drawable
{
protected:
    virtual MultiMaterialHolder OnPrepareMaterial() const override;
    Model(ModelMesh mesh);
public:
    static constexpr auto TYPENAME = u"Model";
    ModelMesh Mesh;
    Model(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const Wrapper<oglu::oglWorker>& asyncer = {});
    ~Model() override;
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    virtual void Draw(Drawcall& drawcall) const override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#Drawable#Model")
    virtual void Serialize(SerializeUtil& context, ejson::JObject& object) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};

}