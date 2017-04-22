#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclPlatform : public NonCopyable
{
private:
	friend class ::oclu::oclUtil;
	bool checkGL() const;
	wstring getStr(const cl_platform_info type) const;
	const cl_platform_id platformID;
	vector<oclDevice> devs;
	oclDevice defDev;
	_oclPlatform(const cl_platform_id pID);
public:
	const wstring name, ver;
	const bool isCurrentGL;
	const vector<oclDevice>& getDevices() const { return devs; }
	const oclDevice& getDefaultDevice() const { return defDev; }
	oclContext createContext() const;
};


}
using oclPlatform = Wrapper<detail::_oclPlatform>;

}