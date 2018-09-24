#pragma once
#include "ResourcePackagerRely.h"
#include "common/EasierJson.hpp"

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
    virtual void Deserialize(DeserializeUtil&, const ejson::JObjectRef<true>&) {}
};

#define RESPAK_OVERRIDE_TYPE(type) static constexpr auto SERIALIZE_TYPE = type; \
    static ::std::unique_ptr<::xziar::respak::Serializable> DoDeserialize(::xziar::respak::DeserializeUtil&, const ::xziar::ejson::JObjectRef<true>& object); \
    virtual ::std::string_view SerializedType() const override { return SERIALIZE_TYPE; }
#define RESPAK_REGIST_DESERIALZER(type) namespace \
    { \
        const static auto RESPAK_DESERIALIZER_##type = ::xziar::respak::DeserializeUtil::RegistDeserializer(type::SERIALIZE_TYPE, type::DoDeserialize); \
    }
#define RESPAK_DESERIALIZER(type) ::std::unique_ptr<::xziar::respak::Serializable> type::DoDeserialize(::xziar::respak::DeserializeUtil& context, const ::xziar::ejson::JObjectRef<true>& object)

class RESPAKAPI SerializeUtil : public NonCopyable, public NonMovable
{
public:
    using FilterFunc = std::function<string(SerializeUtil&, const Serializable&)>;
private:
    BufferedFileWriter DocWriter;
    BufferedFileWriter ResWriter;
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
    ejson::JObjectRef<false> Root;
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

    template<typename T>
    ejson::JObject NewObject(const T& data) { return DocRoot.NewObject(data); }
    template<typename T>
    ejson::JArray NewArray(const T& data) { return DocRoot.NewArray(data); }
    ejson::JObject NewObject() { return DocRoot.NewObject(); }
    ejson::JArray NewArray() { return DocRoot.NewArray(); }

    //simply add node to docroot, bypassing filter
    void AddObject(const string& name, ejson::JDoc& node);
    //simply add object, will be filtered to get proper path
    string AddObject(const Serializable& object);
    string LookupObject(const Serializable& object) const;
    //add object to an object, bypassing filter
    void AddObject(ejson::JObject& target, const string& name, const Serializable& object);
    void AddObject(ejson::JObjectRef<false>& target, const string& name, const Serializable& object);
    //add object to an array, bypassing filter
    void AddObject(ejson::JArray& target, const Serializable& object);
    void AddObject(ejson::JArrayRef<false>& target, const Serializable& object);

    string PutResource(const void* data, const size_t size, const string& id = "");
    string LookupResource(const string& id) const;

    void Finish();
};


class RESPAKAPI DeserializeUtil : public NonCopyable, public NonMovable
{
public:
    using DeserializeFunc = std::function<std::unique_ptr<Serializable>(DeserializeUtil&, const ejson::JObjectRef<true>&)>;
private:
    static std::unordered_map<std::string_view, DeserializeFunc>& DeserializeMap();

    BufferedFileReader ResReader;
    ejson::JObject DocRoot;

    unordered_map<intptr_t, string> ObjectLookup;
    vector<detail::ResourceItem> ResourceList;
    unordered_map<bytearray<32>, uint32_t, detail::SHA256Hash> ResourceSet;
    unordered_map<string, uint32_t> ResourceLookup;
    std::unique_ptr<Serializable> InnerDeserialize(const ejson::JObjectRef<true>& object);
public:
    static uint32_t RegistDeserializer(const std::string_view& type, const DeserializeFunc& func);

    const ejson::JObjectRef<true> Root;
    DeserializeUtil(const fs::path& fileName);
    ~DeserializeUtil();

    template<typename T>
    std::unique_ptr<T> Deserialize(const ejson::JObjectRef<true>& object)
    {
        auto ret = InnerDeserialize(object);
        if constexpr(std::is_same_v<T, Serializable>)
            return ret;
        else
            return std::unique_ptr<T>(dynamic_cast<T*>(ret.release()));
    }
    template<typename T>
    std::shared_ptr<T> DeserializeShare(const ejson::JObjectRef<true>& object)
    {
        return std::shared_ptr<T>(Deserialize<T>(object));
    }

};


}