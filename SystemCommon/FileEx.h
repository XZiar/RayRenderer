#pragma once
#include "SystemCommonRely.h"
#include "common/FileBase.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"

#include <cstdio>
#include <vector>


namespace common::file
{


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI FileObject : public NonCopyable, public NonMovable
{
    friend class FileStream;
private:
    MAKE_ENABLER();

    fs::path FilePath;
    FILE* FHandle;
    OpenFlag Flag;

    FileObject(const fs::path& path, FILE* fp, const OpenFlag flag);
public:
    ~FileObject();

    const fs::path& Path() const { return FilePath; }
    std::u16string ExtName() const { return FilePath.extension().u16string(); }
    FILE* Raw() { return FHandle; }

    //==========Open=========//

    static std::shared_ptr<FileObject> OpenFile(const fs::path& path, const OpenFlag flag);
    static std::shared_ptr<FileObject> OpenThrow(const fs::path& path, const OpenFlag flag);
};


class SYSCOMMONAPI FileStream
{
protected:
    std::shared_ptr<FileObject> File;

    FileStream(std::shared_ptr<FileObject>&& file) noexcept;
    ~FileStream();

    bool FSeek(const int64_t offset, const SeekWhere whence);
    size_t FTell() const;
    FILE* GetFP() const;
    void WriteCheck() const;
    void ReadCheck() const;
    void CheckError(FileErrReason fileop);
    size_t LeftSpace();

    //==========RandomStream=========//
    size_t GetSize();
    size_t CurrentPos() const;
    bool SetPos(const size_t offset);
};


class SYSCOMMONAPI FileInputStream : private FileStream, public io::RandomInputStream
{
public:
    FileInputStream(std::shared_ptr<FileObject> file);
    FileInputStream(FileInputStream&& stream) noexcept;
    virtual ~FileInputStream() override;

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


class SYSCOMMONAPI FileOutputStream : private FileStream, public io::RandomOutputStream
{
public:
    FileOutputStream(std::shared_ptr<FileObject> file);
    FileOutputStream(FileOutputStream&& stream) noexcept;
    virtual ~FileOutputStream() override;
    
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

template<typename T>
inline void ReadAll(const fs::path& fpath, T& output)
{
    FileInputStream fis(FileObject::OpenThrow(fpath, OpenFlag::ReadBinary));
    fis.ReadInto(output, SIZE_MAX);
}

template<typename T>
inline std::vector<T> ReadAll(const fs::path& fpath)
{
    std::vector<T> output;
    ReadAll(fpath, output);
    return output;
}

template<typename Char = char>
inline std::basic_string<Char> ReadAllText(const fs::path& fpath)
{
    std::basic_string<Char> output;
    ReadAll(fpath, output);
    return output;
}


template<typename T>
inline void WriteAll(const fs::path& fpath, const T& input)
{
    using Helper = common::container::ContiguousHelper<T>;
    static_assert(Helper::IsContiguous, "WriteAll Only accept contiguous type");
    FileOutputStream fis(FileObject::OpenThrow(fpath, OpenFlag::CreateNewBinary));
    fis.Write(Helper::Count(input) * Helper::EleSize, Helper::Data(input));
}

}
