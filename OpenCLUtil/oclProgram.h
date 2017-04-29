#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclPromiseTask.h"


namespace oclu
{

namespace detail
{

class _oclProgram;

class OCLUAPI _oclKernel
{
	friend class _oclProgram;
private:
	const string name;
	const std::shared_ptr<_oclProgram> prog;
	const cl_kernel kernel;
	cl_kernel createKernel() const;
	_oclKernel(const std::shared_ptr<_oclProgram>& prog_, const string& name_);
public:
	~_oclKernel();
	optional<wstring> setArg(const cl_uint idx, const oclBuffer& buf);
	optional<wstring> setArg(const cl_uint idx, const void *dat, const size_t size);
	template<class T, size_t N>
	optional<wstring> setArg(const cl_uint idx, const T(&dat)[N])
	{
		return setArg(idx, dat, N * sizeof(T));
	}
	template<class T>
	optional<wstring> setArg(const cl_uint idx, const vector<T>& dat)
	{
		return setArg(idx, dat.data(), dat.size() * sizeof(T));
	}
	optional<oclPromise> run(const uint32_t workdim, const oclCmdQue que, const size_t *worksize, bool isBlock = true, const size_t *workoffset = nullptr, const size_t *localsize = nullptr);
	template<uint32_t N>
	optional<oclPromise> run(const oclCmdQue que, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
	{
		return run(N, que, worksize, isBlock, workoffset, nullptr);
	}
	template<uint32_t N>
	optional<oclPromise> run(const oclCmdQue que, const size_t(&worksize)[N], const size_t(&localsize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
	{
		return run(N, que, worksize, isBlock, workoffset, localsize);
	}
};

}
using oclKernel = Wrapper<detail::_oclKernel>;


namespace detail
{

class OCLUAPI _oclProgram : public std::enable_shared_from_this<_oclProgram>
{
	friend class _oclContext;
	friend class _oclKernel;
private:
	//static void CL_CALLBACK onNotify(const cl_program pid, void *user_data);
	const std::shared_ptr<const _oclContext> ctx;
	const string src;
	const cl_program progID;
	vector<string> kers;
	vector<oclDevice> getDevs() const;
	wstring getBuildLog(const oclDevice& dev) const;
	cl_program loadProgram() const;
	static string loadFromFile(FILE *fp); 
	void initKers();
public:
	_oclProgram(std::shared_ptr<_oclContext>& ctx_, const string& str);
	_oclProgram(std::shared_ptr<_oclContext>& ctx_, FILE *file) : _oclProgram(ctx_, loadFromFile(file)) {};
	~_oclProgram();
	void build();
	void build(const oclDevice& dev);
	oclKernel getKernel(const string& name);
};


}
using oclProgram = Wrapper<detail::_oclProgram>;


}

