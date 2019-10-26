#include "oclPch.h"
#include "oclMem.h"
#include "oclUtil.h"


namespace oclu
{
using xziar::img::TexFormatUtil;

//MAKE_ENABLER_IMPL(oclMem_::oclMapPtr_)


common::span<std::byte> oclMapPtr::Get() const noexcept { return Ptr->MemSpace; }

class oclMapPtr::oclMemInfo : public common::AlignedBuffer::ExternBufInfo
{
    std::shared_ptr<const oclMem_::oclMapPtr_> Ptr;
    [[nodiscard]] size_t GetSize() const noexcept override 
    {
        return Ptr->MemSpace.size();
    }
    [[nodiscard]] std::byte* GetPtr() const noexcept override 
    {
        return Ptr->MemSpace.data();
    }
public:
    oclMemInfo(std::shared_ptr<const oclMem_::oclMapPtr_> ptr) noexcept : Ptr(std::move(ptr)) { }
    ~oclMemInfo() override {}
};
common::AlignedBuffer oclMapPtr::AsBuffer() const noexcept
{
    return common::AlignedBuffer::CreateBuffer(std::make_unique<oclMemInfo>(Ptr));
}


oclMem_::oclMapPtr_::oclMapPtr_(oclCmdQue que, const oclMem_& mem, common::span<std::byte> space) :
    Queue(std::move(que)), Mem(mem), MemSpace(space) {}

oclMem_::oclMapPtr_::~oclMapPtr_()
{
    cl_event e;
    const auto ret = clEnqueueUnmapMemObject(Queue->CmdQue, Mem.MemID, MemSpace.data(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        oclLog().error(u"cannot unmap clObject : {}\n", oclUtil::GetErrorString(ret));
}


oclMem_::oclMem_(oclContext ctx, cl_mem mem, const MemFlag flag) : Context(std::move(ctx)), MemID(mem), Flag(flag)
{ }
oclMem_::~oclMem_()
{
    if (Context->ShouldDebugResurce())
    {
        uint32_t refCount = 0;
        clGetMemObjectInfo(MemID, CL_MEM_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
        if (refCount != 1)
        {
            oclLog().warning(u"oclMem {:p} has {} reference and not able to release.\n", (void*)MemID, refCount);
        }
    }
    clReleaseMemObject(MemID);
}

oclMapPtr oclMem_::Map(oclCmdQue que, const MapFlag mapFlag)
{
    auto rawptr = MapObject(que->CmdQue, mapFlag);
    auto ptr = new oclMapPtr_(std::move(que), *this, rawptr);
    return oclMapPtr(std::shared_ptr<const oclMapPtr_>(shared_from_this(), ptr));
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
