#pragma once

#include "RenderElement.h"
#include "Material.h"
#include "SystemCommon/AsyncAgent.h"
#include "Model/ModelMesh.h"

namespace dizz
{

class alignas(16) Model : public Drawable
{
protected:
    virtual MultiMaterialHolder OnPrepareMaterial() const override;
    Model(ModelMesh mesh);
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"dizz#Drawable#Model"sv;
    }
public:
    static constexpr auto TYPENAME = u"Model";
    ModelMesh Mesh;
    Model(const u16string& fname, const std::shared_ptr<TextureLoader>& texLoader, const std::shared_ptr<oglu::oglWorker>& asyncer = {});
    ~Model() override;
    virtual void PrepareGL(const oglu::oglDrawProgram& prog, const std::map<string, string>& translator = std::map<string, string>()) override;
    virtual void Draw(Drawcall& drawcall) const override;
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Drawable#Model")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};

}