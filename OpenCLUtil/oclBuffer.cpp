#include "oclRely.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu::detail
{


cl_mem _oclBuffer::createMem() const
{
	cl_int errcode;
	auto id = clCreateBuffer(ctx->context, (cl_mem_flags)type, size, NULL, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create memory", errcode));
	return id;
}

_oclBuffer::_oclBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const size_t size_, const cl_mem id)
	:ctx(ctx_), type(type_), size(size_), memID(id)
{
}

_oclBuffer::_oclBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const size_t size_)
	: ctx(ctx_), type(type_), size(size_), memID(createMem())
{
}

_oclBuffer::~_oclBuffer()
{
	clReleaseMemObject(memID);
}

optional<oclPromise> _oclBuffer::read(const oclCmdQue que, void *buf, const size_t size_, const size_t offset, const bool shouldBlock) const
{
	if (offset >= size)
		COMMON_THROW(BaseException, L"offset overflow");
	cl_event e;
	auto ret = clEnqueueReadBuffer(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, offset, min(size - offset, size_), buf, 0, nullptr, &e);
	if (ret != CL_SUCCESS && ret != CL_OUT_OF_RESOURCES)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot read clMemory", ret));
	if (shouldBlock)
		return {};
	else
		return oclPromise(e);
}

optional<oclPromise> _oclBuffer::write(const oclCmdQue que, const void *buf, const size_t size_, const size_t offset, const bool shouldBlock) const
{
	if (offset >= size)
		COMMON_THROW(BaseException, L"offset overflow");
	cl_event e;
	auto ret = clEnqueueWriteBuffer(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, offset, min(size - offset, size_), buf, 0, nullptr, &e);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot read clMemory", ret));
	if (shouldBlock)
		return {};
	else
		return oclPromise(e);
}



cl_mem _oclGLBuffer::createMem(const std::shared_ptr<_oclContext>& ctx_, const oglu::oglBuffer buf_) const
{
	cl_int errcode;
	auto id = clCreateFromGLBuffer(ctx_->context, (cl_mem_flags)type, buf_->bufferID, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create buffer from glBuffer", errcode));
	return id;
}

cl_mem _oclGLBuffer::createMem(const std::shared_ptr<_oclContext>& ctx_, const oglu::oglTexture tex_) const
{
	cl_int errcode;
	auto id = clCreateFromGLTexture(ctx_->context, (cl_mem_flags)type, (cl_GLenum)tex_->type, 0, tex_->textureID, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create buffer from glTexture", errcode));
	return id;
}

_oclGLBuffer::_oclGLBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const oglu::oglBuffer buf_)
	: _oclBuffer(ctx_, type_, INT32_MAX, createMem(ctx_, buf_))
{
}

_oclGLBuffer::_oclGLBuffer(const std::shared_ptr<_oclContext>& ctx_, const MemType type_, const oglu::oglTexture tex_)
	: _oclBuffer(ctx_, type_, INT32_MAX, createMem(ctx_, tex_))
{
}

_oclGLBuffer::~_oclGLBuffer()
{
}

optional<int32_t> _oclGLBuffer::lock(const oclCmdQue& que) const
{
	glFlush();
	cl_int ret = clEnqueueAcquireGLObjects(que->cmdque, 1, &memID, 0, NULL, NULL);
	if (ret == CL_SUCCESS)
		return {};
	else
		return ret;
}

optional<int32_t> _oclGLBuffer::unlock(const oclCmdQue& que) const
{
	clFlush(que->cmdque);
	cl_int ret = clEnqueueReleaseGLObjects(que->cmdque, 1, &memID, 0, NULL, NULL);
	if (ret == CL_SUCCESS)
		return {};
	else
		return ret;
}

}
