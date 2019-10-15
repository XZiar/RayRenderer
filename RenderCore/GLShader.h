#pragma once
#include "RenderCoreRely.h"


namespace rayr
{

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
struct RAYCOREAPI GLShader : public virtual common::Controllable, public virtual xziar::respak::Serializable
{
private:
    void RegistControllable();
    string Source;
    oglu::ShaderConfig Config;
public:
    oglu::oglDrawProgram Program;
    GLShader(const u16string& name, const string& source, const oglu::ShaderConfig& config = {});
    ~GLShader() {}
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"GLShader"sv;
    }
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
    RESPAK_DECL_COMP_DESERIALIZE("rayr#GLShader")
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}
