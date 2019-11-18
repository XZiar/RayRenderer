#include "oglPch.h"
#include "oglException.h"
#include "oglProgram.h"
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
    DSA->ogluGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
    return static_cast<T>(ret);
}


string_view ProgramResource::GetTypeName() const noexcept
{
    switch (ResType & ProgResType::CategoryMask)
    {
    case ProgResType::InputCat:   return "Attrib"sv;
    case ProgResType::UBOCat:     return "UniBlk"sv;
    case ProgResType::UniformCat:
        switch (ResType&ProgResType::TypeCatMask)
        {
        case ProgResType::TexType:    return "TexUni"sv;
        case ProgResType::ImgType:    return "ImgUni"sv;
        default:                      return "Other"sv;
        }
    default:                      return "Other"sv;
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

ProgramResource GenProgRes(const string& name, const GLenum type, const GLuint ProgramID, const GLint idx)
{
    ProgramResource progres;
    progres.Name = name;
    progres.Type = type;
    progres.ifidx = (uint8_t)idx;
    progres.ResType = ProgResType::Empty;
    if (type == GL_UNIFORM_BLOCK)
    {
        progres.Valtype = GL_UNIFORM_BLOCK;
        progres.ResType |= ProgResType::UBOCat;
        progres.location = DSA->ogluGetProgramResourceIndex(ProgramID, type, name.c_str());
        progres.size = ProgResGetValue<uint16_t>(ProgramID, type, idx, GL_BUFFER_DATA_SIZE);
        progres.len = 1;
    }
    else
    {
        progres.Valtype = ProgResGetValue<GLenum>(ProgramID, type, idx, GL_TYPE);
        switch (type)
        {
        case GL_PROGRAM_INPUT:
            progres.ResType |= ProgResType::InputCat;
            break;
        case GL_UNIFORM:
            progres.ResType |= ProgResType::UniformCat;
            if (progres.Valtype >= GL_IMAGE_1D && progres.Valtype <= GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
                progres.ResType |= ProgResType::ImgType;
            else if ((progres.Valtype >= GL_SAMPLER_1D && progres.Valtype <= GL_SAMPLER_2D_RECT_SHADOW)
                || (progres.Valtype >= GL_SAMPLER_1D_ARRAY && progres.Valtype <= GL_SAMPLER_CUBE_SHADOW)
                || (progres.Valtype >= GL_INT_SAMPLER_1D && progres.Valtype <= GL_UNSIGNED_INT_SAMPLER_BUFFER)
                || (progres.Valtype >= GL_SAMPLER_2D_MULTISAMPLE && progres.Valtype <= GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY))
                progres.ResType |= ProgResType::TexType;
            else
                progres.ResType |= ProgResType::PrimitiveType;
            break;
        }
        progres.location = DSA->ogluGetProgramResourceLocation(ProgramID, type, name.c_str());
        progres.size = 0;
        progres.len = ProgResGetValue<GLuint>(ProgramID, type, idx, GL_ARRAY_SIZE);
    }
    return progres;
}



struct ProgRecCtxConfig : public CtxResConfig<false, GLuint>
{
    GLuint Construct() const { return 0; }
};
static ProgRecCtxConfig PROGREC_CTXCFG;

oglProgram_::oglProgram_(const u16string& name, const oglProgStub* stub, const bool isDraw) : Name(name)
{
    ProgramID = DSA->ogluCreateProgram();
    DSA->ogluSetObjectLabel(GL_PROGRAM, ProgramID, name);
    for (const auto& [type, shader] : stub->Shaders)
    {
        if ((type != ShaderType::Compute) == isDraw)
        {
            Shaders.emplace(type, shader);
            DSA->ogluAttachShader(ProgramID, shader->ShaderID);
        }
    }
    DSA->ogluLinkProgram(ProgramID);

    int result;
    DSA->ogluGetProgramiv(ProgramID, GL_LINK_STATUS, &result);
    int len;
    DSA->ogluGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &len);
    string logstr((size_t)len, '\0');
    DSA->ogluGetProgramInfoLog(ProgramID, len, &len, logstr.data());
    if (len > 0 && logstr.back() == '\0')
        logstr.pop_back(); //null-terminated so pop back
    const auto logdat = common::strchset::to_u16string(logstr.c_str(), Charset::UTF8);
    if (!result)
    {
        oglLog().warning(u"Link program failed.\n{}\n", logdat);
        DSA->ogluDeleteProgram(ProgramID);
        COMMON_THROWEX(OGLException, OGLException::GLComponent::Compiler, u"Link program failed", logdat);
    }
    oglLog().success(u"Link program success.\n{}\n", logdat);
    ExtInfo = stub->ExtInfo;
    InitLocs();
    InitSubroutines();
    FilterProperties();
}

oglProgram_::~oglProgram_()
{
    if (usethis(*this, false)) //need unuse
    {
        auto& progRec = oglContext_::CurrentContext()->GetOrCreate<false>(PROGREC_CTXCFG);
        DSA->ogluUseProgram(progRec = 0);
    }
    DSA->ogluDeleteProgram(ProgramID);
}

bool oglProgram_::usethis(oglProgram_& prog, const bool change)
{
    prog.CheckCurrent();
    auto& progRec = oglContext_::CurrentContext()->GetOrCreate<false>(PROGREC_CTXCFG);
    if (progRec == prog.ProgramID)
        return true;
    if (!change)//only return status
        return false;

    DSA->ogluUseProgram(progRec = prog.ProgramID);
    prog.RecoverState();
    return true;
}

void oglProgram_::RecoverState()
{
    CheckCurrent();
    SetSubroutine();
    auto& texMan = oglTexBase_::getTexMan();
    texMan.unpin();
    auto& imgMan = oglImgBase_::getImgMan();
    imgMan.unpin();
    auto& uboMan = oglUniformBuffer_::getUBOMan();
    uboMan.unpin();
    SetTexture(texMan, TexBindings, true);
    SetImage(imgMan, ImgBindings, true);
    SetUBO(uboMan, UBOBindings, true);
}


void oglProgram_::InitLocs()
{
    CheckCurrent();
    set<ProgramResource, ProgramResource::Lesser> dataMap;
    string nameBuf;
    GLenum datatypes[] = { GL_UNIFORM_BLOCK,GL_UNIFORM,GL_PROGRAM_INPUT };
    for (const GLenum dtype : datatypes)
    {
        GLint cnt = 0;
        DSA->ogluGetProgramInterfaceiv(ProgramID, dtype, GL_ACTIVE_RESOURCES, &cnt);
        GLint maxNameLen = 0;
        DSA->ogluGetProgramInterfaceiv(ProgramID, dtype, GL_MAX_NAME_LENGTH, &maxNameLen);
        for (GLint a = 0; a < cnt; ++a)
        {
            string_view resName;
            {
                nameBuf.resize(maxNameLen);
                GLsizei nameLen = 0;
                DSA->ogluGetProgramResourceName(ProgramID, dtype, a, maxNameLen, &nameLen, nameBuf.data());
                resName = string_view(nameBuf.c_str(), nameLen);
            }
            common::str::CutStringViewSuffix(resName, '.');//remove struct
            uint32_t arraylen = 0;
            if (auto cutpos = common::str::CutStringViewSuffix(resName, '['); cutpos != resName.npos)//array
            {
                arraylen = atoi(nameBuf.c_str() + cutpos + 1) + 1;
            }
            nameBuf.resize(resName.size());
            if (auto it = FindInSet(dataMap, nameBuf))
            {
                it->len = common::max(it->len, arraylen);
                continue;
            }

            ProgramResource datinfo = GenProgRes(nameBuf, dtype, ProgramID, a);

            if (datinfo.location != (GLint)GL_INVALID_INDEX)
            {
                dataMap.insert(datinfo); //must not exist
            }
        }
    }
    oglLog().debug(u"Active {} locations\n", dataMap.size());
    GLuint maxUniLoc = 0;
    for (const auto& info : dataMap)
    {
        switch (info.ResType & ProgResType::CategoryMask)
        {
        case ProgResType::InputCat:
            AttrRess.insert(info); break;
        case ProgResType::UBOCat:
            UBORess.insert(info); break;
        case ProgResType::UniformCat:
            switch (info.ResType & ProgResType::TypeCatMask)
            {
            case ProgResType::TexType:  TexRess.insert(info); break;
            case ProgResType::ImgType:  ImgRess.insert(info); break;
            default:                    break;
            } break;
        default:                break;
        }
        if ((info.ResType & ProgResType::CategoryMask) != ProgResType::InputCat)
            maxUniLoc = common::max(maxUniLoc, info.location + info.len);
        ProgRess.insert(info);
        oglLog().debug(u"--{:>7}{:<3} -[{:^5}]- {}[{}]({}) size[{}]\n", info.GetTypeName(), info.ifidx, info.location, info.Name, info.len, info.GetValTypeName(), info.size);
    }
    UniBindCache.clear();
    UniBindCache.resize(maxUniLoc, GL_INVALID_INDEX);
}

void oglProgram_::InitSubroutines()
{
    CheckCurrent();
    SubroutineRess.clear();
    subrLookup.clear();
    SubroutineBindings.clear();
    SubroutineSettings.clear();
    fmt::basic_memory_buffer<char16_t> strBuffer;
    {
        constexpr std::u16string_view tmp = u"SubRoutine Resource: \n";
        strBuffer.append(tmp.data(), tmp.data() + tmp.size());
    }
    string nameBuf;
    for (const auto stype : common::container::KeySet(Shaders))
    {
        const auto stage = common::enum_cast(stype);
        GLint count;
        DSA->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &count);
        GLint maxNameLen = 0;
        DSA->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &maxNameLen);
        {
            GLint maxUNameLen = 0;
            DSA->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH, &maxUNameLen);
            maxNameLen = std::max(maxNameLen, maxUNameLen);
        }
        nameBuf.resize(maxNameLen);
        for (int a = 0; a < count; ++a)
        {
            string uniformName;
            {
                GLint nameLen = 0;
                DSA->ogluGetActiveSubroutineUniformName(ProgramID, stage, a, maxNameLen, &nameLen, nameBuf.data());
                uniformName.assign(nameBuf, 0, nameLen);
            }
            auto uniformLoc = DSA->ogluGetSubroutineUniformLocation(ProgramID, stage, uniformName.data());
            fmt::format_to(strBuffer, u"SubRoutine {} at [{}]:\n", uniformName, uniformLoc);
            GLint srcnt = 0;
            DSA->ogluGetActiveSubroutineUniformiv(ProgramID, stage, a, GL_NUM_COMPATIBLE_SUBROUTINES, &srcnt);
            vector<GLint> compSRs(srcnt, GL_INVALID_INDEX);
            DSA->ogluGetActiveSubroutineUniformiv(ProgramID, stage, a, GL_COMPATIBLE_SUBROUTINES, compSRs.data());
            
            vector<SubroutineResource::Routine> routines;
            for (const auto subridx : compSRs)
            {
                string subrName;
                {
                    GLint nameLen = 0;
                    DSA->ogluGetActiveSubroutineName(ProgramID, stage, subridx, maxNameLen, &nameLen, nameBuf.data());
                    subrName.assign(nameBuf, 0, nameLen);
                }
                fmt::format_to(strBuffer, FMT_STRING(u"--[{}]: {}\n"), subridx, subrName);
                routines.push_back(SubroutineResource::Routine(subrName, subridx));
            }
            const auto it = SubroutineRess.emplace(stype, uniformLoc, uniformName, std::move(routines)).first;
            for (auto& routine : it->Routines)
                subrLookup[&routine] = &(*it);
        }
        GLint locCount = 0;
        DSA->ogluGetProgramStageiv(ProgramID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &locCount);
        SubroutineSettings[stype].resize(locCount);
    }
    oglLog().debug(strBuffer);
    
    for (const auto& subr : SubroutineRess)
    {
        const auto& routine = subr.Routines[0];
        SubroutineBindings[&subr] = &routine;
        SubroutineSettings[subr.Stage][subr.UniLoc] = routine.Id;
    }
}

static bool MatchType(const ShaderExtProperty& prop, const ProgramResource& res)
{
    const auto glType = res.Valtype;
    switch (prop.Type)
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

void oglProgram_::FilterProperties()
{
    CheckCurrent();
    set<ShaderExtProperty, ShaderExtProperty::Lesser> newProperties;
    for (const auto& prop : ExtInfo.Properties)
    {
        oglLog().debug(u"prop[{}], typeof [{}], data[{}]\n", prop.Name, (uint8_t)prop.Type, prop.Data.has_value() ? "Has" : "None");
        if (auto res = FindInSet(ProgRess, prop.Name))
            if (MatchType(prop, *res))
            {
                newProperties.insert(prop);
                switch (prop.Type)
                {
                case ShaderPropertyType::Color:
                    {
                        Vec4 vec;
                        DSA->ogluGetUniformfv(ProgramID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Range:
                    {
                        b3d::Coord2D vec;
                        DSA->ogluGetUniformfv(ProgramID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Bool:
                    {
                        int32_t flag = 0;
                        DSA->ogluGetUniformiv(ProgramID, res->location, &flag);
                        UniValCache.insert_or_assign(res->location, (bool)flag);
                    } break;
                case ShaderPropertyType::Int:
                    {
                        int32_t val = 0;
                        DSA->ogluGetUniformiv(ProgramID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Uint:
                    {
                        uint32_t val = 0;
                        DSA->ogluGetUniformuiv(ProgramID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Float:
                    {
                        float val = 0;
                        DSA->ogluGetUniformfv(ProgramID, res->location, &val);
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
    ExtInfo.Properties.swap(newProperties);
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


GLenum oglProgram_::ParseUniformType(const UniformType type)
{
    switch (type)
    {
    case UniformType::FLOAT:    return GL_FLOAT;
    case UniformType::UINT:     return GL_UNSIGNED_INT;
    case UniformType::INT:      return GL_INT;
    case UniformType::BOOL:     return GL_BOOL;
    case UniformType::VEC4:     return GL_FLOAT_VEC4;
    case UniformType::VEC3:     return GL_FLOAT_VEC3;
    case UniformType::VEC2:     return GL_FLOAT_VEC2;
    case UniformType::MAT4:     return GL_FLOAT_MAT4;
    case UniformType::MAT3:     return GL_FLOAT_MAT3;
    default:                    return GL_INVALID_ENUM;
    }
}
GLint oglProgram_::GetLoc(const ProgramResource* res, UniformType valtype) const
{
    if (res && res->Valtype == ParseUniformType(valtype))
        return res->location;
    return GL_INVALID_INDEX;
}
GLint oglProgram_::GetLoc(const string_view name,     UniformType valtype) const
{
    auto obj = (name.empty() || name[0] != '@') ? FindInSet(ProgRess, name) 
        : FindInMap(ResNameMapping, string_view(&name[1], name.length() - 1), std::in_place).value_or(nullptr);
    if (obj && obj->Valtype == ParseUniformType(valtype))
        return obj->location;
    return GL_INVALID_INDEX;
}
GLint oglProgram_::GetLoc(const string& name) const
{
    auto obj = (name.empty() || name[0] != '@') ? FindInSet(ProgRess, name) 
        : FindInMap(ResNameMapping, string_view(&name[1], name.length() - 1), std::in_place).value_or(nullptr);
    if (obj)
        return obj->location;
    return GL_INVALID_INDEX;
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


void oglProgram_::SetTexture(detail::TextureManager& texMan, const GLint pos, const oglTexBase& tex, const bool shouldPin)
{
    CheckCurrent();
    auto& obj = UniBindCache[pos];
    const GLsizei val = tex ? texMan.bind(tex, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    DSA->ogluProgramUniform1i(ProgramID, pos, obj = val);
}

void oglProgram_::SetTexture(detail::TextureManager& texMan, const map<GLuint, oglTexBase>& texs, const bool shouldPin)
{
    CheckCurrent();
    switch (texs.size())
    {
    case 0:
        return;
    case 1:
        SetTexture(texMan, texs.begin()->first, texs.begin()->second, shouldPin);
        break;
    default:
        texMan.bindAll(ProgramID, texs, UniBindCache, shouldPin);
        break;
    }
}

void oglProgram_::SetImage(detail::TexImgManager& texMan, const GLint pos, const oglImgBase& img, const bool shouldPin)
{
    CheckCurrent();
    auto& obj = UniBindCache[pos];
    const GLsizei val = img ? texMan.bind(img, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    DSA->ogluProgramUniform1i(ProgramID, pos, obj = val);
}

void oglProgram_::SetImage(detail::TexImgManager& texMan, const map<GLuint, oglImgBase>& imgs, const bool shouldPin)
{
    CheckCurrent();
    switch (imgs.size())
    {
    case 0:
        return;
    case 1:
        SetImage(texMan, imgs.begin()->first, imgs.begin()->second, shouldPin);
        break;
    default:
        texMan.bindAll(ProgramID, imgs, UniBindCache, shouldPin);
        break;
    }
}

void oglProgram_::SetUBO(detail::UBOManager& uboMan, const GLint pos, const oglUBO& ubo, const bool shouldPin)
{
    CheckCurrent();
    auto& obj = UniBindCache[pos];
    const auto val = ubo ? uboMan.bind(ubo, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    DSA->ogluUniformBlockBinding(ProgramID, pos, obj = val);
}

void oglProgram_::SetUBO(detail::UBOManager& uboMan, const map<GLuint, oglUBO>& ubos, const bool shouldPin)
{
    CheckCurrent();
    switch (ubos.size())
    {
    case 0:
        return;
    case 1:
        SetUBO(uboMan, ubos.begin()->first, ubos.begin()->second, shouldPin);
        break;
    default:
        uboMan.bindAll(ProgramID, ubos, UniBindCache, shouldPin);
        break;
    }
}

void oglProgram_::SetSubroutine()
{
    CheckCurrent();
    for (const auto&[stype, subrs] : SubroutineSettings)
    {
        GLsizei cnt = (GLsizei)subrs.size();
        if (cnt > 0)
            DSA->ogluUniformSubroutinesuiv(common::enum_cast(stype), cnt, subrs.data());
    }
}

void oglProgram_::SetSubroutine(const SubroutineResource* subr, const SubroutineResource::Routine* routine)
{
    CheckCurrent();
    auto& oldRoutine = SubroutineBindings[subr];
    auto& srvec = SubroutineSettings[subr->Stage][subr->UniLoc];
    srvec = routine->Id;
    oldRoutine = routine;
}

void oglProgram_::SetUniform(const GLint pos, const b3d::Coord2D& vec, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        DSA->ogluProgramUniform2fv(ProgramID, pos, 1, vec);
    }
}
void oglProgram_::SetUniform(const GLint pos, const miniBLAS::Vec3& vec, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        DSA->ogluProgramUniform3fv(ProgramID, pos, 1, vec);
    }
}
void oglProgram_::SetUniform(const GLint pos, const miniBLAS::Vec4& vec, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        DSA->ogluProgramUniform4fv(ProgramID, pos, 1, vec);
    }
}
void oglProgram_::SetUniform(const GLint pos, const miniBLAS::Mat3x3& mat, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, mat);
        DSA->ogluProgramUniformMatrix4fv(ProgramID, pos, 1, GL_FALSE, mat.inv());
    }
}
void oglProgram_::SetUniform(const GLint pos, const miniBLAS::Mat4x4& mat, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, mat);
        DSA->ogluProgramUniformMatrix4fv(ProgramID, pos, 1, GL_FALSE, mat.inv());
    }
}
void oglProgram_::SetUniform(const GLint pos, const bool val, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        DSA->ogluProgramUniform1i(ProgramID, pos, val);
    }
}
void oglProgram_::SetUniform(const GLint pos, const int32_t val, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        DSA->ogluProgramUniform1i(ProgramID, pos, val);
    }
}
void oglProgram_::SetUniform(const GLint pos, const uint32_t val, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        DSA->ogluProgramUniform1ui(ProgramID, pos, val);
    }
}
void oglProgram_::SetUniform(const GLint pos, const float val, const bool keep)
{
    CheckCurrent();
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        DSA->ogluProgramUniform1f(ProgramID, pos, val);
    }
}


ProgState::~ProgState()
{
    if (oglProgram_::usethis(Prog, false)) //self used, then changed to keep pinned status
    {
        Prog.RecoverState();
    }
}


ProgState& ProgState::SetTexture(const oglTexBase& tex, const string& name, const GLuint idx)
{
    const auto it = Prog.TexRess.find(name);
    if (it != Prog.TexRess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        Prog.TexBindings.insert_or_assign(pos, tex);
    }
    return *this;
}

ProgState& ProgState::SetTexture(const oglTexBase& tex, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        Prog.TexBindings.insert_or_assign(pos, tex);
    }
    return *this;
}

ProgState& ProgState::SetImage(const oglImgBase& img, const string& name, const GLuint idx)
{
    const auto it = Prog.ImgRess.find(name);
    if (it != Prog.ImgRess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        Prog.ImgBindings.insert_or_assign(pos, img);
    }
    return *this;
}

ProgState& ProgState::SetImage(const oglImgBase& img, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        Prog.ImgBindings.insert_or_assign(pos, img);
    }
    return *this;
}

ProgState& ProgState::SetUBO(const oglUBO& ubo, const string& name, const GLuint idx)
{
    const auto it = Prog.UBORess.find(name);
    if (it != Prog.UBORess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        Prog.UBOBindings.insert_or_assign(pos, ubo);
    }
    return *this;
}

ProgState& ProgState::SetUBO(const oglUBO& ubo, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        Prog.UBOBindings.insert_or_assign(pos, ubo);
    }
    return *this;
}

ProgState& ProgState::SetSubroutine(const SubroutineResource::Routine* routine)
{
    if (auto pSubr = FindInMap(Prog.subrLookup, routine))
        Prog.SetSubroutine(*pSubr, routine);
    else
        oglLog().warning(u"cannot find subroutine for routine {}\n", routine->Name);
    return *this;
}

ProgState& ProgState::SetSubroutine(const string_view& subrName, const string_view& routineName)
{
    if (auto pSubr = FindInSet(Prog.SubroutineRess, subrName))
    {
        if (auto pRoutine = FindInVec(pSubr->Routines, [&routineName](const SubroutineResource::Routine& routine) { return routine.Name == routineName; }))
            Prog.SetSubroutine(pSubr, pRoutine);
        else
            oglLog().warning(u"cannot find routine {} for subroutine {}\n", routineName, subrName);
    }
    else
        oglLog().warning(u"cannot find subroutine {}\n", subrName);
    return *this;
}


oglDrawProgram_::oglDrawProgram_(const std::u16string& name, const oglProgStub* stub)
    : oglProgram_(name, stub, true)
{
    map<string, string> defaultMapping =
    {
        { "ProjectMat",  "oglu_matProj" },
        { "ViewMat",     "oglu_matView" },
        { "ModelMat",    "oglu_matModel" },
        { "MVPNormMat",  "oglu_matNormal" },
        { "MVPMat",      "oglu_matMVP" },
        { "CamPosVec",   "oglu_camPos" },
        { "DrawID",      "oglu_drawId" },
        { "VertPos",     "oglu_vertPos" },
        { "VertNorm",    "oglu_vertNorm" },
        { "VertTexc",    "oglu_texPos" },
        { "VertColor",   "oglu_vertColor" },
        { "VertTan",     "oglu_vertTan" },
    };
    ExtInfo.ResMappings.merge(defaultMapping);
    ResNameMapping.clear();
    for (const auto& [target, varName] : ExtInfo.ResMappings)
    {
        if (auto obj = FindInSet(ProgRess, varName); obj)
            ResNameMapping.insert_or_assign(target, obj);
    }
    for (const auto& [target, res] : ResNameMapping)
    {
        switch (hash_(target))
        {
        case "ProjectMat"_hash:  Uni_projMat   = res->location; break; //projectMatrix
        case "ViewMat"_hash:     Uni_viewMat   = res->location; break; //viewMatrix
        case "ModelMat"_hash:    Uni_modelMat  = res->location; break; //modelMatrix
        case "MVPMat"_hash:      Uni_mvpMat    = res->location; break; //model-view-project-Matrix
        case "MVPNormMat"_hash:  Uni_normalMat = res->location; break; //model-view-project-Matrix
        case "CamPosVec"_hash:   Uni_camPos    = res->location; break; //camera position
        }
    }
}

void oglDrawProgram_::SetProject(const Mat4x4& projMat)
{
    matrix_Proj = projMat;
    SetUniform(Uni_projMat, matrix_Proj);
}

void oglDrawProgram_::SetView(const Mat4x4 & viewMat)
{
    matrix_View = viewMat;
    SetUniform(Uni_viewMat, matrix_View);
}

ProgDraw oglDrawProgram_::Draw(const Mat4x4& modelMat, const Mat3x3& normMat) noexcept
{
    return ProgDraw(*this, modelMat, normMat);
}
ProgDraw oglDrawProgram_::Draw(const Mat4x4& modelMat) noexcept
{
    return Draw(modelMat, (Mat3x3)modelMat);
}

oglDrawProgram oglDrawProgram_::Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config)
{
    auto stub = oglProgram_::Create();
    stub.AddExtShaders(extSrc, config, false, true);
    return stub.LinkDrawProgram(name);
}


thread_local common::SpinLocker ProgDrawLocker;
ProgDraw::ProgDraw(oglDrawProgram_& prog, const Mat4x4& modelMat, const Mat3x3& normMat) noexcept
    : Prog(prog), Lock(ProgDrawLocker.LockScope()),
    TexMan(oglTexBase_::getTexMan()), ImgMan(oglImgBase_::getImgMan()), UboMan(oglUniformBuffer_::getUBOMan())
{
    oglProgram_::usethis(Prog);
    SetPosition(modelMat, normMat);
}
ProgDraw::~ProgDraw()
{
    Restore();
}

ProgDraw& ProgDraw::Restore(const bool quick)
{
    Prog.CheckCurrent();
    if (quick) // only set binding-delegate
    {
        for (const auto&[pos, binding] : UniBindBackup)
        {
            switch (binding.second)
            {
            case ProgResType::TexType:
                if (const auto ptrTex = FindInMap(Prog.TexBindings, pos); ptrTex)
                    TexCache.insert_or_assign(pos, *ptrTex);
                break;
            case ProgResType::ImgType:
                if (const auto ptrImg = FindInMap(Prog.ImgBindings, pos); ptrImg)
                    ImgCache.insert_or_assign(pos, *ptrImg);
                break;
            case ProgResType::UBOCat:
                if (const auto ptrUbo = FindInMap(Prog.UBOBindings, pos); ptrUbo)
                    UBOCache.insert_or_assign(pos, *ptrUbo);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        for (const auto&[pos, binding] : UniBindBackup)
        {
            auto& obj = Prog.UniBindCache[pos];
            const auto val = binding.first;
            if (obj != val)
            {
                switch (binding.second)
                {
                case ProgResType::TexType:
                case ProgResType::ImgType:
                    DSA->ogluProgramUniform1i(Prog.ProgramID, pos, obj = val);
                    break;
                case ProgResType::UBOCat:
                    DSA->ogluUniformBlockBinding(Prog.ProgramID, pos, obj = val);
                    break;
                default: break;
                }
            }
        }
        UniBindBackup.clear();
    }

    for (const auto&[pos, val] : UniValBackup)
    {
        std::visit([&, pos=pos](auto&& arg) { Prog.SetUniform(pos, arg, false); }, val);
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

ProgDraw& ProgDraw::SetPosition(const Mat4x4& modelMat, const Mat3x3& normMat)
{
    if (Prog.Uni_mvpMat != (GLint)GL_INVALID_INDEX)
    {
        const auto mvpMat = Prog.matrix_Proj * Prog.matrix_View * modelMat;
        Prog.SetUniform(Prog.Uni_mvpMat, mvpMat);
    }
    Prog.SetUniform(Prog.Uni_modelMat, modelMat, false);
    Prog.SetUniform(Prog.Uni_normalMat, normMat, false);
    return *this;
}
ProgDraw& ProgDraw::Draw(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
    Prog.SetTexture(TexMan, TexCache);
    Prog.SetImage(ImgMan, ImgCache);
    Prog.SetUBO(UboMan, UBOCache);
    Prog.SetSubroutine();
    vao->Draw(size, offset);
    TexCache.clear();
    ImgCache.clear();
    UBOCache.clear();
    return *this;
}

ProgDraw& ProgDraw::Draw(const oglVAO& vao)
{
    Prog.SetTexture(TexMan, TexCache);
    Prog.SetImage(ImgMan, ImgCache);
    Prog.SetUBO(UboMan, UBOCache);
    Prog.SetSubroutine();
    vao->Draw();
    TexCache.clear();
    ImgCache.clear();
    UBOCache.clear();
    return *this;
}

ProgDraw& ProgDraw::SetTexture(const oglTexBase& tex, const string& name, const GLuint idx)
{
    const auto it = Prog.TexRess.find(name);
    if (it != Prog.TexRess.cend() && idx < it->len)
    {
        const auto pos = it->location + idx;
        TexCache.insert_or_assign(pos, tex);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::TexType);
    }
    return *this;
}

ProgDraw& ProgDraw::SetTexture(const oglTexBase& tex, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        TexCache.insert_or_assign(pos, tex);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::TexType);
    }
    return *this;
}

ProgDraw& ProgDraw::SetImage(const oglImgBase& img, const string& name, const GLuint idx)
{
    const auto it = Prog.ImgRess.find(name);
    if (it != Prog.ImgRess.cend() && idx < it->len)
    {
        const auto pos = it->location + idx;
        ImgCache.insert_or_assign(pos, img);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::ImgType);
    }
    return *this;
}

ProgDraw& ProgDraw::SetImage(const oglImgBase& img, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        ImgCache.insert_or_assign(pos, img);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::ImgType);
    }
    return *this;
}

ProgDraw& ProgDraw::SetUBO(const oglUBO& ubo, const string& name, const GLuint idx)
{
    const auto it = Prog.UBORess.find(name);
    if (it != Prog.UBORess.cend() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        UBOCache.insert_or_assign(pos, ubo);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::UBOCat);
    }
    return *this;
}

ProgDraw& ProgDraw::SetUBO(const oglUBO& ubo, const GLuint pos)
{
    if (pos < Prog.UniBindCache.size())
    {
        UBOCache.insert_or_assign(pos, ubo);
        const auto oldVal = Prog.UniBindCache[pos];
        if (oldVal != (GLint)GL_INVALID_INDEX)
            UniBindBackup.try_emplace(pos, oldVal, ProgResType::UBOCat);
    }
    return *this;
}

ProgDraw& ProgDraw::SetSubroutine(const SubroutineResource::Routine* routine)
{
    if (auto pSubr = FindInMap(Prog.subrLookup, routine))
    {
        if (auto pOldRoutine = FindInMap(Prog.SubroutineBindings, *pSubr))
            SubroutineCache.try_emplace(*pSubr, *pOldRoutine);
        Prog.SetSubroutine(*pSubr, routine);
    }
    else
        oglLog().warning(u"cannot find subroutine for routine {}\n", routine->Name);
    return *this;
}

ProgDraw& ProgDraw::SetSubroutine(const string_view& subrName, const string_view& routineName)
{
    if (auto pSubr = FindInSet(Prog.SubroutineRess, subrName))
    {
        if (auto pRoutine = FindInVec(pSubr->Routines, [&routineName](const SubroutineResource::Routine& routine) { return routine.Name == routineName; }))
        {
            if (auto pOldRoutine = FindInMap(Prog.SubroutineBindings, pSubr))
                SubroutineCache.try_emplace(pSubr, *pOldRoutine);
            Prog.SetSubroutine(pSubr, pRoutine);
        }
        else
            oglLog().warning(u"cannot find routine {} for subroutine {}\n", routineName, subrName);
    }
    else
        oglLog().warning(u"cannot find subroutine {}\n", subrName);
    return *this;
}


oglComputeProgram_::oglComputeProgram_(const std::u16string& name, const oglProgStub* stub)
    : oglProgram_(name, stub, false)
{
    DSA->ogluGetProgramiv(ProgramID, GL_COMPUTE_WORK_GROUP_SIZE, reinterpret_cast<GLint*>(LocalSize.data()));
    oglLog().debug(u"Compute Shader has a LocalSize [{}x{}x{}]\n", LocalSize[0], LocalSize[1], LocalSize[2]);
}

void oglComputeProgram_::Run(const uint32_t groupX, const uint32_t groupY, const uint32_t groupZ)
{
    CheckCurrent();
    usethis(*this);
    DSA->ogluDispatchCompute(groupX, groupY, groupZ);
}

oglComputeProgram oglComputeProgram_::Create(const std::u16string& name, const std::string& extSrc, const ShaderConfig& config)
{
    auto stub = oglProgram_::Create();
    stub.AddExtShaders(extSrc, config, true, false);
    return stub.LinkComputeProgram(name);
}

bool oglu::oglComputeProgram_::CheckSupport()
{
    return DSA->SupportComputeShader;
}



}