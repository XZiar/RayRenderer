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
    virtual MultiMaterialHolder PrepareMaterial() const override;
    Model(ModelMesh mesh, const Wrapper<oglu::oglWorker>& asyncer = {});
public:
    static constexpr auto TYPENAME = u"Model";
    ModelMesh Mesh;
    Model(const u16string& fname, std::shared_ptr<detail::TextureLoader>& texLoader, const Wrapper<oglu::oglWorker>& asyncer = {});
    ~Model() override;
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    virtual void Draw(Drawcall& drawcall) const override;
    RESPAK_OVERRIDE_TYPE("rayr#Drawable#Model")
    virtual ejson::JObject Serialize(SerializeUtil& context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};

}