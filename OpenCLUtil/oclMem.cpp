#include "oclPch.h"
#include "oclMem.h"
#include "oclUtil.h"


namespace oclu
{
using xziar::img::TexFormatUtil;

//MAKE_ENABLER_IMPL(oclMem_::oclMapPtr_)


void* oclMapPtr::Get() const { return Ptr->Pointer; }


oclMem_::oclMapPtr_::oclMapPtr_(oclCmdQue que, const oclMem_& mem, void* pointer) : 
    Queue(std::move(que)), Mem(mem), Pointer(pointer) {}

oclMem_::oclMapPtr_::~oclMapPtr_()
{
    cl_event e;
    const auto ret = clEnqueueUnmapMemObject(Queue->CmdQue, Mem.MemID, Pointer, 0, nullptr, &e); 
    if (ret != CL_SUCCESS)
        oclLog().error(u"cannot unmap clObject : {}\n", oclUtil::GetErrorString(ret));
}


oclMem_::oclMem_(oclContext ctx, cl_mem mem, const MemFlag flag) : Context(std::move(ctx)), MemID(mem), Flag(flag)
{ }
oclMem_::~oclMem_()
{
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

oclMapPtr oclMem_::Map(oclCmdQue que, const MapFlag mapFlag)
{
    auto rawptr = MapObject(que->CmdQue, mapFlag);
    auto ptr = new oclMapPtr_(std::move(que), *this, rawptr);
    return oclMapPtr(std::shared_ptr<oclMem_::oclMapPtr_>(shared_from_this(), ptr));
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




}
