#include "ResourcePackagerRely.h"
#include "SerializeUtil.h"
#include "ResourceUtil.h"
#include "common/Linq.hpp"
#include "3rdParty/rapidjson/pointer.h"
#include "3rdParty/boost.stacktrace/stacktrace.h"

// ResFile Structure
// |------------|
// |ResourceItem|
// | resource 1 |
// |------------|
// |   ......   |
// |------------|
// |ResourceItem|
// | resource N |
// |------------|
// |  res list  |
// |ResourceItem|
// |------------|

using common::linq::Linq;
using common::container::FindInMap;

static constexpr std::string_view TypeFieldName = "#Type";

namespace xziar::respak
{
static constexpr size_t RESITEM_SIZE = sizeof(detail::ResourceItem);

namespace detail
{

ResourceItem::ResourceItem(bytearray<32>&& sha256, const uint64_t size, const uint64_t offset, const uint32_t index)
    : SHA256(sha256), Size(ToLEByteArray(size)), Offset(ToLEByteArray(offset)), Index(ToLEByteArray(index))
{ }

string ResourceItem::ExtractHandle() const
{
    return "@" + ResourceUtil::Hex2Str(SHA256);
}

uint64_t ResourceItem::GetSize() const
{
    return FromLEByteArray<uint64_t>(Size);
}
uint64_t ResourceItem::GetOffset() const
{
    return FromLEByteArray<uint64_t>(Offset);
}
uint32_t ResourceItem::GetIndex() const
{
    return FromLEByteArray<uint32_t>(Index);
}
bool ResourceItem::CheckSHA(const bytearray<32>& sha256) const
{
    for (uint32_t i = 0; i < 32; ++i)
    {
        if (SHA256[i] != sha256[i])
            return false;
    }
    return true;
}
bool ResourceItem::operator==(const ResourceItem& other) const
{
    return memcmp(this, &other, RESITEM_SIZE) == 0;
}
bool ResourceItem::operator!=(const ResourceItem& other) const
{
    return !operator==(other);
}

}

SerializeUtil::SerializeUtil(const fs::path& fileName)
    : DocWriter(FileObject::OpenThrow(fs::path(fileName).replace_extension(u".xzrp.json"), OpenFlag::CREATE | OpenFlag::BINARY | OpenFlag::WRITE), 65536),
    ResWriter(FileObject::OpenThrow(fs::path(fileName).replace_extension(u".xzrp"), OpenFlag::CREATE | OpenFlag::BINARY | OpenFlag::WRITE), 65536),
    Root(DocRoot)
{
    auto config = DocRoot.NewObject();
    config.Add("identity", "xziar-respak");
    config.Add("version", 0.1);
    DocRoot.Add("#config", config);
}


SerializeUtil::~SerializeUtil()
{
    DocWriter.Flush();
}

ejson::JObject SerializeUtil::Serialize(const Serializable & object)
{
    auto result = object.Serialize(*this);
    result.Add(TypeFieldName, object.SerializedType());
    return result;
}

void SerializeUtil::CheckFinished() const
{
    if (HasFinished)
        COMMON_THROW(BaseException, u"Serializer has finished and unable to add more data");
}

static void CheckName(const string& name)
{
    if (name.empty() || name[0] == '#')
        COMMON_THROW(BaseException, u"invalid node name");
}

void SerializeUtil::AddObject(const string& name, ejson::JDoc& node)
{
    CheckName(name);
    DocRoot.Add(name, node);
}

string SerializeUtil::AddObject(const Serializable& object)
{
    CheckFinished();
    const auto objptr = reinterpret_cast<intptr_t>(&object);
    if (const auto it = FindInMap(ObjectLookup, objptr); it != nullptr)
        return *it;

    auto& mempool = DocRoot.GetMemPool();

    string path;
    for (const auto& filter : Filters)
    {
        path = filter(*this, object);
        if (!path.empty()) break;
    }
    if (path.empty())
        path = "/#globals";
    bool isExist = false;
    auto& target = rapidjson::Pointer(path.data(), path.size()).Create(DocRoot.ValRef(), mempool, &isExist);
    if (!isExist)
        target.SetObject();

    const auto strptr = std::to_string(objptr);
    target.AddMember(ejson::SharedUtil::ToJString(strptr, mempool), static_cast<rapidjson::Value>(Serialize(object)), mempool);
    string id = '$' + strptr;
    ObjectLookup.emplace(objptr, id);
    return id;
}

string SerializeUtil::LookupObject(const Serializable & object) const
{
    const auto objptr = reinterpret_cast<intptr_t>(&object);
    const auto it = FindInMap(ObjectLookup, objptr);
    return it ? *it : "";
}

void SerializeUtil::AddObject(ejson::JObject& target, const string & name, const Serializable & object)
{
    CheckFinished();
    CheckName(name);
    target.Add(name, Serialize(object));
}
void SerializeUtil::AddObject(ejson::JObjectRef<false>& target, const string & name, const Serializable & object)
{
    CheckFinished();
    CheckName(name);
    target.Add(name, Serialize(object));
}

void SerializeUtil::AddObject(ejson::JArray& target, const Serializable & object)
{
    CheckFinished();
    target.Push(Serialize(object));
}
void SerializeUtil::AddObject(ejson::JArrayRef<false>& target, const Serializable & object)
{
    CheckFinished();
    target.Push(Serialize(object));
}

string SerializeUtil::PutResource(const void * data, const size_t size, const string& id)
{
    CheckFinished();
    detail::ResourceItem metadata(ResourceUtil::SHA256(data, size), size, ResOffset, ResCount);

    const auto findres = FindInMap(ResourceSet, metadata.SHA256, std::in_place);
    if (findres)
        return ResourceList[findres.value()].ExtractHandle();
    ResourceSet.try_emplace(metadata.SHA256, ResCount);
    ResWriter.Write(metadata);
    ResWriter.Write(size, data);
    ResourceList.push_back(metadata);
    if (!id.empty())
        ResourceLookup.insert_or_assign(id, ResCount);
    ResCount++;
    ResOffset += RESITEM_SIZE + size;
    return metadata.ExtractHandle();
}

string SerializeUtil::LookupResource(const string & id) const
{
    const auto it = FindInMap(ResourceLookup, id, std::in_place);
    return it ? ResourceList[it.value()].ExtractHandle() : "";
}

void SerializeUtil::Finish()
{
    CheckFinished();
    ResWriter.Write(ResCount * RESITEM_SIZE, ResourceList.data());
    detail::ResourceItem sumdata(ResourceUtil::SHA256(ResourceList.data(), ResourceList.size() * RESITEM_SIZE), 0, ResOffset, ResCount);
    sumdata.Dummy[0] = byte('X');
    sumdata.Dummy[1] = byte('Z');
    sumdata.Dummy[2] = byte('P');
    sumdata.Dummy[3] = byte('K');
    ResWriter.Write(sumdata);
    ResWriter.Flush();
    DocRoot.Stringify(DocWriter, IsPretty);
    DocWriter.Flush();
    HasFinished = true;
}


std::unordered_map<std::string_view, DeserializeUtil::DeserializeFunc>& DeserializeUtil::DeserializeMap()
{
    static std::unordered_map<std::string_view, DeserializeFunc> desMap;
    return desMap;
}
uint32_t DeserializeUtil::RegistDeserializer(const std::string_view& type, const DeserializeFunc& func)
{
    DeserializeMap()[type] = func;
    return 0;
}

DeserializeUtil::DeserializeUtil(const fs::path & fileName)
    : ResReader(FileObject::OpenThrow(fs::path(fileName).replace_extension(u".xzrp"), OpenFlag::BINARY | OpenFlag::READ), 65536),
    DocRoot(ejson::JDoc::Parse(common::file::ReadAllText(fs::path(fileName).replace_extension(u".xzrp.json")))),
    Root(ejson::JObjectRef<true>(DocRoot))
{
    const auto size = ResReader.GetSize();
    if (size < RESITEM_SIZE)
        COMMON_THROWEX(BaseException, u"wrong respak size");
    detail::ResourceItem sumdata;
    ResReader.Rewind(size - RESITEM_SIZE);
    ResReader.Read(sumdata);
    if (sumdata.Dummy[0] != byte('X') || sumdata.Dummy[1] != byte('Z') || sumdata.Dummy[2] != byte('P') || sumdata.Dummy[3] != byte('K'))
        COMMON_THROWEX(BaseException, u"wrong respak signature");
    const auto itemcount = sumdata.GetIndex();
    const auto offset = sumdata.GetOffset();
    const size_t indexsize = itemcount * RESITEM_SIZE;
    if (offset + indexsize != size - sizeof(sumdata))
        COMMON_THROWEX(BaseException, u"wrong respak size");
    ResourceList.resize(itemcount);
    ResReader.Rewind(offset);
    ResReader.Read(indexsize, ResourceList.data());
    if (!sumdata.CheckSHA(ResourceUtil::SHA256(ResourceList.data(), indexsize)))
        COMMON_THROWEX(BaseException, u"wrong checksum for resource index");
    if (Linq::FromIterable(ResourceList)
        .Where([index = 0u](const detail::ResourceItem& item) mutable { return item.GetIndex() != index++; })
        .TryGetFirst())
        COMMON_THROWEX(BaseException, u"wrong index list for resource index");
    ResourceSet = Linq::FromIterable(ResourceList)
        .ToMap(ResourceSet, [](const detail::ResourceItem& item) { return item.ExtractHandle(); },
            [index = 0](const auto&) mutable { return index++; });
}

DeserializeUtil::~DeserializeUtil()
{
}

common::AlignedBuffer DeserializeUtil::GetResource(const string & handle)
{
    if (handle.size() != 64 + 1 || handle[0] != '@')
        COMMON_THROWEX(BaseException, u"wrong reource handle");
    const auto findres = FindInMap(ResourceSet, handle, std::in_place);
    if (!findres)
        return {};
    const auto& metadata = ResourceList[findres.value()];
    ResReader.Rewind(metadata.GetOffset());
    detail::ResourceItem item;
    ResReader.Read(item);
    if (metadata != item)
        COMMON_THROWEX(BaseException, u"unmatch resource metadata with resource index");
    common::AlignedBuffer ret((size_t)metadata.GetSize());
    ResReader.Read((size_t)metadata.GetSize(), ret.GetRawPtr());
    return ret;
}

std::unique_ptr<xziar::respak::Serializable> DeserializeUtil::InnerDeserialize(const ejson::JObjectRef<true>& object, 
    std::unique_ptr<Serializable>(*fallback)(DeserializeUtil&, const ejson::JObjectRef<true>&))
{
    const auto type = object.Get<string_view>(TypeFieldName);
    const auto it = FindInMap(DeserializeMap(), type);
    if (it)
        return (*it)(*this, object);
    if (fallback)
        return (*fallback)(*this, object);
    return nullptr;
}

}


