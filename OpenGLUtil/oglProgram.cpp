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
using common::container::FindInVec;
using common::container::ReplaceInVec;


static detail::ContextResource<GLuint> CTX_PROG_MAP;



GLint ProgramResource::getValue(const GLuint pid, const GLenum prop)
{
    GLint ret;
    glGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
    return ret;
}


const char* ProgramResource::GetTypeName() const noexcept
{
    switch (type)
    {
    case GL_UNIFORM_BLOCK:
        return "UniBlk";
    case GL_UNIFORM:
        return isTexture() ? "TexUni" : "Uniform";
    case GL_PROGRAM_INPUT:
        return "Attrib";
    default:
        return nullptr;
    }
}


const char* ProgramResource::GetValTypeName() const noexcept
{
    switch (valtype)
    {
    case GL_UNIFORM_BLOCK:
        return "uniBlock";
    default:
        return FindInMap(detail::GLENUM_STR, valtype, std::in_place).value_or(nullptr);
    }
}

void ProgramResource::initData(const GLuint pid, const GLint idx)
{
    ifidx = (uint8_t)idx;
    if (type == GL_UNIFORM_BLOCK)
    {
        valtype = GL_UNIFORM_BLOCK;
        size = getValue(pid, GL_BUFFER_DATA_SIZE);
    }
    else
    {
        valtype = (GLenum)getValue(pid, GL_TYPE);
        len = getValue(pid, GL_ARRAY_SIZE);
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


namespace detail
{

void _oglProgram::ProgState::init()
{
    for (const auto& sru : prog.SubroutineRess)
    {
        srSettings[&sru] = &sru.Routines[0];
        srCache[sru.Stage].push_back(sru.Routines[0].Id);
    }
}

_oglProgram::ProgState::ProgState(_oglProgram& prog_) :prog(prog_)
{
}

void _oglProgram::ProgState::setTexture(TextureManager& texMan, const GLint pos, const oglTexture& tex, const bool shouldPin) const
{
    auto& obj = prog.uniCache[pos];
    const GLsizei val = tex ? texMan.bind(tex, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    glProgramUniform1i(prog.programID, pos, obj = val);
}

void _oglProgram::ProgState::setTexture(TextureManager& texMan, const bool shouldPin) const
{
    switch (texCache.size())
    {
    case 0:
        return;
    case 1:
        setTexture(texMan, texCache.begin()->first, texCache.begin()->second);
        break;
    default:
        texMan.bindAll(prog.programID, texCache, prog.uniCache, shouldPin);
        break;
    }
}

void _oglProgram::ProgState::setUBO(UBOManager& uboMan, const GLint pos, const oglUBO& ubo, const bool shouldPin) const
{
    auto& obj = prog.uniCache[pos];
    const auto val = ubo ? uboMan.bind(ubo, shouldPin) : 0;
    if (obj == val)//no change
        return;
    //change value and update uniform-hold map
    glUniformBlockBinding(prog.programID, pos, obj = val);
}

void _oglProgram::ProgState::setUBO(UBOManager& uboMan, const bool shouldPin) const
{
    switch (uboCache.size())
    {
    case 0:
        return;
    case 1:
        setUBO(uboMan, uboCache.begin()->first, uboCache.begin()->second);
        break;
    default:
        uboMan.bindAll(prog.programID, uboCache, prog.uniCache, shouldPin);
        break;
    }
}

void _oglProgram::ProgState::setSubroutine() const
{
    for (const auto& stagePair : srCache)
    {
        GLsizei cnt = (GLsizei)stagePair.second.size();
        if(cnt > 0)
            glUniformSubroutinesuiv((GLenum)stagePair.first, cnt, stagePair.second.data());
    }
}

void _oglProgram::ProgState::end()
{
    if (_oglProgram::usethis(prog, false)) //self used, then changed to keep pinned status
    {
        auto& texMan = _oglTexture::getTexMan();
        texMan.unpin();
        auto& uboMan = _oglUniformBuffer::getUBOMan();
        uboMan.unpin();
        setTexture(texMan, true);
        setUBO(uboMan, true);
        setSubroutine();
    }
}


_oglProgram::ProgState& _oglProgram::ProgState::setTexture(const oglTexture& tex, const string& name, const GLuint idx)
{
    const auto it = prog.TexRess.find(name);
    if (it != prog.TexRess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        texCache.insert_or_assign(pos, tex);
    }
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::setTexture(const oglTexture& tex, const GLuint pos)
{
    if (pos < prog.uniCache.size())
    {
        texCache.insert_or_assign(pos, tex);
    }
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::setUBO(const oglUBO& ubo, const string& name, const GLuint idx)
{
    const auto it = prog.UBORess.find(name);
    if (it != prog.UBORess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        uboCache.insert_or_assign(pos, ubo);
    }
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::setUBO(const oglUBO& ubo, const GLuint pos)
{
    if (pos < prog.uniCache.size())
    {
        uboCache.insert_or_assign(pos, ubo);
    }
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::setSubroutine(const SubroutineResource::Routine& sr)
{
    if (auto ppSru = FindInMap(prog.subrLookup, &sr))
    {
        auto& theSr = srSettings[*ppSru];
        auto& srvec = srCache[(**ppSru).Stage];
        std::replace(srvec.begin(), srvec.end(), theSr->Id, sr.Id); //ensured has value
        theSr = &sr;
    }
    else
        oglLog().warning(u"cannot find subroutine {}\n", sr.Name);
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::setSubroutine(const string& sruname, const string& srname)
{
    if (auto pSru = FindInSet(prog.SubroutineRess, sruname))
    {
        if (auto pSr = FindInVec(pSru->Routines, [&srname](const SubroutineResource::Routine& srr) { return srr.Name == srname; }))
        {
            auto& theSr = srSettings[pSru];
            auto& srvec = srCache[pSru->Stage];
            std::replace(srvec.begin(), srvec.end(), theSr->Id, pSr->Id); //ensured has value
            theSr = pSr;
        }
        else
            oglLog().warning(u"cannot find subroutine {} for object {}\n", srname, sruname);
    }
    else
        oglLog().warning(u"cannot find subroutine object {}\n", sruname);
    return *this;
}

_oglProgram::ProgState& _oglProgram::ProgState::getSubroutine(const string& sruname, string& srname)
{
    if (const SubroutineResource* pSru = FindInSet(prog.SubroutineRess, sruname))
        srname = srSettings[pSru]->Name;
    else
        oglLog().warning(u"cannot find subroutine object {}\n", sruname);
    return *this;
}


_oglProgram::ProgDraw::ProgDraw(const ProgState& pstate, const Mat4x4& modelMat, const Mat3x3& normMat) 
    : ProgState(pstate.prog), TexMan(_oglTexture::getTexMan()), UboMan(_oglUniformBuffer::getUBOMan())
{
    _oglProgram::usethis(prog);
    if (prog.Uni_mvpMat != GL_INVALID_INDEX)
    {
        const auto mvpMat = prog.matrix_Proj * prog.matrix_View * modelMat;
        prog.setMat(prog.Uni_mvpMat, mvpMat);
    }
    prog.setMat(prog.Uni_modelMat, modelMat);
    prog.setMat(prog.Uni_normalMat, normMat);
}

void _oglProgram::ProgDraw::end()
{
    _oglVAO::unbind();
    for (const auto& item : UniformBackup)
    {
        const auto pos = item.first;
        auto& obj = prog.uniCache[pos];
        const auto val = item.second.first;
        if (obj != val)
        {
            if (item.second.second) //tex
                glProgramUniform1i(prog.programID, pos, obj = val);
            else //ubo
                glUniformBlockBinding(prog.programID, pos, obj = val);
        }
    }
    UniformBackup.clear();
}


_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
    ProgState::setTexture(TexMan);
    ProgState::setUBO(UboMan);
    ProgState::setSubroutine();
    vao->draw(size, offset);
    texCache.clear();
    uboCache.clear();
    srCache.clear();
    return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao)
{
    ProgState::setTexture(TexMan);
    ProgState::setUBO(UboMan);
    ProgState::setSubroutine();
    vao->draw();
    texCache.clear();
    uboCache.clear();
    srCache.clear();
    return *this;
}

oglu::detail::_oglProgram::ProgDraw& _oglProgram::ProgDraw::setTexture(const oglTexture& tex, const string& name, const GLuint idx)
{
    const auto it = prog.TexRess.find(name);
    if (it != prog.TexRess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        texCache.insert_or_assign(pos, tex);
        UniformBackup.try_emplace(pos, std::make_pair(prog.uniCache[pos], true));
    }
    return *this;
}

oglu::detail::_oglProgram::ProgDraw& _oglProgram::ProgDraw::setTexture(const oglTexture& tex, const GLuint pos)
{
    if (pos < prog.uniCache.size())
    {
        texCache.insert_or_assign(pos, tex);
        UniformBackup.try_emplace(pos, std::make_pair(prog.uniCache[pos], true));
    }
    return *this;
}

oglu::detail::_oglProgram::ProgDraw& _oglProgram::ProgDraw::setUBO(const oglUBO& ubo, const string& name, const GLuint idx)
{
    const auto it = prog.UBORess.find(name);
    if (it != prog.UBORess.end() && idx < it->len)//legal
    {
        const auto pos = it->location + idx;
        uboCache.insert_or_assign(pos, ubo);
        UniformBackup.try_emplace(pos, std::make_pair(prog.uniCache[pos], false));
    }
    return *this;
}

oglu::detail::_oglProgram::ProgDraw& _oglProgram::ProgDraw::setUBO(const oglUBO& ubo, const GLuint pos)
{
    if (pos < prog.uniCache.size())
    {
        uboCache.insert_or_assign(pos, ubo);
        UniformBackup.try_emplace(pos, std::make_pair(prog.uniCache[pos], false));
    }
    return *this;
}


_oglProgram::_oglProgram(const u16string& name) :gState(*this), Name(name)
{
    programID = glCreateProgram();
}

_oglProgram::~_oglProgram()
{
    if (programID != 0 && usethis(*this, false)) //need unuse
    {
        CTX_PROG_MAP.InsertOrAssign([&](auto dummy)
        {
            glUseProgram(0);
            glDeleteProgram(programID);
            return 0;
        });
    }
}

bool _oglProgram::usethis(_oglProgram& prog, const bool change)
{
    if (CTX_PROG_MAP.TryGet() == prog.programID)
        return true;
    if (!change)//only return status
        return false;
    CTX_PROG_MAP.InsertOrAssign([&](auto dummy) 
    {
        glUseProgram(prog.programID);
        return prog.programID;
    });

    prog.RecoverState();
    return true;
}

void _oglProgram::RecoverState()
{
    gState.setSubroutine();
    auto& texMan = _oglTexture::getTexMan();
    texMan.unpin();
    auto& uboMan = _oglUniformBuffer::getUBOMan();
    uboMan.unpin();
    gState.setTexture(texMan, true);
    gState.setUBO(uboMan, true);
}

void _oglProgram::setMat(const GLint pos, const Mat4x4& mat) const
{
    if (pos != GL_INVALID_INDEX)
        glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat.inv());
}

void _oglProgram::initLocs()
{
    set<ProgramResource, std::less<>> dataMap;
    GLchar name[256];
    GLenum datatypes[] = { GL_UNIFORM_BLOCK,GL_UNIFORM,GL_PROGRAM_INPUT };
    for (const GLenum dtype : datatypes)
    {
        GLint cnt = 0;
        glGetProgramInterfaceiv(programID, dtype, GL_ACTIVE_RESOURCES, &cnt);
        for (GLint a = 0; a < cnt; ++a)
        {
            uint32_t arraylen = 0;
            glGetProgramResourceName(programID, dtype, a, 240, nullptr, name);
            char* chpos = nullptr;
            //printf("@@query %d\t\t%s\n", binfo.iftype, name);
            chpos = strchr(name, '.');
            if (chpos != nullptr)//remove struct
                *chpos = '\0';
            chpos = strchr(name, '[');
            if (chpos != nullptr)//array
            {
                arraylen = atol(chpos + 1) + 1;
                *chpos = '\0';
            }
            const string namestr(name);
            if (auto it = FindInSet(dataMap, namestr))
            {
                it->len = common::max(it->len, arraylen);
                continue;
            }
            ProgramResource datinfo(dtype, namestr);
            datinfo.initData(programID, a);
            if (dtype == GL_UNIFORM_BLOCK)
                datinfo.location = glGetProgramResourceIndex(programID, dtype, name);
            else
                datinfo.location = glGetProgramResourceLocation(programID, dtype, name);

            if (datinfo.location != GL_INVALID_INDEX)
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
        }
        ProgRess.insert(info);
        oglLog().debug(u"--{:>7}{:<3} -[{:^5}]- {}[{}]({}) size[{}]\n", info.GetTypeName(), info.ifidx, info.location, info.Name, info.len, info.GetValTypeName(), info.size);
    }
    uniCache.resize(maxUniLoc, static_cast<GLint>(UINT32_MAX));
}

void _oglProgram::initSubroutines()
{
    const GLenum stages[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    char strbuf[4096];
    auto& writer = common::mlog::detail::StrFormater<char16_t>::GetWriter();
    writer.write(u"SubRoutine Resource: \n");
    SubroutineRess.clear();
    subrLookup.clear();
    for (auto stage : stages)
    {
        GLint count;
        glGetProgramStageiv(programID, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &count);
        for (int a = 0; a < count; ++a)
        {
            glGetActiveSubroutineUniformName(programID, stage, a, sizeof(strbuf), nullptr, strbuf);
            string uniformName(strbuf);
            auto uniformLoc = glGetSubroutineUniformLocation(programID, stage, strbuf);
            writer.write(u"SubRoutine {} at [{}]:\n", uniformName, uniformLoc);
            GLint srcnt;
            glGetActiveSubroutineUniformiv(programID, stage, a, GL_NUM_COMPATIBLE_SUBROUTINES, &srcnt);
            vector<GLint> compSRs(srcnt, GL_INVALID_INDEX);
            vector<SubroutineResource::Routine> routines;
            glGetActiveSubroutineUniformiv(programID, stage, a, GL_COMPATIBLE_SUBROUTINES, compSRs.data());
            for(auto idx : compSRs)
            {
                glGetActiveSubroutineName(programID, stage, idx, sizeof(strbuf), nullptr, strbuf);
                string subrname(strbuf);
                writer.write(u"--[{}]: {}\n", idx, subrname);
                routines.push_back(SubroutineResource::Routine(subrname, idx));
            }
            auto[it, isAdd] = SubroutineRess.emplace(stage, uniformLoc, uniformName, std::move(routines));
            for (auto& routine : it->Routines)
                subrLookup[&routine] = &(*it);
        }
    }
    oglLog().debug(writer.c_str());
    gState.init();
}

void _oglProgram::addShader(const oglShader& shader)
{
    auto [it, isAdd] = shaders.insert(shader);
    if (isAdd)
        glAttachShader(programID, shader->shaderID);
    else
        oglLog().warning(u"Repeat adding shader {} to program [{}]\n", shader->shaderID, Name);
}

void _oglProgram::link()
{
    glLinkProgram(programID);
    int result;

    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    if (!result)
    {
        int len;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &len);
        string logstr((size_t)len, '\0');
        glGetProgramInfoLog(programID, len, &len, logstr.data());
        const auto logdat = common::str::to_u16string(logstr.c_str());
        oglLog().warning(u"Link program failed.\n{}\n", logdat);
        glDeleteProgram(programID);
        COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, L"Link program failed", logdat);
    }
    initLocs();
    initSubroutines();
}


void _oglProgram::registerLocation(const string(&VertAttrName)[4], const string(&MatrixName)[5])
{
    //initialize uniform location
    Uni_projMat = getLoc(MatrixName[0]);//projectMatrix
    Uni_viewMat = getLoc(MatrixName[1]);//viewMatrix
    Uni_modelMat = getLoc(MatrixName[2]);//modelMatrix
    Uni_normalMat = getLoc(MatrixName[3]);//model-view-project-Matrix
    Uni_mvpMat = getLoc(MatrixName[4]);//model-view-project-Matrix

    //initialize vertex attribute location
    Attr_Vert_Pos = getLoc(VertAttrName[0]);
    Attr_Vert_Norm = getLoc(VertAttrName[1]);
    Attr_Vert_Texc = getLoc(VertAttrName[2]);
    Attr_Vert_Color = getLoc(VertAttrName[3]);
}

const ProgramResource* _oglProgram::getResource(const string& name) const
{
    return FindInSet(ProgRess, name);
}

const SubroutineResource* _oglProgram::getSubroutines(const string& name) const
{
    return FindInSet(SubroutineRess, name);
}

GLint _oglProgram::getLoc(const string& name) const
{
    if (auto obj = FindInSet(ProgRess, name))
        return obj->location;
    return GL_INVALID_INDEX;
}

void _oglProgram::setProject(const Camera& cam, const int wdWidth, const int wdHeight)
{
    //glViewport((wdWidth & 0x3f) / 2, (wdHeight & 0x3f) / 2, cam.width & 0xffc0, cam.height & 0xffc0);
    //assert(cam.aspect > std::numeric_limits<float>::epsilon());
    if (cam.aspect <= std::numeric_limits<float>::epsilon())
        return;
    const float cotHalfFovy = 1 / tan(b3d::ang2rad(cam.fovy / 2));
    const float viewDepthR = 1 / (cam.zFar - cam.zNear);
    /*   cot(fovy/2)
    *  -------------		0		   0			0
    *     aspect
    *
    *       0         cot(fovy/2)	   0			0
    *
    *								  F+N         2*F*N
    *       0              0		 -----	   - -------
    *								  F-N          F-N
    *
    *       0              0           1			0
    *
    **/
    matrix_Proj = Mat4x4(Vec4(cotHalfFovy / cam.aspect, 0.f, 0.f, 0.f),
        Vec4(0.f, cotHalfFovy, 0.f, 0.f),
        Vec4(0.f, 0.f, (cam.zFar + cam.zNear) * viewDepthR, (-2 * cam.zFar * cam.zNear) * viewDepthR),
        Vec4(0.f, 0.f, 1.f, 0.f));

    setMat(Uni_projMat, matrix_Proj);
}

void _oglProgram::setCamera(const Camera & cam)
{
    //LookAt
    //matrix_View = glm::lookAt(vec3(cam.position), vec3(Vertex(cam.position + cam.n)), vec3(cam.v));
    const auto rMat = cam.camMat.inv();
    matrix_View = Mat4x4::TranslateMat(cam.position * -1, rMat);

    setMat(Uni_viewMat, matrix_View);
    if (Uni_camPos != GL_INVALID_INDEX)
        glProgramUniform3fv(programID, Uni_camPos, 1, cam.position);
}

_oglProgram::ProgDraw _oglProgram::draw(const Mat4x4& modelMat, const Mat3x3& normMat)
{
    return ProgDraw(gState, modelMat, normMat);
}

_oglProgram::ProgDraw _oglProgram::draw(const Mat4x4& modelMat)
{
    return draw(modelMat, (Mat3x3)modelMat);
}

_oglProgram::ProgDraw _oglProgram::draw(topIT begin, topIT end)
{
    Mat4x4 matModel = Mat4x4::identity();
    Mat3x3 matNormal = Mat3x3::identity();
    for (topIT cur = begin; cur != end; ++cur)
    {
        const TransformOP& trans = *cur;
        oglUtil::applyTransform(matModel, matNormal, trans);
    }
    return draw(matModel, matNormal);
}

_oglProgram::ProgState& _oglProgram::globalState()
{
    return gState;
}

}

}