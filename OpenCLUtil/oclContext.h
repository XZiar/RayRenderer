#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclCmdQue.h"
#include "oclProgram.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclContext : public NonCopyable
{
	friend class _oclPlatform;
	friend class _oclCmdQue;
private:
	static void CL_CALLBACK onNotify(const char *errinfo, const void *private_info, size_t cb, void *user_data);
	const cl_context context;
	cl_context createContext(const cl_context_properties props[], const vector<oclDevice>& devices) const;
	_oclContext(const cl_context_properties props[], const vector<oclDevice>& devices);
public:
	MessageCallBack onMessage = nullptr;
	~_oclContext();
	oclCmdQue createCmdQue(const oclDevice& dev) const;
	oclProgram loadProgram(const string& src) const;
};

}
using oclContext = Wrapper<detail::_oclContext>;


}