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

enum class OpenMappingFlag : uint8_t
{
    READ = 0b1, WRITE = 0b10, CREATE = 0b100, TEXT = 0b00000, BINARY = 0b10000,
    APPEND = 0b1110, TRUNC = 0b0110,
    CreateNewBinary = CREATE | WRITE | BINARY, CreateNewText = CREATE | WRITE | TEXT,
};
MAKE_ENUM_BITFIELD(OpenMappingFlag)


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI FileMappingObject : public NonCopyable, public NonMovable
{
    friend class FileMappingStream;
private:
    MAKE_ENABLER();
#if defined(_WIN32)
    using HandleType = void*;
#else
    using HandleType = int32_t;
#endif
    std::shared_ptr<RawFileObject> RawFile;
    std::byte* Ptr;
    HandleType MappingHandle;
    OpenMappingFlag Flag;

    FileMappingObject(const fs::path& path, const HandleType fileHandle, const OpenMappingFlag flag);
public:
    ~FileMappingObject();

    const std::shared_ptr<RawFileObject>& GetRawFile() const { return RawFile; }
    //==========Open=========//

    static std::shared_ptr<FileMappingObject> OpenMapping(std::shared_ptr<RawFileObject> rawFile, const OpenMappingFlag flag);
    static std::shared_ptr<FileMappingObject> OpenThrow(std::shared_ptr<RawFileObject> rawFile, const OpenMappingFlag flag);
};


class SYSCOMMONAPI FileMappingStream
{
protected:
    std::shared_ptr<FileMappingObject> MappingObject;

    FileMappingStream(std::shared_ptr<FileMappingObject>&& file) noexcept;
    ~FileMappingStream();

    void WriteCheck() const;
    void ReadCheck() const;
    void CheckError(FileErrReason fileop);
    size_t LeftSpace();

    //==========RandomStream=========//
    size_t GetSize();
    size_t CurrentPos() const;
    bool SetPos(const size_t offset);
};


class SYSCOMMONAPI FileMappingInputStream : private FileMappingStream, public io::MemoryInputStream
{
public:
    FileMappingInputStream(std::shared_ptr<FileMappingObject> file);
    FileMappingInputStream(FileMappingInputStream&& stream) noexcept;
    virtual ~FileMappingInputStream() override;

    //==========InputStream=========//
    virtual size_t AvaliableSpace() override;
    virtual bool Read(const size_t len, void* ptr) override;
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override;
    virtual bool Skip(const size_t len) override;
    virtual bool IsEnd() override;
    virtual std::byte ReadByteNE(bool& isSuccess) override;
    virtual std::byte ReadByteME() override;

    //==========RandomStream=========//
    virtual size_t GetSize() override;
    virtual size_t CurrentPos() const override;
    virtual bool SetPos(const size_t offset) override;
};


class SYSCOMMONAPI FileMappingOutputStream : private FileMappingStream, public io::MemoryOutputStream
{
public:
    FileMappingOutputStream(std::shared_ptr<FileMappingObject> file);
    FileMappingOutputStream(FileMappingOutputStream&& stream) noexcept;
    virtual ~FileMappingOutputStream() override;
    
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
