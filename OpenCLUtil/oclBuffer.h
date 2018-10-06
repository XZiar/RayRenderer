#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclMem.h"

namespace oclu
{

namespace detail
{

class OCLUAPI _oclBuffer : public _oclMem
{
    friend class _oclKernel;
    friend class _oclContext;
public:
    const size_t Size;
protected:
    _oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) override;
public:
    _oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size);
    _oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr);
    virtual ~_oclBuffer();
    PromiseResult<void> Read(const oclCmdQue& que, void *buf, const size_t size, const size_t offset = 0, const bool shouldBlock = true) const;
    template<class T, class A>
    PromiseResult<void> Read(const oclCmdQue& que, vector<T, A>& buf, size_t count = 0, const size_t offset = 0, const bool shouldBlock = true) const
    {
        if (offset >= Size)
            COMMON_THROW(BaseException, u"offset overflow");
        if (count == 0)
            count = (Size - offset) / sizeof(T);
        else if(count * sizeof(T) + offset > Size)
            COMMON_THROW(BaseException, u"read size overflow");
        buf.resize(count);
        return Read(que, buf.data(), count * sizeof(T), offset, shouldBlock);
    }
    PromiseResult<void> Write(const oclCmdQue& que, const void * const buf, const size_t size, const size_t offset = 0, const bool shouldBlock = true) const;
    template<class T, class A>
    PromiseResult<void> Write(const oclCmdQue& que, const vector<T, A>& buf, size_t count = 0, const size_t offset = 0, const bool shouldBlock = true) const
    {
        const auto vsize = buf.size();
        if (count == 0)
            count = vsize;
        else if (count > vsize)
            COMMON_THROW(BaseException, u"write size overflow");
        const auto wsize = count * sizeof(T);
        return Write(que, buf.data(), wsize, offset, shouldBlock);
    }
    template<class T, size_t N>
    PromiseResult<void> Write(const oclCmdQue& que, const T(&buf)[N], const size_t offset = 0, const bool shouldBlock = true) const
    {
        auto wsize = N * sizeof(T);
        return Write(que, buf, wsize, offset, shouldBlock);
    }
};

class OCLUAPI _oclGLBuffer : public _oclBuffer
{
    template<typename> friend class _oclGLObject;
private:
    _oclGLBuffer(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
public:
    const oglu::oglBuffer GLBuf;
    virtual ~_oclGLBuffer() {}
};

class OCLUAPI _oclGLInterBuf : public _oclGLObject<_oclGLBuffer>
{
public:
    _oclGLInterBuf(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
};


}
using oclBuffer = Wrapper<detail::_oclBuffer>;
using oclGLBuffer = Wrapper<detail::_oclGLBuffer>;
using oclGLInterBuf = Wrapper<detail::_oclGLInterBuf>;

}