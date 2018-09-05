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
    CheckCurrent();
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
        COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, u"Compile shader failed", logdat);
    }
}

}

oglShader oglShader::LoadFromFile(const ShaderType type, const fs::path& path)
{
    using namespace common::file;
    string txt = FileObject::OpenThrow(path, OpenFlag::BINARY | OpenFlag::READ).ReadAllText();
    oglShader shader(type, txt);
    return shader;
}


vector<oglShader> oglShader::LoadFromFiles(const u16string& fname)
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
            auto shader = LoadFromFile(type.second, fpath);
            shaders.push_back(shader);
        }
        catch (const FileException& fe)
        {
            oglLog().warning(u"skip loading {} due to Exception[{}]", fpath.u16string(), fe.message);
        }
    }
    return shaders;
}

constexpr static auto OGLU_DEFS = R"(
#if defined(OGLU_VERT)
#   define GLVARY out
#elif defined(OGLU_FRAG)
#   define GLVARY in
#else
#   define GLVARY 
#endif
)";
constexpr static auto OGLU_EXT_STR = "\r\n\r\n\r\n//Below generated by OpenGLUtil\r\n";
constexpr static auto OGLU_EXT_STR2 = "\r\n//Above generated by OpenGLUtil\r\n\r\n";

static const map<uint64_t, ShaderPropertyType> ShaderPropertyTypeMap = 
{
    { "VEC"_hash,    ShaderPropertyType::Vector   },
    { "MAT"_hash,    ShaderPropertyType::Matrix   },
    { "COLOR"_hash,  ShaderPropertyType::Color    },
    { "BOOL"_hash,   ShaderPropertyType::Bool     },
    { "INT"_hash,    ShaderPropertyType::Int      },
    { "UINT"_hash,   ShaderPropertyType::Uint     },
    { "RANGE"_hash,  ShaderPropertyType::Range    },
    { "FLOAT"_hash,  ShaderPropertyType::Float    }
};

struct OgluAttribute
{
    string_view Name;
    vector<string_view> Params;
    OgluAttribute(const string_view& line)
    {
        const auto p0 = line.find_first_not_of(' ');
        const auto p1 = line.find_first_of(" (", p0);
        if (p0 < p1)
        {
            Name = line.substr(p0, p1 - p0);
            const auto p2 = line.find_first_of("(", p1);
            const auto p3 = line.find_last_of(")");
            if (p2 < p3)
            {
                bool inRegion = false;
                str::SplitAndDo<char>(line.substr(p2 + 1, p3 - p2 - 1), [&](const char ch)
                {
                    if (ch == '"')
                    {
                        inRegion = !inRegion;
                        return false;
                    }
                    return !inRegion && ch == ',';
                }, [&](const char *pos, size_t len)
                {
                    while (len > 0 && *pos == ' ')
                        len--, pos++;
                    while (len > 0 && pos[len - 1] == ' ')
                        len--;
                    if (len > 0 && *pos == '"')
                        len--, pos++;
                    if (len > 0 && pos[len - 1] == '"')
                        len--;
                    Params.emplace_back(pos, len);
                }, true);
            }
        }
    }
};
static std::optional<ShaderExtProperty> ParseExtProperty(const vector<string_view>& parts)
{
    if (parts.size() < 2)
        return {};
    if (auto type = common::container::FindInMap(ShaderPropertyTypeMap, hash_(parts[1])))
    {
        if (parts.size() == 2)
            return ShaderExtProperty(string(parts[0]), *type);
        string description(parts[2]);
        try
        {
            std::any data;
            if (parts.size() > 3)
                switch (*type)
                {
                case ShaderPropertyType::Float:
                case ShaderPropertyType::Range:
                    data = std::make_pair(std::stof((string)parts[3]), std::stof((string)parts[4])); break;
                case ShaderPropertyType::Int:
                    data = std::make_pair(std::stoi((string)parts[3]), std::stoi((string)parts[4])); break;
                case ShaderPropertyType::Uint:
                    data = std::make_pair((uint32_t)std::stoul((string)parts[3]), (uint32_t)std::stoul((string)parts[4])); break;
                default: break;
                }
            return ShaderExtProperty(string(parts[0]), *type, description, data);
        }
        catch (...)
        {
            vector<string> realParts;
            std::transform(parts.cbegin(), parts.cend(), std::back_inserter(realParts), [](const string_view& sv) { return string(sv); });
            COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Error in parsing property", realParts);
        }
    }
    else
        return {};
}
static std::optional<ShaderExtProperty> ParseExtProperty(const string_view& line)
{
    const auto parts = str::Split<char>(line, '|', true);
    return ParseExtProperty(parts);
}

vector<oglShader> oglShader::LoadFromExSrc(const string& src, ShaderExtInfo& info, const bool allowCompute, const bool allowDraw)
{
    info.Properties.clear();
    info.ResMappings.clear();
    vector<oglShader> shaders;
    set<string_view> stypes;
    string_view partVersion;
    uint32_t lineCnt = 0, lineNum = 0;

    str::SplitAndDo<char>(src, [](const char ch) { return ch == '\r' || ch == '\n'; },
        [&](const char *pos, const size_t len) 
        {
            lineCnt++;
            string_view config(pos, len);
            if (str::IsBeginWith(config, "#version"))
            {
                partVersion = string_view(src.c_str(), pos - src.c_str() + len);
                lineNum = lineCnt + 1;
                return;
            }
            if (config.size() <= 6)
                return;
            if (str::IsBeginWith(config, "//@OGLU@"))
            {
                OgluAttribute ogluAttr(config.substr(8));
                switch (hash_(ogluAttr.Name))
                {
                case "Mapping"_hash:
                    if (ogluAttr.Params.size() == 2)
                        info.ResMappings.insert_or_assign(string(ogluAttr.Params[0]), string(ogluAttr.Params[1]));
                    break;
                case "Stage"_hash:
                    stypes.insert(ogluAttr.Params.cbegin(), ogluAttr.Params.cend());
                    break;
                case "Property"_hash:
                    if (auto prop = ParseExtProperty(ogluAttr.Params))
                        info.Properties.insert(prop.value());
                    break;
                default:
                    break;
                }
            }
        }, false);

    if (stypes.empty())
        COMMON_THROW(BaseException, u"Invalid shader source");
    string_view partOther(src.c_str() + partVersion.size(), src.size() - partVersion.size());
    string lineFix = "#line " + std::to_string(lineNum); //fix line number
    for (const auto& sv : stypes)
    {
        ShaderType shaderType;
        const char *scopeDef = nullptr;
        switch (hash_(sv))
        {
        case "Vertex"_hash:
        case "VERT"_hash:
            {
                shaderType = ShaderType::Vertex;
                scopeDef = "#define OGLU_VERT\r\n";
            } break;
        case "Fragment"_hash:
        case "FRAG"_hash:
            {
                shaderType = ShaderType::Fragment;
                scopeDef = "#define OGLU_FRAG\r\n";
            } break;
        case "Geometry"_hash:
        case "GEOM"_hash:
            {
                shaderType = ShaderType::Geometry;
                scopeDef = "#define OGLU_GEOM\r\n";
            } break;
        case "Compute"_hash:
        case "COMP"_hash:
            {
                shaderType = ShaderType::Compute;
                scopeDef = "#define OGLU_COMP\r\n";
            } break;
        default:
            oglLog().warning(u"meet shader type [{}], ignoreed.\n", sv);
            continue;
        }
        if (shaderType == ShaderType::Compute && !allowCompute) continue;
        if (shaderType != ShaderType::Compute && !allowDraw) continue;
        oglShader shader(shaderType, str::Concat<char>(partVersion, OGLU_EXT_STR, scopeDef, OGLU_DEFS, OGLU_EXT_STR2, lineFix, partOther));
        shaders.push_back(shader);
    }
    return shaders;
}

}
