#pragma once
#include "ResourcePackagerRely.h"
#include "common/EasierJson.hpp"

namespace xziar::respak
{

class SerializeUtil;
class DeserializeUtil;

class RESPAKAPI Serializable
{
    friend class SerializeUtil;
    friend class DeserializeUtil;
public:
    virtual ~Serializable() {}
protected:
    virtual string_view SerializedType() const = 0;
    virtual void Serialize(SerializeUtil&, ejson::JObject&) const {}
    virtual void Deserialize(DeserializeUtil&, const ejson::JObjectRef<true>&) {}
};

#define RESPAK_DECL_SIMP_DESERIALIZE(typestr) static constexpr auto SERIALIZE_TYPE = typestr; \
    static ::std::unique_ptr<::xziar::respak::Serializable> DoDeserialize(::xziar::respak::DeserializeUtil&, const ::xziar::ejson::JObjectRef<true>& object); \
    virtual ::std::string_view SerializedType() const override { return SERIALIZE_TYPE; }

#define RESPAK_IMPL_SIMP_DESERIALIZE(type) namespace \
    { \
        const static auto RESPAK_DESERIALIZER_##type = ::xziar::respak::DeserializeUtil::RegistDeserializer(type::SERIALIZE_TYPE, type::DoDeserialize); \
    } \
    ::std::unique_ptr<::xziar::respak::Serializable> type::DoDeserialize(::xziar::respak::DeserializeUtil& context, const ::xziar::ejson::JObjectRef<true>& object) \
    { \
        auto obj = new type(); \
        obj->Deserialize(context, object); \
        return std::unique_ptr<Serializable>(obj); \
    }

#define RESPAK_DECL_COMP_DESERIALIZE(typestr) static constexpr auto SERIALIZE_TYPE = typestr; \
    static ::std::unique_ptr<::xziar::respak::Serializable> DoDeserialize(::xziar::respak::DeserializeUtil&, const ::xziar::ejson::JObjectRef<true>& object); \
    virtual ::std::string_view SerializedType() const override { return SERIALIZE_TYPE; } \
    static ::std::any DeserializeArg(::xziar::respak::DeserializeUtil&, const ::xziar::ejson::JObjectRef<true>& object);

#define RESPAK_IMPL_COMP_DESERIALIZE(type, ...) namespace \
    { \
        const static auto RESPAK_DESERIALIZER_##type = ::xziar::respak::DeserializeUtil::RegistDeserializer(type::SERIALIZE_TYPE, type::DoDeserialize); \
    } \
    ::std::unique_ptr<::xziar::respak::Serializable> type::DoDeserialize(::xziar::respak::DeserializeUtil& context, const ::xziar::ejson::JObjectRef<true>& object) \
    { \
        auto args = DeserializeArg(context, object); \
        if (!args.has_value()) return {}; \
        auto obj = std::apply([](auto&&... args) { return new type(std::forward<decltype(args)>(args)...); }, \
            std::move(*std::any_cast<std::tuple<__VA_ARGS__>>(&args))); \
        obj->Deserialize(context, object); \
        return std::unique_ptr<Serializable>(obj); \
    } \
    ::std::any type::DeserializeArg([[maybe_unused]] ::xziar::respak::DeserializeUtil& context, [[maybe_unused]] const ::xziar::ejson::JObjectRef<true>& object)


namespace detail
{

struct RESPAKAPI ResourceItem
{
public:
    bytearray<32> SHA256;
    bytearray<8> Size;
    bytearray<8> Offset;
    bytearray<4> Index;
    bytearray<12> Dummy = { byte(0) };
    ResourceItem() {}
    ResourceItem(bytearray<32>&& sha256, const uint64_t size, const uint64_t offset, const uint32_t index);
    string ExtractHandle() const;
    uint64_t GetSize() const;
    uint64_t GetOffset() const;
    uint32_t GetIndex() const;
    bool CheckSHA(const bytearray<32>& sha256) const;
    bool operator==(const ResourceItem& other) const;
    bool operator!=(const ResourceItem& other) const;
};

}

class RESPAKAPI SerializeUtil : public NonCopyable, public NonMovable
{
public:
    using FilterFunc = std::function<string(const string_view&)>;
private:
    BufferedFileWriter DocWriter;
    BufferedFileWriter ResWriter;
    ejson::JObject DocRoot;
    ejson::JObjectRef<false> SharedMap;
    vector<FilterFunc> Filters;

    // lookup table for global object
    unordered_map<string, string> ObjectLookup;
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

    //template<typename... Ts>
    //static bool IsAnyType(const Serializable& object)
    //{
    //    return (... || (dynamic_cast<const std::remove_cv_t<std::remove_reference_t<Ts>>*>(&object) != nullptr));
    //}
    //template<typename... Ts>
    //static bool IsAllType(const Serializable& object)
    //{
    //    return (... && (dynamic_cast<const std::remove_cv_t<std::remove_reference_t<Ts>>*>(&object) != nullptr));
    //}
    void AddFilter(const FilterFunc& filter) { Filters.push_back(filter); }

    template<typename T>
    ejson::JObject NewObject(const T& data) { return DocRoot.NewObject(data); }
    template<typename T>
    ejson::JArray NewArray(const T& data) { return DocRoot.NewArray(data); }
    ejson::JObject NewObject() { return DocRoot.NewObject(); }
    ejson::JArray NewArray() { return DocRoot.NewArray(); }

    //simply add node to docroot, bypassing filter
    void AddObject(const string& name, ejson::JDoc& node);
    //simply add a global object, will be filtered to get proper path
    string AddObject(const Serializable& object, string id = "");
    //simply add a global object (already parssed), will be filtered to get proper path
    string AddObject(ejson::JObject&& object, string id);
    //string LookupObject(const Serializable& object) const;
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
    class CheckHasDoDeserialize
    {
    private:
        template <typename T>
        static auto HasDoDes(int) -> decltype (
            T::DoDeserialize(std::declval<DeserializeUtil&>(), std::declval<const ejson::JObjectRef<true>&>()),
            std::true_type{});
        template <typename T>
        static std::false_type HasDoDes(...);
    public:
        template<typename T>
        static constexpr bool Check() { return decltype(HasDoDes<T>(0))::value; }
    };

    static std::unordered_map<std::string_view, DeserializeFunc>& DeserializeMap();
    BufferedFileReader ResReader;
    ejson::JObject DocRoot;
    
    // store cookies injected by deserialize host
    map<string, std::any, std::less<>> Cookies;
    // store deserialized shared object acoording to json-node
    unordered_map<ejson::JObjectRef<true>, std::shared_ptr<Serializable>, ejson::JNodeHash> ObjectCache;
    // store share object lookup
    map<string_view, string_view, std::less<>> SharedObjectLookup;
    // store deserialized shared resource acoording to res-handle
    unordered_map<string, common::AlignedBuffer> ResourceCache;
    // resource index
    vector<detail::ResourceItem> ResourceList;
    // resource lookup [handle->residx]
    unordered_map<string, uint32_t> ResourceSet;
    std::unique_ptr<Serializable> InnerDeserialize(const ejson::JObjectRef<true>& object, std::unique_ptr<Serializable>(*fallback)(DeserializeUtil&, const ejson::JObjectRef<true>&));
    ejson::JObjectRef<true> InnerFindShare(const string_view& id);
public:
    static uint32_t RegistDeserializer(const std::string_view& type, const DeserializeFunc& func);

    const ejson::JObjectRef<true> Root;
    DeserializeUtil(const fs::path& fileName);
    ~DeserializeUtil();

    template<typename T>
    void SetCookie(const string_view& name, T&& cookie)
    {
        Cookies.insert_or_assign(string(name), std::any(cookie));
    }
    template<typename T>
    const T* GetCookie(const string_view& name) const
    {
        const auto it = common::container::FindInMap(Cookies, name);
        if (it)
            return std::any_cast<const T>(it);
        else 
            return nullptr;
    }
    common::AlignedBuffer GetResource(const string& handle, const bool cache = true);

    template<typename T = Serializable>
    std::unique_ptr<T> Deserialize(const ejson::JObjectRef<true>& object)
    {
        std::unique_ptr<Serializable> ret;
        if constexpr (CheckHasDoDeserialize::Check<T>())
            ret = InnerDeserialize(object, &T::DoDeserialize);
        else
            ret = InnerDeserialize(object, nullptr);
        if constexpr(std::is_same_v<T, Serializable>)
            return ret;
        else
            return std::unique_ptr<T>(dynamic_cast<T*>(ret.release()));
    }
    template<typename T>
    std::shared_ptr<T> DeserializeShare(const ejson::JObjectRef<true>& object, const bool cache = true)
    {
        if (cache)
        {
            if (auto ret = common::container::FindInMapOrDefault(ObjectCache, object); ret)
                return std::dynamic_pointer_cast<T>(ret);
        }
        auto obj = std::shared_ptr<T>(Deserialize<T>(object));
        if (cache)
            ObjectCache.emplace(object, obj);
        return obj;
    }
    template<typename T = Serializable>
    std::unique_ptr<T> Deserialize(const string_view& id)
    {
        const auto& node = InnerFindShare(id);
        return node.IsNull() ? std::unique_ptr<T>{} : Deserialize<T>(node);
    }
    template<typename T>
    std::shared_ptr<T> DeserializeShare(const string_view& id, const bool cache = true)
    {
        const auto& node = InnerFindShare(id);
        return node.IsNull() ? std::shared_ptr<T>{} : DeserializeShare<T>(node, cache);
    }
};


}