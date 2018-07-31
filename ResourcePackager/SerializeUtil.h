#pragma once
#include "ResourcePackagerRely.h"
#include "EasierJson.hpp"

namespace xziar::respak
{

class SerializeUtil;

namespace detail
{

struct RESPAKAPI ResourceItem
{
    bytearray<32> SHA256;
    bytearray<8> Size;
    bytearray<8> Offset;
    bytearray<4> Index;
    bytearray<12> Dummy = { byte(0) };
    ResourceItem(bytearray<32>&& sha256, const uint64_t size, const uint64_t offset, const uint32_t index);
    string ExtractHandle() const;
    uint64_t GetSize() const;
};

class RESPAKAPI BinaryWriter : public NonCopyable
{
public:
    virtual ~BinaryWriter() {}
    virtual void Write(const void* data, const size_t size) = 0;
    virtual void Flush() = 0;
};


class RESPAKAPI FileBinaryWriter : public BinaryWriter
{
private:
    BufferedFileWriter File;
protected:
    virtual void Write(const void* data, const size_t size) override
    {
        File.Write(size, data);
    }
    virtual void Flush() override
    {
        File.Flush();
    }
public:
    FileBinaryWriter(FileObject&& file) : File(std::move(file), 65536) {}
    FileBinaryWriter(const fs::path& fileName) : File(FileObject::OpenThrow(fileName, OpenFlag::CREATE | OpenFlag::BINARY | OpenFlag::WRITE), 65536) {}
    virtual ~FileBinaryWriter() {}
};

}

class RESPAKAPI Serializable
{
    friend class SerializeUtil;
protected:
    virtual ~Serializable() {}
    virtual string_view SerializedType() const = 0;
    virtual ejson::JObject Serialize(SerializeUtil& context) const = 0;
};

class RESPAKAPI SerializeUtil : public NonCopyable, public NonMovable
{
public:
    using FilterFunc = std::function<string(SerializeUtil&, const Serializable&)>;
private:
    BufferedFileWriter DocWriter;
    std::unique_ptr<detail::BinaryWriter> ResWriter;
    ejson::JObject DocRoot;
    vector<FilterFunc> Filters;

    unordered_map<intptr_t, string> ObjectLookup;
    vector<detail::ResourceItem> ResourceList;
    unordered_map<bytearray<32>, uint32_t, detail::SHA256Hash> ResourceSet;
    unordered_map<string, uint32_t> ResourceLookup;
    uint64_t ResOffset = 0;
    uint32_t ResCount = 0;
    bool HasFinished = false;
    ejson::JObject Serialize(const Serializable& object);
    void CheckFinished() const;
public:
    SerializeUtil(const fs::path& fileName);
    ~SerializeUtil();
    void AddFilter(const FilterFunc& filter) { Filters.push_back(filter); }

    ejson::JObject NewObject() { return DocRoot.NewObject(); }
    ejson::JArray NewArray() { return DocRoot.NewArray(); }

    //simply add node to docroot, bypassing filter
    void AddObject(const string& name, ejson::JDoc& node);
    //simply add object, will be filtered to get proper path
    string AddObject(const Serializable& object);
    string LookupObject(const Serializable& object) const;
    //add object to an object, bypassing filter
    void AddObject(ejson::JObject& target, const string& name, const Serializable& object);
    //add object to an array, bypassing filter
    void AddObject(ejson::JArray& target, const Serializable& object);

    string PutResource(const void* data, const size_t size, const string& id = "");
    string LookupResource(const string& id) const;

    void Finish();
};

}