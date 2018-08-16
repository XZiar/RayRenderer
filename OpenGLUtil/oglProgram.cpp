#include "oglRely.h"
#include "oglException.h"
#include "oglProgram.h"
#include "oglContext.h"
#include "oglUtil.h"
#include "BindingManager.h"
#include "oglStringify.hpp"


namespace oglu
{
using common::container::FindInSet;
using common::container::FindInMap;
using common::container::FindInMapOrDefault;
using common::container::FindInVec;
using common::container::ReplaceInVec;


static detail::ContextResource<GLuint> CTX_PROG_MAP;
using namespace std::literals;


GLint ProgramResource::GetValue(const GLuint pid, const GLenum prop)
{
    GLint ret;
    glGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
    return ret;
}


string_view ProgramResource::GetTypeName() const noexcept
{
    switch (type)
    {
    case GL_UNIFORM_BLOCK:
        return "UniBlk"sv;
    case GL_UNIFORM:
        return isTexture() ? "TexUni"sv : "Uniform"sv;
    case GL_PROGRAM_INPUT:
        return "Attrib"sv;
    default:
        return nullptr;
    }
}


string_view ProgramResource::GetValTypeName() const noexcept
{
    switch (valtype)
    {
    case GL_UNIFORM_BLOCK:
        return "uniBlock"sv;
    default:
        return FindInMap(detail::GLENUM_STR, valtype, std::in_place).value_or("UNKNOWN"sv);
    }
}

void ProgramResource::InitData(const GLuint pid, const GLint idx)
{
    ifidx = (uint8_t)idx;
    if (type == GL_UNIFORM_BLOCK)
    {
        valtype = GL_UNIFORM_BLOCK;
        size = static_cast<uint16_t>(GetValue(pid, GL_BUFFER_DATA_SIZE));
    }
    else
    {
        valtype = (GLenum)GetValue(pid, GL_TYPE);
        len = GetValue(pid, GL_ARRAY_SIZE);
    }
}

bool ProgramResource::isTexture() const noexcept
{
    if (type != GL_UNIFORM)
        return false;
    if (valtype >= GL_SAMPLER_1D && valtype <= GL_SAMPLER_2D_RECT_SHADOW)
        return true;
    else if (valtype >= GL_SAMPLER_1D_ARRAY && valtype <= GL_SAMPLER_CUBE_SHADOW)
        return true;
    else if (valtype >= GL_INT_SAMPLER_1D && valtype <= GL_UNSIGNED_INT_SAMPLER_BUFFER)
        return true;
    else if (valtype >= GL_SAMPLER_2D_MULTISAMPLE && valtype <= GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
        return true;
    else
        return false;
}

bool ProgramResource::isImage() const noexcept
{
    if (type != GL_UNIFORM)
        return false;
    if (valtype >= GL_IMAGE_1D && valtype <= GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY)
        return true;
    else
        return false;
}


namespace detail
{


_oglProgram::_oglProgram(const u16string& name) : Name(name)
{
    programID = glCreateProgram();
}

_oglProgram::~_oglProgram()
{
    if (programID != 0 && usethis(*this, false)) //need unuse
    {
        CTX_PROG_MAP.InsertOrAssign([&](auto)
        {
            glUseProgram(0);
            glDeleteProgram(programID);
            return 0;
        });
        programID = 0;
    }
}

bool _oglProgram::usethis(_oglProgram& prog, const bool change)
{
    if (CTX_PROG_MAP.TryGet() == prog.programID)
        return true;
    if (!change)//only return status
        return false;
    CTX_PROG_MAP.InsertOrAssign([&](auto) 
    {
        glUseProgram(prog.programID);
        return prog.programID;
    });

    prog.RecoverState();
    return true;
}

void _oglProgram::RecoverState()
{
    SetSubroutine();
    auto& texMan = _oglTexBase::getTexMan();
    texMan.unpin();
    auto& imgMan = _oglImgBase::getImgMan();
    imgMan.unpin();
    auto& uboMan = _oglUniformBuffer::getUBOMan();
    uboMan.unpin();
    SetTexture(texMan, TexBindings, true);
    SetImage(imgMan, ImgBindings, true);
    SetUBO(uboMan, UBOBindings, true);
}


void _oglProgram::InitLocs()
{
    set<ProgramResource, std::less<>> dataMap;
    string nameBuf;
    GLenum datatypes[] = { GL_UNIFORM_BLOCK,GL_UNIFORM,GL_PROGRAM_INPUT };
    for (const GLenum dtype : datatypes)
    {
        GLint cnt = 0;
        glGetProgramInterfaceiv(programID, dtype, GL_ACTIVE_RESOURCES, &cnt);
        GLint maxNameLen = 0;
        glGetProgramInterfaceiv(programID, dtype, GL_MAX_NAME_LENGTH, &maxNameLen);
        for (GLint a = 0; a < cnt; ++a)
        {
            string_view resName;
            {
                nameBuf.resize(maxNameLen);
                GLsizei nameLen = 0;
                glGetProgramResourceName(programID, dtype, a, maxNameLen, &nameLen, nameBuf.data());
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
            ProgramResource datinfo(dtype, nameBuf);
            datinfo.InitData(programID, a);
            if (dtype == GL_UNIFORM_BLOCK)
                datinfo.location = glGetProgramResourceIndex(programID, dtype, nameBuf.c_str());
            else
                datinfo.location = glGetProgramResourceLocation(programID, dtype, nameBuf.c_str());

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
        if (info.isAttrib())
            AttrRess.insert(info);
        else
        {
            maxUniLoc = common::max(maxUniLoc, info.location + info.len);
            if (info.isUniformBlock())
                UBORess.insert(info);
            else if (info.isTexture())
                TexRess.insert(info);
            else if (info.isImage())
                ImgRess.insert(info);
        }
        ProgRess.insert(info);
        oglLog().debug(u"--{:>7}{:<3} -[{:^5}]- {}[{}]({}) size[{}]\n", info.GetTypeName(), info.ifidx, info.location, info.Name, info.len, info.GetValTypeName(), info.size);
    }
    UniBindCache.clear();
    UniBindCache.resize(maxUniLoc, GL_INVALID_INDEX);
}

void _oglProgram::InitSubroutines()
{
    const GLenum stages[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    SubroutineRess.clear();
    subrLookup.clear();
    SubroutineBindings.clear();
    SubroutineSettings.clear();
    auto& strBuffer = common::mlog::detail::StrFormater<char16_t>::GetBuffer();
    strBuffer.resize(0);
    {
        constexpr std::u16string_view tmp = u"SubRoutine Resource: \n";
        strBuffer.append(tmp.data(), tmp.data() + tmp.size());
    }
    string nameBuf;
    for (auto stage : stages)
    {
        GLint count;
        glGetProgramStageiv(programID, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &count);
        GLint maxNameLen = 0;
        glGetProgramStageiv(programID, stage, GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &maxNameLen);
        {
            GLint maxUNameLen = 0;
            glGetProgramStageiv(programID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH, &maxUNameLen);
            maxNameLen = std::max(maxNameLen, maxUNameLen);
        }
        nameBuf.resize(maxNameLen);
        for (int a = 0; a < count; ++a)
        {
            string uniformName;
            {
                GLint nameLen = 0;
                glGetActiveSubroutineUniformName(programID, stage, a, maxNameLen, &nameLen, nameBuf.data());
                uniformName.assign(nameBuf, 0, nameLen);
            }
            auto uniformLoc = glGetSubroutineUniformLocation(programID, stage, uniformName.data());
            fmt::format_to(strBuffer, u"SubRoutine {} at [{}]:\n", uniformName, uniformLoc);
            GLint srcnt = 0;
            glGetActiveSubroutineUniformiv(programID, stage, a, GL_NUM_COMPATIBLE_SUBROUTINES, &srcnt);
            vector<GLint> compSRs(srcnt, GL_INVALID_INDEX);
            glGetActiveSubroutineUniformiv(programID, stage, a, GL_COMPATIBLE_SUBROUTINES, compSRs.data());
            
            vector<SubroutineResource::Routine> routines;
            for (const auto subridx : compSRs)
            {
                string subrName;
                {
                    GLint nameLen = 0;
                    glGetActiveSubroutineName(programID, stage, subridx, maxNameLen, &nameLen, nameBuf.data());
                    subrName.assign(nameBuf, 0, nameLen);
                }
                fmt::format_to(strBuffer, u"--[{}]: {}\n", subridx, subrName);
                routines.push_back(SubroutineResource::Routine(subrName, subridx));
            }
            const auto it = SubroutineRess.emplace(stage, uniformLoc, uniformName, std::move(routines)).first;
            for (auto& routine : it->Routines)
                subrLookup[&routine] = &(*it);
        }
        GLint locCount = 0;
        glGetProgramStageiv(programID, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &locCount);
        SubroutineSettings[(ShaderType)stage].resize(locCount);
    }
    oglLog().debug(strBuffer);
    
    for (const auto& subr : SubroutineRess)
    {
        const auto& routine = subr.Routines[0];
        SubroutineBindings[&subr] = &routine;
        SubroutineSettings[subr.Stage][subr.UniLoc] = routine.Id;
    }
}

void _oglProgram::FilterProperties()
{
    set<ShaderExtProperty, std::less<>> newProperties;
    for (const auto& prop : ExtInfo.Properties)
    {
        oglLog().debug(u"prop[{}], typeof [{}], data[{}]\n", prop.Name, (uint8_t)prop.Type, prop.Data.has_value() ? "Has" : "None");
        if (auto res = FindInSet(ProgRess, prop.Name))
            if (prop.MatchType(res->valtype))
            {
                newProperties.insert(prop);
                switch (prop.Type)
                {
                case ShaderPropertyType::Color:
                    {
                        Vec4 vec;
                        glGetUniformfv(programID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Range:
                    {
                        b3d::Coord2D vec;
                        glGetUniformfv(programID, res->location, vec);
                        UniValCache.insert_or_assign(res->location, vec);
                    } break;
                case ShaderPropertyType::Bool:
                    {
                        int32_t flag = 0;
                        glGetUniformiv(programID, res->location, &flag);
                        UniValCache.insert_or_assign(res->location, (bool)flag);
                    } break;
                case ShaderPropertyType::Int:
                    {
                        int32_t val = 0;
                        glGetUniformiv(programID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Uint:
                    {
                        uint32_t val = 0;
                        glGetUniformuiv(programID, res->location, &val);
                        UniValCache.insert_or_assign(res->location, val);
                    } break;
                case ShaderPropertyType::Float:
                    {
                        float val = 0;
                        glGetUniformfv(programID, res->location, &val);
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


void _oglProgram::AddShader(const oglShader& shader)
{
    if (shaders.insert(shader).second)
        glAttachShader(programID, shader->shaderID);
    else
        oglLog().warning(u"Repeat adding shader {} to program [{}]\n", shader->shaderID, Name);
}


void _oglProgram::Link()
{
    glLinkProgram(programID);

    int result;
    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    int len;
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &len);
    string logstr((size_t)len, '\0');
    glGetProgramInfoLog(programID, len, &len, logstr.data());
    logstr.pop_back(); //null-terminated so pop back
    if (!result)
    {
        oglLog().warning(u"Link program failed.\n{}\n", logstr);
        glDeleteProgram(programID);
        COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, u"Link program failed", logstr);
    }
    oglLog().success(u"Link program success.\n{}\n", logstr);
    InitLocs();
    InitSubroutines();
    FilterProperties();

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
    for (const auto&[target, name] : ExtInfo.ResMappings)
    {
        if (auto obj = FindInSet(ProgRess, name); obj)
            ResNameMapping.insert_or_assign(target, obj);
    }
    OnPrepare();
}


GLint _oglProgram::GetLoc(const ProgramResource* res, GLenum valtype) const
{
    if (res && res->valtype == valtype)
        return res->location;
    return GL_INVALID_INDEX;
}
GLint _oglProgram::GetLoc(const string& name, GLenum valtype) const
{
    auto obj = (name.empty() || name[0] != '@') ? FindInSet(ProgRess, name) 
        : FindInMap(ResNameMapping, string_view(&name[1], name.length() - 1), std::in_place).value_or(nullptr);
    if (obj && obj->valtype == valtype)
        return obj->location;
    return GL_INVALID_INDEX;
}
GLint _oglProgram::GetLoc(const string& name) const
{
    auto obj = (name.empty() || name[0] != '@') ? FindInSet(ProgRess, name) 
        : FindInMap(ResNameMapping, string_view(&name[1], name.length() - 1), std::in_place).value_or(nullptr);
    if (obj)
        return obj->location;
    return GL_INVALID_INDEX;
}

const SubroutineResource::Routine* _oglProgram::GetSubroutine(const string& sruname)
{
    if (const SubroutineResource* pSru = FindInSet(SubroutineRess, sruname))
        return SubroutineBindings[pSru];
    oglLog().warning(u"cannot find subroutine object {}\n", sruname);
    return nullptr;
}

ProgState _oglProgram::State() noexcept
{
    return ProgState(*this);
}


void _oglProgram::SetTexture(TextureManager& texMan, const GLint pos, const oglTexBase& tex, const bool shouldPin)
{
    auto& obj = UniBindCache[pos];
    const GLsizei val = tex ? texMan.bind(tex, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    glProgramUniform1i(programID, pos, obj = val);
}

void _oglProgram::SetTexture(TextureManager& texMan, const map<GLuint, oglTexBase>& texs, const bool shouldPin)
{
    switch (texs.size())
    {
    case 0:
        return;
    case 1:
        SetTexture(texMan, texs.begin()->first, texs.begin()->second, shouldPin);
        break;
    default:
        texMan.bindAll(programID, texs, UniBindCache, shouldPin);
        break;
    }
}

void _oglProgram::SetImage(TexImgManager& texMan, const GLint pos, const oglImgBase& img, const bool shouldPin)
{
    auto& obj = UniBindCache[pos];
    const GLsizei val = img ? texMan.bind(img, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    glProgramUniform1i(programID, pos, obj = val);
}

void _oglProgram::SetImage(TexImgManager& texMan, const map<GLuint, oglImgBase>& imgs, const bool shouldPin)
{
    switch (imgs.size())
    {
    case 0:
        return;
    case 1:
        SetImage(texMan, imgs.begin()->first, imgs.begin()->second, shouldPin);
        break;
    default:
        texMan.bindAll(programID, imgs, UniBindCache, shouldPin);
        break;
    }
}

void _oglProgram::SetUBO(UBOManager& uboMan, const GLint pos, const oglUBO& ubo, const bool shouldPin)
{
    auto& obj = UniBindCache[pos];
    const auto val = ubo ? uboMan.bind(ubo, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    glUniformBlockBinding(programID, pos, obj = val);
}

void _oglProgram::SetUBO(UBOManager& uboMan, const map<GLuint, oglUBO>& ubos, const bool shouldPin)
{
    switch (ubos.size())
    {
    case 0:
        return;
    case 1:
        SetUBO(uboMan, ubos.begin()->first, ubos.begin()->second, shouldPin);
        break;
    default:
        uboMan.bindAll(programID, ubos, UniBindCache, shouldPin);
        break;
    }
}

void _oglProgram::SetSubroutine()
{
    for (const auto&[stage, subrs] : SubroutineSettings)
    {
        GLsizei cnt = (GLsizei)subrs.size();
        if (cnt > 0)
            glUniformSubroutinesuiv((GLenum)stage, cnt, subrs.data());
    }
}

void _oglProgram::SetSubroutine(const SubroutineResource* subr, const SubroutineResource::Routine* routine)
{
    auto& oldRoutine = SubroutineBindings[subr];
    auto& srvec = SubroutineSettings[subr->Stage][subr->UniLoc];
    srvec = routine->Id;
    oldRoutine = routine;
}

void _oglProgram::SetUniform(const GLint pos, const b3d::Coord2D& vec, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        glProgramUniform2fv(programID, pos, 1, vec);
    }
}
void _oglProgram::SetUniform(const GLint pos, const miniBLAS::Vec3& vec, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        glProgramUniform3fv(programID, pos, 1, vec);
    }
}
void _oglProgram::SetUniform(const GLint pos, const miniBLAS::Vec4& vec, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, vec);
        glProgramUniform4fv(programID, pos, 1, vec);
    }
}
void _oglProgram::SetUniform(const GLint pos, const miniBLAS::Mat3x3& mat, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, mat);
        glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat.inv());
    }
}
void _oglProgram::SetUniform(const GLint pos, const miniBLAS::Mat4x4& mat, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, mat);
        glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat.inv());
    }
}
void _oglProgram::SetUniform(const GLint pos, const bool val, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        glProgramUniform1i(programID, pos, val);
    }
}
void _oglProgram::SetUniform(const GLint pos, const int32_t val, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        glProgramUniform1i(programID, pos, val);
    }
}
void _oglProgram::SetUniform(const GLint pos, const uint32_t val, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        glProgramUniform1ui(programID, pos, val);
    }
}
void _oglProgram::SetUniform(const GLint pos, const float val, const bool keep)
{
    if (pos != (GLint)GL_INVALID_INDEX)
    {
        if (keep)
            UniValCache.insert_or_assign(pos, val);
        glProgramUniform1f(programID, pos, val);
    }
}


ProgState::~ProgState()
{
    if (_oglProgram::usethis(Prog, false)) //self used, then changed to keep pinned status
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

ProgState& ProgState::SetSubroutine(const string& subrName, const string& routineName)
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


void _oglDrawProgram::OnPrepare()
{
    RegisterLocation();
}

void _oglDrawProgram::AddExtShaders(const string& src)
{
    ExtShaderSource = src;
    for (auto shader : oglShader::LoadFromExSrc(src, ExtInfo, false))
    {
        shader->compile();
        AddShader(shader);
    }
}

void _oglDrawProgram::RegisterLocation()
{
    for (const auto&[target, res] : ResNameMapping)
    {
        switch (hash_(target))
        {
        case "ProjectMat"_hash:  Uni_projMat = res->location; break; //projectMatrix
        case "ViewMat"_hash:     Uni_viewMat = res->location; break; //viewMatrix
        case "ModelMat"_hash:    Uni_modelMat = res->location; break; //modelMatrix
        case "MVPMat"_hash:      Uni_mvpMat = res->location; break; //model-view-project-Matrix
        case "MVPNormMat"_hash:  Uni_normalMat = res->location; break; //model-view-project-Matrix
        case "CamPosVec"_hash:   Uni_camPos = res->location; break; //camera position
        }
    }
}

void _oglDrawProgram::SetProject(const Mat4x4& projMat)
{
    matrix_Proj = projMat;
    SetUniform(Uni_projMat, matrix_Proj);
}

void _oglDrawProgram::SetView(const Mat4x4 & viewMat)
{
    matrix_View = viewMat;
    SetUniform(Uni_viewMat, matrix_View);
}

ProgDraw _oglDrawProgram::Draw(const Mat4x4& modelMat, const Mat3x3& normMat) noexcept
{
    return ProgDraw(*this, modelMat, normMat);
}
ProgDraw _oglDrawProgram::Draw(const Mat4x4& modelMat) noexcept
{
    return Draw(modelMat, (Mat3x3)modelMat);
}


ProgDraw::ProgDraw(_oglDrawProgram& prog, const Mat4x4& modelMat, const Mat3x3& normMat) noexcept
    : Prog(prog), TexMan(_oglTexBase::getTexMan()), ImgMan(_oglImgBase::getImgMan()), UboMan(_oglUniformBuffer::getUBOMan())
{
    _oglProgram::usethis(Prog);
    SetPosition(modelMat, normMat);
}
ProgDraw::~ProgDraw()
{
    Restore();
}

ProgDraw& ProgDraw::Restore(const bool quick)
{
    if (quick) // only set binding-delegate
    {
        for (const auto&[pos, binding] : UniBindBackup)
        {
            switch (binding.second)
            {
            case UniformType::Tex:
                if (const auto ptrTex = FindInMap(Prog.TexBindings, pos); ptrTex)
                    TexCache.insert_or_assign(pos, *ptrTex);
                break;
            case UniformType::Img:
                if (const auto ptrImg = FindInMap(Prog.ImgBindings, pos); ptrImg)
                    ImgCache.insert_or_assign(pos, *ptrImg);
                break;
            case UniformType::Ubo:
                if (const auto ptrUbo = FindInMap(Prog.UBOBindings, pos); ptrUbo)
                    UBOCache.insert_or_assign(pos, *ptrUbo);
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
                case UniformType::Tex:
                case UniformType::Img:
                    glProgramUniform1i(Prog.programID, pos, obj = val);
                    break;
                case UniformType::Ubo:
                    glUniformBlockBinding(Prog.programID, pos, obj = val);
                    break;
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
std::weak_ptr<_oglDrawProgram> ProgDraw::GetProg() const noexcept
{
    return std::dynamic_pointer_cast<_oglDrawProgram>(Prog.shared_from_this());
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Tex);
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Tex);
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Img);
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Img);
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Ubo);
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
            UniBindBackup.try_emplace(pos, oldVal, UniformType::Ubo);
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

ProgDraw& ProgDraw::SetSubroutine(const string& subrName, const string& routineName)
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


_oglComputeProgram::_oglComputeProgram(const u16string name, const oglShader& shader) : _oglProgram(name)
{ 
    if (shader->shaderType != ShaderType::Compute)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Only Compute Shader can be add to compute program");
    AddShader(shader);
    Link();
}
_oglComputeProgram::_oglComputeProgram(const u16string name, const string& src) : _oglProgram(name)
{ 
    ExtShaderSource = src;
    const auto s = oglShader::LoadFromExSrc(src, ExtInfo, true, false);
    if (s.empty())
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"no available Computer Shader found");
    auto shader = *s.cbegin();
    shader->compile();
    AddShader(shader);
    Link();
}
void _oglComputeProgram::Run(const uint32_t groupX, const uint32_t groupY, const uint32_t groupZ)
{
    usethis(*this, false);
    glDispatchCompute(groupX, groupY, groupZ);
}

}

}