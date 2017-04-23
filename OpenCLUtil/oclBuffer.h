#pragma once

#include "oclRely.h"
#include "oclPromiseTask.h"
#include "oclCmdQue.h"

namespace oclu
{

enum class MemType : cl_mem_flags
{
	ReadOnly = CL_MEM_READ_ONLY, WriteOnly = CL_MEM_WRITE_ONLY, ReadWrite = CL_MEM_READ_WRITE,
	HostUse = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy = CL_MEM_COPY_HOST_PTR
};

namespace detail
{

class OCLUAPI _oclBuffer : public NonCopyable
{
	friend class _oclKernel;
	friend class _oclContext;
protected:
	const std::shared_ptr<const _oclContext> ctx;
	const MemType type;
	const size_t size;
	const cl_mem memID;
	cl_mem createMem() const;
	_oclBuffer(const std::shared_ptr<const _oclContext>& ctx_, const MemType type_, const size_t size_, const cl_mem id);
public:
	_oclBuffer(const std::shared_ptr<const _oclContext>& ctx_, const MemType type_, const size_t size_);
	virtual ~_oclBuffer();
	optional<oclPromise> read(const oclCmdQue que, void *buf, const size_t size_, const size_t offset = 0, const bool shouldBlock = true) const;
	template<class T>
	optional<oclPromise> read(const oclCmdQue que, vector<T>& buf, const size_t offset = 0, const bool shouldBlock = true) const
	{
		if (offset >= size)
			COMMON_THROW(BaseException, L"offset overflow");
		auto count = (size - offset) / sizeof(T);
		buf.resize(count);
		return read(que, buf.data(), count * sizeof(T), offset, shouldBlock);
	}
	optional<oclPromise> write(const oclCmdQue que, const void *buf, const size_t size_, const size_t offset = 0, const bool shouldBlock = true) const;
	template<class T>
	optional<oclPromise> write(const oclCmdQue que, const vector<T>& buf, const size_t offset = 0, const bool shouldBlock = true) const 
	{
		auto wsize = buf.size() * sizeof(T);
		return write(que, buf.data(), wsize, offset, shouldBlock);
	}
};

class OCLUAPI _oclGLBuffer : public _oclBuffer
{
private:
	const oglu::oglBuffer buf;
	const oglu::oglTexture tex;
	cl_mem createMem(const std::shared_ptr<const _oclContext>& ctx_, const oglu::oglBuffer buf_) const;
	cl_mem createMem(const std::shared_ptr<const _oclContext>& ctx_, const oglu::oglTexture tex_) const;
public:
	_oclGLBuffer(const std::shared_ptr<const _oclContext>& ctx_, const MemType type_, const oglu::oglBuffer buf_);
	_oclGLBuffer(const std::shared_ptr<const _oclContext>& ctx_, const MemType type_, const oglu::oglTexture tex_);
	~_oclGLBuffer() override;
	optional<int32_t> lock(const oclCmdQue& que) const;
	optional<int32_t> unlock(const oclCmdQue& que) const;
};

}
using oclBuffer = Wrapper<detail::_oclBuffer>;
using oclGLBuffer = Wrapper<detail::_oclGLBuffer>;

}