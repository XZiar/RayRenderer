#include "oglPch.h"
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
using common::str::Encoding;
using common::file::FileException;
using common::fs::path;
using common::container::FindInMap;
using common::container::FindInSet;
using common::container::FindInVec;
using namespace std::literals;

MAKE_ENABLER_IMPL(oglShader_)



oglShader_::oglShader_(const ShaderType type, const string & txt) :
    Src(txt), ShaderID(GL_INVALID_INDEX), Type(type)
{
    if (Type == ShaderType::Compute && !CtxFunc->SupportComputeShader)
        oglLog().warning(u"Attempt to create ComputeShader on unsupported context\n");
    else if ((Type == ShaderType::TessCtrl || Type == ShaderType::TessEval) && !CtxFunc->SupportTessShader)
        oglLog().warning(u"Attempt to create TessShader on unsupported context\n");
    auto ptr = txt.c_str();
    ShaderID = CtxFunc->CreateShader(common::enum_cast(type));
    CtxFunc->ShaderSource(ShaderID, 1, &ptr, NULL);
}

oglShader_::~oglShader_()
{
    if (ShaderID != GL_INVALID_INDEX)
        CtxFunc->DeleteShader(ShaderID);
}

void oglShader_::compile()
{
    CheckCurrent();
    CtxFunc->CompileShader(ShaderID);

    GLint result;

    CtxFunc->GetShaderiv(ShaderID, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLsizei len = 0;
        CtxFunc->GetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &len);
        string logstr((size_t)len, '\0');
        CtxFunc->GetShaderInfoLog(ShaderID, len, &len, logstr.data());
        const auto logdat = common::str::to_u16string(logstr.c_str(), Encoding::UTF8);
        oglLog().warning(u"Compile shader failed:\n{}\n", logdat);
        oglLog().verbose(u"source:\n{}\n\n", Src);
        common::SharedString<char16_t> log(logdat);
        COMMON_THROWEX(OGLException, OGLException::GLComponent::Compiler, u"Compile shader failed")
            .Attach("source", Src)
            .Attach("detail", log)
            .Attach("buildlog", log);
    }
}


oglShader oglShader_::LoadFromFile(const ShaderType type, const path& path)
{
    string txt = common::file::ReadAllText(path);
    return MAKE_ENABLER_SHARED(oglShader_, (type, txt));
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
            oglLog().warning(u"skip loading {} due to Exception[{}]", fname.u16string(), fe.Message());
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
        return common::str::SplitStream(paramPart.substr(p2 + 1, p3 - p2 - 1), [inRegion = false](const char ch) mutable
            {
                if (ch == '"')
                {
                    inRegion = !inRegion;
                    return false;
                }
                return !inRegion && ch == ',';
            }, true)
        .Select([](std::string_view sv)// (const char* pos, size_t len)
            {
                sv = common::str::TrimStringView(sv, ' ');
                sv = common::str::TrimPairStringView(sv, '"');
                return sv;
            })
        .ToVector();
    }
    return {};
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
    if (auto type = common::container::FindInMap(ShaderPropertyTypeMap, common::DJBHash::HashC(parts[1])))
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
            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Error in parsing property")
                .Attach("extparts", common::linq::FromIterable(parts).Cast<string>().ToVector());
        }
    }
    else
        return {};
}

constexpr static string_view OGLU_EXT_REQS = R"(
#line 1 1 // OGLU_EXT_REQS

#ifdef GL_ES
#   extension GL_OES_shader_io_blocks           : enable
#   extension GL_EXT_shader_io_blocks           : enable
#endif
#if defined(OGLU_VERT)
#   extension GL_ARB_explicit_attrib_location   : enable
#   extension GL_ARB_shader_draw_parameters     : enable
#elif defined(OGLU_GEOM)
#   extension GL_ARB_geometry_shader4           : enable
#endif

#   extension GL_ARB_shader_image_load_store	: enable
#   extension GL_EXT_shader_image_load_store	: enable

#extension GL_ARB_explicit_uniform_location     : enable

#extension GL_ARB_bindless_texture              : enable
#extension GL_NV_bindless_texture               : enable

#extension GL_ARB_gpu_shader_int64              : enable
#extension GL_AMD_gpu_shader_int64              : enable
#extension GL_NV_gpu_shader5                    : enable

#extension GL_ARB_shader_viewport_layer_array   : enable
#extension GL_AMD_vertex_shader_layer           : enable

#extension GL_ARB_draw_instanced                : enable
#extension GL_EXT_draw_instanced                : enable
)";

constexpr static string_view OGLU_DEFS = R"(
#line 1 3 // OGLU_DEFS
#ifdef GL_ES
#   if defined(GL_OES_shader_io_blocks) || defined(GL_EXT_shader_io_blocks)
#       define OGLU_IBLK_IO 1
#   endif
#   if __VERSION__ >= 310 || defined(GL_ARB_shader_image_load_store)
#       define OGLU_IMG_LS 1
#   endif
#else
#   if __VERSION__ >= 150
#       define OGLU_IBLK_IO 1
#   endif
#   if defined(GL_ARB_shader_image_load_store) || defined(GL_EXT_shader_image_load_store)
#       define OGLU_IMG_LS 1
#   endif
#endif
#ifndef OGLU_IBLK_IO
#   define OGLU_IBLK_IO 0
#endif
#ifndef OGLU_IMG_LS
#   define OGLU_IMG_LS 0
#endif

#ifndef OGLU_bindless_tex
#   if !defined(GL_NV_bindless_texture) && defined(GL_ARB_bindless_texture)
#       define OGLU_bindless_tex 1
#   else
#       define OGLU_bindless_tex 0
#   endif
#endif
#if OGLU_bindless_tex
#   define OGLU_TEX layout(bindless_sampler) 
#   define OGLU_IMG layout(bindless_image) 
#   define OGLU_TEX_LAYOUT ,bindless_sampler
#   define OGLU_IMG_LAYOUT ,bindless_image
#else
#   define OGLU_TEX  
#   define OGLU_IMG 
#   define OGLU_TEX_LAYOUT
#   define OGLU_IMG_LAYOUT
#endif

#if defined(GL_ARB_gpu_shader_int64) || defined(GL_AMD_gpu_shader_int64) || defined(GL_NV_gpu_shader5)
#   define OGLU_int64 1
#   define OGLU_HANDLE uint64_t
#else
#   define OGLU_int64 0
#   define OGLU_HANDLE uvec2
#endif

#if !defined(GL_ES) && __VERSION__ >= 430 && (defined(GL_ARB_shader_viewport_layer_array) || defined(GL_AMD_vertex_shader_layer))
#   define OGLU_VS_layer 1
#else
#   define OGLU_VS_layer 0
#endif
#if !defined(GL_ES) && __VERSION__ >= 430 // gl_Layer exists in FS since 4.3
#   define OGLU_FS_layer 1
#else
#   define OGLU_FS_layer 0
#endif

#if defined(OGLU_VERT)

#   define GLVARY out

#   if defined(GL_ARB_shader_draw_parameters)
#       define ogluDrawId gl_DrawIDARB
#       define ogluBaseInstance gl_BaseInstance
#   else
        //@OGLU@Mapping(DrawID, "ogluDrawId")
        in int ogluDrawId;
        //@OGLU@Mapping(BaseInstance, "ogluBaseInstance")
        uniform int ogluBaseInstance;
#   endif

#   if OGLU_VS_layer
#       define ogluLayer_ gl_Layer
#   elif OGLU_IBLK_IO
        out ogluVertData
        {
            flat int ogluLayer;
        } ogluData;
#       define ogluLayer_ ogluData.ogluLayer
#   else
        out flat int ogluLayer_;
#   endif
#   if OGLU_VS_layer
        void ogluSetLayer(const in  int layer)
        {
            gl_Layer = ogluLayer_ = layer;
        }
        void ogluSetLayer(const in uint layer)
        {
            gl_Layer = ogluLayer_ = int(layer);
        }
#   elif OGLU_IBLK_IO
        void ogluSetLayer(const in  int layer)
        {
            ogluData.ogluLayer = layer;
        }
        void ogluSetLayer(const in uint layer)
        {
            ogluData.ogluLayer = int(layer);
        }
#   else
        void ogluSetLayer(const in  int layer)
        {
            ogluLayer_ = layer;
        }
        void ogluSetLayer(const in uint layer)
        {
            ogluLayer_ = int(layer);
        }
#   endif

#elif defined(OGLU_GEOM)

#   if !OGLU_IBLK_IO
#       error with geometry shader, should support io_interface_block
#   endif
    in  ogluVertData
    {
        flat int ogluLayer;
    } ogluDataIn[];
#   if !OGLU_FS_layer
    out ogluVertData
    {
        flat int ogluLayer;
    } ogluData;
#   endif
    // to please vmware which does not support varadic macro function
#   define ogluLayerIn      ogluDataIn[0].ogluLayer
#   define ogluGetLayer(x)  ogluDataIn[x].ogluLayer
#   if OGLU_FS_layer
        void ogluSetLayer(const in  int layer)
        {
            gl_Layer = layer;
        }
        void ogluSetLayer(const in uint layer)
        {
            gl_Layer = int(layer);
        }
#   else
        void ogluSetLayer(const in  int layer)
        {
            gl_Layer            = layer;
            ogluData.ogluLayer  = layer;
        }
        void ogluSetLayer(const in uint layer)
        {
            gl_Layer            = int(layer);
            ogluData.ogluLayer  = int(layer);
        }
#   endif

#elif defined(OGLU_FRAG)

#   define GLVARY in

#   if OGLU_FS_layer
#       define ogluLayer gl_Layer
#   elif OGLU_IBLK_IO
    in  ogluVertData
    {
        flat int ogluLayer;
    } ogluDataIn;
#       define ogluLayer ogluDataIn.ogluLayer
#   else
    in flat int ogluLayer_;
#       define ogluLayer ogluLayer_
#   endif

#else

#   define GLVARY 

#endif
)"sv;

struct SubroutineItem
{
    mutable map<string_view, size_t, std::less<>> Routines;
    mutable string Routine;
    string_view SubroutineName;
    string_view RoutineVal;
    string_view ReturnType;
    string_view FuncParams;
    const size_t LineNum;
    SubroutineItem(const string_view& line, const size_t lineNum) : LineNum(lineNum)
    {
        const auto p0 = line.find_first_not_of(' ');
        const auto p1 = line.find_first_of("(", p0);
        if (p0 < p1)
        {
            const auto params = ExtractParams(line, p1);
            if (params.size() < 3) return;
            SubroutineName = params[0];
            RoutineVal = params[1];
            ReturnType = params[2];
            if (params.size() > 3)
                FuncParams = string_view(params[3].data(), params.back().data() + params.back().size() - params[3].data());
        }
    }
    bool SetRoutine(const string& routine) const
    {
        if (FindInMap(Routines, routine) != nullptr)
        {
            Routine = routine;
            return true;
        }
        return false;
    }
    void Apply(const bool emulate, vector<std::variant<string_view, string>>& lines, 
        std::map<std::string, ShaderExtInfo::EmSubroutine>& emulateOutput) const
    {
        std::string entry, prefix;
        if (!Routine.empty())
            entry  = "#define {1} {4}", 
            prefix = "";
        else if (!emulate)
            entry  = "subroutine {2} {0}({3}); subroutine uniform {0} {1};", 
            prefix = "subroutine("s.append(SubroutineName).append(") ");
        else
        {
            ShaderExtInfo::EmSubroutine emulateInfo;
            emulateInfo.UniformName = SubroutineName;
            const auto args = FuncParams.empty() ?
                "" :
                common::str::SplitStream(FuncParams, ',')
                    .Select([](const auto& param) { return param.substr(param.find_last_of(" ") + 1); })
                    .Reduce([](string& ret, const auto& arg, size_t idx)
                        {
                            if (idx > 0) ret.append(", ");
                            ret.append(arg);
                        }, string());
            entry.append("uniform uint {0};\r\n");
            entry.append("#line 1 4 // OGLU_SR ").append(SubroutineName).append("\r\n");
            for (const auto& [routine, rtline] : Routines)
            {
                fmt::format_to(std::back_inserter(entry), "{} {}({});\r\n", ReturnType, routine, FuncParams);
            }
            entry.append("{2} {1}({3})\r\n");
            entry.append("{{\r\n");
            entry.append("    switch ({0})\r\n");
            entry.append("    {{\r\n");
            for(const auto & [srname, srline] : Routines)
            {
                fmt::format_to(std::back_inserter(entry), "    case {}u: return {}({});\r\n", srline, srname, args);
                emulateInfo.Routines.emplace_back(srname, srline);
            }
            entry.append("    }}\r\n");
            entry.append("}}\r\n");
            entry.append("#line ").append(std::to_string(LineNum + 1)).append(" 0 \r\n");
            prefix = "";
            const bool notReplace = emulateOutput.insert_or_assign(std::string(RoutineVal), std::move(emulateInfo)).second;
            if (!notReplace)
                oglLog().warning(u"Routine [{}]'s previous emulate info is overwrited, may cause bug.", SubroutineName);
        }
        static thread_local std::vector<char> buf;
        buf.reserve(100);
        {
            buf.resize(0);
            fmt::format_to(std::back_inserter(buf), entry, SubroutineName, RoutineVal, ReturnType, FuncParams, Routine);
            lines[LineNum] = string(buf.data(), buf.size());
        }
        for (const auto&[routine, rtline] : Routines)
        {
            buf.resize(0);
            fmt::format_to(std::back_inserter(buf), "{0}{1} {2}({3})", prefix, ReturnType, routine, FuncParams);
            lines[rtline] = string(buf.data(), buf.size());
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
    set<SubroutineItem, common::container::SetKeyLess<SubroutineItem, &SubroutineItem::RoutineVal>> subroutines;
    vector<std::variant<string_view, string>> lines;
    size_t verLineNum = string::npos, stageLineNum = string::npos;

    info.ResMappings.insert_or_assign("DrawID", "ogluDrawId");
    for (std::string_view line : common::str::SplitStream(src, '\n', true))
    {
        const size_t curLine = lines.size();
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1); // fix for "\r\n"
        lines.push_back(line);
        const auto p0 = line.find_first_not_of(' ');
        const string_view realline = line.substr(p0 == string_view::npos ? 0 : p0);
        if (realline.size() <= 6) continue;

        if (common::str::IsBeginWith(realline, "#version"))
        {
            verLineNum = curLine;
        }
        else if (common::str::IsBeginWith(realline, "//@OGLU@"))
        {
            OgluAttribute ogluAttr(realline.substr(8));
            switch (common::DJBHash::HashC(ogluAttr.Name))
            {
            case "Mapping"_hash:
                if (ogluAttr.Params.size() == 2)
                    info.ResMappings.insert_or_assign(string(ogluAttr.Params[0]), string(ogluAttr.Params[1]));
                break;
            case "Stage"_hash:
                stageLineNum = curLine;
                stypes.insert(ogluAttr.Params.cbegin(), ogluAttr.Params.cend());
                break;
            case "StageIf"_hash:
                stageLineNum = curLine;
                if (ogluAttr.Params.size() > 1)
                {
                    const bool allmatch = common::str::SplitStream(ogluAttr.Params[0], ';', false)
                        .AllIf([&](string_view ext)
                            {
                                const bool flip = ext[0] == '!';
                                if (flip)
                                    ext.remove_prefix(1);
                                if (common::str::IsBeginWith(ext, "GL_"))
                                    return flip ^ CtxFunc->Extensions.Has(ext);
                                else if (common::str::IsBeginWith(ext, "GLVERSION"))
                                {
                                    const uint32_t needVersion = std::stoi(string(ext.substr(9)));
                                    return flip ^ (CtxFunc->Version >= needVersion);
                                }
                                else
                                    return flip ^ (config.Defines.Has(ext));
                            });
                    if (allmatch)
                        stypes.insert(ogluAttr.Params.cbegin() + 1, ogluAttr.Params.cend());
                }
                break;
            case "Property"_hash:
                if (auto prop = ParseExtProperty(ogluAttr.Params))
                    info.Properties.emplace(prop.value());
                break;
            default:
                break;
            }
        }
        else if (common::str::IsBeginWith(realline, "OGLU_ROUTINE("))
        {
            const auto& [it, ret] = subroutines.insert(SubroutineItem(line, curLine));
            if (!ret)
                oglLog().warning(u"Repeat routine found: [{}]\n Previous was: [{}]\n", line, std::get<string_view>(lines[it->LineNum]));
        }
        else if (common::str::IsBeginWith(realline, "OGLU_SUBROUTINE("))
        {
            const auto sub = SubroutineItem::TryParseSubroutine(line);
            if (sub.has_value())
            {
                const auto ptrRoutine = FindInVec(subroutines, [&](const auto& r) { return r.SubroutineName == sub->first; });// FindInSet(routines, sub->first);
                if (ptrRoutine)
                {
                    const auto& [it, ret] = ptrRoutine->Routines.emplace(sub->second, curLine);
                    if (!ret)
                        oglLog().warning(u"Repeat subroutine found: [{}]\n Previous was: [{}]\n", line, std::get<string_view>(lines[it->second]));
                }
                else
                    oglLog().warning(u"No routine [{}] found for subroutine [{}]\n", sub->first, sub->second);
            }
            else
                oglLog().warning(u"Unknown subroutine declare: [{}]\n", line);
        }
    }

    //clean up removed stages
    {
        vector<string_view> removeStages; 
        removeStages.reserve(stypes.size() * 2);
        for (const auto stage : stypes)
            if (stage[0] == '!')
                removeStages.emplace_back(stage), removeStages.emplace_back(stage.substr(1));
        for (const auto stage : removeStages)
            stypes.erase(stage);
    }
    if (stypes.empty())
        COMMON_THROWEX(BaseException, u"Invalid shader source");

    const string_view partHead = verLineNum == string::npos ? "" : string_view(src.data(), std::get<string_view>(lines[verLineNum]).data() - src.data());
    if (stageLineNum == string::npos)
        stageLineNum = verLineNum;
    size_t restLineNum = stageLineNum != string::npos ? stageLineNum + 1 : 0;
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
                oglLog().warning(u"unsupported GLSL version, fallback to highest support version");
        }
    }
    if (version == UINT32_MAX)
    {
        if (CtxFunc->ContextType == GLType::ES)
        {
            if (CtxFunc->Version >= 30)
                version = CtxFunc->Version * 10, verSuffix = " es";
            else
                version = 100;
        }
        else
        {
            if (CtxFunc->Version >= 33)
                version = CtxFunc->Version * 10;
            else
                switch (CtxFunc->Version)
                {
                case 32:    version = 150; break;
                case 31:    version = 140; break;
                case 30:    version = 130; break;
                case 21:    version = 120; break;
                case 20:    version = 110; break;
                default:    COMMON_THROWEX(BaseException, u"no correct GLSL version found, unsupported GL version");
                }
        }
        if (verPrefix.empty())
            verPrefix = "#version ";
    }

    string extReqs;

    //apply routines
    for (const auto&[rname, srname] : config.Routines)
    {
        const auto ptrRoutine = FindInSet(subroutines, rname);
        if (ptrRoutine)
        {
            if (ptrRoutine->SetRoutine(srname))
                oglLog().verbose(u"Static use subroutine [{}] for routine [{}].\n", srname, rname);
            else
                oglLog().warning(u"Unknown subroutine [{}] for routine [{}] in the config.\n", srname, rname);
        }
        else
            oglLog().warning(u"Unknown routine [{}] with subroutine [{}] in the config.\n", rname, srname);
    }
    const bool needEmulate = !CtxFunc->SupportSubroutine;
    for (const auto& sr : subroutines)
        sr.Apply(needEmulate, lines, info.EmulateSubroutines);
    if (common::linq::FromIterable(subroutines)
        .ContainsIf([](const auto& sr) { return sr.Routine.empty(); }))
    {
        extReqs.append("#extension GL_ARB_shader_subroutine : enable\r\n");
        if (!CtxFunc->SupportSubroutine)
            oglLog().warning(u"subroutine requested on a unspported context, will try to emulate it.\n");
    }

    //apply defines
    string defs = "\r\n// Below are defines injected by configs\r\n\r\n";
    for (const auto def : config.Defines)
    {
        defs.append("#define ").append(def.Key).append(" ");
        std::visit([&](auto&& val)
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, string_view>)
                defs.append(val);
            else if constexpr (!std::is_same_v<T, std::monostate>)
                defs.append(std::to_string(val));
        }, def.Val);
        defs.append("\r\n");
    }

    //process content between version and stage declare
    string beforeInit;
    {
        size_t line = (verLineNum == string::npos ? 0 : verLineNum + 1);
        beforeInit.append("#line ").append(std::to_string(line + 1)).append(" 0 \r\n");
        while (line < restLineNum)
            std::visit([&](const auto& val) { beforeInit.append(val).append("\r\n"); }, lines[line++]);
    }

    //prepare rest content
    string rest; 
    rest.reserve(src.size() + 1024);
    for (auto it = lines.cbegin() + restLineNum; it != lines.cend(); ++it)
        std::visit([&](const auto& val) { rest.append(val).append("\r\n"); }, *it);
    const string lineFix = "#line " + std::to_string(restLineNum + 1) + " 0\r\n"; //fix line number

    string defFix;
    if (!CtxFunc->SupportBindlessTexture)
        defFix.append("\r\n#define OGLU_bindless_tex 0 // OGLU reports not supported");
    string finalShader;
    finalShader.reserve(src.size() + 4096); 
    for (const auto& stype : stypes)
    {
        uint32_t curVer = version;
        ShaderType shaderType;
        string_view scopeDef;
        switch (common::DJBHash::HashC(stype))
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
                curVer = std::max(curVer, CtxFunc->ContextType == GLType::ES ? 310u : 430u);
            } break;
        default:
            oglLog().warning(u"meet shader type [{}], ignored.\n", stype);
            continue;
        }
        if (shaderType == ShaderType::Compute && !allowCompute) continue;
        if (shaderType != ShaderType::Compute && !allowDraw) continue;
        finalShader.clear();
        finalShader
            .append(partHead).append(fmt::format("{}{}{}\r\n", verPrefix, curVer, verSuffix))

            .append("\r\n\r\n//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n//Below generated by OpenGLUtil\r\n\r\n")
            .append(scopeDef)
            .append(OGLU_EXT_REQS).append(extReqs)
            .append("\r\n//Above generated by OpenGLUtil\r\n//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n\r\n\r\n")

            .append(beforeInit)

            .append("\r\n\r\n//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n//Below generated by OpenGLUtil\r\n\r\n")
            .append("\r\n#line 1 2 // OGLU_DEFS_FIX").append(defFix)
            .append(OGLU_DEFS).append(defs)
            .append("\r\n//Above generated by OpenGLUtil\r\n//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n\r\n\r\n")

            .append(lineFix)
            .append(rest);

        auto shader = MAKE_ENABLER_SHARED(oglShader_, (shaderType, finalShader));
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
