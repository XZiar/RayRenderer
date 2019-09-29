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
    string Src;
    GLuint ShaderID;
public:
    const ShaderType Type;
    _oglShader(const ShaderType type, const string& txt);
    ~_oglShader();

    void compile();
    const string& SourceText() const { return Src; }
};

}

enum class ShaderPropertyType : uint8_t { Vector, Color, Range, Matrix, Float, Bool, Int, Uint };

struct ShaderExtProperty
{
    string Name;
    string Description;
    ShaderPropertyType Type;
    std::any Data;
    ShaderExtProperty(string name, const ShaderPropertyType type, string desc = "", std::any data = {}) :
        Name(std::move(name)), Description(std::move(desc)), Type(type), Data(std::move(data)) {}
    using Lesser = common::container::SetKeyLess<ShaderExtProperty, &ShaderExtProperty::Name>;
};

struct ShaderExtInfo
{
    set<ShaderExtProperty, ShaderExtProperty::Lesser> Properties;
    map<string, string> ResMappings;
};

struct ShaderConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, float, double, std::string>;
    map<string, DefineVal> Defines;
    map<string, string> Routines;
};

class OGLUAPI oglShader : public Wrapper<detail::_oglShader>
{
private:
public:
    using Wrapper::Wrapper;
    static oglShader LoadFromFile(const ShaderType type, const fs::path& path);
    static vector<oglShader> LoadFromFiles(fs::path fname);
    static vector<oglShader> LoadFromExSrc(const string& src, ShaderExtInfo& info, const ShaderConfig& config, const bool allowCompute = true, const bool allowDraw = true);
    static vector<oglShader> LoadDrawFromExSrc(const string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    { return LoadFromExSrc(src, info, config, false); }
    static vector<oglShader> LoadComputeFromExSrc(const string& src, ShaderExtInfo& info, const ShaderConfig& config = {})
    { return LoadFromExSrc(src, info, config, true, false); }
    static vector<oglShader> LoadFromExSrc(const string& src, const ShaderConfig& config = {}) 
    {
        ShaderExtInfo dummy;
        return LoadFromExSrc(src, dummy, config);
    }
    static string_view GetStageName(const ShaderType type);
};


}

