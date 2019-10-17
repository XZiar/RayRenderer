#include "RawFileEx.h"
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN 1
# define NOMINMAX 1
# include <Windows.h>
#else
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>


namespace common::file
{
using std::string;
using std::u16string;
using std::byte;
MAKE_ENABLER_IMPL(RawFileObject)


RawFileObject::RawFileObject(const fs::path& path, const HandleType fileHandle, const OpenFlag flag) :
    FilePath(path), FileHandle(fileHandle), Flag(flag)
{
}
RawFileObject::~RawFileObject()
{
#if defined(_WIN32)
    CloseHandle(FileHandle);
#else
    close(FileHandle);
#endif
}

std::shared_ptr<RawFileObject> RawFileObject::OpenFile(const fs::path& path, const OpenFlag flag)
{
    if (!fs::exists(path) && !HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
        return {};
#if defined(_WIN32)
    DWORD accessMode = 0L, shareMode = 0L, createMode = 0, realFlag = FILE_ATTRIBUTE_NORMAL;

    if (HAS_FIELD(flag, OpenFlag::FLAG_READ))           accessMode |= GENERIC_READ;
    if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))          accessMode |= GENERIC_WRITE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_SHARE_READ))     shareMode  |= FILE_SHARE_READ;
    if (HAS_FIELD(flag, OpenFlag::FLAG_SHARE_WRITE))    shareMode  |= FILE_SHARE_WRITE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))     realFlag   |= FILE_FLAG_NO_BUFFERING;
    if (HAS_FIELD(flag, OpenFlag::FLAG_WriteThrough))   realFlag   |= FILE_FLAG_WRITE_THROUGH;
    if (HAS_FIELD(flag, OpenFlag::FLAG_DeleteOnClose))  realFlag   |= FILE_FLAG_DELETE_ON_CLOSE;

    if (HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
        createMode = HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? CREATE_ALWAYS      : CREATE_NEW;
    else
        createMode = HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? TRUNCATE_EXISTING  : OPEN_EXISTING;

    auto handle = CreateFile(
        path.wstring().c_str(), 
        accessMode, 
        shareMode, 
        NULL, 
        createMode,
        realFlag, 
        nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return {};
#else
    int realFlag = O_LARGEFILE;
    if (HAS_FIELD(flag, OpenFlag::FLAG_READ))
    {
        if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))
            realFlag |= O_RDWR;
        else
            realFlag |= O_RDONLY;
    }
    else
    {
        if (HAS_FIELD(flag, OpenFlag::FLAG_WRITE))
            realFlag |= O_WRONLY;
    }

    if (HAS_FIELD(flag, OpenFlag::FLAG_APPEND))
        realFlag |= O_APPEND;

    if (HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
    {
        realFlag |= O_CREAT;
        realFlag |= HAS_FIELD(flag, OpenFlag::FLAG_TRUNC) ? O_TRUNC : O_EXCL;
    }
    else if (HAS_FIELD(flag, OpenFlag::FLAG_TRUNC))
        realFlag |= O_TRUNC;

    if (HAS_FIELD(flag, OpenFlag::FLAG_WriteThrough))
        realFlag |= O_DSYNC;

    if (HAS_FIELD(flag, OpenFlag::FLAG_DeleteOnClose))
        realFlag |= O_CLOEXEC;

# if !defined(__APPLE__)
    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))
        realFlag |= O_DIRECT;
# endif

    auto handle = open(path.string().c_str(), realFlag);

# if defined(__APPLE__)
    if (HAS_FIELD(flag, OpenFlag::FLAG_DontBuffer))
        fcntl(handle, F_NOCACHE, 1);
# endif
    if (handle == -1)
        return {};
#endif
    return MAKE_ENABLER_SHARED(RawFileObject, path, handle, flag);
}

std::shared_ptr<RawFileObject> RawFileObject::OpenThrow(const fs::path& path, const OpenFlag flag)
{
    if (!fs::exists(path) && !HAS_FIELD(flag, OpenFlag::FLAG_CREATE))
        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist, path, u"target file not exist");
    auto obj = OpenFile(path, flag);
    if (obj)
        return obj;
#if defined(_WIN32)
    switch (GetLastError())
    {
    default:        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::UnknowErr,         path, u"cannot open target file");
    }
#else
    error_t err = errno;
    switch (err)
    {
    case ENOENT:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::NotExist,          path, u"cannot open target file, not exists");
    case ENOMEM:    throw std::bad_alloc(/*"fopen reported no enough memory error"*/);
    case EACCES:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::PermissionDeny,    path, u"cannot open target file, no permission");
    case EEXIST:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::AlreadyExist,      path, u"cannot open target file, already exists");
    case EISDIR:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::IsDir,             path, u"cannot open target file, is a directory");
    case EINVAL:    COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::WrongParam,        path, u"cannot open target file, invalid params");
    default:        COMMON_THROW(FileException, FileErrReason::OpenFail | FileErrReason::UnknowErr,         path, u"cannot open target file");
    }
#endif
}


}
