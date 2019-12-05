#pragma once
#include "oglRely.h"
#include "common/ContainerEx.hpp"


namespace oglu
{
class oglShader_;
using oglShader = std::shared_ptr<oglShader_>;


enum class ShaderType : GLenum
{
    Vertex   = 0x8B31/*GL_VERTEX_SHADER*/, 
    Geometry = 0x8DD9/*GL_GEOMETRY_SHADER*/, 
    Fragment = 0x8B30/*GL_FRAGMENT_SHADER*/,
    TessCtrl = 0xBE88/*GL_TESS_CONTROL_SHADER*/, 
    TessEval = 0x8E87/*GL_TESS_EVALUATION_SHADER*/,
    Compute  = 0x91B9/*GL_COMPUTE_SHADER*/
};

enum class ShaderPropertyType : uint8_t { Vector, Color, Range, Matrix, Float, Bool, Int, Uint };

struct ShaderExtProperty
{
    std::string Name;
    std::string Description;
    ShaderPropertyType Type;
    std::any Data;
    ShaderExtProperty(std::string name, const ShaderPropertyType type, std::string desc = "", std::any data = {}) :
        Name(std::move(name)), Description(std::move(desc)), Type(type), Data(std::move(data)) {}
    using Lesser = common::container::SetKeyLess<ShaderExtProperty, &ShaderExtProperty::Name>;
};

struct ShaderExtInfo
{
    std::set<ShaderExtProperty, ShaderExtProperty::Lesser> Properties;
    std::map<std::string, std::vector<std::pair<std::string, size_t>>> EmulateRoutines;
    std::map<std::string, std::string> ResMappings;
};

struct ShaderConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, float, double, std::string>;
    std::map<std::string, DefineVal> Defines;
    std::map<std::string, std::string> Routines;
};


class OGLUAPI oglShader_ : public detail::oglCtxObject<true>
{
    friend class oglProgram_;
private:
    MAKE_ENABLER();
    std::string Src;
    GLuint ShaderID;
    oglShader_(const ShaderType type, const std::string& txt);
public:
    const ShaderType Type;
    ~oglShader_();

    void compile();
    [[nodiscard]] const std::string& SourceText() const { return Src; }

    [[nodiscard]] static oglShader LoadFromFile(const ShaderType type, const common::fs::path& path);
    [[nodiscard]] static std::vector<oglShader> LoadFromFiles(common::fs::path fname);
    [[nodiscard]] static std::vector<oglShader> LoadFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config, const bool allowCompute = true, const bool allowDraw = true);
    [[nodiscard]] static std::vector<oglShader> LoadDrawFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    {
        return LoadFromExSrc(src, info, config, false, true);
    }
    [[nodiscard]] static std::vector<oglShader> LoadComputeFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    {
        return LoadFromExSrc(src, info, config, true, false);
    }
    [[nodiscard]] static std::vector<oglShader> LoadFromExSrc(const std::string& src, const ShaderConfig& config = {})
    {
        ShaderExtInfo dummy;
        return LoadFromExSrc(src, dummy, config);
    }
    [[nodiscard]] static std::string_view GetStageName(const ShaderType type);
};


}

