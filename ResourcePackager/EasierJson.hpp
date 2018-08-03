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
class JNull;
class JObject;
class JArray;
template<bool IsConst>
class JObjectRef;
template<bool IsConst>
class JArrayRef;

class DocumentHandle
{
protected:
    std::shared_ptr<rapidjson::MemoryPoolAllocator<>> MemPool;
public:
    DocumentHandle() : MemPool(std::make_shared<rapidjson::MemoryPoolAllocator<>>()) {}
    DocumentHandle(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : MemPool(mempool) {}
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
    template<typename T>
    static bool FromVal(const rapidjson::Value& value, T& val)
    {
        if constexpr(std::is_same_v<T, bool>)
        {
            if (!value.IsBool()) return false;
            val = value.GetBool();
        }
        else if constexpr(std::is_integral_v<T>)
        {
            if constexpr(std::is_signed_v<T>)
            {
                if(!value.IsInt64()) return false;
                val = static_cast<T>(value.GetInt64());
            }
            else
            {
                if(!value.IsUint64()) return false;
                val = static_cast<T>(value.GetUint64());
            }
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            if (!value.IsDouble()) return false;
            val = static_cast<T>(value.GetDouble());
        }
        else if constexpr(std::is_same_v<T, string>)
        {
            if (!value.IsString()) return false;
            val.assign(value.GetString(), value.GetStringLength());
        }
        else if constexpr(std::is_same_v<T, JArrayRef<true>>)
        {
            if (!value.IsArray()) return false;
            val.Val = &value;
        }
        else if constexpr(std::is_same_v<T, JObjectRef<true>>)
        {
            if (!value.IsObject()) return false;
            val.Val = &value;
        }
        else
        {
            static_assert(!ReturnTrue<T>, "unsupported type");
            return false;
        }
        return true;
    }
};

namespace detail
{
template<class T>
class WriteStreamWrapper
{
private:
    T & Backend;
public:
    using Ch = char;
    WriteStreamWrapper(T& backend) : Backend(backend) { }
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
template<class T>
class ReadStreamWrapper
{
private:
    T & Backend;
public:
    using Ch = char;
    ReadStreamWrapper(T& backend) : Backend(backend) { }
    char Peek() const
    {
        const auto pos = Backend.CurrentPos();
        const char data = Take();
        Backend.Rewind(pos);
        return data;
    }
    char Take()
    {
        return static_cast<char>(Backend.ReadByteNE());
    }
    size_t Tell() const
    {
        return Backend.CurrentPos();
    }
    char* PutBegin() { assert(false); return nullptr; }
    void Put(char c) { assert(false); }
    void Flush() { assert(false); }
    size_t PutEnd(char*) { assert(false); return 0; }
};
}

template<typename Child>
class JNode : public NonCopyable, public DocumentHandle
{
    friend class DocumentHandle;
    friend struct SharedUtil;
private:
    using DocumentHandle::DocumentHandle;
protected:
    template<typename T>
    forceinline void InnerPush(rapidjson::MemoryPoolAllocator<>& mempool, T&& val)
    {
        ValRef().PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool);
    }
    template<typename T, typename... Ts>
    forceinline void InnerPush(rapidjson::MemoryPoolAllocator<>& mempool, T&& val, Ts&&... vals)
    {
        ValRef().PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool);
        InnerPush(mempool, std::forward<Ts>(vals)...);
    }
    template<typename... T>
    forceinline void Push(T&&... val)
    {
        InnerPush(*MemPool, std::forward<T>(val)...);
    }
    template<typename T, typename U>
    forceinline void Add(T&& name, U&& val)
    {
        auto& mempool = *MemPool;
        auto key = SharedUtil::ToJString(std::forward<T>(name), mempool);
        auto value = SharedUtil::ToVal(std::forward<U>(val), mempool);
        ValRef().AddMember(key, value, mempool);
    }

    template<typename T>
    bool TryGet(const std::string_view& name, T& val) const
    {
        const auto& valref = ValRef();
        if (const auto it = valref.FindMember(name.data()); it != valref.MemberEnd())
            return SharedUtil::FromVal(it->value, val);
        return false;
    }
    template<typename T>
    T Get(const std::string_view& name, T val = {}) const
    {
        TryGet<T>(name, val);
        return val;
    }
    template<typename T>
    bool TryGet(const std::string_view& name, T& val)
    {
        auto& valref = ValRef();
        if (auto it = valref.FindMember(name.data()); it != valref.MemberEnd())
            return SharedUtil::FromVal(it->value, val);
        return false;
    }
    template<typename T>
    T Get(const std::string_view& name, T val = {})
    {
        TryGet<T>(name, val);
        return val;
    }
    JObjectRef<true> GetObject(const std::string_view& name) const;
    JArrayRef<true> GetArray(const std::string_view& name) const;
public:
    rapidjson::Value& ValRef()
    {
        return reinterpret_cast<Child*>(this)->GetValRef();
    }
    const rapidjson::Value& ValRef() const
    {
        return reinterpret_cast<const Child*>(this)->GetValRef();
    }
    string Stringify(const bool pretty = false) const
    {
        rapidjson::StringBuffer strBuf;
        if (pretty)
        {
            rapidjson::PrettyWriter writer(strBuf);
            ValRef().Accept(writer);
        }
        else
        {
            rapidjson::Writer writer(strBuf);
            ValRef().Accept(writer);
        }
        return strBuf.GetString();
    }
    template<class T>
    void Stringify(T& writeBackend, const bool pretty = false) const
    {
        detail::WriteStreamWrapper<T> streamer(writeBackend);
        if (pretty)
        {
            rapidjson::PrettyWriter writer(streamer);
            ValRef().Accept(writer);
        }
        else
        {
            rapidjson::Writer writer(streamer);
            ValRef().Accept(writer);
        }
    }
    rapidjson::MemoryPoolAllocator<>& GetMemPool() { return *MemPool; }

};

class JDoc : public JNode<JDoc>
{
    friend class JNode<JDoc>;
    friend class JArray;
    friend class JObject;
private:
    JDoc() : JNode() {}
    rapidjson::Value& GetValRef() { return Val; }
    const rapidjson::Value& GetValRef() const { return Val; }
protected:
    rapidjson::Value Val;
    JDoc(const rapidjson::Type type) : JNode(), Val(type) {}
    JDoc(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const rapidjson::Type type) : JNode(mempool), Val(type) {}
    using JNode<JDoc>::InnerPush;
public:
    explicit operator rapidjson::Value() { return std::move(Val); }
    static JDoc Parse(const string& json)
    {
        JDoc doc;
        rapidjson::Document rawdoc(doc.MemPool.get());
        rawdoc.Parse(json.data());
        doc.Val.Swap(rawdoc);
        return doc;
    }
};

class JNull : public JDoc
{
private:
    using JDoc::GetMemPool;
    using JDoc::NewArray;
    using JDoc::NewObject;
public:
    JNull() : JDoc({}, rapidjson::kNullType) {}
};

class JArray : public JDoc
{
    friend class DocumentHandle;
    friend class JObject;
private:
    JArray(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDoc(mempool, rapidjson::kArrayType) {}
public:
    JArray() : JDoc(rapidjson::kArrayType) {}
    explicit JArray(JDoc&& doc) : JArray(doc.MemPool)
    {
        if (!doc.Val.IsArray())
            COMMON_THROW(BaseException, L"value is not an array");
        Val = doc.Val;
    }
    template<typename... T>
    JArray& Push(T&&... val)
    {
        JNode<JDoc>::Push(std::forward<T>(val)...);
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
    explicit JObject(JDoc&& doc) : JObject(doc.MemPool)
    {
        if (!doc.Val.IsObject())
            COMMON_THROW(BaseException, L"value is not an object");
        Val = doc.Val;
    }
    template<typename T, typename U>
    JObject& Add(T&& name, U&& val)
    {
        JNode<JDoc>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    using JNode<JDoc>::TryGet;
    using JNode<JDoc>::Get;
};

template<bool IsConst>
class JDocRef : public JNode<JDocRef<IsConst>>
{
    friend class JNode<JDocRef<IsConst>>;
    friend class JArray;
    friend class JObject;
private:
protected:
    using InnerValType = std::conditional_t<IsConst, const rapidjson::Value*, rapidjson::Value*>;
    template<typename = std::enable_if_t<!IsConst>>
    rapidjson::Value& GetValRef() { return *Val; }
    const rapidjson::Value& GetValRef() const { return *Val; }
    InnerValType Val;
    JDocRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val) : JNode(mempool), Val(val) {}
public:
    template<typename = std::enable_if_t<!IsConst>>
    explicit operator rapidjson::Value() { return std::move(Val); }
};

template<bool IsConst>
class JArrayRef : public JDocRef<IsConst>
{
    friend struct SharedUtil;
private:
    using JDocRef<IsConst>::JDocRef;
    JArrayRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JNode(mempool), Val(nullptr) {}
public:
    explicit JArrayRef(JDocRef<IsConst>&& doc) : JDocRef(doc.MemPool, doc.Val)
    {
        if (!doc.Val.IsArray())
            COMMON_THROW(BaseException, L"value is not an array");
    }
    template<typename... T>
    JArrayRef<IsConst>& Push(T&&... val)
    {
        JNode<JDocRef<IsConst>>::Push(std::forward<T>(val)...);
        return *this;
    }
};

template<bool IsConst>
class JObjectRef : public JDocRef<IsConst>
{
    friend struct SharedUtil;
private:
    using JDocRef<IsConst>::JDocRef;
    JObjectRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JNode(mempool), Val(nullptr) {}
public:
    explicit JObjectRef(JDocRef<IsConst>&& doc) : JDocRef(doc.MemPool, doc.Val)
    {
        if (!doc.Val.IsObject())
            COMMON_THROW(BaseException, L"value is not an object");
    }
    template<typename T, typename U>
    JObjectRef<IsConst>& Add(T&& name, U&& val)
    {
        JNode<JDocRef<IsConst>>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    using JNode<JDocRef<IsConst>>::TryGet;
    using JNode<JDocRef<IsConst>>::Get;
};


template<typename Child>
forceinline JObjectRef<true> JNode<Child>::GetObject(const std::string_view& name) const
{
    JObjectRef<true> val(MemPool);
    TryGet<JObjectRef<true>>(name, val);
    return val;
}
template<typename Child>
forceinline JArrayRef<true> JNode<Child>::GetArray(const std::string_view& name) const
{
    JArrayRef<true> val(MemPool);
    TryGet<JObjectRef<true>>(name, val);
    return val;
}

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


