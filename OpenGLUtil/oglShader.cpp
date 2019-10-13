#include "oglPch.h"
#include "oglException.h"
#include "oglShader.h"
#include <regex>


namespace oglu
{
using std::string;
using std::string_view;
using std::u16string;
using std::u16string_view;
using std::set;
using std::map;
using std::pair;
using std::vector;
using std::regex;
using std::smatch;
using common::BaseException;
using common::str::Charset;
using common::file::FileException;
using common::fs::path;
using common::linq::Linq;
using common::container::FindInMap;
using common::container::FindInSet;
using common::container::FindInVec;
using namespace std::literals;

MAKE_ENABLER_IMPL(oglShader_)



oglShader_::oglShader_(const ShaderType type, const string & txt) :
    Src(txt), ShaderID(GL_INVALID_INDEX), Type(type)
{
    auto ptr = txt.c_str();
    ShaderID = glCreateShader(common::enum_cast(type));
    glShaderSource(ShaderID, 1, &ptr, NULL);
}

oglShader_::~oglShader_()
{
    if (ShaderID != GL_INVALID_INDEX)
        glDeleteShader(ShaderID);
}

void oglShader_::compile()
{
    CheckCurrent();
    glCompileShader(ShaderID);

    GLint result;

    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLsizei len = 0;
        glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &len);
        string logstr((size_t)len, '\0');
        glGetShaderInfoLog(ShaderID, len, &len, logstr.data());
        const auto logdat = common::strchset::to_u16string(logstr.c_str(), Charset::UTF8);
        oglLog().warning(u"Compile shader failed:\n{}\n", logdat);
        COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, u"Compile shader failed", logdat);
    }
}


oglShader oglShader_::LoadFromFile(const ShaderType type, const path& path)
{
    string txt = common::file::ReadAllText(path);
    return MAKE_ENABLER_SHARED(oglShader_, type, txt);
}


vector<oglShader> oglShader_::LoadFromFiles(path fname)
{
    static constexpr pair<u16string_view, ShaderType> types[] =
    {
        { u".vert", ShaderType::Vertex },
        { u".frag", ShaderType::Fragment },
        { u".geom", ShaderType::Geometry },
        { u".comp", ShaderType::Compute },
        { u".tscl", ShaderType::TessCtrl },
        { u".tsev", ShaderType::TessEval }
    };
    vector<oglShader> shaders;
    for (const auto& [ext,type] : types)
    {
        fname.replace_extension(ext);
        try
        {
            auto shader = LoadFromFile(type, fname);
            shaders.emplace_back(shader);
        }
        catch (const FileException& fe)
        {
            oglLog().warning(u"skip loading {} due to Exception[{}]", fname.u16string(), fe.message);
        }
    }
    return shaders;
}


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

static vector<string_view> ExtractParams(const string_view& paramPart, const size_t startPos = 0)
{
    vector<string_view> params;
    const auto p2 = paramPart.find_first_of("(", startPos);
    const auto p3 = paramPart.find_last_of(")");
    if (p2 < p3)
    {
        bool inRegion = false;
        common::str::SplitAndDo<char>(paramPart.substr(p2 + 1, p3 - p2 - 1), [&](const char ch)
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
            if (len > 1 && *pos == '"' && pos[len - 1] == '"')
                len-=2, pos++;
            params.emplace_back(pos, len);
        }, true);
    }
    return params;
}

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
            Params = ExtractParams(line, p1);
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
            COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Error in parsing property",
                Linq::FromIterable(parts).Cast<string>().ToVector());
        }
    }
    else
        return {};
}

constexpr static auto OGLU_DEFS = R"(
#extension GL_ARB_shader_draw_parameters : enable
#if defined(OGLU_VERT)
#   define GLVARY out
#   if defined(GL_ARB_shader_draw_parameters) && GL_ARB_shader_draw_parameters
#       define ogluDrawId gl_DrawIDARB
#   else
        //@OGLU@Mapping(DrawID, "ogluDrawId")
        in int ogluDrawId;
#endif
#elif defined(OGLU_FRAG)
#   define GLVARY in
#else
#   define GLVARY 
#endif
)"sv;

struct RoutineItem
{
    mutable map<string_view, size_t, std::less<>> Subroutines;
    mutable string Subroutine;
    string_view RoutineName;
    string_view RoutineVal;
    string_view ReturnType;
    string_view FuncParams;
    const size_t LineNum;
    RoutineItem(const string_view& line, const size_t lineNum) : LineNum(lineNum)
    {
        const auto p0 = line.find_first_not_of(' ');
        const auto p1 = line.find_first_of("(", p0);
        if (p0 < p1)
        {
            const auto params = ExtractParams(line, p1);
            if (params.size() < 3) return;
            RoutineName = params[0];
            RoutineVal = params[1];
            ReturnType = params[2];
            if (params.size() > 3)
                FuncParams = string_view(params[3].data(), params.back().data() + params.back().size() - params[3].data());
        }
    }
    bool SetSubroutine(const string& srname) const
    {
        if (FindInMap(Subroutines, srname) != nullptr)
        {
            Subroutine = srname;
            return true;
        }
        return false;
    }
    void Apply(vector<std::variant<string_view, string>>& lines) const
    {
        static thread_local fmt::basic_memory_buffer<char> buf;
        {
            buf.resize(0);
            fmt::format_to(buf, Subroutine.empty() ? "subroutine {2} {0}({3}); subroutine uniform {0} {1};" : "#define {1} {4}",
                RoutineName, RoutineVal, ReturnType, FuncParams, Subroutine);
            lines[LineNum] = string(buf.data(), buf.size());
        }
        const string srprefix = Subroutine.empty() ? "subroutine("s.append(RoutineName).append(") ") : ""s;
        for (const auto&[srname, srline] : Subroutines)
        {
            buf.resize(0);
            fmt::format_to(buf, "{0}{1} {2}({3})", srprefix, ReturnType, srname, FuncParams);
            lines[srline] = string(buf.data(), buf.size());
        }
    }
    static std::optional<std::pair<string_view, string_view>> TryParseSubroutine(const string_view& line)
    {
        const auto p0 = line.find_first_not_of(' ');
        const auto p1 = line.find_first_of(" (", p0);
        if (p0 < p1)
        {
            const auto params = ExtractParams(line, p1);
            if (params.size() == 2)
                return std::pair{ params[0],params[1] };
        }
        return {};
    }
};


vector<oglShader> oglShader_::LoadFromExSrc(const string& src, ShaderExtInfo& info, const ShaderConfig& config, const bool allowCompute, const bool allowDraw)
{
    info.Properties.clear();
    info.ResMappings.clear();
    vector<oglShader> shaders;
    set<string_view> stypes;
    set<RoutineItem, common::container::SetKeyLess<RoutineItem, &RoutineItem::RoutineVal>> routines;
    vector<std::variant<string_view, string>> lines;
    string finalShader;
    size_t verLineNum = string::npos, initLineNum = string::npos;

    finalShader.reserve(src.size() + 1024);
    info.ResMappings.insert_or_assign("DrawID", "ogluDrawId");
    common::str::SplitAndDo<char>(src, '\n',
        [&](const char *pos, size_t len) 
        {
            const size_t curLine = lines.size();
            if (pos[len - 1] == '\r') len--; // fix for "\r\n"
            string_view line(pos, len);
            lines.push_back(line);
            const auto p0 = line.find_first_not_of(' ');
            const string_view realline = line.substr(p0 == string_view::npos ? 0 : p0);
            if (realline.size() <= 6) return;
            if (common::str::IsBeginWith(realline, "#version"))
            {
                verLineNum = curLine;
            }
            else if (common::str::IsBeginWith(realline, "//@OGLU@"))
            {
                OgluAttribute ogluAttr(realline.substr(8));
                switch (hash_(ogluAttr.Name))
                {
                case "Mapping"_hash:
                    if (ogluAttr.Params.size() == 2)
                        info.ResMappings.insert_or_assign(string(ogluAttr.Params[0]), string(ogluAttr.Params[1]));
                    break;
                case "Stage"_hash:
                    initLineNum = curLine;
                    stypes.insert(ogluAttr.Params.cbegin(), ogluAttr.Params.cend());
                    break;
                case "Property"_hash:
                    if (auto prop = ParseExtProperty(ogluAttr.Params))
                        info.Properties.emplace(prop.value());
                    break;
                default:
                    break;
                }
            }
            else if(common::str::IsBeginWith(realline, "OGLU_ROUTINE("))
            {
                const auto&[it,ret] = routines.insert(RoutineItem(line, curLine));
                if (!ret)
                    oglLog().warning(u"Repeat routine found: [{}]\n Previous was: [{}]\n", line, std::get<string_view>(lines[it->LineNum]));
            }
            else if(common::str::IsBeginWith(realline, "OGLU_SUBROUTINE("))
            {
                const auto sub = RoutineItem::TryParseSubroutine(line);
                if (sub.has_value())
                {
                    const auto ptrRoutine = FindInVec(routines, [&](const auto& r) { return r.RoutineName == sub->first; });// FindInSet(routines, sub->first);
                    if (ptrRoutine)
                    {
                        const auto&[it, ret] = ptrRoutine->Subroutines.emplace(sub->second, curLine);
                        if (!ret)
                            oglLog().warning(u"Repeat subroutine found: [{}]\n Previous was: [{}]\n", line, std::get<string_view>(lines[it->second]));
                    }
                    else
                        oglLog().warning(u"No routine [{}] found for subroutine [{}]\n", sub->first, sub->second);
                }
                else
                    oglLog().warning(u"Unknown subroutine declare: [{}]\n", line);
            }
        }, true);

    const string_view partHead = verLineNum == string::npos ? "" : string_view(src.data(), std::get<string_view>(lines[verLineNum]).data() - src.data());
    if (initLineNum == string::npos) 
        initLineNum = verLineNum;
    size_t restLineNum = initLineNum != string::npos ? initLineNum + 1 : 0;
    string verPrefix, verSuffix;
    uint32_t version = UINT32_MAX;
    if (verLineNum != string::npos)
    {
        std::smatch mth;
        const auto verLine = string(std::get<string_view>(lines[verLineNum]));
        if (std::regex_search(verLine, mth, std::regex(R"((\#\s*version\s+)([1-9][0-9]0))")))
        {
            verPrefix = mth[1], verSuffix = verLine.substr(mth[0].length());
            version = std::stoi(mth[2]);
            constexpr std::array vers{ 110u,120u,130u,140u,150u,330u,400u,410u,420u,430u,440u,450u,460u };
            if (!std::binary_search(vers.cbegin(), vers.cend(), version))
                COMMON_THROW(BaseException, u"unsupported GLSL version");
        }
    }
    if (version == UINT32_MAX)
        COMMON_THROW(BaseException, u"no correct GLSL version found");
    if (stypes.empty())
        COMMON_THROW(BaseException, u"Invalid shader source");
    //apply routines
    for (const auto&[rname, srname] : config.Routines)
    {
        const auto ptrRoutine = FindInSet(routines, rname);
        if (ptrRoutine)
        {
            if (ptrRoutine->SetSubroutine(srname))
                oglLog().verbose(u"Static use subroutine [{}] for routine [{}].\n", srname, rname);
            else
                oglLog().warning(u"Unknown subroutine [{}] for routine [{}] in the config.\n", srname, rname);
        }
        else
            oglLog().warning(u"Unknown routine [{}] with subroutine [{}] in the config.\n", rname, srname);
    }
    for (const auto sr : routines)
        sr.Apply(lines);
    //apply defines
    string defs;
    for (const auto& def : config.Defines)
    {
        defs.append("#define ").append(def.first).append(" ");
        std::visit([&](auto&& val)
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, string>)
                defs.append(val);
            else if constexpr (!std::is_same_v<T, std::monostate>)
                defs.append(std::to_string(val));
        }, def.second);
        defs.append("\r\n");
    }
    //
    string beforeInit;
    for (size_t line = verLineNum == string::npos ? 0 : verLineNum+1; line < restLineNum; ++line)
        std::visit([&](const auto& val) { beforeInit.append(val).append("\r\n"); }, lines[line]);

    //prepare rest content
    string rest; 
    rest.reserve(src.size() + 1024);
    for (auto it = lines.cbegin() + restLineNum; it != lines.cend(); ++it)
        std::visit([&](const auto& val) { rest.append(val).append("\r\n"); }, *it);

    const string lineFix = "#line " + std::to_string(restLineNum + 1) + "\r\n"; //fix line number
    for (const auto& stype : stypes)
    {
        uint32_t curVer = version;
        ShaderType shaderType;
        const char *scopeDef = nullptr;
        switch (hash_(stype))
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
                if (curVer < 430) curVer = 430;
                    shaderType = ShaderType::Compute;
                scopeDef = "#define OGLU_COMP\r\n";
            } break;
        default:
            oglLog().warning(u"meet shader type [{}], ignoreed.\n", stype);
            continue;
        }
        if (shaderType == ShaderType::Compute && !allowCompute) continue;
        if (shaderType != ShaderType::Compute && !allowDraw) continue;
        finalShader.clear();
        finalShader.append(partHead).append(fmt::format("{}{}{}\r\n", verPrefix, curVer, verSuffix))
            .append(beforeInit)
            .append("\r\n\r\n//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n//Below generated by OpenGLUtil\r\n")
            .append(scopeDef).append(OGLU_DEFS).append(defs);
        
        finalShader.append("\r\n//Above generated by OpenGLUtil\r\n//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n\r\n")
            .append(lineFix).append(rest);
        auto shader = MAKE_ENABLER_SHARED(oglShader_, shaderType, finalShader);
        shaders.push_back(shader);
    }
    return shaders;
}


string_view oglShader_::GetStageName(const ShaderType type)
{
    switch (type)
    {
    case ShaderType::Vertex:    return "Vertex"sv;
    case ShaderType::Fragment:  return "Fragment"sv;
    case ShaderType::Geometry:  return "Geometry"sv;
    case ShaderType::TessCtrl:  return "TessCtrl"sv;
    case ShaderType::TessEval:  return "TessEval"sv;
    case ShaderType::Compute:   return "Compute"sv;
    default:                    return "Unknown"sv;
    }
}


}
