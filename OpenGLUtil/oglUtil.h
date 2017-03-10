#pragma once
#include "oglRely.h"
#include "oglComponent.h"
#include "oglProgram.h"

#include <boost/circular_buffer.hpp>

namespace oglu
{

enum class MsgSrc :uint8_t { OpenGL = 0x1, System = 0x2, Compiler = 0x4, ThreeRD = 0x8, Application = 0x10, Other = 0x20 };
enum class MsgType :uint16_t
{
	Error = 0x1, Deprecated = 0x2, UndefBehav = 0x4, NonPortable = 0x8, Performance = 0x10, Other = 0x20,
	Maker = 0x40, PushGroup = 0x80, PopGroup = 0x100
};
enum class MsgLevel :uint8_t { High = 3, Medium = 2, Low = 1, Notfication = 0 };

struct OGLUAPI DebugMessage
{
	friend class oglUtil;
private:
	static MsgSrc __cdecl parseSrc(const GLenum src)
	{
		return static_cast<MsgSrc>(1 << (src - GL_DEBUG_SOURCE_API));
	}
	static MsgType __cdecl parseType(const GLenum type)
	{
		if(type <= GL_DEBUG_TYPE_OTHER)
			return static_cast<MsgType>(1 << (type - GL_DEBUG_TYPE_ERROR));
		else
			return static_cast<MsgType>(0x40 << (type - GL_DEBUG_TYPE_MARKER));
	}
	static MsgLevel __cdecl parseLv(const GLenum lv)
	{
		switch (lv)
		{
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return MsgLevel::Notfication;
		case GL_DEBUG_SEVERITY_LOW:
			return MsgLevel::Low;
		case GL_DEBUG_SEVERITY_MEDIUM:
			return MsgLevel::Medium;
		case GL_DEBUG_SEVERITY_HIGH:
			return MsgLevel::High;
		default:
			return MsgLevel::Notfication;
		}
	}
public:
	const MsgType type;
	const MsgSrc from;
	const MsgLevel level;
	wstring msg;
	
	DebugMessage(const GLenum from_, const GLenum type_, const GLenum lv)
		:from(parseSrc(from_)), type(parseType(type_)), level(parseLv(lv)) { }
};

class OGLUAPI oglUtil
{
private:
	struct DBGLimit
	{
		uint16_t type;
		uint8_t src;
		uint8_t minLV;
	};
	static boost::circular_buffer<std::shared_ptr<DebugMessage>> msglist;
	static boost::circular_buffer<std::shared_ptr<DebugMessage>> errlist;
	static void GLAPIENTRY onMsg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
public:
	static void __cdecl init();
	static void __cdecl setDebug(uint8_t src, uint16_t type, MsgLevel minLV);
	static string __cdecl getVersion();
	static OPResult<GLenum> __cdecl getError();
	//load Vertex and Fragment Shader(with suffix of (.vert) and (.frag)
	static OPResult<wstring> __cdecl loadShader(oglProgram& prog, const wstring& fname);
	static void applyTransform(Mat4x4& matModel, const TransformOP& op);
	static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
};

}