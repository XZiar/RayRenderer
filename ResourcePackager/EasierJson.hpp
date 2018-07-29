#pragma once
#include "ResourcePackagerRely.h"

namespace xziar::respak::ejson
{

class JObject;
class JArray;

class DocumentHandle : public NonCopyable
{
private:
    variant<std::unique_ptr<rapidjson::MemoryPoolAllocator<>>, rapidjson::MemoryPoolAllocator<>*> MemPool;
protected:
    rapidjson::MemoryPoolAllocator<>& GetMemPool()
    {
        if (MemPool.index() == 0)
            return *std::get<0>(MemPool);
        else
            return *std::get<1>(MemPool);
    }
public:
    DocumentHandle() : MemPool(std::make_unique<rapidjson::MemoryPoolAllocator<>>()) {}
    DocumentHandle(rapidjson::MemoryPoolAllocator<>& mempool) : MemPool(&mempool) {}
    JObject NewObject();
    JArray NewArray();
};

struct SharedUtil
{
    template<typename T>
    constexpr static bool ReturnTrue() { return true; }
    template<typename T>
    constexpr static bool IsString()
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_same_v<string, PlainType> || std::is_same_v<string_view, PlainType>)
            return true;
        if constexpr (std::is_convertible_v<const T&, string_view> || std::is_convertible_v<const T&, string>)
            return true;
        return false;
    }
    template<typename T, typename Alloc>
    static rapidjson::Value ToJString(const T& name, [[maybe_unused]] Alloc& alloc)
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        rapidjson::Value jstr;
        if constexpr(!std::is_same_v<string_view, PlainType> && !std::is_convertible_v<const T&, string_view>)
            jstr.SetString(name.data(), static_cast<uint32_t>(name.size()), alloc);
        else
        {
            const string_view namesv(name);
            jstr.SetString(rapidjson::StringRef(namesv.data(), namesv.size()));
        }
        return jstr;
    }
    template<typename T, typename Alloc>
    static rapidjson::Value ToVal(/*[[maybe_unused]]*/ T&& val, [[maybe_unused]] Alloc& alloc)
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr(IsString<PlainType>())
            return ToJString(std::forward<T>(val), alloc);
        else if constexpr(std::is_convertible_v<T, rapidjson::Value>)
            return static_cast<rapidjson::Value>(val);
        else if constexpr(std::is_constructible_v<rapidjson::Value, T>)
            return rapidjson::Value(std::forward<T>(val));
        else
        {
            static_assert(!ReturnTrue<T>, "unsupported type");
            //return {};
        }
    }
};

class JDoc : public DocumentHandle
{
    friend class DocumentHandle;
private:
    template<class T>
    class StreamWrapper 
    {
    private:
        T & Backend;
    public:
        using Ch = char;
        StreamWrapper(T& backend) : Backend(backend) { }
        char Peek() const { assert(false); return '\0'; }
        char Take() { assert(false); return '\0'; }
        size_t Tell() const {  }
        char* PutBegin() { assert(false); return 0; }
        void Put(char c) 
        { 
            Backend.WriteByteNE(c);
        }
        void Flush() 
        { 
            Backend.Flush();
        }
        size_t PutEnd(char*) { assert(false); return 0; }
    };
protected:
    rapidjson::Value Val;
    JDoc(const rapidjson::Type type) : DocumentHandle(), Val(type) {}
    JDoc(rapidjson::MemoryPoolAllocator<>& mempool, const rapidjson::Type type) : DocumentHandle(mempool), Val(type) {}
public:
    explicit operator rapidjson::Value() { return std::move(Val); }
    string Stringify() const
    {
        rapidjson::StringBuffer strBuf;
        rapidjson::Writer writer(strBuf);
        Val.Accept(writer);
        return strBuf.GetString();
    }
    template<class T>
    void Stringify(T& writeBackend) const
    {
        StreamWrapper<T> streamer(writeBackend);
        rapidjson::Writer writer(streamer);
        Val.Accept(writer);
    }
};

class JArray : public JDoc
{
    friend class DocumentHandle;
private:
    JArray(rapidjson::MemoryPoolAllocator<>& mempool) : JDoc(mempool, rapidjson::kArrayType) {}
public:
    JArray() : JDoc(rapidjson::kArrayType) {}
    template<typename T>
    void Push(T&& val)
    {
        auto& mempool = GetMemPool();
        Val.PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool);
    }
};

class JObject : public JDoc
{
    friend class DocumentHandle;
private:
    JObject(rapidjson::MemoryPoolAllocator<>& mempool) : JDoc(mempool, rapidjson::kObjectType) {}
public:
    JObject() : JDoc(rapidjson::kObjectType) {}
    template<typename T, typename U>
    void Add(T&& name, U&& val)
    {
        auto& mempool = GetMemPool();
        auto key = SharedUtil::ToJString(std::forward<T>(name), mempool);
        auto value = SharedUtil::ToVal(std::forward<U>(val), mempool);
        Val.AddMember(key, value, mempool);
    }
};

JObject DocumentHandle::NewObject()
{
    return JObject(GetMemPool());
}
JArray DocumentHandle::NewArray()
{
    return JArray(GetMemPool());
}


}

