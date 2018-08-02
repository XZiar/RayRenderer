#pragma once
#include "ResourcePackagerRely.h"
#include "3rdParty/rapidjson/document.h"
#include "3rdParty/rapidjson/encodings.h"
#include "3rdParty/rapidjson/stringbuffer.h"
#include "3rdParty/rapidjson/writer.h"
#include "3rdParty/rapidjson/prettywriter.h"

namespace xziar::respak::ejson
{

class JDoc;
class JObject;
class JArray;

class DocumentHandle : public NonCopyable
{
protected:
    std::shared_ptr<rapidjson::MemoryPoolAllocator<>> MemPool;
public:
    DocumentHandle() : MemPool(std::make_shared<rapidjson::MemoryPoolAllocator<>>()) {}
    DocumentHandle(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : MemPool(mempool) {}
    JObject NewObject();
    JArray NewArray(); 
    JDoc NewNullValue();
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
    template<typename T>
    static rapidjson::Value ToJString(const T& name, [[maybe_unused]] rapidjson::MemoryPoolAllocator<>& mempool)
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        rapidjson::Value jstr;
        if constexpr(std::is_same_v<string, PlainType>)
            jstr.SetString(name.data(), static_cast<uint32_t>(name.size()), mempool);
        else if constexpr(std::is_same_v<string_view, PlainType> || std::is_convertible_v<const T&, string_view>)
        {
            const string_view namesv(name);
            jstr.SetString(rapidjson::StringRef(namesv.data(), namesv.size()));
        }
        else if constexpr(std::is_convertible_v<const T&, string>)
        {
            const string namestr(name);
            jstr.SetString(namestr.data(), static_cast<uint32_t>(namestr.size()), mempool);
        }
        else
        {
            static_assert(!ReturnTrue<T>, "unsupported type");
        }
        return jstr;
    }
    template<typename T>
    static rapidjson::Value ToVal(T&& val, [[maybe_unused]] rapidjson::MemoryPoolAllocator<>& mempool)
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr(IsString<PlainType>())
            return ToJString(std::forward<T>(val), mempool);
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
        size_t Tell() const { assert(false); return 0; }
        char* PutBegin() { assert(false); return nullptr; }
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
    JDoc(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const rapidjson::Type type) : DocumentHandle(mempool), Val(type) {}
public:
    explicit operator rapidjson::Value() { return std::move(Val); }
    string Stringify(const bool pretty = false) const
    {
        rapidjson::StringBuffer strBuf;
        if (pretty)
        {
            rapidjson::PrettyWriter writer(strBuf);
            Val.Accept(writer);
        }
        else
        {
            rapidjson::Writer writer(strBuf);
            Val.Accept(writer);
        }
        return strBuf.GetString();
    }
    template<class T>
    void Stringify(T& writeBackend, const bool pretty = false) const
    {
        StreamWrapper<T> streamer(writeBackend);
        if (pretty)
        {
            rapidjson::PrettyWriter writer(streamer);
            Val.Accept(writer);
        }
        else
        {
            rapidjson::Writer writer(streamer);
            Val.Accept(writer);
        }
    }
    rapidjson::Value& ValRef() { return Val; }
    rapidjson::MemoryPoolAllocator<>& GetMemPool() { return *MemPool; }
};

class JNull : public JDoc
{
private:
    using JDoc::GetMemPool;
public:
    JNull() : JDoc({}, rapidjson::kNullType) {}
};

class JArray : public JDoc
{
    friend class DocumentHandle;
private:
    JArray(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDoc(mempool, rapidjson::kArrayType) {}
    template<typename T>
    forceinline void InnerPush(rapidjson::MemoryPoolAllocator<>& mempool, T&& val)
    {
        Val.PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool);
    }
    template<typename T, typename... Ts>
    forceinline void InnerPush(rapidjson::MemoryPoolAllocator<>& mempool, T&& val, Ts&&... vals)
    {
        Val.PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool);
        InnerPush(mempool, std::forward<Ts>(vals)...);
    }
public:
    JArray() : JDoc(rapidjson::kArrayType) {}
    template<typename... T>
    JArray& Push(T&&... val)
    {
        InnerPush(*MemPool, std::forward<T>(val)...);
        return *this;
    }
};

class JObject : public JDoc
{
    friend class DocumentHandle;
private:
    JObject(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDoc(mempool, rapidjson::kObjectType) {}
public:
    JObject() : JDoc(rapidjson::kObjectType) {}
    template<typename T, typename U>
    JObject& Add(T&& name, U&& val)
    {
        auto& mempool = *MemPool;
        auto key = SharedUtil::ToJString(std::forward<T>(name), mempool);
        auto value = SharedUtil::ToVal(std::forward<U>(val), mempool);
        Val.AddMember(key, value, mempool);
        return *this;
    }
};

forceinline JObject DocumentHandle::NewObject()
{
    return JObject(MemPool);
}
forceinline JArray DocumentHandle::NewArray()
{
    return JArray(MemPool);
}


}
#define EJSON_ADD_MEMBER(jobject, field) jobject.Add(u8"" #field, field)
#define EJOBJECT_ADD(field) Add(u8"" #field, field)


