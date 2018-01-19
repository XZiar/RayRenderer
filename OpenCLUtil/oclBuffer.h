#pragma once

#include "oclRely.h"
#include "oclPromiseTask.h"
#include "oclCmdQue.h"

namespace oclu
{

enum class MemType : cl_mem_flags
{
	ReadOnly = CL_MEM_READ_ONLY, WriteOnly = CL_MEM_WRITE_ONLY, ReadWrite = CL_MEM_READ_WRITE,
	UseHost = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy = CL_MEM_COPY_HOST_PTR,
	HostWriteOnly = CL_MEM_HOST_WRITE_ONLY, HostReadOnly = CL_MEM_HOST_READ_ONLY, HostNoAccess = CL_MEM_HOST_NO_ACCESS
};
MAKE_ENUM_BITFIELD(MemType)

namespace detail
{

class OCLUAPI _oclBuffer : public NonCopyable
{
	friend class _oclKernel;
	friend class _oclContext;
protected:
	const std::shared_ptr<_oclContext> ctx;
public:
	const MemType type;
	const size_t size;
protected:
	const cl_mem memID;
	cl_mem createMem() const;
	_oclBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const size_t size_, const cl_mem id);
public:
	_oclBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const size_t size_);
	virtual ~_oclBuffer();
	optional<oclPromise> read(const oclCmdQue que, void *buf, const size_t size_, const size_t offset = 0, const bool shouldBlock = true) const;
	template<class T, class A>
	optional<oclPromise> read(const oclCmdQue que, vector<T, A>& buf, size_t count = 0, const size_t offset = 0, const bool shouldBlock = true) const
	{
		if (offset >= size)
			COMMON_THROW(BaseException, L"offset overflow");
		if (count == 0)
			count = (size - offset) / sizeof(T);
		else if(count * sizeof(T) + offset > size)
			COMMON_THROW(BaseException, L"read size overflow");
		buf.resize(count);
		return read(que, buf.data(), count * sizeof(T), offset, shouldBlock);
	}
	optional<oclPromise> write(const oclCmdQue que, const void * const buf, const size_t size_, const size_t offset = 0, const bool shouldBlock = true) const;
	template<class T, class A>
	optional<oclPromise> write(const oclCmdQue que, const vector<T, A>& buf, size_t count = 0, const size_t offset = 0, const bool shouldBlock = true) const
	{
		const auto vsize = buf.size();
		if (count == 0)
			count = buf.size();
		else if (count > buf.size())
			COMMON_THROW(BaseException, L"write size overflow");
		const auto wsize = count * sizeof(T);
		return write(que, buf.data(), wsize, offset, shouldBlock);
	}
	template<class T, size_t N>
	optional<oclPromise> write(const oclCmdQue que, const T(&buf)[N], const size_t offset = 0, const bool shouldBlock = true) const
	{
		auto wsize = N * sizeof(T);
		return write(que, buf, wsize, offset, shouldBlock);
	}
};

class OCLUAPI _oclGLBuffer : public _oclBuffer
{
private:
	const oglu::oglBuffer buf;
	const oglu::oglTexture tex;
	cl_mem createMem(const std::shared_ptr<_oclContext>& ctx_, const oglu::oglBuffer buf_) const;
	cl_mem createMem(const std::shared_ptr<_oclContext>& ctx_, const oglu::oglTexture tex_) const;
public:
	_oclGLBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const oglu::oglBuffer buf_);
	_oclGLBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const oglu::oglTexture tex_);
	~_oclGLBuffer() override;
	optional<int32_t> lock(const oclCmdQue& que) const;
	optional<int32_t> unlock(const oclCmdQue& que) const;
};

}
using oclBuffer = Wrapper<detail::_oclBuffer>;
using oclGLBuffer = Wrapper<detail::_oclGLBuffer>;

}