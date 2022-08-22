#pragma once
#include "SystemCommonRely.h"
#include "FileEx.h"

#include <cstdio>


namespace common::file
{
class FileMappingObject;


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class RawFileObject
{
    friend class FileMappingObject;
    friend class RawFileStream;
public:
#if COMMON_OS_WIN
    using HandleType = void*;
#else
    using HandleType = int32_t;
#endif
private:
    MAKE_ENABLER();
    fs::path FilePath;
    HandleType FileHandle;
    OpenFlag Flag;

    SYSCOMMONAPI RawFileObject(const fs::path& path, const HandleType fileHandle, const OpenFlag flag);
public:
    COMMON_NO_COPY(RawFileObject)
    COMMON_NO_MOVE(RawFileObject)
    SYSCOMMONAPI ~RawFileObject();

    [[nodiscard]] const fs::path& Path() const { return FilePath; }
    [[nodiscard]] std::u16string ExtName() const { return FilePath.extension().u16string(); }

    //==========Open=========//

    SYSCOMMONAPI [[nodiscard]] static std::shared_ptr<RawFileObject> OpenFile(const fs::path& path, const OpenFlag flag);
    SYSCOMMONAPI [[nodiscard]] static std::shared_ptr<RawFileObject> OpenThrow(const fs::path& path, const OpenFlag flag);
};


class RawFileStream
{
protected:

    std::shared_ptr<RawFileObject> File;

    RawFileStream(std::shared_ptr<RawFileObject>&& file) noexcept;
    ~RawFileStream();

    [[nodiscard]] RawFileObject::HandleType GetHandle() const;
    void WriteCheck() const;
    void ReadCheck() const;
    bool FSeek(const int64_t offset, const SeekWhere whence);
    [[nodiscard]] size_t LeftSpace();

    //==========RandomStream=========//
    [[nodiscard]] size_t GetSize();
    [[nodiscard]] size_t CurrentPos() const;
    bool SetPos(const size_t offset);
};


class SYSCOMMONAPI RawFileInputStream : private RawFileStream, public io::RandomInputStream
{
public:
    RawFileInputStream(std::shared_ptr<RawFileObject> file);
    RawFileInputStream(RawFileInputStream&& stream) noexcept;
    ~RawFileInputStream() override;

    //==========InputStream=========//
    [[nodiscard]] size_t AvaliableSpace() override;
    bool Read(const size_t len, void* ptr) override;
    size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override;
    bool Skip(const size_t len) override;
    [[nodiscard]] bool IsEnd() override;

    //==========RandomStream=========//
    [[nodiscard]] size_t GetSize() override;
    [[nodiscard]] size_t CurrentPos() const override;
    bool SetPos(const size_t offset) override;
};


class SYSCOMMONAPI RawFileOutputStream : private RawFileStream, public io::RandomOutputStream
{
public:
    RawFileOutputStream(std::shared_ptr<RawFileObject> file);
    RawFileOutputStream(RawFileOutputStream&& stream) noexcept;
    ~RawFileOutputStream() override;

    //==========OutputStream=========//
    size_t AcceptableSpace() override;
    bool Write(const size_t len, const void* ptr) override;
    size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override;
    void Flush() override;

    //==========RandomStream=========//
    [[nodiscard]] size_t GetSize() override;
    [[nodiscard]] size_t CurrentPos() const override;
    bool SetPos(const size_t offset) override;
};


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}
