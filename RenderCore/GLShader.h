#pragma once
#include "RenderCoreRely.h"
#include "common/Controllable.hpp"


namespace rayr
{


struct GLShader : public common::Controllable, public xziar::respak::Serializable
{
private:
    void RegistControllable();
public:
    oglu::oglDrawProgram Program;
    oglu::ShaderConfig Config;
    string Source;
    GLShader(const u16string& name, const string& source, const oglu::ShaderConfig& config = {});
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"GLShader"sv;
    }
    RESPAK_OVERRIDE_TYPE("rayr#GLShader")
    virtual ejson::JObject Serialize(SerializeUtil& context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};


}
