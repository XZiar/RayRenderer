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
class oclBuffer_;
using oclBuffer = std::shared_ptr<oclBuffer_>;

class OCLUAPI oclBuffer_ : public oclMem_
{
    friend class oclKernel_;
    friend class oclContext_;
private:
    MAKE_ENABLER();
protected:
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id);
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr);
    virtual common::span<std::byte> MapObject(const cl_command_queue& que, const MapFlag mapFlag) override;
public:
    const size_t Size;
    virtual ~oclBuffer_();
    common::PromiseResult<void> ReadSpan(const oclCmdQue& que, common::span<std::byte> buf, const size_t offset = 0, const bool shouldBlock = true) const;
    template<typename T>
    common::PromiseResult<void> ReadSpan(const oclCmdQue& que, T& buf, const size_t offset = 0, const bool shouldBlock = true) const
    {
        return ReadSpan(que, common::as_writable_bytes(common::to_span(buf)), offset, shouldBlock);
    }
    template<typename T>
    common::PromiseResult<void> Read(const oclCmdQue& que, T& buf, const size_t offset = 0, const bool shouldBlock = true) const
    {
        return ReadSpan(que, common::span<std::byte>(reinterpret_cast<std::byte*>(&buf), sizeof(buf)), offset, shouldBlock);
    }
    common::PromiseResult<common::AlignedBuffer> Read(const oclCmdQue& que, const size_t offset = 0) const;

    common::PromiseResult<void> WriteSpan(const oclCmdQue& que, common::span<const std::byte> buf, const size_t offset = 0, const bool shouldBlock = true) const;
    template<typename T>
    common::PromiseResult<void> WriteSpan(const oclCmdQue& que, const T& buf, const size_t offset = 0, const bool shouldBlock = true) const
    {
        return WriteSpan(que, common::as_bytes(common::to_span(buf)), offset, shouldBlock);
    }
    template<typename T>
    common::PromiseResult<void> Write(const oclCmdQue& que, const T& buf, const size_t offset = 0, const bool shouldBlock = true) const
    {
        return WriteSpan(que, common::span<const std::byte>(reinterpret_cast<const std::byte*>(&buf), sizeof(buf)), offset, shouldBlock);
    }

    [[nodiscard]] static oclBuffer Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr = nullptr);
};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
