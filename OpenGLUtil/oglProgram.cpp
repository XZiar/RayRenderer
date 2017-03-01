#include "oglProgram.h"
#include "BindingManager.h"

namespace oglu
{


_oglProgram::ProgDraw::ProgDraw(_oglProgram& prog_, const Mat4x4& modelMat) :prog(prog_)
{
	_oglProgram::usethis(prog);
	if (prog.Uni_mvpMat != GL_INVALID_INDEX)
	{
		const auto mvpMat = prog.matrix_Proj * prog.matrix_View * modelMat;
		prog.setMat(prog.Uni_mvpMat, mvpMat);
	}
	prog.setMat(prog.Uni_modelMat, modelMat);
}

void _oglProgram::ProgDraw::setTexture(const GLint pos, const oglTexture& tex)
{
	auto& obj = prog.uniCache[pos];
	const auto val = tex ? _oglTexture::getTexMan().bind(tex) : 0;
	if (obj == val)//no change
		return;
	//change value and update uniform-hold map
	glProgramUniform1i(prog.programID, pos, obj = val);
}

void _oglProgram::ProgDraw::setTexture()
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
	texCache.clear();
}

void _oglProgram::ProgDraw::setUBO(const GLint pos, const oglBuffer& ubo)
{
	auto& obj = prog.uniCache[pos];
	const auto val = ubo ? _oglBuffer::getUBOMan().bind(ubo) : 0;
	if (obj == val)//no change
		return;
	//change value and update uniform-hold map
	glUniformBlockBinding(prog.programID, pos, obj = val);
}

void _oglProgram::ProgDraw::setUBO()
{
	switch (uboCache.size())
	{
	case 0:
		return;
	case 1:
		setUBO(uboCache.begin()->first, uboCache.begin()->second);
		break;
	default:
		_oglBuffer::getUBOMan().bindAll(prog.programID, uboCache, prog.uniCache);
		break;
	}
	uboCache.clear();
}

void _oglProgram::ProgDraw::drawIndex(const oglVAO& vao, const GLsizei size, const void *offset)
{
	glDrawElements((GLenum)vao->vaoMode, size, (GLenum)vao->indexType, offset);
}

void _oglProgram::ProgDraw::drawIndexs(const oglVAO& vao, const GLsizei count, const GLsizei *size, const void * const *offset)
{
	glMultiDrawElements((GLenum)vao->vaoMode, size, (GLenum)vao->indexType, offset, count);
}

void _oglProgram::ProgDraw::drawArray(const oglVAO& vao, const GLsizei size, const GLint offset)
{
	glDrawArrays((GLenum)vao->vaoMode, offset, size);
}

void _oglProgram::ProgDraw::drawArrays(const oglVAO& vao, const GLsizei count, const GLsizei *size, const GLint *offset)
{
	glMultiDrawArrays((GLenum)vao->vaoMode, offset, size, count);
}


_oglProgram::ProgDraw& _oglProgram::ProgDraw::setTexture(const oglTexture& tex, const string& name, const GLuint idx, const bool immediate)
{
	const auto it = prog.texMap.find(name);
	if (it != prog.texMap.end() && idx < it->second.len)//legal
	{
		const auto pos = it->second.location + idx;
		if (immediate)
			setTexture(pos, tex);
		else
			texCache.insert_or_assign(pos, tex);
	}
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::setTexture(const oglTexture& tex, const GLuint pos, const bool immediate)
{
	if (pos < prog.uniCache.size())
	{
		if (immediate)
			setTexture(pos, tex);
		else
			texCache.insert_or_assign(pos, tex);
	}
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::setUBO(const oglBuffer& ubo, const string& name, const GLuint idx, const bool immediate)
{
	const auto it = prog.uboMap.find(name);
	if (it != prog.uboMap.end() && idx < it->second.len)//legal
	{
		const auto pos = it->second.location + idx;
		if (immediate)
			setUBO(pos, ubo);
		else
			uboCache.insert_or_assign(pos, ubo);
	}
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::setUBO(const oglBuffer& ubo, const GLuint pos, const bool immediate)
{
	if (pos < prog.uniCache.size())
	{
		if (immediate)
			setUBO(pos, ubo);
		else
			uboCache.insert_or_assign(pos, ubo);
	}
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
	setTexture();
	setUBO();
	vao->bind();
	if (vao->index)
		drawIndex(vao, size, (void*)(offset * vao->indexSizeof));
	else
		drawArray(vao, size, offset);
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao)
{
	setTexture();
	setUBO();
	vao->bind();
	switch (vao->drawMethod)
	{
	case _oglVAO::DrawMethod::Array:
		drawArray(vao, vao->sizes[0], vao->ioffsets[0]); break;
	case _oglVAO::DrawMethod::Index:
		drawIndex(vao, vao->sizes[0], vao->poffsets[0]); break;
	case _oglVAO::DrawMethod::Arrays:
		drawArrays(vao, (GLsizei)vao->sizes.size(), vao->sizes.data(), vao->ioffsets.data()); break;
	case _oglVAO::DrawMethod::Indexs:
		drawIndexs(vao, (GLsizei)vao->sizes.size(), vao->sizes.data(), vao->poffsets.data()); break;
	}
	return *this;
}


GLint _oglProgram::DataInfo::getValue(const GLuint pid, const GLenum prop)
{
	GLint ret;
	glGetProgramResourceiv(pid, type, ifidx, 1, &prop, 1, NULL, &ret);
	return ret;
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


_oglProgram::_oglProgram()
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
			int arraylen = 0;
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
			datinfo.location = glGetProgramResourceIndex(programID, datinfo.type, name);
			if (datinfo.location != GL_INVALID_INDEX)//record index
			{
				dataMap.insert_or_assign(name, datinfo);
				locMap.insert_or_assign(name, datinfo.location);
			}
		}
	}
	GLint maxUniLoc = 0;
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
	#ifdef _DEBUG
		printf("@@@@%7s%2d:  [%2d][%3dele]\t%s\n", info.getTypeName(), info.ifidx, info.location,
			info.len, di.first.c_str());
	#endif
	}
	uniCache.resize(maxUniLoc, static_cast<GLint>(UINT32_MAX));
}

void _oglProgram::addShader(oglShader && shader)
{
	glAttachShader(programID, shader->shaderID);
	shaders.push_back(std::move(shader));
}

OPResult<> _oglProgram::link(const string(&MatrixName)[4], const string(&VertAttrName)[4])
{
	glLinkProgram(programID);

	int result;
	char logstr[4096] = { 0 };

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(logstr), NULL, logstr);
		glDeleteProgram(programID);
		return OPResult<>(false, logstr);
	}

	initLocs();

	//initialize uniform location
	Uni_projMat = getLoc(MatrixName[0]);//projectMatrix
	Uni_viewMat = getLoc(MatrixName[1]);//viewMatrix
	Uni_modelMat = getLoc(MatrixName[2]);//modelMatrix
	Uni_mvpMat = getLoc(MatrixName[3]);//model-view-project-Matrix

	//initialize vertex attribute location
	for (auto a = 0; a < 4; ++a)
	{
		auto it = attrMap.find(VertAttrName[a]);
		if (it != attrMap.end())
			(&Attr_Vert_Pos)[a] = it->second.location;
	}
	Attr_Vert_Pos = getLoc(VertAttrName[0]);
	Attr_Vert_Norm = getLoc(VertAttrName[1]);
	Attr_Vert_Texc = getLoc(VertAttrName[2]);
	Attr_Vert_Color = getLoc(VertAttrName[3]);

	return true;
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

_oglProgram::ProgDraw _oglProgram::draw(const Mat4x4& modelMat)
{
	return ProgDraw(*this, modelMat);
}

_oglProgram::ProgDraw _oglProgram::draw(topIT begin, topIT end)
{
	Mat4x4 matModel = Mat4x4::identity();
	for (topIT cur = begin; cur != end; ++cur)
	{
		const TransformOP& trans = *cur;
		switch (trans.type)
		{
		case TransformType::RotateXYZ:
			matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, trans.vec.z)) *
				Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, trans.vec.y)) *
				Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, trans.vec.x))) * matModel;
			break;
		case TransformType::Rotate:
			matModel = Mat4x4(Mat3x3::RotateMat(trans.vec)) * matModel;
			break;
		case TransformType::Translate:
			matModel = Mat4x4::TranslateMat(trans.vec) * matModel;
			break;
		case TransformType::Scale:
			matModel = Mat4x4(Mat3x3::ScaleMat(trans.vec)) * matModel;
			break;
		}
	}
	return ProgDraw(*this, matModel);
}

}