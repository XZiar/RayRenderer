#include "ResourcePackagerRely.h"
#include "SerializeUtil.h"
#include "ResourceUtil.h"

using common::container::FindInMap;

namespace xziar::respak
{

namespace detail
{

string ResourceItem::ExtractHandle() const
{
    return "@" + ResourceUtil::Hex2Str(SHA256);
}

uint64_t ResourceItem::GetSize() const
{
    uint64_t num = 0;
    for (uint8_t i = 0; i < 8; ++i)
        num = num * 256 + Size[i];
    return num;
}

string BlobPool::PutResource(const void * data, const size_t size)
{
    ResourceItem metadata;
    metadata.SHA256 = ResourceUtil::SHA256(data, size);
    ToLE(CurIndex, metadata.Index);
    ToLE(CurOffset, metadata.Offset);
    ToLE(size, metadata.Size);

    const auto findres = FindInMap(ResourceSet, metadata.SHA256, std::in_place);
    if (findres)
    {
        const auto& oldmeta = ResourceList[findres.value()];
        if (oldmeta.GetSize() == size)
            return oldmeta.ExtractHandle();
        COMMON_THROW(BaseException, L"Hash collision detected!");
    }

    Write(&metadata, sizeof(metadata));
    ResourceList.push_back(metadata);
    CurIndex++;
    CurOffset += sizeof(metadata) + size;
    Write(data, size);
    return metadata.ExtractHandle();
}

void BlobPool::EndWrite()
{
    const auto resSize = CurOffset;
    Write(ResourceList.data(), CurIndex * sizeof(ResourceItem));
    ResourceItem sumdata;
    sumdata.SHA256 = ResourceUtil::SHA256(ResourceList.data(), CurIndex * sizeof(ResourceItem));
    ToLE(CurIndex, sumdata.Index);
    ToLE(CurOffset, sumdata.Offset);
    sumdata.Dummy[0] = 'X';
    sumdata.Dummy[1] = 'Z';
    sumdata.Dummy[2] = 'P';
    sumdata.Dummy[3] = 'K';
    Write(&sumdata, sizeof(sumdata));
}

}

SerializeUtil::SerializeUtil(const fs::path& fileName)
    : DocWriter(FileObject::OpenThrow(fs::path(fileName).replace_extension(u".xzrp.json"), OpenFlag::CREATE | OpenFlag::BINARY | OpenFlag::WRITE), 65536),
    ResBlob(std::make_unique<detail::FileBlobPool>(fs::path(fileName).replace_extension(u".xzrp")))
{
    auto config = ResDoc.NewObject();
    config.Add("identity", "xziar-respak");
    config.Add("version", 0.1);
    ResDoc.Add("#config", config);
}


SerializeUtil::~SerializeUtil()
{
    DocWriter.Flush();
}

void SerializeUtil::AddObject(const string & name, const detail::Serializable & object)
{
    if (name.empty() || name[0] == '#')
        COMMON_THROW(BaseException, L"invalid node name");
    const auto objType = object.SerializedType();
    auto result = object.Serialize(ResDoc, *ResBlob);
    result.Add("#Type", objType);
    ResDoc.Add(name.data(), result);
    
}

void SerializeUtil::Finish()
{
    ResBlob->EndWrite();
    ResDoc.Stringify(DocWriter);
    DocWriter.Flush();
}


}


