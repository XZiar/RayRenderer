#include "oclRely.h"
#include "oclMem.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu
{
using xziar::img::TexFormatUtil;


oclMapPtr_::~oclMapPtr_()
{
    cl_event e;
    auto ret = clEnqueueUnmapMemObject(Queue->cmdque, MemId, Pointer, 0, nullptr, &e); 
    if (ret != CL_SUCCESS)
        oclLog().error(u"cannot unmap clObject : {}\n", oclUtil::GetErrorString(ret));
}

oclMem_::oclMem_(const oclContext& ctx, cl_mem mem, const MemFlag flag) : Context(ctx), MemID(mem), Flag(flag)
{ }
oclMem_::~oclMem_()
{
    MapPtr.reset();
#ifdef _DEBUG
    uint32_t refCount = 0;
    clGetMemObjectInfo(MemID, CL_MEM_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
    if (refCount == 1)
    {
        oclLog().debug(u"oclMem {:p} has {} reference being release.\n", (void*)MemID, refCount);
        clReleaseMemObject(MemID);
    }
    else
        oclLog().warning(u"oclMem {:p} has {} reference and not able to release.\n", (void*)MemID, refCount);
#else
    clReleaseMemObject(MemID);
#endif
}
oclMapPtr oclMem_::TryGetMap() const
{
    return MapPtr;
}
oclMapPtr oclMem_::Map(const oclCmdQue& que, const MapFlag mapFlag)
{
    if (MapPtr)
        return MapPtr;
    auto ptr = TmpPtr.lock();
    if (!ptr) // may need initialize
    {
        common::SpinLocker locker(LockObj);//enter critical section
        //check again, someone may has created it
        ptr = TmpPtr.lock();
        if (!ptr) // need to create it
        {
            auto rawptr = MapObject(que, mapFlag);
            ptr = std::shared_ptr<oclMapPtr_>(new oclMapPtr_(que, MemID, rawptr));
            TmpPtr = ptr;
        }
    }
    return ptr;
}
//oclMapPtr oclMem_::PersistMap(const oclCmdQue& que)
//{
//    MapFlag mapFlag;
//    if (HAS_FIELD(Flag, MemFlag::HostWriteOnly))
//        mapFlag = MapFlag::InvalidWrite;
//    else if (HAS_FIELD(Flag, MemFlag::HostReadOnly))
//        mapFlag = MapFlag::Read;
//    else
//        mapFlag = MapFlag::ReadWrite;
//    MapPtr = Map(que, mapFlag);
//    return MapPtr;
//}



GLResLocker::GLResLocker(const oclCmdQue& que, const cl_mem mem) : Queue(que), Mem(mem)
{
    if (!Queue->SupportImplicitGLSync())
        glFinish();
    cl_int ret = clEnqueueAcquireGLObjects(Queue->cmdque, 1, &mem, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot lock oglObject for oclMemObject");
}
GLResLocker::~GLResLocker()
{
    if (!Queue->SupportImplicitGLSync())
        Queue->Flush(); // assume promise is correctly waited before relase lock
    cl_int ret = clEnqueueReleaseGLObjects(Queue->cmdque, 1, &Mem, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS)
        oclLog().error(u"cannot unlock oglObject for oclObject : {}\n", oclUtil::GetErrorString(ret));
}

cl_mem GLInterOP::CreateMemFromGLBuf(const oclContext ctx, MemFlag flag, const oglu::oglBuffer& buf)
{
    cl_int errcode;
    const auto id = clCreateFromGLBuffer(ctx->context, (cl_mem_flags)flag, buf->BufferID, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create clMem from glBuffer");
    return id;
}
cl_mem GLInterOP::CreateMemFromGLTex(const oclContext ctx, MemFlag flag, const oglu::oglTexBase& tex)
{
    if (HAS_FIELD(flag, MemFlag::HostAccessMask) || HAS_FIELD(flag, MemFlag::HostInitMask))
    {
        flag &= MemFlag::DeviceAccessMask;
        oclLog().warning(u"When Create CLGLImage, only DeviceAccessFlag can be use, others are ignored.\n");
    }
    cl_int errcode;
    if (TexFormatUtil::IsCompressType(tex->GetInnerFormat()))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"OpenCL does not support Comressed Texture");
    const auto id = clCreateFromGLTexture(ctx->context, (cl_mem_flags)flag, (GLenum)tex->Type, 0, tex->textureID, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create clMem from glTexture");
    return id;
}


}
