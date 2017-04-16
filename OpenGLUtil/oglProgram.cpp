#include "oglRely.h"
#include "oglException.h"
#include "oglProgram.h"
#include "oglUtil.h"
#include "BindingManager.h"
#include "oglInternal.h"

namespace oglu::inner
{


_oglProgram::ProgState::ProgState(_oglProgram& prog_) :prog(prog_)
{
}

void _oglProgram::ProgState::setTexture(const GLint pos, const oglTexture& tex) const
{
	auto& obj = prog.uniCache[pos];
	const GLsizei val = tex ? _oglTexture::getTexMan().bind(tex) : 0;
	if (obj == val)//no change
		return;
	//change value and update uniform-hold map
	glProgramUniform1i(prog.programID, pos, obj = val);
}

void _oglProgram::ProgState::setTexture() const
{
	switch (texCache.size())
	{
	case 0:
		return;
	case 1:
		setTexture(texCache.begin()->first, texCache.begin()->second);
		break;
	default:
		_oglTexture::getTexMan().bindAll(prog.programID, texCache, prog.uniCache);
		break;
	}
}

void _oglProgram::ProgState::setUBO(const GLint pos, const oglUBO& ubo) const
{
	auto& obj = prog.uniCache[pos];
	const auto val = ubo ? _oglUniformBuffer::getUBOMan().bind(ubo) : 0;
	if (obj == val)//no change
		return;
	//change value and update uniform-hold map
	glUniformBlockBinding(prog.programID, pos, obj = val);
}

void _oglProgram::ProgState::setUBO() const
{
	switch (uboCache.size())
	{
	case 0:
		return;
	case 1:
		setUBO(uboCache.begin()->first, uboCache.begin()->second);
		break;
	default:
		_oglUniformBuffer::getUBOMan().bindAll(prog.programID, uboCache, prog.uniCache);
		break;
	}
}

void _oglProgram::ProgState::end()
{
}


_oglProgram::ProgState& _oglProgram::ProgState::setTexture(const oglTexture& tex, const string& name, const GLuint idx)
{
	const auto it = prog.texMap.find(name);
	if (it != prog.texMap.end() && idx < it->second.len)//legal
	{
		const auto pos = it->second.location + idx;
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
	const auto it = prog.uboMap.find(name);
	if (it != prog.uboMap.end() && idx < it->second.len)//legal
	{
		const auto pos = it->second.location + idx;
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


_oglProgram::ProgDraw::ProgDraw(const ProgState& pstate, const Mat4x4& modelMat, const Mat3x3& normMat) :ProgState(pstate.prog)
{
	_oglProgram::usethis(prog);
	if (prog.Uni_mvpMat != GL_INVALID_INDEX)
	{
		const auto mvpMat = prog.matrix_Proj * prog.matrix_View * modelMat;
		prog.setMat(prog.Uni_mvpMat, mvpMat);
	}
	prog.setMat(prog.Uni_modelMat, modelMat);
	prog.setMat(prog.Uni_normalMat, normMat);
	texCache = pstate.texCache;
	uboCache = pstate.uboCache;
	pstate.setTexture();
	pstate.setUBO();
}

void _oglProgram::ProgDraw::end()
{
	_oglVAO::unbind();
}


_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
	ProgState::setTexture();
	texCache.clear();
	ProgState::setUBO();
	uboCache.clear();
	vao->draw(size, offset);
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao)
{
	ProgState::setTexture();
	texCache.clear();
	ProgState::setUBO();
	uboCache.clear();
	vao->draw();
	return *this;
}


GLint _oglProgram::DataInfo::getValue(const GLuint pid, const GLenum prop)
{
	GLint ret;
	glGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
	return ret;
}


const char * _oglProgram::DataInfo::getTypeName() const
{
	static const char name[][8] = { "UniBlk","Uniform","Attrib" };
	switch (type)
	{
	case GL_UNIFORM_BLOCK:
		return name[0];
	case GL_UNIFORM:
		return name[1];
	case GL_PROGRAM_INPUT:
		return name[2];
	default:
		return nullptr;
	}
}

void _oglProgram::DataInfo::initData(const GLuint pid, const GLint idx)
{
	ifidx = (uint8_t)idx;
	if (type == GL_UNIFORM_BLOCK)
	{
		valtype = GL_UNIFORM_BLOCK;
	}
	else
	{
		len = getValue(pid, GL_ARRAY_SIZE);
		valtype = (GLenum)getValue(pid, GL_TYPE);
	}
}

bool _oglProgram::DataInfo::isTexture() const
{
	if (type != GL_UNIFORM)
		return false;
	if (valtype >= GL_SAMPLER_1D && valtype <= GL_SAMPLER_2D_RECT_SHADOW)
		return true;
	else if(valtype >= GL_SAMPLER_1D_ARRAY && valtype <= GL_SAMPLER_CUBE_SHADOW)
		return true;
	else if (valtype >= GL_INT_SAMPLER_1D && valtype <= GL_UNSIGNED_INT_SAMPLER_BUFFER)
		return true;
	else if (valtype >= GL_SAMPLER_2D_MULTISAMPLE && valtype <= GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY)
		return true;
	else
		return false;
}


_oglProgram::_oglProgram() :gState(*this)
{
	programID = glCreateProgram();
}

_oglProgram::~_oglProgram()
{
	if (programID != GL_INVALID_INDEX)
	{
		bool shouldUnUse = usethis(*this, false);
		glDeleteProgram(programID);
		programID = GL_INVALID_INDEX;
		if(shouldUnUse)
			usethis(*this, true);
	}
}

bool _oglProgram::usethis(_oglProgram& prog, const bool change)
{
	static thread_local GLuint curPID = GL_INVALID_INDEX;
	if (curPID != prog.programID)
	{
		if (!change)//only return status
			return false;
		if (prog.programID == GL_INVALID_INDEX)
		{
			glUseProgram(curPID = 0);
		}
		else
		{
			glUseProgram(curPID = prog.programID);
			//prog.recoverBindings();
		}
	}
	return true;
}

void _oglProgram::setMat(const GLint pos, const Mat4x4& mat) const
{
	if (pos != GL_INVALID_INDEX)
		glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat.inv());
}

void _oglProgram::initLocs()
{
	map<string, DataInfo> dataMap;
	GLchar name[256];
	DataInfo baseinfo[] = { GL_UNIFORM_BLOCK,GL_UNIFORM,GL_PROGRAM_INPUT };
	for(const DataInfo& binfo : baseinfo)
	{
		GLint cnt = 0;
		glGetProgramInterfaceiv(programID, binfo.type, GL_ACTIVE_RESOURCES, &cnt);
		for (GLint a = 0; a < cnt; ++a)
		{
			uint32_t arraylen = 0;
			DataInfo datinfo(binfo);
			glGetProgramResourceName(programID, binfo.type, a, 240, nullptr, name);
			char* chpos = nullptr;
			//printf("@@query %d\t\t%s\n", binfo.iftype, name);
			datinfo.initData(programID, a);
			chpos = strchr(name, '.');
			if (chpos != nullptr)//remove struct
				*chpos = '\0';
			chpos = strchr(name, '[');
			if (chpos != nullptr)//array
			{
				arraylen = atol(chpos + 1) + 1;
				*chpos = '\0';
			}
			auto it = dataMap.find(name);
			if (it != dataMap.end())
			{
				it->second.len = miniBLAS::max(it->second.len, arraylen);
				continue;
			}
			if(datinfo.type != GL_UNIFORM_BLOCK)
				datinfo.location = glGetProgramResourceLocation(programID, datinfo.type, name);
			else
				datinfo.location = glGetProgramResourceIndex(programID, datinfo.type, name);
			if (datinfo.location != GL_INVALID_INDEX)//record index
			{
				dataMap.insert_or_assign(name, datinfo);
				locMap.insert_or_assign(name, datinfo.location);
			}
		}
	}
	oglLog().debug(L"Active {} locations\n", dataMap.size());
	GLuint maxUniLoc = 0;
	for (const auto& di : dataMap)
	{
		const auto& info = di.second;
		if (info.isAttrib())
			attrMap.insert(di);
		else
		{
			maxUniLoc = miniBLAS::max(maxUniLoc, info.location + info.len);
			if (info.isUniformBlock())
				uboMap.insert(di);
			else if (info.isTexture())
				texMap.insert(di);
		}
		oglLog().debug(L"--{:>7}{:<3}  -[{:^5}]-  {}[{}]\n", info.getTypeName(), info.ifidx, info.location, di.first, info.len);
	}
	uniCache.resize(maxUniLoc, static_cast<GLint>(UINT32_MAX));
}

void _oglProgram::addShader(oglShader && shader)
{
	glAttachShader(programID, shader->shaderID);
	shaders.push_back(std::move(shader));
}

void _oglProgram::link()
{
	glLinkProgram(programID);
	int result;
	char logstr[4096] = { 0 };

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(logstr), NULL, logstr);
		glDeleteProgram(programID);
		COMMON_THROW(OGLException, OGLException::GLComponent::Compiler, to_wstring(logstr));
	}
	initLocs();
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

GLint _oglProgram::getLoc(const string& name) const
{
	auto it = locMap.find(name);
	if (it != locMap.end())
		return it->second;
	else //not existed
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

