#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclMem.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

class OCLUAPI oclBuffer_ : public oclMem_
{
    friend class oclKernel_;
    friend class oclContext_;
private:
    MAKE_ENABLER();
protected:
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id);
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) override;
public:
    const size_t Size;
    virtual ~oclBuffer_();
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
    template<typename T>
    PromiseResult<void> WriteSpan(const oclCmdQue& que, const T& buf, size_t count = 0, const size_t offset = 0, const bool shouldBlock = true) const
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "Only accept contiguous type");
        const auto vcount = Helper::Count(buf);
        if (count == 0)
            count = vcount;
        else if (count > vcount)
            COMMON_THROW(BaseException, u"write size overflow");
        return Write(que, Helper::Data(buf), count * Helper::EleSize, offset * Helper::EleSize, shouldBlock);
    }

    static oclBuffer Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr = nullptr);
};

class OCLUAPI oclGLBuffer_ : public oclBuffer_
{
    friend class oclGLInterBuf_;
private:
    MAKE_ENABLER();
    oclGLBuffer_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
public:
    const oglu::oglBuffer GLBuf;
    virtual ~oclGLBuffer_();
};
MAKE_ENABLER_IMPL(oclGLBuffer_)

class OCLUAPI oclGLInterBuf_ : public oclGLObject_<oclGLBuffer_>
{
private:
    MAKE_ENABLER();
    oclGLInterBuf_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
public:
    static oclGLInterBuf Create(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
};



}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
