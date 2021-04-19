#pragma once
#include "SystemCommonRely.h"
#include "RawFileEx.h"
#include "common/FileBase.hpp"
#include "common/MemoryStream.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"

#include <cstdio>
#include <vector>


namespace common::file
{

enum class MappingFlag : uint8_t
{
    ReadOnly, ReadWrite, CopyOnWrite
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI FileMappingObject : public NonCopyable, public NonMovable
{
    friend class FileMappingStream;
public:
#if COMMON_OS_WIN
    using HandleType = std::pair<void*, void*>;
#else
    using HandleType = void*;
#endif
private:
    MAKE_ENABLER();
    std::shared_ptr<RawFileObject> RawFile;
    uint64_t Offset;
    HandleType MappingHandle;
    std::byte* Ptr;
    size_t Size;
    MappingFlag Flag;

    FileMappingObject(std::shared_ptr<RawFileObject> file, const MappingFlag flag,
        const HandleType handle, const uint64_t offset, std::byte* ptr, const size_t size);
    RawFileObject::HandleType GetRawFileHandle() const noexcept { return RawFile->FileHandle; }
public:
    ~FileMappingObject();

    [[nodiscard]] constexpr const std::shared_ptr<RawFileObject>& GetRawFile() const noexcept { return RawFile; }
    [[nodiscard]] const fs::path& Path() const noexcept { return RawFile->FilePath; }
    //==========Open=========//

    [[nodiscard]] static std::shared_ptr<FileMappingObject> OpenMapping(std::shared_ptr<RawFileObject> rawFile, const MappingFlag flag,
        const std::pair<uint64_t, uint64_t>& region = { 0, UINT64_MAX });
    [[nodiscard]] static std::shared_ptr<FileMappingObject> OpenThrow(std::shared_ptr<RawFileObject> rawFile, const MappingFlag flag,
        const std::pair<uint64_t, uint64_t>& region = { 0, UINT64_MAX });
};


class SYSCOMMONAPI FileMappingStream
{
protected:
    std::shared_ptr<FileMappingObject> MappingObject;

    FileMappingStream(std::shared_ptr<FileMappingObject>&& mapping) noexcept;
    ~FileMappingStream();

    void WriteCheck() const;
    void ReadCheck() const;
    [[nodiscard]] common::span<std::byte> GetSpan() const noexcept;
    void FlushRange(const size_t offset, const size_t len, const bool async = true) const noexcept;
};


class SYSCOMMONAPI FileMappingInputStream : private FileMappingStream, public io::MemoryInputStream
{
public:
    FileMappingInputStream(std::shared_ptr<FileMappingObject> mapping);
    FileMappingInputStream(FileMappingInputStream&& stream) noexcept;
    virtual ~FileMappingInputStream() override;
};


class SYSCOMMONAPI FileMappingOutputStream : private FileMappingStream, public io::MemoryOutputStream
{
private:
    std::pair<size_t, size_t> ModifyRange;
public:
    FileMappingOutputStream(std::shared_ptr<FileMappingObject> mapping);
    FileMappingOutputStream(FileMappingOutputStream&& stream) noexcept;
    virtual ~FileMappingOutputStream() override;
    virtual bool SetPos(const size_t pos) noexcept override;
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override;
    virtual void Flush() noexcept override;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


SYSCOMMONAPI FileMappingInputStream MapFileForRead(const fs::path& fpath);

}
