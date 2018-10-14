#pragma once
#include "RenderCoreRely.h"


namespace rayr
{

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
struct RAYCOREAPI GLShader : public virtual Controllable, public virtual xziar::respak::Serializable
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
    RESPAK_OVERRIDE_TYPE("rayr#GLShader")
    virtual ejson::JObject Serialize(SerializeUtil& context) const override;
    virtual void Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object) override;
};
#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}
