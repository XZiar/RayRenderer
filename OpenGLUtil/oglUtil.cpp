#include "oglUtil.h"
#include "privateAPI.h"


namespace oglu
{
using std::forward;

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
	glGenBuffers(1, &bID);
}

_oglBuffer::~_oglBuffer()
{
	glDeleteBuffers(1, &bID);
}

void _oglBuffer::write(const void * dat, const size_t size, const DrawMode mode)
{
	glBindBuffer((GLenum)bufferType, bID);
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
	glBindTexture((GLenum)type, tID);
}

void _oglTexture::unbind() const
{
	glBindTexture((GLenum)type, 0);
}

_oglTexture::_oglTexture(const TextureType _type) : type(_type)
{
	glGenTextures(1, &tID);
}

_oglTexture::~_oglTexture()
{
	glDeleteTextures(1, &tID);
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
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf->bID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, NULL);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//unbind();
}


_oglVAO::VAOPrep& _oglVAO::VAOPrep::set(const oglBuffer& vbo, const GLuint attridx, const uint16_t stride, const uint8_t size, const GLint offset)
{
	glBindBuffer((GLenum)vbo->bufferType, vbo->bID);
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


_oglProgram::ProgDraw::ProgDraw(const _oglProgram& prog_, const Mat4x4& modelMat, const Mat4x4& normMat) :prog(prog_)
{
	_oglProgram::usethis(prog);
	prog.setMat(prog.Uni_modelMat, modelMat);
	prog.setMat(prog.Uni_normMat, normMat);
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

OPResult<> _oglProgram::link(const string(&MatrixName)[4], const string(&BasicUniform)[3], const uint8_t texcount)
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
	if (MatrixName[3] != "")//normalMatrix

	Uni_normMat = getUniLoc(MatrixName[3]);
	if (BasicUniform[0] != "")//textureUniform
	{
		Uni_Texture = getUniLoc(BasicUniform[0]);
		texs.resize(texcount);
	}

	return true;
}

GLint _oglProgram::getUniLoc(const string & name)
{
	auto it = uniLocs.find(name);
	if (it != uniLocs.end())
		return it->second;
	//not existed yet
	GLint loc = glGetUniformLocation(programID, name.c_str());
	uniLocs.insert(make_pair(name, loc));
	return loc;
}

void _oglProgram::setProject(const Camera & cam, const int wdWidth, const int wdHeight)
{
	glViewport((wdWidth & 0x3f) / 2, (wdHeight & 0x3f) / 2, cam.width & 0xffc0, cam.height & 0xffc0);

	//mat4 projMat = glm::frustum(-RProjZ, +RProjZ, -Aspect*RProjZ, +Aspect*RProjZ, 1.0, 32768.0);

	assert(cam.aspect > std::numeric_limits<float>::epsilon());
	const float tanHalfFovy = tan(b3d::ang2rad(cam.fovy / 2));
	const float viewDepthN = cam.zNear - cam.zFar;

	matrix_Proj = Mat4x4(Vec4(1 / (cam.aspect*tanHalfFovy), 0.f, 0.f, 0.f),
		Vec4(0.f, 1 / tanHalfFovy, 0.f, 0.f),
		Vec4(0.f, 0.f, (cam.zFar + cam.zNear) / viewDepthN, (2 * cam.zFar * cam.zNear) / viewDepthN),
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

void _oglProgram::setLight(const uint8_t id, const Light & light)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ID_lgtVBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 96 * id, 96, &light);
}

void _oglProgram::setMaterial(const Material & mt)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ID_mtVBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 80, &mt);
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

_oglProgram::ProgDraw _oglProgram::draw(const Mat4x4& modelMat, const Mat4x4& normMat)
{
	return ProgDraw(*this, modelMat, normMat);
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
