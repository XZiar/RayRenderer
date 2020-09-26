#include "oclPch.h"
#include "oclMem.h"
#include "oclUtil.h"
#include "oclException.h"


namespace oclu
{
using xziar::img::TexFormatUtil;

MAKE_ENABLER_IMPL(oclMem_::oclMapPtr_)


common::span<std::byte> oclMapPtr::Get() const noexcept { return Ptr->MemSpace; }

class oclMapPtr::oclMemInfo : public common::AlignedBuffer::ExternBufInfo
{
    oclMem Mem;
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
    oclMemInfo(oclMem mem, std::shared_ptr<const oclMem_::oclMapPtr_> ptr) noexcept :
        Mem(std::move(mem)), Ptr(std::move(ptr)){ }
    ~oclMemInfo() override {}
};
common::AlignedBuffer oclMapPtr::AsBuffer() const noexcept
{
    return common::AlignedBuffer::CreateBuffer(std::make_unique<oclMemInfo>(Mem, Ptr));
}
bool oclMapPtr::CheckIsCLBuffer(const common::AlignedBuffer& buffer) noexcept
{
    return buffer.TryGetOwner<oclMemInfo>() != nullptr;
}


oclMem_::oclMapPtr_::oclMapPtr_(oclCmdQue&& que, oclMem_* mem, const MapFlag mapFlag) :
    Queue(std::move(que)), Mem(*mem)
{
    MemSpace = Mem.MapObject(Queue->CmdQue, mapFlag);
}

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
    return oclMapPtr(shared_from_this(), MAKE_ENABLER_SHARED(const oclMapPtr_, (std::move(que), this, mapFlag)));
}

void oclMem_::Flush(const oclCmdQue& que)
{
    if (HAS_FIELD(Flag, MemFlag::UseHost))
    {
        [[maybe_unused]] const auto dummy = Map(que, MapFlag::Read);
    }
    else
    {
        que->Flush();
    }
}

MemFlag oclMem_::ProcessMemFlag(const oclContext_& context, MemFlag flag, const void* ptr)
{
    if (!CheckMemFlagDevAccess(flag))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"MemFlag's DeviceAccess conflict");
    if (!CheckMemFlagHostAccess(flag))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"MemFlag's HostAccess conflict");
    if (!CheckMemFlagHostInit(flag))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"MemFlag's HostInit conflict");
    if (!HAS_FIELD(flag, MemFlag::UseHost) && ptr != nullptr)
        flag |= MemFlag::HostCopy;
    if (context.Version < 12)
        flag = REMOVE_MASK(flag, MemFlag::HostAccessMask);
    return flag;
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
