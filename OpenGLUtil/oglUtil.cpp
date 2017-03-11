#include "oglUtil.h"

namespace oglu
{

boost::circular_buffer<std::shared_ptr<oglu::DebugMessage>> oglUtil::msglist(512);
boost::circular_buffer<std::shared_ptr<oglu::DebugMessage>> oglUtil::errlist(128);

void GLAPIENTRY oglUtil::onMsg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	DebugMessage msg(source, type, severity);
	const DBGLimit& limit = *(DBGLimit*)userParam;
	if (((limit.src & (uint8_t)msg.from) != 0) 
		&& ((limit.type & (uint16_t)msg.type) != 0) 
		&& limit.minLV <= (uint8_t)msg.level)
	{
		auto theMsg = std::make_shared<DebugMessage>(msg);
		theMsg->msg.assign(message, message + length);
		if (theMsg->type == MsgType::Error)
			errlist.push_back(theMsg);
		msglist.push_back(theMsg);
	#ifdef _DEBUG
		if (msg.from == MsgSrc::OpenGL)
			printf("@@OpenGL API Message:\n%ls\n", theMsg->msg.c_str());
	#endif
	}
}

void oglUtil::init()
{
	glewInit();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#ifdef _DEBUG
	setDebug(0x2f, 0x2f, MsgLevel::Notfication);
#endif
}

void oglUtil::setDebug(uint8_t src, uint16_t type, MsgLevel minLV)
{
	static DBGLimit limit;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	limit = { type, src, (uint8_t)minLV };
	glDebugMessageCallback(oglUtil::onMsg, &limit);
}

string oglUtil::getVersion()
{
	return string((const char*)glGetString(GL_VERSION));
}

OPResult<GLenum> oglUtil::getError()
{
	const auto err = glGetError();
	switch (err)
	{
	case GL_NO_ERROR:
		return OPResult<GLenum>(true, L"GL_NO_ERROR", err);
	case GL_INVALID_ENUM:
		return OPResult<GLenum>(false, L"GL_INVALID_ENUM", err);
	case GL_INVALID_VALUE:
		return OPResult<GLenum>(false, L"GL_INVALID_VALUE", err);
	case GL_INVALID_OPERATION:
		return OPResult<GLenum>(false, L"GL_INVALID_OPERATION", err);
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return OPResult<GLenum>(false, L"GL_INVALID_FRAMEBUFFER_OPERATION", err);
	case GL_OUT_OF_MEMORY:
		return OPResult<GLenum>(false, L"GL_OUT_OF_MEMORY", err);
	case GL_STACK_UNDERFLOW:
		return OPResult<GLenum>(false, L"GL_STACK_UNDERFLOW", err);
	case GL_STACK_OVERFLOW:
		return OPResult<GLenum>(false, L"GL_STACK_OVERFLOW", err);
	default:
		return OPResult<GLenum>(false, L"undefined error with code " + std::to_wstring(err), err);
	}
}

OPResult<wstring> oglUtil::loadShader(oglProgram& prog, const wstring& fname)
{
	{
		FILE *fp = nullptr;
		wstring fn = fname + L".vert";
		_wfopen_s(&fp, fn.c_str(), L"rb");
		if (fp == nullptr)
			return OPResult<wstring>(false, L"ERROR on Vertex Shader Compiler", fn);
		oglShader vert(ShaderType::Vertex, fp);
		auto ret = vert->compile();
		if (ret)
			prog->addShader(std::move(vert));
		else
		{
			fclose(fp);
			return OPResult<wstring>(false, L"ERROR on Vertex Shader Compiler", ret.msg);
		}
		fclose(fp);
	}
	{
		FILE *fp = nullptr;
		wstring fn = fname + L".frag";
		_wfopen_s(&fp, fn.c_str(), L"rb");
		if (fp == nullptr)
			return OPResult<wstring>(false, L"ERROR on Vertex Shader Compiler", fn);
		oglShader frag(ShaderType::Fragment, fp);
		auto ret = frag->compile();
		if (ret)
			prog->addShader(std::move(frag));
		else
		{
			fclose(fp);
			return OPResult<wstring>(false, L"ERROR on Fragment Shader Compiler", ret.msg);
		}
		fclose(fp);
	}
	return true;
}

void oglUtil::applyTransform(Mat4x4& matModel, const TransformOP& op)
{
	switch (op.type)
	{
	case TransformType::RotateXYZ:
		matModel = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
			Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
			Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x))) * matModel;
		return;
	case TransformType::Rotate:
		matModel = Mat4x4(Mat3x3::RotateMat(op.vec)) * matModel;
		return;
	case TransformType::Translate:
		matModel = Mat4x4::TranslateMat(op.vec) * matModel;
		return;
	case TransformType::Scale:
		matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
		return;
	}
}

void oglUtil::applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op)
{
	switch (op.type)
	{
	case TransformType::RotateXYZ:
		{
			const auto rMat = Mat4x4(Mat3x3::RotateMat(Vec4(0.0f, 0.0f, 1.0f, op.vec.z)) *
				Mat3x3::RotateMat(Vec4(0.0f, 1.0f, 0.0f, op.vec.y)) *
				Mat3x3::RotateMat(Vec4(1.0f, 0.0f, 0.0f, op.vec.x)));
			matModel = rMat * matModel;
			matNormal = rMat * matNormal;
		}return;
	case TransformType::Rotate:
		{
			const auto rMat = Mat4x4(Mat3x3::RotateMat(op.vec));
			matModel = rMat * matModel;
			matNormal = rMat * matNormal;
		}return;
	case TransformType::Translate:
		{
			matModel = Mat4x4::TranslateMat(op.vec) * matModel;
		}return;
	case TransformType::Scale:
		{
			matModel = Mat4x4(Mat3x3::ScaleMat(op.vec)) * matModel;
		}return;
	}
}

}