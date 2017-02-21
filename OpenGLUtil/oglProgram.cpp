#include "oglUtil.h"
#include "privateAPI.h"


namespace oglu
{

_oglProgram::ProgDraw::ProgDraw(const _oglProgram& prog_, const Mat4x4& modelMat) :prog(prog_)
{
	_oglProgram::usethis(prog);
	if (prog.Uni_mvpMat != GL_INVALID_INDEX)
	{
		const auto mvpMat = prog.matrix_Proj * prog.matrix_View * modelMat;
		prog.setMat(prog.Uni_mvpMat, mvpMat);
	}
	prog.setMat(prog.Uni_modelMat, modelMat);
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const GLsizei size, const GLint offset)
{
	glBindVertexArray(vao->vaoID);
	if (vao->index)
	{
		vao->index->bind();
		const uint16_t perEle = (vao->indexType == IndexSize::Byte ? 1 : (vao->indexType == IndexSize::Short ? 2 : 4));
		glDrawElements((GLenum)vao->vaoMode, size, (GLenum)vao->indexType, (void*)(offset*perEle));
		//vao->index->unbind();
	}
	else//draw array
		glDrawArrays((GLenum)vao->vaoMode, offset, size);
	return *this;
}

_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao)
{
	return draw(vao, vao->size, vao->offset);
}



_oglProgram::_oglProgram()
{
	programID = glCreateProgram();
}

_oglProgram::~_oglProgram()
{
	if (programID != GL_INVALID_INDEX)
	{
		glDeleteProgram(programID);
	}
}

oglu::TextureManager& _oglProgram::getTexMan()
{
	static thread_local TextureManager texMan;
	return texMan;
}

bool _oglProgram::usethis(const _oglProgram& prog, const bool change)
{
	static thread_local GLuint curPID = static_cast<GLuint>(-1);
	if (curPID != prog.programID)
	{
		if (!change)//only return status
			return false;
		glUseProgram(curPID = prog.programID);
		auto& texMan = getTexMan();
		for (uint32_t a = 0; a < prog.texs.size(); ++a)
		{
			if (prog.texs[a].tid != UINT32_MAX)
			{
				const auto tupos = texMan.bindTexture(prog, prog.texs[a].tex);
				glProgramUniform1i(prog.programID, prog.Uni_Texture + a, tupos);
			}
		}
	}
	return true;
}

void _oglProgram::setMat(const GLuint pos, const Mat4x4& mat) const
{
	if (pos != GL_INVALID_INDEX)
		glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat.inv());
}

void _oglProgram::addShader(oglShader && shader)
{
	glAttachShader(programID, shader->shaderID);
	shaders.push_back(std::move(shader));
}

OPResult<> _oglProgram::link(const string(&MatrixName)[4], const string(&BasicUniform)[3], const string(&VertAttrName)[4], const uint8_t texcount)
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

	//initialize uniform location
	if (MatrixName[0] != "")//projectMatrix
		Uni_projMat = getUniLoc(MatrixName[0]);
	if (MatrixName[1] != "")//viewMatrix
		Uni_viewMat = getUniLoc(MatrixName[1]);
	if (MatrixName[2] != "")//modelMatrix
		Uni_modelMat = getUniLoc(MatrixName[2]);
	if (MatrixName[3] != "")//model-view-project-Matrix
		Uni_mvpMat = getUniLoc(MatrixName[3]);

	if (BasicUniform[0] != "")//textureUniform
	{
		Uni_Texture = getUniLoc(BasicUniform[0]);
		texs.resize(texcount);
	}

	//initialize vertex attribute location
	if (VertAttrName[0] != "")//Vertex Position
		Attr_Vert_Pos = getAttrLoc(VertAttrName[0]);
	if (VertAttrName[1] != "")//Vertex Normal
		Attr_Vert_Norm = getAttrLoc(VertAttrName[1]);
	if (VertAttrName[2] != "")//Vertex Texture Coordinate
		Attr_Vert_Texc = getAttrLoc(VertAttrName[2]);
	if (VertAttrName[3] != "")//Vertex Color
		Attr_Vert_Color = getAttrLoc(VertAttrName[3]);

	return true;
}

GLuint _oglProgram::getAttrLoc(const string & name)
{
	auto it = attrLocs.find(name);
	if (it != attrLocs.end())
		return it->second;
	//not existed yet
	GLuint loc = glGetAttribLocation(programID, name.c_str());
	attrLocs.insert(make_pair(name, loc));
	return loc;
}

GLuint _oglProgram::getUniLoc(const string & name)
{
	auto it = uniLocs.find(name);
	if (it != uniLocs.end())
		return it->second;
	//not existed yet
	GLuint loc = glGetUniformLocation(programID, name.c_str());
	uniLocs.insert(make_pair(name, loc));
	return loc;
}

void _oglProgram::setProject(const Camera & cam, const int wdWidth, const int wdHeight)
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
	auto& texMan = getTexMan();
	auto& obj = texs[pos];
	if (obj.tid != UINT32_MAX)//has old tex, unbind it
	{
		//unbind operation should always be done, since it may release the true texture and the tID may be recycled later
		texMan.unbindTexture(*this, obj.tex);
		obj = TexPair(tex);
	}
	if (tex)//bind the new tex
	{
		if (usethis(*this, false))//bind it
		{
			const auto tupos = texMan.bindTexture(*this, tex);
			glProgramUniform1i(programID, Uni_Texture + pos, tupos);
		}
		//or, just virtually bind it
		obj = TexPair(tex);
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
			matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, trans.vec.z))) *
				Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, trans.vec.y))) *
				Mat4x4(Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, trans.vec.x))) * matModel;
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