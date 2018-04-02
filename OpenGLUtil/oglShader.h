#pragma once
#include "oglRely.h"

namespace oglu
{

enum class ShaderType : GLenum
{
    Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
    TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
    Compute = GL_COMPUTE_SHADER
};


namespace detail
{

class OGLUAPI _oglShader : public NonCopyable
{
    friend class _oglProgram;
public:
    const ShaderType shaderType;
private:
    GLuint shaderID = GL_INVALID_INDEX;
    string src;
public:
    _oglShader(const ShaderType type, const string& txt);
    ~_oglShader();

    void compile();
    const string& SourceText() const { return src; }
};

}

enum class ShaderPropertyType : uint8_t { Vector, Color, Range, Matrix, Float, Bool, Int, Uint };

struct OGLUAPI ShaderExtProperty : public common::container::NamedSetValue<ShaderExtProperty, string>
{
    const string Name;
    const string Description;
    const ShaderPropertyType Type;
    const std::any Data;
    ShaderExtProperty(const string& name, const ShaderPropertyType type, const string& desc = "", const std::any& data = {}) :Name(name), Description(desc), Type(type), Data(data) {}
    bool MatchType(const GLenum glType) const
    {
        switch (Type)
        {
        case ShaderPropertyType::Vector: return (glType >= GL_FLOAT_VEC2 && glType <= GL_INT_VEC4) || (glType >= GL_BOOL_VEC2 && glType <= GL_BOOL_VEC4) ||
            (glType >= GL_UNSIGNED_INT_VEC2 && glType <= GL_UNSIGNED_INT_VEC4) || (glType >= GL_DOUBLE_VEC2 && glType <= GL_DOUBLE_VEC4);
        case ShaderPropertyType::Bool: return glType == GL_BOOL;
        case ShaderPropertyType::Int: return glType == GL_INT;
        case ShaderPropertyType::Uint: return glType == GL_UNSIGNED_INT;
        case ShaderPropertyType::Float: return glType == GL_FLOAT;
        case ShaderPropertyType::Color: return glType == GL_FLOAT_VEC4;
        case ShaderPropertyType::Range: return glType == GL_FLOAT_VEC2;
        case ShaderPropertyType::Matrix: return (glType >= GL_FLOAT_MAT2 && glType <= GL_FLOAT_MAT4) || (glType >= GL_DOUBLE_MAT2 && glType <= GL_DOUBLE_MAT4x3);
        default: return false;
        }
    }
};

class OGLUAPI oglShader : public Wrapper<detail::_oglShader>
{
public:
    using Wrapper::Wrapper;
    static oglShader __cdecl loadFromFile(const ShaderType type, const fs::path& path);
    static vector<oglShader> __cdecl loadFromFiles(const u16string& fname);
    static vector<oglShader> __cdecl loadFromExSrc(const string& src, set<ShaderExtProperty, std::less<>>& properties);
    static vector<oglShader> __cdecl loadFromExSrc(const string& src) 
    {
        set<ShaderExtProperty, std::less<>> dummy;
        return loadFromExSrc(src, dummy);
    }
};


}

