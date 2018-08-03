#pragma once
#include "ResourcePackagerRely.h"
#include "EasierJson.hpp"

namespace xziar::respak
{

class SerializeUtil;
class DeserializeUtil;

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
class RESPAKAPI BinaryReader : public NonCopyable
{
public:
    virtual ~BinaryReader() {}
    virtual void Read(void* data, const size_t size) = 0;
    virtual void Rewind(const uint64_t offset) = 0;
    virtual uint64_t CurPos() = 0;
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
class RESPAKAPI FileBinaryReader : public BinaryReader
{
private:
    BufferedFileReader File;
protected:
    virtual void Read(void* data, const size_t size) override
    {
        File.Read(size, data);
    }
    virtual void Rewind(const uint64_t offset)
    {
        File.Rewind(offset);
    }
    virtual uint64_t CurPos()
    {
        return File.CurrentPos();
    }
public:
    FileBinaryReader(FileObject&& file) : File(std::move(file), 65536) {}
    FileBinaryReader(const fs::path& fileName) : File(FileObject::OpenThrow(fileName, OpenFlag::BINARY | OpenFlag::READ), 65536) {}
    virtual ~FileBinaryReader() {}
};

}

class RESPAKAPI Serializable
{
    friend class SerializeUtil;
    friend class DeserializeUtil;
public:
    virtual ~Serializable() {}
protected:
    virtual string_view SerializedType() const = 0;
    virtual ejson::JObject Serialize(SerializeUtil& context) const = 0;
};

#define RESPAK_OVERRIDE_TYPE(type) static constexpr auto SERIALIZE_TYPE = type; \
    static ::std::unique_ptr<::xziar::respak::Serializable> Deserialize(::xziar::respak::DeserializeUtil&, const ::xziar::respak::ejson::JObject& object); \
    virtual ::std::string_view SerializedType() const override { return SERIALIZE_TYPE; }
#define RESPAK_REGIST_DESERIALZER(type) namespace \
    { \
        const static auto RESPAK_DESERIALIZER_##type = ::xziar::respak::DeserializeUtil::RegistDeserializer(type::SERIALIZE_TYPE, type::Deserialize); \
    }
#define RESPAK_DESERIALIZER(type) ::std::unique_ptr<::xziar::respak::Serializable> type::Deserialize(::xziar::respak::DeserializeUtil& context, const ::xziar::respak::ejson::JObject& object)

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
    bool IsPretty = false;
    SerializeUtil(const fs::path& fileName);
    ~SerializeUtil();

    template<typename... Ts>
    static bool IsAnyType(const Serializable& object)
    {
        return (... || (dynamic_cast<const std::remove_cv_t<std::remove_reference_t<Ts>>*>(&object) != nullptr));
    }
    template<typename... Ts>
    static bool IsAllType(const Serializable& object)
    {
        return (... && (dynamic_cast<const std::remove_cv_t<std::remove_reference_t<Ts>>*>(&object) != nullptr));
    }
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


class RESPAKAPI DeserializeUtil : public NonCopyable, public NonMovable
{
public:
    using DeserializeFunc = std::function<std::unique_ptr<Serializable>(DeserializeUtil&, const ejson::JObject&)>;
private:
    static std::unordered_map<std::string_view, DeserializeFunc>& DeserializeMap();
public:
    static uint32_t RegistDeserializer(const std::string_view& type, const DeserializeFunc& func);
    template<typename T>
    std::unique_ptr<T> Deserialize(const ejson::JObject& object);
    
    template<typename T>
    std::shared_ptr<T> DeserializeShare(const ejson::JObject& object)
    {
        return std::shared_ptr<T>(Deserialize<T>(object));
    }

};

template<typename T>
forceinline std::unique_ptr<T> DeserializeUtil::Deserialize(const ejson::JObject& object)
{
    return dynamic_cast<T*>(Deserialize<Serializable>(object).release());
}
template<>
std::unique_ptr<Serializable> DeserializeUtil::Deserialize<Serializable>(const ejson::JObject& object);

}