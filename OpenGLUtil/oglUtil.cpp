#include "oglUtil.h"


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
	if (shaderID)
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

_oglTexture::_oglTexture(const TextureType _type) : type(_type)
{
	glGenTextures(1, &tID);
}

_oglTexture::~_oglTexture()
{
	glDeleteTextures(1, &tID);
}

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const void * data)
{
	glBindTexture((GLenum)type, tID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, data);
	//glBindTexture((GLenum)type, 0);
}

void _oglTexture::setData(const TextureFormat format, const GLsizei w, const GLsizei h, const oglBuffer& buf)
{
	glBindTexture((GLenum)type, tID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf->bID);

	GLint intertype;
	GLenum datatype, comptype;
	parseFormat(format, intertype, datatype, comptype);
	glTexImage2D((GLenum)type, 0, intertype, w, h, 0, comptype, datatype, NULL);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//glBindTexture((GLenum)type, 0);
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


_oglProgram::_oglProgram()
{
	static _Init init;
	programID = glCreateProgram();
	glGenBuffers(1, &ID_lgtVBO);
	glGenBuffers(1, &ID_mtVBO);
}

_oglProgram::~_oglProgram()
{
	if (programID)
	{
		glDeleteProgram(programID);
	}
}

void _oglProgram::usethis(const GLuint programID)
{
	static thread_local GLuint curPID = static_cast<GLuint>(-1);
	if (curPID != programID)
	{
		glUseProgram(curPID = programID);
	}
}

void _oglProgram::setMat(const GLuint pos, const Mat4x4& mat) const
{
	if (pos != GL_INVALID_INDEX)
		glProgramUniformMatrix4fv(programID, pos, 1, GL_FALSE, mat);
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
		IDX_projMat = getUniLoc(MatrixName[0]);
	if (MatrixName[1] != "")//viewMatrix
		IDX_viewMat = getUniLoc(MatrixName[1]);
	if (MatrixName[2] != "")//modelMatrix
		IDX_modelMat = getUniLoc(MatrixName[2]);
	if (MatrixName[3] != "")//normalMatrix

	IDX_normMat = getUniLoc(MatrixName[3]);
	if (BasicUniform[0] != "")//textureUniform
	{
		IDX_Uni_Texture = getUniLoc(BasicUniform[0]);
		texs.resize(texcount);
		for (uint8_t a = 0; a < texcount; ++a)
			glProgramUniform1i(programID, IDX_Uni_Texture + a, a);
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
	const float tanHalfFovy = tan(cam.fovy / 2);

	matrix_Proj = Mat4x4::zero();
	matrix_Proj(0, 0) = 1 / (cam.aspect*tanHalfFovy);
	matrix_Proj(1, 1) = 1 / tanHalfFovy;
	matrix_Proj(2, 3) = 1;
	matrix_Proj(2, 2) = -(cam.zFar + cam.zNear) / (cam.zFar - cam.zNear);
	matrix_Proj(3, 2) = -(2 * cam.zFar * cam.zNear) / (cam.zFar - cam.zNear);

	setMat(IDX_projMat, matrix_Proj);
}

void _oglProgram::setCamera(const Camera & cam)
{
	//LookAt
	//matrix_View = glm::lookAt(vec3(cam.position), vec3(Vertex(cam.position + cam.n)), vec3(cam.v));
	matrix_View = Mat4x4(
		Vec4(cam.u, -(cam.u.dot(cam.position))),
		Vec4(cam.v, -(cam.v.dot(cam.position))),
		Vec4(cam.n*-1, cam.n.dot(cam.position)),
		Vec4(1, 1, 1, 1)).inved();

	setMat(IDX_viewMat, matrix_View);
	if (IDX_camPos != GL_INVALID_INDEX)
		glProgramUniform3fv(programID, IDX_camPos, 1, cam.position);
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
	if (tex)
	{
		texs[pos] = tex;
		glActiveTexture(GL_TEXTURE0 + pos);
		glBindTexture((GLenum)tex->type, tex->tID);
	}
	else
		texs[pos].release();
}

_oglProgram::ProgDraw _oglProgram::draw(const Mat4x4& modelMat, const Mat4x4& normMat)
{
	return ProgDraw(*this, modelMat, normMat);
}


_oglProgram::ProgDraw& _oglProgram::ProgDraw::draw(const oglVAO& vao, const GLsizei size, const GLint offset)
{
	glBindVertexArray(vao->vaoID);
	glDrawArrays((GLenum)vao->vaoMode, offset, size);
	return *this;
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
