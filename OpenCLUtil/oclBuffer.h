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
class oclSubBuffer_;
using oclBuffer = std::shared_ptr<oclBuffer_>;
using oclSubBuffer = std::shared_ptr<oclSubBuffer_>;

class OCLUAPI oclSubBuffer_ : public oclMem_
{
    friend class oclKernel_;
    friend class oclContext_;
protected:
    MAKE_ENABLER();
    oclSubBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id);
    virtual common::span<std::byte> MapObject(const cl_command_queue& que, const MapFlag mapFlag) override;
public:
    const size_t Size;
    virtual ~oclSubBuffer_();
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<std::byte> buf, const size_t offset = 0) const;
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const oclCmdQue& que, common::span<std::byte> buf, const size_t offset = 0) const
    {
        return ReadSpan({}, que, buf, offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const common::PromiseStub& pmss, const oclCmdQue& que, T& buf, const size_t offset = 0) const
    {
        return ReadSpan(pmss, que, common::as_writable_bytes(common::to_span(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const oclCmdQue& que, T& buf, const size_t offset = 0) const
    {
        return ReadSpan({}, que, common::as_writable_bytes(common::to_span(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> Read(const common::PromiseStub& pmss, const oclCmdQue& que, T& buf, const size_t offset = 0) const
    {
        return ReadSpan(pmss, que, common::span<std::byte>(reinterpret_cast<std::byte*>(&buf), sizeof(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> Read(const oclCmdQue& que, T& buf, const size_t offset = 0) const
    {
        return ReadSpan({}, que, common::span<std::byte>(reinterpret_cast<std::byte*>(&buf), sizeof(buf)), offset);
    }
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> Read(const common::PromiseStub& pmss, const oclCmdQue& que, const size_t offset = 0) const;
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> Read(const oclCmdQue& que, const size_t offset = 0) const
    {
        return Read({}, que, offset);
    }

    [[nodiscard]] common::PromiseResult<void> WriteSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<const std::byte> buf, const size_t offset = 0) const;
    [[nodiscard]] common::PromiseResult<void> WriteSpan(const oclCmdQue& que, common::span<const std::byte> buf, const size_t offset = 0) const
    {
        return WriteSpan({}, que, common::as_bytes(common::to_span(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> WriteSpan(const common::PromiseStub& pmss, const oclCmdQue& que, const T& buf, const size_t offset = 0) const
    {
        return WriteSpan(pmss, que, common::as_bytes(common::to_span(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> WriteSpan(const oclCmdQue& que, const T& buf, const size_t offset = 0) const
    {
        return WriteSpan({}, que, common::as_bytes(common::to_span(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> Write(const common::PromiseStub& pmss, const oclCmdQue& que, const T& buf, const size_t offset = 0) const
    {
        return WriteSpan(pmss, que, common::span<const std::byte>(reinterpret_cast<const std::byte*>(&buf), sizeof(buf)), offset);
    }
    template<typename T>
    [[nodiscard]] common::PromiseResult<void> Write(const oclCmdQue& que, const T& buf, const size_t offset = 0) const
    {
        return WriteSpan({}, que, common::span<const std::byte>(reinterpret_cast<const std::byte*>(&buf), sizeof(buf)), offset);
    }

};


class OCLUAPI oclBuffer_ : public oclSubBuffer_
{
    friend class oclKernel_;
    friend class oclContext_;
private:
    MAKE_ENABLER();
protected:
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr);
    oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id);
public:
    [[nodiscard]] oclSubBuffer CreateSubBuffer(const size_t offset, const size_t size, MemFlag flag) const;
    [[nodiscard]] oclSubBuffer CreateSubBuffer(const size_t offset, const size_t size) const
    { 
        return CreateSubBuffer(offset, size, Flag);
    }
    [[nodiscard]] static oclBuffer Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr = nullptr);
};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif
