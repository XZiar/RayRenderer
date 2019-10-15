#pragma once
#include "RenderCoreRely.h"
#include "ModelMesh.h"
#include "fmt/chrono.h"



namespace rayr::detail
{


class OBJSaver
{
private:
    static constexpr std::byte BOM_UTF16LE[2] = { std::byte(0xff), std::byte(0xfe) };
    const std::shared_ptr<_ModelMesh> Mesh;
    common::fs::path PathObj;
    common::fs::path PathMtl;
public:
    OBJSaver(const common::fs::path& fpath, const std::shared_ptr<_ModelMesh>& mesh) : Mesh(mesh)
    {
        PathObj = fpath;
        PathObj.replace_extension(".obj");
        PathMtl = fpath;
        PathMtl.replace_extension(".mtl");
    }
    void WriteMtl() const
    {
        using namespace common::file;
        FileOutputStream mtlFile(FileObject::OpenThrow(PathMtl, OpenFlag::CreatNewBinary));
        mtlFile.WriteFrom(BOM_UTF16LE);
        const auto header = fmt::format(u"#XZiar Dizz Renderer MTL Exporter\r\n#Created at {:%Y-%m-%d %H:%M:%S}\r\n\r\n", 
            common::SimpleTimer::getCurLocalTime());
        mtlFile.Write(header.size() * sizeof(char16_t), header.data());
    }
    void WriteObj() const
    {
        using namespace common::file;
        FileOutputStream objFile(FileObject::OpenThrow(PathObj, OpenFlag::CreatNewBinary));
        objFile.WriteFrom(BOM_UTF16LE);
        const auto header = fmt::format(u"#XZiar Dizz Renderer OBJ Exporter\r\n#Created at {:%Y-%m-%d %H:%M:%S}\r\n\r\n", 
            common::SimpleTimer::getCurLocalTime());
        objFile.Write(header.size() * sizeof(char16_t), header.data());
    }
};


}
