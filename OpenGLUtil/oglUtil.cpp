#include "oglUtil.h"
#include "MTWorker.hpp"
#include "oglInternal.h"
#include "../common/PromiseTask.hpp"
#include <GL/wglew.h>

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
		
		msglist.push_back(theMsg);
		if (theMsg->type == MsgType::Error)
		{
			errlist.push_back(theMsg);
			oglLog().error(L"OpenGL ERROR\n{}\n", theMsg->msg);
		}
		else
		{
			oglLog().debug(L"OpenGL message\n{}\n", theMsg->msg);
		}
	}
}

detail::MTWorker& oglUtil::getWorker(const uint8_t idx)
{
	static detail::MTWorker syncGL(L"SYNC"), asyncGL(L"ASYNC");
	return idx == 0 ? syncGL : asyncGL;
}

void oglUtil::init()
{
	glewInit();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#if defined(_DEBUG) || 1
	setDebug(0x2f, 0x2f, MsgLevel::Notfication);
#endif
	oglLog().info(L"GL Version:{}\n", getVersion());
	auto hdc = wglGetCurrentDC();
	auto hrc = wglGetCurrentContext();
	int ctxAttrb[] =
	{
		/*WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,*/
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
		0
	};
	getWorker(0).start(hdc, wglCreateContextAttribsARB(hdc, hrc, ctxAttrb));
	getWorker(1).start(hdc, wglCreateContextAttribsARB(hdc, NULL, ctxAttrb));
}

void oglUtil::setDebug(uint8_t src, uint16_t type, MsgLevel minLV)
{
	static DBGLimit limit;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	limit = { type, src, (uint8_t)minLV };
	glDebugMessageCallback(oglUtil::onMsg, &limit);
}

wstring oglUtil::getVersion()
{
	const auto str = (const char*)glGetString(GL_VERSION);
	const auto len = strlen(str);
	return wstring(str, str + len);
}

OPResult<GLenum> oglUtil::getError()
{
	const auto err = glGetError();
	if(err == GL_NO_ERROR)
		return OPResult<GLenum>(true, L"GL_NO_ERROR", err);
	else
		return OPResult<GLenum>(false, (char*)glewGetErrorString(err), err);
}

OPResult<wstring> oglUtil::loadShader(oglProgram& prog, const wstring& fname)
{
	{
		FILE *fp = nullptr;
		wstring fn = fname + L".vert";
		_wfopen_s(&fp, fn.c_str(), L"rb");
		if (fp == nullptr)
			return OPResult<wstring>(false, L"Fail to open Vertex Shader file", fn);
		oglShader vert(ShaderType::Vertex, fp);
		auto ret = vert->compile();
		if (ret)
			prog->addShader(std::move(vert));
		else
		{
			fclose(fp);
			return OPResult<wstring>(false, L"Fail to compile Vertex Shader", ret.msg);
		}
		fclose(fp);
	}
	{
		FILE *fp = nullptr;
		wstring fn = fname + L".frag";
		_wfopen_s(&fp, fn.c_str(), L"rb");
		if (fp == nullptr)
			return OPResult<wstring>(false, L"Fail to open Fragment Shader file", fn);
		oglShader frag(ShaderType::Fragment, fp);
		auto ret = frag->compile();
		if (ret)
			prog->addShader(std::move(frag));
		else
		{
			fclose(fp);
			return OPResult<wstring>(false, L"Fail to open Fragment Shader file", ret.msg);
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

std::unique_ptr<PromiseResult<void>> oglUtil::invokeSyncGL(std::function<void __cdecl(void)> task)
{
	PromiseResult<void> *ret = new PromiseResultSTD<void>(getWorker(0).doWork(task));
	return std::unique_ptr<PromiseResult<void>>(ret);
}

std::unique_ptr<PromiseResult<void>> oglUtil::invokeAsyncGL(std::function<void __cdecl(void)> task)
{
	PromiseResult<void> *ret = new PromiseResultSTD<void>(getWorker(1).doWork(task));
	return std::unique_ptr<PromiseResult<void>>(ret);
}


}