#include "oglRely.h"
#include "oglException.h"
#include "oglShader.h"

namespace oglu
{
namespace detail
{

_oglShader::_oglShader(const ShaderType type, const string & txt) : shaderType(type), src(txt)
{
    auto ptr = txt.c_str();
    shaderID = glCreateShader(GLenum(type));
    glShaderSource(shaderID, 1, &ptr, NULL);
}

_oglShader::~_oglShader()
{
    if (shaderID != GL_INVALID_INDEX)
        glDeleteShader(shaderID);
}

void _oglShader::compile()
{
    glCompileShader(shaderID);

    GLint result;

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLsizei len = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &len);
        string logstr((size_t)len, '\0');
        glGetShaderInfoLog(shaderID, len, &len, logstr.data());
        const auto logdat = str::to_u16string(logstr.c_str());
        oglLog().warning(u"Compile shader failed:\n{}\n", logdat);
        COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, L"Compile shader failed", logdat);
    }
}

}

oglShader __cdecl oglShader::loadFromFile(const ShaderType type, const fs::path& path)
{
    using namespace common::file;
    string txt = FileObject::OpenThrow(path, OpenFlag::BINARY | OpenFlag::READ).ReadAllText();
    oglShader shader(type, txt);
    return shader;
}


vector<oglShader> __cdecl oglShader::loadFromFiles(const u16string& fname)
{
    static pair<u16string, ShaderType> types[] =
    {
        { u".vert",ShaderType::Vertex },
        { u".frag",ShaderType::Fragment },
        { u".geom",ShaderType::Geometry },
        { u".comp",ShaderType::Compute },
        { u".tscl",ShaderType::TessCtrl },
        { u".tsev",ShaderType::TessEval }
    };
    vector<oglShader> shaders;
    for (const auto& type : types)
    {
        fs::path fpath = fname + type.first;
        try
        {
            auto shader = loadFromFile(type.second, fpath);
            shaders.push_back(shader);
        }
        catch (const FileException& fe)
        {
            oglLog().warning(u"skip loading {} due to Exception[{}]", fpath.u16string(), fe.message);
        }
    }
    return shaders;
}


vector<oglShader> __cdecl oglShader::loadFromExSrc(const string& src)
{
    using std::string_view;
    vector<oglShader> shaders;
    vector<string_view> params;
    str::SplitAndDo(src, [](const char ch) { return ch == '\r' || ch == '\n'; },
        [&](const char *pos, const size_t len) 
        {
            string_view config(pos, len);
            if (!str::IsBeginWith(config, "//@@$$"))
                return;
            config.remove_prefix(6);
            str::Split(config, '|', params, false);
        }, false);
    if (params.empty())
        COMMON_THROW(BaseException, L"Invalid shader source");
    constexpr static auto glDefs = R"(
#if defined(OGLU_VERT)
#   define GLVARY out
#elif defined(OGLU_FRAG)
#   define GLVARY in
#else
#   define GLVARY 
#endif
)";
    for (const auto& sv : params)
    {
        ShaderType shaderType;
        const char *scopeDef = nullptr;
        switch (hash_(sv))
        {
        case "VERT"_hash:
            {
                shaderType = ShaderType::Vertex;
                scopeDef = "#define OGLU_VERT\n";
            } break;
        case "FRAG"_hash:
            {
                shaderType = ShaderType::Fragment;
                scopeDef = "#define OGLU_FRAG\n";
            } break;
        case "GEOM"_hash:
            {
                shaderType = ShaderType::Geometry;
                scopeDef = "#define OGLU_GEOM\n";
            } break;
        default:
            continue;
        }
        oglShader shader(shaderType, str::Concat<char>(scopeDef, glDefs, src));
        shaders.push_back(shader);
    }
    return shaders;
}

}
