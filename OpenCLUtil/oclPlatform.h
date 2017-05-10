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
	bool isBrand(const wstring& brand) const;
	wstring getStr(const cl_platform_info type) const;
	const cl_platform_id platformID;
	vector<oclDevice> devs;
	oclDevice defDev;
	_oclPlatform(const cl_platform_id pID);
	bool isCurGL;
public:
	const wstring name, ver;
	const bool isNVIDIA, isAMD, isINTEL;
	const vector<oclDevice>& getDevices() const { return devs; }
	const oclDevice& getDefaultDevice() const { return defDev; }
	bool isCurrentGL() const { return isCurGL; }
	oclContext createContext() const;
};


}
using oclPlatform = Wrapper<detail::_oclPlatform>;

}