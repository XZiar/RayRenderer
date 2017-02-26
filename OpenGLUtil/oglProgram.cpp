#include "oglProgram.h"
#include "ResDister.hpp"
#include "oglUtil.h"

namespace oglu
{

//----ResourceDistrubutor
class TextureManager : public ResDister<TextureManager, oglTexture>
{
	friend class ResDister<TextureManager, oglTexture>;
protected:
	GLuint getID(const oglTexture& obj) const
	{
		return obj->textureID;
	}
	void innerBind(const oglTexture& obj, const uint8_t pos) const
	{
		obj->bind(pos);
	}
	bool hasRes(const _oglProgram& prog, const GLuint id) const
	{
		for (const auto& t : prog.texs)
			if (t.tid == id)
				return true;
		return false;
	}
public:
	TextureManager() :ResDister((GLenum)GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) { }
	using ResDister::bind;
	using ResDister::unbind;
};

class UBOManager : public ResDister<UBOManager, oglBuffer>
{
	friend class ResDister<UBOManager, oglBuffer>;
protected:
	GLuint getID(const oglBuffer& obj) const
	{
		return obj->bufferID;
	}
	void innerBind(const oglBuffer& obj, const uint8_t pos) const
	{
		//obj->bind(pos);
	}
	bool hasRes(const _oglProgram& prog, const GLuint id) const
	{
		for (const auto& b : prog.ubos)
			if (b.bid == id)
				return true;
		return false;
	}
public:
	UBOManager() :ResDister((GLenum)GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) { }
	using ResDister::bind;
	using ResDister::unbind;
};
//ResourceDistrubutor----


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

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const uint32_t size, const uint32_t offset)
{
	vao->bind();
	if (vao->index)
		drawIndex(vao, size, (void*)(offset * vao->indexSizeof));
	else
		drawArray(vao, size, offset);
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao)
{
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
	glGetProgramResourceiv(pid, (GLenum)iftype, ifidx, 1, &prop, 1, NULL, &ret);
	return ret;
}

void _oglProgram::DataInfo::initData(const GLuint pid, const GLint idx)
{
	ifidx = (uint8_t)idx;
	if (iftype != DataInfo::IFType::UniformBlock)
	{
		len = getValue(pid, GL_ARRAY_SIZE);
	}
}


_oglProgram::_oglProgram()
{
	programID = glCreateProgram();
}

_oglProgram::~_oglProgram()
{
	if (programID != GL_INVALID_INDEX)
	{
		auto& texMan = getTexMan();
		for (const auto& tex : texs)
			if (tex.tid != UINT32_MAX)
				texMan.unbind(*this, tex.tex);
		bool shouldUnUse = usethis(*this, false);
		glDeleteProgram(programID);
		programID = GL_INVALID_INDEX;
		if(shouldUnUse)
			usethis(*this, true);
	}
}

oglu::TextureManager& _oglProgram::getTexMan()
{
	static thread_local TextureManager texMan;
	return texMan;
}

oglu::UBOManager& _oglProgram::getUBOMan()
{
	static thread_local UBOManager uboMan;
	return uboMan;
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
			prog.recoverTexMap();
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
	dataMap.clear();
	GLchar name[256];
	DataInfo baseinfo[] = { DataInfo::IFType::UniformBlock,DataInfo::IFType::Uniform,DataInfo::IFType::Attrib };
	for(const DataInfo& binfo : baseinfo)
	{
		GLint cnt = 0;
		glGetProgramInterfaceiv(programID, (GLenum)binfo.iftype, GL_ACTIVE_RESOURCES, &cnt);
		for (GLint a = 0; a < cnt; ++a)
		{
			int arraylen = 0;
			DataInfo datinfo(binfo);
			glGetProgramResourceName(programID, (GLenum)binfo.iftype, a, 240, nullptr, name);
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
			datinfo.location = glGetProgramResourceIndex(programID, (GLenum)datinfo.iftype, name);
			if (datinfo.location != GL_INVALID_INDEX)//record index
			{
				dataMap.insert_or_assign(name, datinfo);
			}
		}
	}
#ifdef _DEBUG
	for (const auto& di : dataMap)
		printf("@@@@%7s%2d:  [%2d][%6s%3d]\t%s\n", di.second.getTypeName(), di.second.ifidx, di.second.location,
			di.second.isArray() ? "Array-" : "Single", di.second.len, di.first.c_str());
#endif
}

void _oglProgram::recoverTexMap()
{
	if (Uni_Texture == GL_INVALID_INDEX)
		return;
	auto& texMan = getTexMan();
	uint32_t a = 0;
	for (const auto& tex : texs)
	{
		if(tex.tid != UINT32_MAX)
			texvals[a] = texMan.bind(*this, tex.tex);
		a++;
	}
	glProgramUniform1iv(programID, Uni_Texture, (GLsizei)texvals.size(), texvals.data());
}

void _oglProgram::addShader(oglShader && shader)
{
	glAttachShader(programID, shader->shaderID);
	shaders.push_back(std::move(shader));
}

OPResult<> _oglProgram::link(const string(&MatrixName)[4], const string(&BasicUniform)[3], const string(&VertAttrName)[4])
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
	Uni_Texture = getLoc(BasicUniform[0]);//textureUniform
	texs.clear();
	if (Uni_Texture != GL_INVALID_INDEX)
	{
		const auto size = dataMap.find(BasicUniform[0])->second.len;
		texs.resize(size);
		texvals.resize(size);
	}

	//initialize vertex attribute location
	Attr_Vert_Pos = getLoc(VertAttrName[0]);//Vertex Position
	Attr_Vert_Norm = getLoc(VertAttrName[1]);//Vertex Normal
	Attr_Vert_Texc = getLoc(VertAttrName[2]);//Vertex Texture Coordinate
	Attr_Vert_Color = getLoc(VertAttrName[3]);//Vertex Color

	return true;
}

GLint _oglProgram::getLoc(const string& name) const
{
	auto it = dataMap.find(name);
	if (it != dataMap.end())
		return it->second.location;
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

void _oglProgram::setTexture(const oglTexture& tex, const uint8_t pos)
{
	if (Uni_Texture == GL_INVALID_INDEX || pos >= texvals.size())
		return;
	auto& texMan = getTexMan();
	auto& obj = texs[pos];
	if (obj.tid != UINT32_MAX)//has old tex, unbind it
	{//unbind operation should always be done, since it may release the true texture and the tID may be recycled later
		texMan.unbind(*this, obj.tex);
	}
	//update texture-hold map
	obj = TexPair(tex);
	if (tex && usethis(*this, false))//need to bind the new tex
	{//already in use, then really change value
		glProgramUniform1i(programID, Uni_Texture + pos, texvals[pos] = texMan.bind(*this, tex));
	}
}

void _oglProgram::setUBO(const oglBuffer& ubo, const uint8_t pos)
{
	if (Uni_Texture == GL_INVALID_INDEX || pos >= ubovals.size())
		return;
	auto& uboMan = getUBOMan();
	auto& obj = ubos[pos];
	if (obj.bid != UINT32_MAX)//has old tex, unbind it
	{//unbind operation should always be done, since it may release the true ubo and the bID may be recycled later
		uboMan.unbind(*this, obj.ubo);
	}
	//update texture-hold map
	obj = UBOPair(ubo);
	if (ubo && usethis(*this, false))//need to bind the new ubo
	{//already in use, then really change value
		//glBindBufferBase();
		//glUniformBlockBinding(programID)
		//glProgramUniform1i(programID, Uni_Texture + pos, texvals[pos] = texMan.bind(*this, tex));
	}
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