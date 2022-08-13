#pragma once
#include "SystemCommonRely.h"
#include "common/FileBase.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"
#include "Exceptions.h"

#include <cstdio>
#include <vector>


namespace common::file
{


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class SYSCOMMONAPI FileException : public BaseException
{
    COMMON_EXCEPTION_PREPARE(FileException, BaseException,
        fs::path Filepath;
        FileErrReason Reason;
        ExceptionInfo(const std::u16string_view msg, const fs::path& filepath, const FileErrReason reason)
            : TPInfo(TYPENAME, msg), Filepath(filepath), Reason(reason)
        { }
    );
    FileException(const FileErrReason why, const fs::path& file, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, file, why)
    { }
    FileErrReason Reason() const noexcept { return GetInfo().Reason; }
    const fs::path& FilePath() const noexcept { return GetInfo().Filepath; }
};


class FileObject
{
    friend class FileStream;
private:
    MAKE_ENABLER();

    fs::path FilePath;
    FILE* FHandle;
    OpenFlag Flag;

    SYSCOMMONAPI FileObject(const fs::path& path, FILE* fp, const OpenFlag flag);
public:
    COMMON_NO_COPY(FileObject)
    COMMON_NO_MOVE(FileObject)
    SYSCOMMONAPI ~FileObject();

    [[nodiscard]] const fs::path& Path() const { return FilePath; }
    [[nodiscard]] std::u16string ExtName() const { return FilePath.extension().u16string(); }
    [[nodiscard]] FILE* Raw() { return FHandle; }

    //==========Open=========//

    SYSCOMMONAPI [[nodiscard]] static std::shared_ptr<FileObject> OpenFile(const fs::path& path, const OpenFlag flag);
    SYSCOMMONAPI [[nodiscard]] static std::shared_ptr<FileObject> OpenThrow(const fs::path& path, const OpenFlag flag);
};


class SYSCOMMONAPI FileStream
{
protected:
    std::shared_ptr<FileObject> File;

    FileStream(std::shared_ptr<FileObject>&& file) noexcept;
    ~FileStream();

    bool FSeek(const int64_t offset, const SeekWhere whence);
    [[nodiscard]] size_t FTell() const;
    [[nodiscard]] FILE* GetFP() const;
    void WriteCheck() const;
    void ReadCheck() const;
    void CheckError(FileErrReason fileop);
    [[nodiscard]] size_t LeftSpace();

    //==========RandomStream=========//
    [[nodiscard]] size_t GetSize();
    [[nodiscard]] size_t CurrentPos() const;
    bool SetPos(const size_t offset);
};


class SYSCOMMONAPI FileInputStream : private FileStream, public io::RandomInputStream
{
public:
    FileInputStream(std::shared_ptr<FileObject> file);
    FileInputStream(FileInputStream&& stream) noexcept;
    virtual ~FileInputStream() override;

    //==========InputStream=========//
    [[nodiscard]] virtual size_t AvaliableSpace() override;
    virtual bool Read(const size_t len, void* ptr) override;
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override;
    virtual bool Skip(const size_t len) override;
    [[nodiscard]] virtual bool IsEnd() override;
    [[nodiscard]] virtual std::byte ReadByteNE(bool& isSuccess) override;
    [[nodiscard]] virtual std::byte ReadByteME() override;

    //==========RandomStream=========//
    [[nodiscard]] virtual size_t GetSize() override;
    [[nodiscard]] virtual size_t CurrentPos() const override;
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
    [[nodiscard]] virtual size_t GetSize() override;
    [[nodiscard]] virtual size_t CurrentPos() const override;
    virtual bool SetPos(const size_t offset) override;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

template<typename T>
inline void ReadAll(const fs::path& fpath, T& output)
{
    FileInputStream fis(FileObject::OpenThrow(fpath, OpenFlag::ReadBinary));
    fis.ReadTo(output, SIZE_MAX);
}

template<typename T>
[[nodiscard]] inline std::vector<T> ReadAll(const fs::path& fpath)
{
    std::vector<T> output;
    ReadAll(fpath, output);
    return output;
}

template<typename Char = char>
[[nodiscard]] inline std::basic_string<Char> ReadAllText(const fs::path& fpath)
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
