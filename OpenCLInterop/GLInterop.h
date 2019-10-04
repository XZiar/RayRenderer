#pragma once
#include "OpenCLInteropRely.h"

#include "OpenGLUtil/oglBuffer.h"
#include "OpenGLUtil/oglTexture.h"
#include "OpenGLUtil/oglContext.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclGLBuffer_;
using oclGLBuffer       = std::shared_ptr<oclGLBuffer_>;
class oclGLInterBuf_;
using oclGLInterBuf     = std::shared_ptr<oclGLInterBuf_>;

class oclGLImage2D_;
using oclGLImg2D        = std::shared_ptr<oclGLImage2D_>;
class oclGLImage3D_;
using oclGLImg3D        = std::shared_ptr<oclGLImage3D_>;
class oclGLInterImg2D_;
using oclGLInterImg2D   = std::shared_ptr<oclGLInterImg2D_>;
class oclGLInterImg3D_;
using oclGLInterImg3D   = std::shared_ptr<oclGLInterImg3D_>;


class OCLIOPAPI GLInterop
{
    template<typename> friend class oclGLObject_;
    friend class oclGLBuffer_;
    friend class oclGLImage2D_;
    friend class oclGLImage3D_;
private:

    class OCLIOPAPI GLResLocker : public NonCopyable, public NonMovable
    {
        template<typename> friend class oclGLObject_;
    private:
        MAKE_ENABLER();
        const oclCmdQue Queue;
        const cl_mem Mem;
        GLResLocker(const oclCmdQue& que, const cl_mem mem);
    public:
        ~GLResLocker();
    };

    static cl_mem CreateMemFromGLBuf(const oclContext_& ctx, MemFlag flag, const oglu::oglBuffer& bufId);
    static cl_mem CreateMemFromGLTex(const oclContext_& ctx, MemFlag flag, const oglu::oglTexBase& tex);
    static vector<cl_context_properties> GetGLProps(const oclPlatform_& plat, const oglu::oglContext& context);
    static oclDevice GetGLDevice(const oclPlatform& plat, const vector<cl_context_properties>& props);
public:
    static bool CheckIsGLShared(const oclPlatform_& plat, const oglu::oglContext& context);
    static oclContext CreateGLSharedContext(const oglu::oglContext& context);
    static oclContext CreateGLSharedContext(const oclPlatform_& plat, const oglu::oglContext& context);
};
MAKE_ENABLER_IMPL(GLInterop::GLResLocker)


template<typename T>
class oclGLObject_ : public std::enable_shared_from_this<oclGLObject_<T>>
{
protected:
    const std::unique_ptr<T> CLObject;
    oclGLObject_(std::unique_ptr<T>&& obj) : CLObject(std::move(obj)) { }
public:
    class oclGLMem
    {
        friend class oclGLObject_<T>;
    private:
        std::shared_ptr<oclGLObject_<T>> Host;
        std::shared_ptr<GLInterop::GLResLocker> ResLocker;
        oclGLMem(std::shared_ptr<oclGLObject_<T>> host, const oclCmdQue& que, const cl_mem mem) :
            Host(std::move(host)), ResLocker(MAKE_ENABLER_SHARED(GLInterop::GLResLocker, que, mem))
        { }
    public:
        std::shared_ptr<T> Get() const
        {
            return std::shared_ptr<T>(ResLocker, Host->CLObject.get());
        }
        operator const std::shared_ptr<T>& () const { return Get(); }
    };

    ~oclGLObject_() { }
    oclGLMem Lock(const oclCmdQue& que)
    {
        return oclGLMem(this->shared_from_this(), que, CLObject->MemID);
    }
};

class OCLIOPAPI oclGLBuffer_ : public oclBuffer_
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
class OCLIOPAPI oclGLImage2D_ : public oclImage2D_
{
    friend class oclGLInterImg2D_;
private:
    MAKE_ENABLER();
    oclGLImage2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
public:
    const oglu::oglTex2D GLTex;
    virtual ~oclGLImage2D_() override;
};
MAKE_ENABLER_IMPL(oclGLImage2D_)
class OCLIOPAPI oclGLImage3D_ : public oclImage3D_
{
    friend class oclGLInterImg3D_;
private:
    MAKE_ENABLER();
    oclGLImage3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
public:
    const oglu::oglTex3D GLTex;
    virtual ~oclGLImage3D_() override;
};
MAKE_ENABLER_IMPL(oclGLImage3D_)


class OCLIOPAPI oclGLInterBuf_ : public oclGLObject_<oclGLBuffer_>
{
private:
    MAKE_ENABLER();
    oclGLInterBuf_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
public:
    static oclGLInterBuf Create(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf);
};
class OCLIOPAPI oclGLInterImg2D_ : public oclGLObject_<oclGLImage2D_>
{
private:
    MAKE_ENABLER();
    oclGLInterImg2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
public:
    oglu::oglTex2D GetGLTex() const { return CLObject->GLTex; }

    static oclGLInterImg2D Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
};
class OCLIOPAPI oclGLInterImg3D_ : public oclGLObject_<oclGLImage3D_>
{
private:
    MAKE_ENABLER();
    oclGLInterImg3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
public:
    oglu::oglTex3D GetGLTex() const { return CLObject->GLTex; }

    static oclGLInterImg3D Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
