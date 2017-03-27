#pragma once

#include "oclRely.h"

namespace oclu
{

class oclUtil;
using MessageCallBack = std::function<void(void)>;

enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };

namespace inner
{

class _oclPlatform;
class _oclProgram;

class OCLUAPI _oclDevice : public NonCopyable, public std::enable_shared_from_this<_oclDevice>
{
private:
	friend class _oclContext;
	friend class _oclPlatform;
	friend class _oclProgram;
	friend class _oclComQue;
	friend class ::oclu::oclUtil;
	const cl_device_id deviceID;
	string getStr(const cl_device_info type) const;
	DeviceType getDevType() const;
	_oclDevice(const cl_device_id dID);
public:
	const DeviceType type;
	const string name, vendor, version;
};


class OCLUAPI _oclComQue : public NonCopyable
{
private:
	friend class _oclContext;
	static cl_command_queue createQueue(const cl_context context, const cl_device_id dev);
	const Wrapper<_oclContext> context;
	const Wrapper<_oclDevice> device;
	const cl_command_queue comque;
	_oclComQue(const Wrapper<_oclContext>& ctx, const Wrapper<_oclDevice>& dev);
public:
	~_oclComQue();
	void flush() const;
	Wrapper<_oclProgram> loadProgram() const;
};


class OCLUAPI _oclContext : public NonCopyable, public std::enable_shared_from_this<_oclContext>
{
private:
	friend class _oclPlatform;
	friend class _oclComQue;
	static void CL_CALLBACK onNotify(const char *errinfo, const void *private_info, size_t cb, void *user_data);
	//create OpenCL context
	const cl_context context;
	cl_context createContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices) const;
	_oclContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices);
public:
	MessageCallBack onMessage = nullptr;
	~_oclContext();

};


class OCLUAPI _oclPlatform : public NonCopyable
{
private:
	friend class ::oclu::oclUtil;
	bool checkGL() const;
	string getStr(const cl_platform_info type) const;
	const cl_platform_id platformID;
	vector<Wrapper<_oclDevice>> devs;
	Wrapper<_oclDevice> defDev;
	_oclPlatform(const cl_platform_id pID);
public:
	const string name, ver;
	const bool isCurrentGL;
	const vector<Wrapper<_oclDevice>>& getDevices() const { return devs; }
	Wrapper<_oclContext> createContext() const;
};

}
using oclDevice = Wrapper<inner::_oclDevice>;
using oclContext = Wrapper<inner::_oclContext>;
using oclPlatform = Wrapper<inner::_oclPlatform>;



}

