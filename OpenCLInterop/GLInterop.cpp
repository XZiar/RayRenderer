#include "OpenCLUtil/oclInternal.h"
#include "GLInterop.h"
#include "OpenGLUtil/oglUtil.h"

#if defined(_WIN32)
#   pragma comment (lib, "opengl32.lib") // link Microsoft OpenGL lib
#endif



namespace oclu
{
using std::vector;
using common::container::FindInVec;
MAKE_ENABLER_IMPL(oclGLInterBuf_)
MAKE_ENABLER_IMPL(oclGLInterImg2D_)
MAKE_ENABLER_IMPL(oclGLInterImg3D_)


GLInterop::GLResLocker::GLResLocker(const oclCmdQue& que, CLHandle<detail::CLMem> mem) :
    detail::oclCommon(*que), Queue(que), Mem(mem)
{
    if (!Queue->SupportImplicitGLSync())
        oglu::oglContext_::CurrentContext()->FinishGL();
    cl_event evt = nullptr;
    cl_int ret = Funcs->clEnqueueAcquireGLObjects(*Queue->CmdQue, 1, &mem, 0, nullptr, &evt);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot lock oglObject for oclMemObject");
    if (evt)
        Funcs->clWaitForEvents(1, &evt);
}
GLInterop::GLResLocker::~GLResLocker()
{
    if (!Queue->SupportImplicitGLSync())
        Queue->Flush(); // assume promise is correctly waited before relase lock
    cl_event evt = nullptr;
    cl_int ret = Funcs->clEnqueueReleaseGLObjects(*Queue->CmdQue, 1, &Mem, 0, nullptr, &evt);
    if (ret != CL_SUCCESS)
        oclUtil::GetOCLLog().error(u"cannot unlock oglObject for oclObject : {}\n", oclUtil::GetErrorString(ret));
    if (evt)
        Funcs->clWaitForEvents(1, &evt);
}

//common::mlog::MiniLogger<false>& GLInterop::GetOCLLog()
//{
//    return oclUtil::GetOCLLog();
//}

void* GLInterop::CreateMemFromGLBuf(const oclContext_& ctx, MemFlag flag, const oglu::oglBuffer& buf)
{
    cl_int errcode;
    const auto id = ctx.Funcs->clCreateFromGLBuffer(*ctx.Context, common::enum_cast(flag), buf->BufferID, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create clMem from glBuffer");
    return id;
}
void* GLInterop::CreateMemFromGLTex(const oclContext_& ctx, MemFlag flag, const oglu::oglTexBase& tex)
{
    if (HAS_FIELD(flag, MemFlag::HostAccessMask) || HAS_FIELD(flag, MemFlag::HostInitMask))
    {
        flag &= MemFlag::DeviceAccessMask;
        oclUtil::GetOCLLog().warning(u"When Create CLGLImage, only DeviceAccessFlag can be use, others are ignored.\n");
    }
    cl_int errcode;
    if (xziar::img::TexFormatUtil::IsCompressType(tex->GetInnerFormat()))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"OpenCL does not support Comressed Texture");
    const auto id = ctx.Funcs->clCreateFromGLTexture(*ctx.Context, common::enum_cast(flag), common::enum_cast(tex->Type), 0, tex->TextureID, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create clMem from glTexture");
    return id;
}

vector<cl_context_properties> GLInterop::GetGLProps(const oclPlatform_& plat, const oglu::oglContext& context)
{
    auto props = plat.GetCLProps();
    props.pop_back();
#if defined(_WIN32)
    constexpr cl_context_properties glPropName = CL_WGL_HDC_KHR;
#else
    constexpr cl_context_properties glPropName = CL_GLX_DISPLAY_KHR;
#endif
    if (context)
        props.insert(props.cend(),
            {
                //OpenGL context
                CL_GL_CONTEXT_KHR,   (cl_context_properties)context->Hrc,
                //HDC used to create the OpenGL context
                glPropName,          (cl_context_properties)context->Hdc
            });
    props.push_back(0);
    return props;
}

oclDevice GLInterop::GetGLDevice(const oclPlatform_& plat, const vector<cl_context_properties>& props)
{
    if (!plat.Funcs->clGetGLContextInfoKHR || !plat.Extensions.Has("cl_khr_gl_sharing") || 
        plat.BeignetFix/*beignet didn't implement that*/)
        return {};
    {
        cl_device_id dID;
        size_t retSize = 0;
        const auto ret = plat.Funcs->clGetGLContextInfoKHR(props.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &dID, &retSize);
        if (ret == CL_SUCCESS && retSize)
            if (auto dev = FindInVec(plat.Devices, [=](const oclDevice_& dev) { return *dev.DeviceID == dID; }); dev)
                return dev;
        if (ret != CL_SUCCESS && ret != CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
            oclUtil::GetOCLLog().warning(u"Failed to get current device for glContext: [{}]\n", oclUtil::GetErrorString(ret));
    }
    //try context that may be associated 
    /*{
        std::vector<cl_device_id> dIDs(plat->Devices.size());
        size_t retSize = 0;
        const auto ret = plat->FuncClGetGLContext(props.data(), CL_DEVICES_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id) * plat->Devices.size(), dIDs.data(), &retSize);
        if (ret == CL_SUCCESS && retSize)
            if (auto dev = FindInVec(plat->Devices, [=](const oclDevice_& dev) { return dev == dIDs[0]; }); dev)
                return oclDevice(plat, dev);
        if (ret != CL_SUCCESS && ret != CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
            oclUtil::GetOCLLog().warning(u"Failed to get associate device for glContext: [{}]\n", oclUtil::GetErrorString(ret));
    }*/
    return nullptr;
}

bool GLInterop::CheckIsGLShared(const oclPlatform_& plat, const oglu::oglContext& context)
{
    const auto props = GetGLProps(plat, context);
    return (bool)GetGLDevice(plat, props);
}

oclContext GLInterop::CreateGLSharedContext(const oglu::oglContext& context)
{
    for (const auto& plat : oclPlatform_::GetPlatforms())
    {
        const auto props = GetGLProps(*plat, context);
        const auto dev = GetGLDevice(*plat, props);
        if (dev)
            return plat->CreateContext({ dev }, props);
    }
    return {};
}

oclContext GLInterop::CreateGLSharedContext(const oclPlatform_& plat, const oglu::oglContext& context)
{
    const auto props = GetGLProps(plat, context);
    const auto dev = GetGLDevice(plat, props);
    if (dev)
        return plat.CreateContext({ dev }, props);
    return {};
}


oclGLBuffer_::oclGLBuffer_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : oclBuffer_(ctx, flag, SIZE_MAX, GLInterop::CreateMemFromGLBuf(*ctx, flag, buf)), GLBuf(buf) { }
oclGLBuffer_::~oclGLBuffer_() {}

oclGLImage2D_::oclGLImage2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex)
    : oclImage2D_(ctx, flag, tex->GetSize().first, tex->GetSize().second, 1,
        tex->GetInnerFormat(), GLInterop::CreateMemFromGLTex(*ctx, flag, tex)),
    GLTex(tex) { }
oclGLImage2D_::~oclGLImage2D_() {}

oclGLImage3D_::oclGLImage3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex)
    : oclImage3D_(ctx, flag, std::get<0>(tex->GetSize()), std::get<1>(tex->GetSize()), std::get<2>(tex->GetSize()),
        tex->GetInnerFormat(), GLInterop::CreateMemFromGLTex(*ctx, flag, tex)),
    GLTex(tex) { }
oclGLImage3D_::~oclGLImage3D_() {}


oclGLInterBuf_::oclGLInterBuf_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : oclGLObject_<oclGLBuffer_>(MAKE_ENABLER_UNIQUE(oclGLBuffer_, (ctx, flag, buf))) { }
oclGLInterBuf oclGLInterBuf_::Create(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
{
    return MAKE_ENABLER_SHARED(oclGLInterBuf_, (ctx, flag, buf));
}

oclGLInterImg2D_::oclGLInterImg2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex)
    : oclGLObject_<oclGLImage2D_>(MAKE_ENABLER_UNIQUE(oclGLImage2D_, (ctx, flag, tex))) { }
oclGLInterImg2D oclGLInterImg2D_::Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex)
{
    return MAKE_ENABLER_SHARED(oclGLInterImg2D_, (ctx, flag, tex));
}

oclGLInterImg3D_::oclGLInterImg3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex)
    : oclGLObject_<oclGLImage3D_>(MAKE_ENABLER_UNIQUE(oclGLImage3D_, (ctx, flag, tex))) { }
oclGLInterImg3D oclGLInterImg3D_::Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex)
{
    return MAKE_ENABLER_SHARED(oclGLInterImg3D_, (ctx, flag, tex));
}



}
