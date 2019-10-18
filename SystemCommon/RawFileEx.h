#pragma once
#include "SystemCommonRely.h"
#include "common/FileBase.hpp"

#include <cstdio>


namespace common::file
{
class FileMappingObject;


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI RawFileObject : public NonCopyable, public NonMovable
{
    friend class FileMappingObject;
    friend class RawFileStream;
public:
#if defined(_WIN32)
    using HandleType = void*;
#else
    using HandleType = int32_t;
#endif
private:
    MAKE_ENABLER();
    fs::path FilePath;
    HandleType FileHandle;
    OpenFlag Flag;

    RawFileObject(const fs::path& path, const HandleType fileHandle, const OpenFlag flag);
public:
    ~RawFileObject();

    const fs::path& Path() const { return FilePath; }
    std::u16string ExtName() const { return FilePath.extension().u16string(); }

    //==========Open=========//

    static std::shared_ptr<RawFileObject> OpenFile(const fs::path& path, const OpenFlag flag);
    static std::shared_ptr<RawFileObject> OpenThrow(const fs::path& path, const OpenFlag flag);
};


class SYSCOMMONAPI RawFileStream
{
protected:

    std::shared_ptr<RawFileObject> File;

    RawFileStream(std::shared_ptr<RawFileObject>&& file) noexcept;
    ~RawFileStream();

    RawFileObject::HandleType GetHandle() const;
    void WriteCheck() const;
    void ReadCheck() const;
    bool FSeek(const int64_t offset, const SeekWhere whence);
    size_t LeftSpace();

    //==========RandomStream=========//
    size_t GetSize();
    size_t CurrentPos() const;
    bool SetPos(const size_t offset);
};


class SYSCOMMONAPI RawFileInputStream : private RawFileStream, public io::RandomInputStream
{
public:
    RawFileInputStream(std::shared_ptr<RawFileObject> file);
    RawFileInputStream(RawFileInputStream&& stream) noexcept;
    virtual ~RawFileInputStream() override;

    //==========InputStream=========//
    virtual size_t AvaliableSpace() override;
    virtual bool Read(const size_t len, void* ptr) override;
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override;
    virtual bool Skip(const size_t len) override;
    virtual bool IsEnd() override;

    //==========RandomStream=========//
    virtual size_t GetSize() override;
    virtual size_t CurrentPos() const override;
    virtual bool SetPos(const size_t offset) override;
};


class SYSCOMMONAPI RawFileOutputStream : private RawFileStream, public io::RandomOutputStream
{
public:
    RawFileOutputStream(std::shared_ptr<RawFileObject> file);
    RawFileOutputStream(RawFileOutputStream&& stream) noexcept;
    virtual ~RawFileOutputStream() override;

    //==========OutputStream=========//
    virtual size_t AcceptableSpace() override;
    virtual bool Write(const size_t len, const void* ptr) override;
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override;
    virtual void Flush() override;

    //==========RandomStream=========//
    virtual size_t GetSize() override;
    virtual size_t CurrentPos() const override;
    virtual bool SetPos(const size_t offset) override;
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}
