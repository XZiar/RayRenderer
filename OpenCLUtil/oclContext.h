#pragma once

#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclContext : public NonCopyable, public std::enable_shared_from_this<_oclContext>
{
	friend class _oclPlatform;
	friend class _oclCmdQue;
	friend class _oclProgram;
	friend class _oclBuffer;
	friend class _oclGLBuffer;
public:
	const vector<oclDevice> devs;
private:
	static void CL_CALLBACK onNotify(const char *errinfo, const void *private_info, size_t cb, void *user_data);
	const cl_context context;
	cl_context createContext(const cl_context_properties props[]) const;
	_oclContext(const cl_context_properties props[], const vector<oclDevice>& devices);
public:
	MessageCallBack onMessage = nullptr;
	~_oclContext();
};

}
using oclContext = Wrapper<detail::_oclContext>;


}