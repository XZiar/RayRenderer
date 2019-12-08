#include "oglPch.h"
#include "oglException.h"
#include "oglProgram.h"
#include "oglFBO.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "BindingManager.h"
#include "oglStringify.hpp"


namespace oglu
{
using std::string;
using std::string_view;
using std::u16string;
using std::set;
using std::map;
using std::pair;
using std::vector;
using common::str::Charset;
using common::container::FindInSet;
using common::container::FindInMap;
using common::container::FindInMapOrDefault;
using common::container::FindInVec;
using common::container::ReplaceInVec;
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
using namespace std::literals;

MAKE_ENABLER_IMPL(oglDrawProgram_)
MAKE_ENABLER_IMPL(oglComputeProgram_)


template<typename T = GLint>
static T ProgResGetValue(const GLuint pid, const GLenum type, const GLint ifidx, const GLenum prop)
{
    GLint ret;
    CtxFunc->ogluGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
    return static_cast<T>(ret);
}


string_view ProgramResource::GetTypeName() const noexcept
{
    switch (ResType & ProgResType::MASK_CATEGORY)
    {
    case ProgResType::CAT_INPUT:    return "Attrib"sv;
    case ProgResType::CAT_OUTPUT:   return "Output"sv;
    case ProgResType::CAT_UBO:      return "UniBlk"sv;
    case ProgResType::CAT_UNIFORM:
        switch (ResType & ProgResType::MASK_TYPE_CAT)
        {
        case ProgResType::TYPE_TEX: return "TexUni"sv;
        case ProgResType::TYPE_IMG: return "ImgUni"sv;
        default:                    return "RawUni"sv;
        }
    default:                        return "Other "sv;
    }
}

string_view ProgramResource::GetValTypeName() const noexcept
{
    switch (Valtype)
    {
    case GL_UNIFORM_BLOCK:
        return "uniBlock"sv;
    default:
        return FindInMap(detail::GLENUM_STR, Valtype, std::in_place).value_or("UNKNOWN"sv);
    }
}

static ProgramResource GenProgRes(const string& name, const GLenum type, const GLuint ProgramID, const GLint idx)
{
    ProgramResource progres;
    progres.Name = name;
    progres.Type = type;
    progres.ifidx = (uint8_t)idx;
    progres.ResType = ProgResType::Empty;
    if (type == GL_UNIFORM_BLOCK)
    {
        progres.Valtype = GL_UNIFORM_BLOCK;
        progres.ResType |= ProgResType::CAT_UBO;
        progres.location = CtxFunc->ogluGetProgramResourceIndex(ProgramID, type, name.c_str());
        progres.size = ProgResGetValue<uint16_t>(ProgramID, type, idx, GL_BUFFER_DATA_SIZE);
        progres.len = 1;
    }
    else
    {
        progres.Valtype = ProgResGetValue<GLenum>(ProgramID, type, idx, GL_TYPE);
        switch (type)
        {
        case GL_PROGRAM_INPUT:
            progres.ResType |= ProgResType::CAT_INPUT;
            break;
        case GL_PROGRAM_OUTPUT:
            progres.ResType |= ProgResType::CAT_OUTPUT;
            break;
        case GL_UNIFORM:
            progres.ResType |= ProgResType::CAT_UNIFORM;
            if (progres.Valtype >= GL_IMAGE_1D && progres.Valtype <= GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
                progres.ResType |= ProgResType::TYPE_IMG;
            else if ((progres.Valtype >= GL_SAMPLER_1D && progres.Valtype <= GL_SAMPLER_2D_RECT_SHADOW)
                || (progres.Valtype >= GL_SAMPLER_1D_ARRAY && progres.Valtype <= GL_SAMPLER_CUBE_SHADOW)
                || (progres.Valtype >= GL_INT_SAMPLER_1D && progres.Valtype <= GL_UNSIGNED_INT_SAMPLER_BUFFER)
                || (progres.Valtype >= GL_SAMPLER_2D_MULTISAMPLE && progres.Valtype <= GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY))
                progres.ResType |= ProgResType::TYPE_TEX;
            else
                progres.ResType |= ProgResType::TYPE_PRIMITIVE;
            break;
        }
        progres.location = CtxFunc->ogluGetProgramResourceLocation(ProgramID, type, name.c_str());
        progres.size = 0;
        progres.len = ProgResGetValue<GLuint>(ProgramID, type, idx, GL_ARRAY_SIZE);
    }
    return progres;
}


void MappingResource::Init(std::vector<ProgramResource>&& resources, const std::map<std::string, std::string>& mappings)
{
    Resources = resources;
    const auto indexed = common::linq::FromIterable(Resources)
        .Select([](const auto& res) { return std::pair(res.location, &res); })
        .ToVector();
    IndexedResources = indexed;

    std::vector<std::tuple<size_t, size_t, const ProgramResource*>> mapped;
    mapped.reserve(mappings.size());
    size_t index = 0;
    for (const auto& [name, varName] : mappings)
    {
        if (const auto ptr = Resources.Find(varName); ptr)
        {
            MappedNames.append(name);
            mapped.emplace_back(index, name.size(), ptr);
            index += name.size();
        }
    }
    Mappings = common::linq::FromIterable(mapped)
        .Select([this](const auto& tp) 
            {
                const auto& [idx, len, ptr] = tp;
                return MappingPair({ &MappedNames[idx], len }, ptr);
            })
        .ToVector();
}
const ProgramResource* MappingResource::GetResource(const GLint location) const noexcept
{
    const auto targetPair = IndexedResources.Find(location);
    return targetPair ? targetPair->second : nullptr;
}
const ProgramResource* MappingResource::GetResource(std::string_view name) const noexcept
{
    if (!name.empty() && name[0] == '@')
    {
        name.remove_prefix(1);
        const auto targetPair = Mappings.Find(name);
        return targetPair ? targetPair->second : nullptr;
    }
    else
        return Resources.Find(name);
}
std::vector<std::pair<std::string_view, GLint>> MappingResource::GetBindingNames() const noexcept
{
    return common::linq::FromIterable(Resources)
        .Select([](const auto& res) { return std::pair<std::string_view, GLint>(res.Name, res.location); })
        .Concat(common::linq::FromIterable(Mappings)
            .Select([](const auto& pair) { return std::pair(pair.first, pair.second->location); }))
        .ToVector();
}


struct ProgRecCtxConfig : public CtxResConfig<false, GLuint>
{
    GLuint Construct() const { return 0; }
};
static ProgRecCtxConfig PROGREC_CTXCFG;

oglProgram_::oglProgram_(const std::u16string& name, const oglProgStub* stub, const bool isDraw) : Name(name)
{
    ProgramID = CtxFunc->ogluCreateProgram();
    CtxFunc->ogluSetObjectLabel(GL_PROGRAM, ProgramID, name);
    for (const auto& [type, shader] : stub->Shaders)
    {
        if ((type != ShaderType::Compute) == isDraw)
        {
            Shaders.emplace(type, shader);
            CtxFunc->ogluAttachShader(ProgramID, shader->ShaderID);
        }
    }
    CtxFunc->ogluLinkProgram(ProgramID);

    int result;
    CtxFunc->ogluGetProgramiv(ProgramID, GL_LINK_STATUS, &result);
    int len;
    CtxFunc->ogluGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &len);
    string logstr((size_t)len, '\0');
    CtxFunc->ogluGetProgramInfoLog(ProgramID, len, &len, logstr.data());
    if (len > 0 && logstr.back() == '\0')
        logstr.pop_back(); //null-terminated so pop back
    const auto logdat = common::strchset::to_u16string(logstr.c_str(), Charset::UTF8);
    if (!result)
    {
        oglLog().warning(u"Link program failed.\n{}\n", logdat);
        CtxFunc->ogluDeleteProgram(ProgramID);
        COMMON_THROWEX(OGLException, OGLException::GLComponent::Compiler, u"Link program failed", logdat);
    }
    oglLog().success(u"Link program success.\n{}\n", logdat);
    InitLocs(stub->ExtInfo);
    InitSubroutines(stub->ExtInfo);
    FilterProperties(stub->ExtInfo);
    TmpExtInfo = stub->ExtInfo;
}

oglProgram_::~oglProgram_()
{
    if (usethis(*this, false)) //need unuse
    {
        auto& progRec = oglContext_::CurrentContext()->GetOrCreate<false>(PROGREC_CTXCFG);
        CtxFunc->ogluUseProgram(progRec = 0);
    }
    CtxFunc->ogluDeleteProgram(ProgramID);
}

bool oglProgram_::usethis(oglProgram_& prog, const bool change)
{
    prog.CheckCurrent();
    auto& progRec = oglContext_::CurrentContext()->GetOrCreate<false>(PROGREC_CTXCFG);
    if (progRec == prog.ProgramID)
        return true;
    if (!change)//only return status
        return false;

    CtxFunc->ogluUseProgram(progRec = prog.ProgramID);
    prog.RecoverState();
    return true;
}

void oglProgram_::RecoverState()
{
    CheckCurrent();
    SetSubroutine();
}

constexpr auto ResInfoPrinter = [](const auto& res, const auto& prefix)
{
    u16string strBuffer;
    auto strBuf = std::back_inserter(strBuffer);
    for (const auto& info : res)
    {
        if (info.size > 0)
            fmt::format_to(strBuf, FMT_STRING(u"--{:<3} -[{:^5}]- {}[{}]({}) size[{}]\n"), info.ifidx, info.location, info.Name, info.len, info.GetValTypeName(), info.size);
        else
            fmt::format_to(strBuf, FMT_STRING(u"--{:<3} -[{:^5}]- {}[{}]({})\n"), info.ifidx, info.location, info.Name, info.len, info.GetValTypeName());
    }
    if (!strBuffer.empty())
        oglLog().debug(u"{}: \n{}", prefix, strBuffer);
};
void oglProgram_::InitLocs(const ShaderExtInfo& extInfo)
{
    CheckCurrent();
    ProgRess.clear();
    string nameBuf;
    GLenum datatypes[] = { GL_UNIFORM_BLOCK,GL_UNIFORM,GL_PROGRAM_INPUT,GL_PROGRAM_OUTPUT };
    for (const GLenum dtype : datatypes)
    {
        GLint cnt = 0;
        CtxFunc->ogluGetProgramInterfaceiv(ProgramID, dtype, GL_ACTIVE_RESOURCES, &cnt);
        GLint maxNameLen = 0;
        CtxFunc->ogluGetProgramInterfaceiv(ProgramID, dtype, GL_MAX_NAME_LENGTH, &maxNameLen);
        for (GLint a = 0; a < cnt; ++a)
        {
            string_view resName;
            {
                nameBuf.resize(maxNameLen);
                GLsizei nameLen = 0;
                CtxFunc->ogluGetProgramResourceName(ProgramID, dtype, a, maxNameLen, &nameLen, nameBuf.data());
                resName = string_view(nameBuf.c_str(), nameLen);
            }
            common::str::CutStringViewSuffix(resName, '.');//remove struct
            uint32_t arraylen = 0;
            if (auto cutpos = common::str::CutStringViewSuffix(resName, '['); cutpos != resName.npos)//array
            {
                arraylen = atoi(nameBuf.c_str() + cutpos + 1) + 1;
            }
            nameBuf.resize(resName.size());
            if (auto it = FindInSet(ProgRess, nameBuf))
            {
                it->len = common::max(it->len, arraylen);
                continue;
            }

            ProgramResource datinfo = GenProgRes(nameBuf, dtype, ProgramID, a);

            if (datinfo.location != GLInvalidIndex)
            {
                ProgRess.insert(datinfo); //must not exist
            }
        }
    }
    oglLog().debug(u"Active {} resources\n", ProgRess.size());
    GLuint maxUniLoc = 0;
    std::vector<ProgramResource> uniformRess, uboRess, texRess, imgRess;

    for (const auto& info : ProgRess)
    {
        const auto resCategory = info.ResType & ProgResType::MASK_CATEGORY;
        switch (resCategory)
        {
        case ProgResType::CAT_UBO:
            uboRess.push_back(info); break;
        case ProgResType::CAT_UNIFORM:
            switch (info.ResType & ProgResType::MASK_TYPE_CAT)
            {
            case ProgResType::TYPE_TEX:         texRess.    push_back(info); break;
            case ProgResType::TYPE_IMG:         imgRess.    push_back(info); break;
            case ProgResType::TYPE_PRIMITIVE:   uniformRess.push_back(info); break;
            default:                            break;
            } break;
        default: break;
        }
        if (resCategory != ProgResType::CAT_INPUT && resCategory != ProgResType::CAT_OUTPUT)
            maxUniLoc = common::max(maxUniLoc, info.location + info.len);
    }
    UniformRess.Init(std::move(uniformRess), extInfo.ResMappings);
    UBORess.    Init(std::move(uboRess    ), extInfo.ResMappings);
    TexRess.    Init(std::move(texRess    ), extInfo.ResMappings);
    ImgRess.    Init(std::move(imgRess    ), extInfo.ResMappings);

    UniBindCache.clear();
    UniBindCache.resize(maxUniLoc, GL_INVALID_INDEX);

    ResInfoPrinter(UBORess,     u"UBO     resources");
    ResInfoPrinter(TexRess,     u"Tex     resources");
    ResInfoPrinter(ImgRess,     u"Img     resources");
    ResInfoPrinter(UniformRess, u"Uniform resources");
}

void oglProgram_::InitSubroutines(const ShaderExtInfo& extInfo)
{
    CheckCurrent();
    SubroutineRess.clear();
    SubroutineBindings.clear();
    SubroutineSettings.clear();
    if (CtxFunc->SupportSubroutine)
    {
        string nameBuf;
        for (const auto stype : common::container::KeySet(Shaders))
        {
            const auto stage = common::enum_cast(stype);
            GLint count;
            CtxFunc->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &count);
            GLint maxNameLen = 0;
            CtxFunc->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &maxNameLen);
            {
                GLint maxUNameLen = 0;
                CtxFunc->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH, &maxUNameLen);
                maxNameLen = std::max(maxNameLen, maxUNameLen);
            }
            nameBuf.resize(maxNameLen);
            for (GLint a = 0; a < count; ++a)
            {
                GLint uniformNameLen = 0;
                CtxFunc->ogluGetActiveSubroutineUniformName(ProgramID, stage, a, maxNameLen, &uniformNameLen, nameBuf.data());
                const string_view uniformName(nameBuf.data(), uniformNameLen);
                const auto uniformLoc = CtxFunc->ogluGetSubroutineUniformLocation(ProgramID, stage, uniformName.data());
                auto& sr = *SubroutineRess.emplace(stage, uniformLoc, uniformName).first;
                
                GLint srcnt = 0;
                CtxFunc->ogluGetActiveSubroutineUniformiv(ProgramID, stage, a, GL_NUM_COMPATIBLE_SUBROUTINES, &srcnt);
                vector<GLint> srIDs(srcnt, GL_INVALID_INDEX);
                CtxFunc->ogluGetActiveSubroutineUniformiv(ProgramID, stage, a, GL_COMPATIBLE_SUBROUTINES, srIDs.data());
                const auto srinfos = common::linq::FromIterable(srIDs)
                    .Select([&](const auto subridx)
                    {
                        GLint srNameLen = 0;
                        CtxFunc->ogluGetActiveSubroutineName(ProgramID, stage, subridx, maxNameLen, &srNameLen, nameBuf.data());
                        sr.SRNames.append(nameBuf.data(), srNameLen);
                        return std::pair(subridx, srNameLen);
                    })
                    .ToVector();
                sr.Routines = common::linq::FromIterable(srinfos)
                    .Select([&, offset = size_t(0)](const auto& info) mutable
                    {
                        string_view subrName(&sr.SRNames[offset], info.second);
                        SubroutineResource::Routine routine(&sr, subrName, info.first);
                        offset += info.second;
                        return routine;
                    })
                    .ToVector();
            }
            GLint locCount = 0;
            CtxFunc->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &locCount);
            SubroutineSettings[stage].resize(locCount);
        }
    }
    
    if (!extInfo.EmulateSubroutines.empty())
    {
        oglLog().warning(u"Detected emulated subroutine. Native subroutine {}.\n", CtxFunc->SupportSubroutine ? u"support" : u"unsupport");
        for (const auto& [srname, srinfo] : extInfo.EmulateSubroutines)
        {
            const auto uniRes = UniformRess.GetResource(srinfo.UniformName);
            if (!uniRes) continue; // may be optimized or not in the stage
            auto& sr = *SubroutineRess.emplace(GLInvalidEnum, uniRes->location, srname).first;
            for (const auto& routine : srinfo.Routines)
                sr.SRNames.append(routine.first);
            sr.Routines = common::linq::FromIterable(srinfo.Routines)
                .Select([&, offset = size_t(0)](const auto& info) mutable
                {
                    string_view subrName(&sr.SRNames[offset], info.first.size());
                    SubroutineResource::Routine routine(&sr, subrName, static_cast<GLuint>(info.second));
                    offset += info.first.size();
                    return routine;
                })
                .ToVector();
        }
    }
    
    u16string strBuffer;
    auto strBuf = std::back_inserter(strBuffer);
    for (const auto& subr : SubroutineRess)
    {
        fmt::format_to(strBuf, u"SubRoutine {} {} at [{}]:\n", subr.Name, subr.Stage == GLInvalidEnum ? u"(Emulated)" : u"", subr.UniLoc);
        for (const auto& routine : subr.Routines)
        {
            fmt::format_to(strBuf, FMT_STRING(u"--[{:^4}]: {}\n"), routine.Id, routine.Name);
        }
        const auto& routine = subr.GetRoutines()[0];
        SubroutineBindings[&subr] = &routine;
        if (subr.Stage != GLInvalidEnum)
            SubroutineSettings[subr.Stage][subr.UniLoc] = routine.Id;
    }
    if (!strBuffer.empty())
        oglLog().debug(u"SubRoutine Resource: \n{}", strBuffer);
}

static bool MatchType(const ShaderExtProperty& prop, const ProgramResource& res)
{
    const auto glType = res.Valtype;
    switch (prop.Type)
    {
    case ShaderPropertyType::Vector: return (glType >= GL_FLOAT_VEC2 && glType <= GL_INT_VEC4) || (glType >= GL_BOOL_VEC2 && glType <= GL_BOOL_VEC4) ||
        (glType >= GL_UNSIGNED_INT_VEC2 && glType <= GL_UNSIGNED_INT_VEC4) || (glType >= GL_DOUBLE_VEC2 && glType <= GL_DOUBLE_VEC4);
    case ShaderPropertyType::Bool:   return glType == GL_BOOL;
    case ShaderPropertyType::Int:    return glType == GL_INT;
    case ShaderPropertyType::Uint:   return glType == GL_UNSIGNED_INT;
    case ShaderPropertyType::Float:  return glType == GL_FLOAT;
    case ShaderPropertyType::Color:  return glType == GL_FLOAT_VEC4;
    case ShaderPropertyType::Range:  return glType == GL_FLOAT_VEC2;
    case ShaderPropertyType::Matrix: return (glType >= GL_FLOAT_MAT2 && glType <= GL_FLOAT_MAT4) || (glType >= GL_DOUBLE_MAT2 && glType <= GL_DOUBLE_MAT4x3);
    default: return false;
    }
}

void oglProgram_::FilterProperties(const ShaderExtInfo& extInfo)
{
    CheckCurrent();
    ShaderProps.clear();
    for (const auto& prop : extInfo.Properties)
    {
        oglLog().debug(u"prop[{}], typeof [{}], data[{}]\n", prop.Name, (uint8_t)prop.Type, prop.Data.has_value() ? "Has" : "None");
        if (auto res = FindInSet(ProgRess, prop.Name))
            if (MatchType(prop, *res))
            {
                ShaderProps.insert(prop);
                switch (prop.Type)
                {
                case ShaderPropertyType::Color:
                    {
                        Vec4 vec;
                        CtxFunc->ogluGetUniformfv(ProgramID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Range:
                    {
                        b3d::Coord2D vec;
                        CtxFunc->ogluGetUniformfv(ProgramID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Bool:
                    {
                        int32_t flag = 0;
                        CtxFunc->ogluGetUniformiv(ProgramID, res->location, &flag);
                        UniValCache.insert_or_assign(res->location, (bool)flag);
                    } break;
                case ShaderPropertyType::Int:
                    {
                        int32_t val = 0;
                        CtxFunc->ogluGetUniformiv(ProgramID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Uint:
                    {
                        uint32_t val = 0;
                        CtxFunc->ogluGetUniformuiv(ProgramID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Float:
                    {
                        float val = 0;
                        CtxFunc->ogluGetUniformfv(ProgramID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                default:
                    oglLog().verbose(u"ExtProp [{}] of type[{}] and valtype[{}] is not supported to load initial value.\n", prop.Name, (uint8_t)prop.Type, res->GetValTypeName());
                }
            }
            else
                oglLog().warning(u"ExtProp [{}] mismatch type[{}] with valtype[{}]\n", prop.Name, (uint8_t)prop.Type, res->GetValTypeName());
        else
            oglLog().warning(u"ExtProp [{}] cannot find active uniform\n", prop.Name);
    }
}

std::pair<const SubroutineResource*, const SubroutineResource::Routine*> oglProgram_::LocateSubroutine(const std::string_view subrName, const std::string_view routineName) const noexcept
{
    if (auto pSubr = FindInSet(SubroutineRess, subrName); pSubr)
    {
        if (auto pRoutine = pSubr->Routines.Find(routineName); pRoutine)
            return { pSubr, pRoutine };
        else
            oglLog().warning(u"cannot find routine {} for subroutine {}\n", routineName, subrName);
    }
    else
        oglLog().warning(u"cannot find subroutine {}\n", subrName);
    return { nullptr, nullptr };
}


oglProgram_::oglProgStub::oglProgStub()
{ }

oglProgram_::oglProgStub::~oglProgStub()
{ }

void oglProgram_::oglProgStub::AddShader(oglShader shader)
{
    CheckCurrent();
    [[maybe_unused]] const auto [it, newitem] = Shaders.insert_or_assign(shader->Type, std::move(shader));
    if (!newitem)
        oglLog().warning(u"Repeat adding shader {}, replaced\n", shader->ShaderID);
}

bool oglProgram_::oglProgStub::AddExtShaders(const std::string& src, const ShaderConfig& config, const bool allowCompute, const bool allowDraw)
{
    const auto s = oglShader_::LoadFromExSrc(src, ExtInfo, config, allowCompute, allowDraw);
    for (auto shader : s)
    {
        shader->compile();
        AddShader(shader);
    }
    return !s.empty();
}

oglDrawProgram oglProgram_::oglProgStub::LinkDrawProgram(const std::u16string& name)
{
    CheckCurrent();
    if (Shaders.find(ShaderType::Vertex) == Shaders.cend() || Shaders.find(ShaderType::Fragment) == Shaders.cend())
        return {};
    return MAKE_ENABLER_SHARED(oglDrawProgram_, (name, this));
}

oglComputeProgram oglProgram_::oglProgStub::LinkComputeProgram(const std::u16string& name)
{
    CheckCurrent();
    if (Shaders.find(ShaderType::Compute) == Shaders.cend())
        return {};
    return MAKE_ENABLER_SHARED(oglComputeProgram_, (name, this));
}


const SubroutineResource::Routine* oglProgram_::GetSubroutine(const string& sruname)
{
    if (const SubroutineResource* pSru = FindInSet(SubroutineRess, sruname); pSru)
        return GetSubroutine(*pSru);
    oglLog().warning(u"cannot find subroutine object {}\n", sruname);
    return nullptr;
}
const SubroutineResource::Routine* oglProgram_::GetSubroutine(const SubroutineResource& sru)
{
    return FindInMapOrDefault(SubroutineBindings, &sru);
}

ProgState oglProgram_::State() noexcept
{
    return ProgState(*this);
}

oglProgram_::oglProgStub oglProgram_::Create()
{
    return oglProgStub();
}

void oglProgram_::SetSubroutine()
{
    CheckCurrent();
    for (const auto&[stage, subrs] : SubroutineSettings)
    {
        GLsizei cnt = (GLsizei)subrs.size();
        if (cnt > 0)
            CtxFunc->ogluUniformSubroutinesuiv(stage, cnt, subrs.data());
    }
    for (const auto [subr, routine] : SubroutineBindings)
    {
        if (subr->Stage == GLInvalidEnum) // emulate subroutine
        {
            CtxFunc->ogluProgramUniform1ui(ProgramID, subr->UniLoc, routine->Id);
        }
    }
}

void oglProgram_::SetSubroutine(const SubroutineResource::Routine* routine)
{
    const auto& sr = *routine->Host;
    if (SubroutineRess.find(sr) != SubroutineRess.cend())
        SetSubroutine(&sr, routine);
    else
        oglLog().warning(u"routine [{}]-[{}] not in prog [{}]\n", sr.Name, routine->Name, Name);
}

void oglProgram_::SetSubroutine(const SubroutineResource* subr, const SubroutineResource::Routine* routine)
{
    CheckCurrent();
    auto& oldRoutine = SubroutineBindings[subr];
    oldRoutine = routine;
    if (subr->Stage != GLInvalidEnum)
    {
        auto& srvec = SubroutineSettings[subr->Stage][subr->UniLoc];
        srvec = routine->Id;
    }
}


template<typename T>
forceinline static bool CheckResource(const ProgramResource* res, const T&)
{
    if (res && res->location != GLInvalidIndex)
    {
        const auto valType = res->Valtype;
        if constexpr (std::is_same_v<T, b3d::Coord2D>)
            return valType == GL_FLOAT_VEC2;
        else if constexpr (std::is_same_v<T, miniBLAS::Vec3>)
            return valType == GL_FLOAT_VEC2 || valType == GL_FLOAT_VEC3;
        else if constexpr (std::is_same_v<T, miniBLAS::Vec4>)
            return valType == GL_FLOAT_VEC2 || valType == GL_FLOAT_VEC3 || valType == GL_FLOAT_VEC4;
        else if constexpr (std::is_same_v<T, miniBLAS::Mat3x3>)
            return valType == GL_FLOAT_MAT3;
        else if constexpr (std::is_same_v<T, miniBLAS::Mat4x4>)
            return valType == GL_FLOAT_MAT4;
        else if constexpr (std::is_same_v<T, bool>)
            return valType == GL_BOOL;
        else if constexpr (std::is_floating_point_v<T>)
            return valType == GL_FLOAT;
        else if constexpr (std::is_integral_v<T>)
            return valType == GL_INT || valType == GL_UNSIGNED_INT || valType == GL_FLOAT;
    }
    return false;
}

void oglProgram_::SetVec_(const ProgramResource* res, const b3d::Coord2D& vec, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, vec))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, vec);
        CtxFunc->ogluProgramUniform2fv(ProgramID, res->location, 1, vec);
    }
}
void oglProgram_::SetVec_(const ProgramResource* res, const miniBLAS::Vec3& vec, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, vec))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, vec);
        CtxFunc->ogluProgramUniform3fv(ProgramID, res->location, 1, vec);
    }
}
void oglProgram_::SetVec_(const ProgramResource* res, const miniBLAS::Vec4& vec, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, vec))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, vec);
        CtxFunc->ogluProgramUniform4fv(ProgramID, res->location, 1, vec);
    }
}
void oglProgram_::SetMat_(const ProgramResource* res, const miniBLAS::Mat3x3& mat, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, mat))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, mat);
        CtxFunc->ogluProgramUniformMatrix4fv(ProgramID, res->location, 1, GL_FALSE, mat.inv());
    }
}
void oglProgram_::SetMat_(const ProgramResource* res, const miniBLAS::Mat4x4& mat, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, mat))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, mat);
        CtxFunc->ogluProgramUniformMatrix4fv(ProgramID, res->location, 1, GL_FALSE, mat.inv());
    }
}
void oglProgram_::SetVal_(const ProgramResource* res, const bool val, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, val))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, val);
        CtxFunc->ogluProgramUniform1i(ProgramID, res->location, val);
    }
}
void oglProgram_::SetVal_(const ProgramResource* res, const float val, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, val))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, val);
        CtxFunc->ogluProgramUniform1f(ProgramID, res->location, val);
    }
}
void oglProgram_::SetVal_(const ProgramResource* res, const int32_t val, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, val))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, val);
        CtxFunc->ogluProgramUniform1i(ProgramID, res->location, val);
    }
}
void oglProgram_::SetVal_(const ProgramResource* res, const uint32_t val, const bool keep)
{
    CheckCurrent();
    if (CheckResource(res, val))
    {
        if (keep)
            UniValCache.insert_or_assign(res->location, val);
        CtxFunc->ogluProgramUniform1ui(ProgramID, res->location, val);
    }
}


ProgState::~ProgState()
{
    if (oglProgram_::usethis(Prog, false)) //self used, then changed to keep pinned status
    {
        Prog.RecoverState();
    }
}

ProgState& ProgState::SetTexture(const oglTexBase& tex, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.TexRess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        Prog.TexBindings.insert_or_assign(pos, tex);
    }
    return *this;
}

ProgState& ProgState::SetImage(const oglImgBase& img, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.ImgRess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        Prog.ImgBindings.insert_or_assign(pos, img);
    }
    return *this;
}

ProgState& ProgState::SetUBO(const oglUBO& ubo, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.UBORess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        Prog.UBOBindings.insert_or_assign(pos, ubo);
    }
    return *this;
}

ProgState& ProgState::SetSubroutine(const SubroutineResource::Routine* routine)
{
    Prog.SetSubroutine(routine);
    return *this;
}

ProgState& ProgState::SetSubroutine(const string_view subrName, const string_view routineName)
{
    const auto [pSubr, pRoutine] = Prog.LocateSubroutine(subrName, routineName);
    if (pSubr && pRoutine)
        Prog.SetSubroutine(pSubr, pRoutine);
    return *this;
}


oglDrawProgram_::oglDrawProgram_(const std::u16string& name, const oglProgStub* stub)
    : oglProgram_(name, stub, true)
{
    auto& mappings = TmpExtInfo->ResMappings;
    std::vector<ProgramResource> inputRess, outputRess;
    std::optional<const ProgramResource*> defaultOutput;
    for (const auto& info : ProgRess)
    {
        const auto resCategory = info.ResType & ProgResType::MASK_CATEGORY;
        switch (resCategory)
        {
        case ProgResType::CAT_INPUT:    
            inputRess. push_back(info); 
            break;
        case ProgResType::CAT_OUTPUT:   
            outputRess.push_back(info); 
            if (info.location == 0)
                defaultOutput = &info;
            break;
        default: 
            break;
        }
    }
    
    mappings.insert_or_assign("DrawID", "ogluDrawId");
    if (defaultOutput.has_value())
        mappings.insert_or_assign("default", (**defaultOutput).Name);
    InputRess .Init(std::move(inputRess ), mappings);
    OutputRess.Init(std::move(outputRess), mappings);
    OutputBindings = OutputRess.GetBindingNames();
    TmpExtInfo.reset();

    ResInfoPrinter(InputRess,  u"Input  attributes");
    ResInfoPrinter(OutputRess, u"Output resources");
}
oglDrawProgram_::~oglDrawProgram_() { }

ProgDraw oglDrawProgram_::Draw() noexcept
{
    return ProgDraw(this);
}

oglDrawProgram oglDrawProgram_::Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config)
{
    auto stub = oglProgram_::Create();
    stub.AddExtShaders(extSrc, config, false, true);
    return stub.LinkDrawProgram(name);
}


thread_local common::SpinLocker ProgDrawLocker;
ProgDraw::ProgDraw(oglDrawProgram_* prog, FBOPairType&& fboInfo) noexcept
    : Prog(*prog), FBO(*fboInfo.first),
    Lock(ProgDrawLocker.LockScope()), FBOLock(std::move(fboInfo.second)),
    TexMan(oglTexBase_::GetTexMan()), ImgMan(oglImgBase_::GetImgMan()), UboMan(oglUniformBuffer_::GetUBOMan())
{
    oglProgram_::usethis(Prog);
    TexBindings  = Prog.TexBindings;
    ImgBindings  = Prog.ImgBindings;
    UBOBindings  = Prog.UBOBindings;
    // bind outputs
    std::vector<GLenum> outputs(CtxFunc->MaxDrawBuffers, GL_NONE);
    GLint maxPos = 0;
    for (const auto& [name, loc] : Prog.OutputBindings)
    {
        if (outputs[loc] == GL_NONE)
        {
            const auto bindPoint = FBO.GetAttachPoint(name);
            if (bindPoint != GL_NONE)
                maxPos = std::max(maxPos, loc), outputs[loc] = bindPoint;
        }
    }
    FBO.BindDraws(common::to_span(outputs).subspan(0, maxPos + 1));
}
ProgDraw::ProgDraw(oglDrawProgram_* prog) noexcept : ProgDraw(prog, oglFrameBuffer_::LockCurFBOLock())
{ }
ProgDraw::~ProgDraw()
{
    Restore();
}

void ProgDraw::SetBindings() noexcept
{
    TexMan.BindAll(Prog.ProgramID, TexBindings, Prog.UniBindCache);
    ImgMan.BindAll(Prog.ProgramID, ImgBindings, Prog.UniBindCache);
    UboMan.BindAll(Prog.ProgramID, UBOBindings, Prog.UniBindCache);
}

ProgDraw& ProgDraw::Restore()
{
    Prog.CheckCurrent();
    TexBindings = Prog.TexBindings;
    ImgBindings = Prog.ImgBindings;
    UBOBindings = Prog.UBOBindings;

    for (const auto&[pos, val] : UniValBackup)
    {
        switch (val.index())
        {
        case 0: Prog.SetVec_(pos, std::get<0>(val), false); break;
        case 1: Prog.SetVec_(pos, std::get<1>(val), false); break;
        case 2: Prog.SetVec_(pos, std::get<2>(val), false); break;
        case 3: Prog.SetMat_(pos, std::get<3>(val), false); break;
        case 4: Prog.SetMat_(pos, std::get<4>(val), false); break;
        case 5: Prog.SetVal_(pos, std::get<5>(val), false); break;
        case 6: Prog.SetVal_(pos, std::get<6>(val), false); break;
        case 7: Prog.SetVal_(pos, std::get<7>(val), false); break;
        case 8: Prog.SetVal_(pos, std::get<8>(val), false); break;
        }
    }
    UniValBackup.clear();
    if (!SubroutineCache.empty())
    {
        for (const auto&[subr, routine] : SubroutineCache)
            Prog.SetSubroutine(subr, routine);
        SubroutineCache.clear();
        Prog.SetSubroutine();
    }
    return *this;
}
std::weak_ptr<oglDrawProgram_> ProgDraw::GetProg() const noexcept
{
    return std::dynamic_pointer_cast<oglDrawProgram_>(Prog.shared_from_this());
}


ProgDraw& ProgDraw::DrawRange(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
    SetBindings();
    Prog.SetSubroutine();
    vao->RangeDraw(size, offset);
    return *this;
}

ProgDraw& ProgDraw::DrawInstance(const oglVAO& vao, const uint32_t count, const uint32_t base)
{
    SetBindings();
    Prog.SetSubroutine();
    Prog.SetVal_(Prog.InputRess.GetResource("@BaseInstance"), static_cast<int32_t>(base), false);
    vao->InstanceDraw(count, base);
    return *this;
}

ProgDraw& ProgDraw::Draw(const oglVAO& vao)
{
    SetBindings();
    Prog.SetSubroutine();
    vao->Draw();
    return *this;
}

ProgDraw& ProgDraw::SetTexture(const oglTexBase& tex, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.TexRess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        TexBindings.insert_or_assign(pos, tex);
    }
    return *this;
}

ProgDraw& ProgDraw::SetImage(const oglImgBase& img, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.ImgRess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        ImgBindings.insert_or_assign(pos, img);
    }
    return *this;
}

ProgDraw& ProgDraw::SetUBO(const oglUBO& ubo, const std::string_view name, const GLuint idx)
{
    const auto res = Prog.UBORess.GetResource(name);
    if (res && idx < res->len) // legal
    {
        const auto pos = res->location + idx;
        UBOBindings.insert_or_assign(pos, ubo);
    }
    return *this;
}

ProgDraw& ProgDraw::SetSubroutine(const SubroutineResource::Routine* routine)
{
    const auto& sr = *routine->Host;
    if (Prog.SubroutineRess.find(sr) != Prog.SubroutineRess.cend())
    {
        if (auto pOldRoutine = FindInMap(Prog.SubroutineBindings, &sr))
            SubroutineCache.try_emplace(&sr, *pOldRoutine);
        Prog.SetSubroutine(&sr, routine);
    }
    else
        oglLog().warning(u"routine [{}]-[{}] not in prog [{}]\n", sr.Name, routine->Name, Prog.Name);
    return *this;
}

ProgDraw& ProgDraw::SetSubroutine(const string_view subrName, const string_view routineName)
{
    const auto [pSubr, pRoutine] = Prog.LocateSubroutine(subrName, routineName);
    if (pSubr && pRoutine)
    {
        if (auto pOldRoutine = FindInMap(Prog.SubroutineBindings, pSubr))
            SubroutineCache.try_emplace(pSubr, *pOldRoutine);
        Prog.SetSubroutine(pSubr, pRoutine);
    }
    return *this;
}


oglComputeProgram_::oglComputeProgram_(const std::u16string& name, const oglProgStub* stub)
    : oglProgram_(name, stub, false)
{
    CtxFunc->ogluGetProgramiv(ProgramID, GL_COMPUTE_WORK_GROUP_SIZE, reinterpret_cast<GLint*>(LocalSize.data()));
    oglLog().debug(u"Compute Shader has a LocalSize [{}x{}x{}]\n", LocalSize[0], LocalSize[1], LocalSize[2]);
}

void oglComputeProgram_::Run(const uint32_t groupX, const uint32_t groupY, const uint32_t groupZ)
{
    CheckCurrent();
    usethis(*this);
    CtxFunc->ogluDispatchCompute(groupX, groupY, groupZ);
}

oglComputeProgram oglComputeProgram_::Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config)
{
    auto stub = oglProgram_::Create();
    stub.AddExtShaders(extSrc, config, true, false);
    return stub.LinkComputeProgram(name);
}

bool oglComputeProgram_::CheckSupport()
{
    return CtxFunc->SupportComputeShader;
}



}