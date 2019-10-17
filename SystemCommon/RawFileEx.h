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
private:
    MAKE_ENABLER();
#if defined(_WIN32)
    using HandleType = void*;
#else
    using HandleType = int32_t;
#endif
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


}
