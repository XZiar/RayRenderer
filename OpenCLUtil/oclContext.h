#pragma once

#include "oclRely.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclContext : public NonCopyable, public std::enable_shared_from_this<_oclContext>
{
	friend class _oclPlatform;
	friend class _oclComQue;
private:
	static void CL_CALLBACK onNotify(const char *errinfo, const void *private_info, size_t cb, void *user_data);
	//create OpenCL context
	const cl_context context;
	cl_context createContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices) const;
	_oclContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices);
public:
	MessageCallBack onMessage = nullptr;
	~_oclContext();

};


}
using oclContext = Wrapper<detail::_oclContext>;


}