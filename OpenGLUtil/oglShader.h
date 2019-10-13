#pragma once
#include "oglRely.h"

namespace oglu
{

enum class ShaderType : GLenum
{
    Vertex   = 0x8B31/*GL_VERTEX_SHADER*/, 
    Geometry = 0x8DD9/*GL_GEOMETRY_SHADER*/, 
    Fragment = 0x8B30/*GL_FRAGMENT_SHADER*/,
    TessCtrl = 0xBE88/*GL_TESS_CONTROL_SHADER*/, 
    TessEval = 0x8E87/*GL_TESS_EVALUATION_SHADER*/,
    Compute  = 0x91B9/*GL_COMPUTE_SHADER*/
};


namespace detail
{

class OGLUAPI _oglShader : public oglCtxObject<true>
{
    friend class _oglProgram;
private:
    std::string Src;
    GLuint ShaderID;
public:
    const ShaderType Type;
    _oglShader(const ShaderType type, const std::string& txt);
    ~_oglShader();

    void compile();
    const std::string& SourceText() const { return Src; }
};

}

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
    std::map<std::string, std::string> ResMappings;
};

struct ShaderConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, float, double, std::string>;
    std::map<std::string, DefineVal> Defines;
    std::map<std::string, std::string> Routines;
};

class OGLUAPI oglShader : public Wrapper<detail::_oglShader>
{
private:
public:
    using Wrapper::Wrapper;
    static oglShader LoadFromFile(const ShaderType type, const common::fs::path& path);
    static std::vector<oglShader> LoadFromFiles(common::fs::path fname);
    static std::vector<oglShader> LoadFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config, const bool allowCompute = true, const bool allowDraw = true);
    static std::vector<oglShader> LoadDrawFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    { return LoadFromExSrc(src, info, config, false); }
    static std::vector<oglShader> LoadComputeFromExSrc(const std::string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    { return LoadFromExSrc(src, info, config, true, false); }
    static std::vector<oglShader> LoadFromExSrc(const std::string& src, const ShaderConfig& config = {}) 
    {
        ShaderExtInfo dummy;
        return LoadFromExSrc(src, dummy, config);
    }
    static std::string_view GetStageName(const ShaderType type);
};


}

