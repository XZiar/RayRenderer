#include "oglUtil.h"
#include "privateAPI.h"


namespace oglu
{

string _oglShader::loadFromFile(FILE * fp)
{
	fseek(fp, 0, SEEK_END);
	const auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *dat = new char[fsize + 1];
	fread(dat, fsize, 1, fp);
	dat[fsize] = '\0';
	string txt(dat);
	delete[] dat;

	return txt;
}

_oglShader::_oglShader(const ShaderType type, const string & txt) : shaderType(type), src(txt)
{
	auto ptr = txt.c_str();
	shaderID = glCreateShader(GLenum(type));
	glShaderSource(shaderID, 1, &ptr, NULL);
}

_oglShader::~_oglShader()
{
	if (shaderID != GL_INVALID_INDEX)
		glDeleteShader(shaderID);
}

OPResult<> _oglShader::compile()
{
	glCompileShader(shaderID);

	GLint result;
	char logstr[4096] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(logstr), NULL, logstr);
		return OPResult<>(false, logstr);
	}
	return true;
}


_oglBuffer::_oglBuffer(const BufferType _type) :bufferType(_type)
{
	glGenBuffers(1, &bufferID);
}

_oglBuffer::~_oglBuffer()
{
	glDeleteBuffers(1, &bufferID);
}

void _oglBuffer::write(const void * dat, const size_t size, const DrawMode mode)
{
	glBindBuffer((GLenum)bufferType, bufferID);
	glBufferData((GLenum)bufferType, size, dat, (GLenum)mode);
	glBindBuffer((GLenum)bufferType, 0);
}


void _oglTexture::parseFormat(const TextureFormat format, GLint & intertype, GLenum & datatype, GLenum & comptype)
{
	switch (format)
	{
	case TextureFormat::RGB:
		intertype = GL_RGB;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGB;
		break;
	case TextureFormat::RGBA:
		intertype = GL_RGBA;
		datatype = GL_UNSIGNED_BYTE;
		comptype = GL_RGBA;
		break;
	case TextureFormat::RGBf:
		intertype = GL_RGB32F;
		datatype = GL_FLOAT;
		comptype = GL_RGB;
		break;
	case TextureFormat::RGBAf:
		intertype = GL_RGBA32F;
		datatype = GL_FLOAT;
		comptype = GL_RGBA;
		break;
	}
}

void _oglTexture::bind(const uint16_t pos) const
{
	glActiveTexture(GL_TEXTURE0 + pos);
	glBindTexture((GLenum)type, textureID);
}

void _oglTexture::unbind() const
{
	glBindTexture((GLenum)type, 0);
}

_oglTexture::_oglTexture(const TextureType _type) : type(_type)
{
	glGenTextures(1, &textureID);
}

_oglTexture::~_oglTexture()
{
	glDeleteTextures(1, &textureID);
}

void _oglTexture::setProperty(const TextureFilterVal filtertype, const TextureWrapVal wraptype)
{
	bind();
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_S, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_WRAP_T, (GLint)wraptype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MAG_FILTER, (GLint)filtertype);
	glTexParameteri((GLenum)type, GL_TEXTURE_MIN_FILTER, (GLint)filtertype);
	unbind();
}

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void * data)
{
	bind();
	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, data);
	//unbind();
}

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	bind();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf->bufferID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, NULL);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//unbind();
}


_oglVAO::VAOPrep& _oglVAO::VAOPrep::set(const oglBuffer& vbo, const GLuint attridx, const uint16_t stride, const uint8_t size, const GLint offset)
{
	glBindBuffer((GLenum)vbo->bufferType, vbo->bufferID);
	glEnableVertexAttribArray(attridx);//vertex attr index
	glVertexAttribPointer(attridx, size, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	return *this;
}

_oglVAO::_oglVAO(const VAODrawMode _mode) :vaoMode(_mode)
{
	glGenVertexArrays(1, &vaoID);
}

_oglVAO::~_oglVAO()
{
	glDeleteVertexArrays(1, &vaoID);
}


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
	glDrawArrays((GLenum)vao->vaoMode, offset, size);
	return *this;
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
	const float viewDepthNR = 1 / (cam.zNear - cam.zFar);
	/*   cot(fovy/2)
	 *  -------------		0			0			0
	 *     aspect
	 *     
	 *       0         cot(fovy/2)		0			0
	 *       
	 *								   F+N         2*F*N
	 *       0              0		- -----		- -------
	 *								   F-N          F-N
	 *								   
	 *       0              0          -1			0
	 *  
	 **/
	matrix_Proj = Mat4x4(Vec4(cotHalfFovy / cam.aspect, 0.f, 0.f, 0.f),
		Vec4(0.f, cotHalfFovy, 0.f, 0.f),
		Vec4(0.f, 0.f, (cam.zFar + cam.zNear) * viewDepthNR, (2 * cam.zFar * cam.zNear) * viewDepthNR),
		Vec4(0.f, 0.f, -1.f, 0.f));

	setMat(Uni_projMat, matrix_Proj);
}

void _oglProgram::setCamera(const Camera & cam)
{
	//LookAt
	//matrix_View = glm::lookAt(vec3(cam.position), vec3(Vertex(cam.position + cam.n)), vec3(cam.v));
	const auto rMat = Mat3x3(cam.u, cam.v, cam.n * -1).inved();
	matrix_View = Mat4x4(rMat) * Mat4x4::TranslateMat(cam.position * -1);

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


oglu::_oglProgram::ProgDraw _oglProgram::draw(topIT begin, topIT end)
{
	Mat4x4 matModel = Mat4x4::identity();
	for (topIT cur = begin; cur != end; ++cur)
	{
		const TransformOP& trans = *cur;
		switch (trans.type)
		{
		case TransformType::RotateXYZ:
			matModel =  Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, trans.vec.z))) * 
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

void oglUtil::init()
{
	glewInit();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

OPResult<GLenum> oglUtil::getError()
{
	const auto err = glGetError();
	switch (err)
	{
	case GL_NO_ERROR:
		return OPResult<GLenum>(true, err, "GL_NO_ERROR");
	case GL_INVALID_ENUM:
		return OPResult<GLenum>(false, err, "GL_INVALID_ENUM");
	case GL_INVALID_VALUE:
		return OPResult<GLenum>(false, err, "GL_INVALID_VALUE");
	case GL_INVALID_OPERATION:
		return OPResult<GLenum>(false, err, "GL_INVALID_OPERATION");
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return OPResult<GLenum>(false, err, "GL_INVALID_FRAMEBUFFER_OPERATION");
	case GL_OUT_OF_MEMORY:
		return OPResult<GLenum>(false, err, "GL_OUT_OF_MEMORY");
	case GL_STACK_UNDERFLOW:
		return OPResult<GLenum>(false, err, "GL_STACK_UNDERFLOW");
	case GL_STACK_OVERFLOW:
		return OPResult<GLenum>(false, err, "GL_STACK_OVERFLOW");
	default:
		return OPResult<GLenum>(false, err, "undefined error with code " + std::to_string(err));
	}
}

OPResult<string> oglUtil::loadShader(oglProgram& prog, const string& fname)
{
	{
		FILE *fp = nullptr;
		string fn = fname + ".vert";
		fopen_s(&fp, fn.c_str(), "rb");
		if(fp == nullptr)
			return OPResult<string>(false, fn, "ERROR on Vertex Shader Compiler");
		oglShader vert(ShaderType::Vertex, fp);
		auto ret = vert->compile();
		if (ret)
			prog->addShader(std::move(vert));
		else
		{
			fclose(fp);
			return OPResult<string>(false, ret.msg, "ERROR on Vertex Shader Compiler");
		}
		fclose(fp);
	}
	{
		FILE *fp = nullptr;
		string fn = fname + ".frag";
		fopen_s(&fp, fn.c_str(), "rb");
		if (fp == nullptr)
			return OPResult<string>(false, fn, "ERROR on Vertex Shader Compiler");
		oglShader frag(ShaderType::Fragment, fp);
		auto ret = frag->compile();
		if (ret)
			prog->addShader(std::move(frag));
		else
		{
			fclose(fp);
			return OPResult<string>(false, ret.msg, "ERROR on Fragment Shader Compiler");
		}
		fclose(fp);
	}
	return true;
}



}
