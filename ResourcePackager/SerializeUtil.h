#pragma once
#include "ResourcePackagerRely.h"
#include "EasierJson.hpp"

namespace xziar::respak
{

class SerializeUtil;

namespace detail
{

struct ResourceItem
{
    bytearray<32> SHA256 = { byte(0) };
    uint8_t Size[8] = { 0 };
    uint8_t Offset[8] = { 0 };
    uint8_t Index[4] = { 0 };
    uint8_t Dummy[4] = { 0 };
    string ExtractHandle() const;
    uint64_t GetSize() const;
};

class RESPAKAPI BlobPool : public NonCopyable, public NonMovable
{
private:
    vector<ResourceItem> ResourceList;
    unordered_map<bytearray<32>, uint32_t, SHA256Hash> ResourceSet;
    uint64_t CurOffset = 0;
    uint32_t CurIndex = 0;
protected:
    virtual void Write(const void* data, const size_t size) = 0;
public:
    virtual ~BlobPool() {}
    string PutResource(const void* data, const size_t size);
    void EndWrite();
};

class RESPAKAPI Serializable
{
    friend class SerializeUtil;
protected:
    virtual ~Serializable() {}
    virtual string_view SerializedType() const = 0;
    virtual ejson::JObject Serialize(ejson::JObject& doc, BlobPool& pool) const = 0;
};

class RESPAKAPI FileBlobPool : public BlobPool
{
private:
    BufferedFileWriter File;
protected:
    virtual void Write(const void* data, const size_t size) override
    {
        File.Write(size, data);
    }
public:
    FileBlobPool(FileObject&& file) : File(std::move(file), 65536) {}
    FileBlobPool(const fs::path& fileName) : File(FileObject::OpenThrow(fileName, OpenFlag::CREATE | OpenFlag::BINARY | OpenFlag::WRITE), 65536) {}
    virtual ~FileBlobPool() {}
};

}

class RESPAKAPI SerializeUtil : public NonCopyable, public NonMovable
{
private:
    BufferedFileWriter DocWriter;
    std::unique_ptr<detail::BlobPool> ResBlob;
    ejson::JObject ResDoc;
public:
    SerializeUtil(const fs::path& fileName);
    ~SerializeUtil();
    void AddObject(const string& name, const detail::Serializable& object);
    void Finish();
};

}