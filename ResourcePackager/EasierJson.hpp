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
    template<typename V, typename T>
    static bool FromVal(V& value, T& val)
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
        else if constexpr(std::is_same_v<T, string_view>)
        {
            if (!value.IsString()) return false;
            val = string_view(value.GetString(), value.GetStringLength());
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
        else if constexpr(std::is_same_v<T, JArrayRef<false>>)
        {
            static_assert(!std::is_const_v<V>, "cannot create non-const Noderef from const Node");
            if (!value.IsArray()) return false;
            val.Val = &value;
        }
        else if constexpr(std::is_same_v<T, JObjectRef<false>>)
        {
            static_assert(!std::is_const_v<V>, "cannot create non-const Noderef from const Node");
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
class JNode : public DocumentHandle
{
private:
    using DocumentHandle::DocumentHandle;
public:
    rapidjson::Value& ValRef()
    {
        return static_cast<Child*>(this)->GetValRef();
    }
    const rapidjson::Value& ValRef() const
    {
        return static_cast<const Child*>(this)->GetValRef();
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

class JDoc : public NonCopyable, public JNode<JDoc>
{
    friend class JNode<JDoc>;
    friend class JArray;
    friend class JObject;
    template<bool C> friend class JDocRef;
protected:
    rapidjson::Value Val;
    JDoc(const rapidjson::Type type) : JNode<JDoc>(), Val(type) {}
    JDoc(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const rapidjson::Type type) : JNode<JDoc>(mempool), Val(type) {}
    rapidjson::Value& GetValRef() { return Val; }
    const rapidjson::Value& GetValRef() const { return Val; }
public:
    explicit operator rapidjson::Value() { return std::move(Val); }
    static JDoc Parse(const string& json)
    {
        JDoc doc(rapidjson::kNullType);
        rapidjson::Document rawdoc(doc.MemPool.get());
        rawdoc.Parse(json.data());
        doc.Val.Swap(rawdoc);
        return doc;
    }
};

template<bool IsConst>
class JDocRef : public JNode<JDocRef<IsConst>>
{
    template<typename T> friend class JNode;
protected:
    using InnerValType = std::conditional_t<IsConst, const rapidjson::Value*, rapidjson::Value*>;
    InnerValType Val;
    JDocRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val) : JNode<JDocRef<IsConst>>(mempool), Val(val) {}
    template<typename = std::enable_if_t<!IsConst>>
    JDocRef(JDoc& doc) : JNode<JDocRef<IsConst>>(doc.MemPool), Val(&doc.Val) {}
    template<typename = std::enable_if_t<IsConst>>
    JDocRef(const JDoc& doc) : JNode<JDocRef<IsConst>>(doc.MemPool), Val(&doc.Val) {}
    template<typename = std::enable_if_t<!IsConst>>
    rapidjson::Value& GetValRef() { return *Val; }
    const rapidjson::Value& GetValRef() const { return *Val; }
public:
    template<typename = std::enable_if_t<!IsConst>>
    explicit operator rapidjson::Value() { return std::move(Val); }
    //operator bool() { return Val != nullptr; }
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

template<bool IsConst>
struct JMemberIterator : protected JDocRef<IsConst>
{
    template<typename Child, bool IsModifiable> friend class JArrayLike;
protected:
    using JDocRef<IsConst>::JDocRef;
public:
    JMemberIterator<IsConst>& operator++()
    {
        this->Val++; return *this;
    }
    template<bool B>
    bool operator!=(const JMemberIterator<B>& other) { return this->Val != other.Val; }
    JDocRef<IsConst> operator*() const
    {
        return *this;
    }
};

template<typename Child, bool IsModifiable>
class JArrayLike
{
private:
    template<typename V, typename T>
    static forceinline bool GetIf(V& valref, const rapidjson::SizeType index, T& val)
    {
        if (index < valref.Size())
            return SharedUtil::FromVal(valref[index], val);
        return false;
    }
    template<typename... Ts, size_t... Indexes>
    size_t InnerTryGetMany(const rapidjson::SizeType offset, std::index_sequence<Indexes...> indexes, Ts&... val) const
    {
        const auto& valref = static_cast<const Child*>(this)->ValRef();
        return (0 + ... + GetIf(valref, static_cast<rapidjson::SizeType>(Indexes + offset), val));
    }
protected:
    template<typename... T>
    forceinline void Push(T&&... val)
    {
        auto& mempool = static_cast<Child*>(this)->GetMemPool();
        auto& valref = static_cast<Child*>(this)->ValRef();
        (valref.PushBack(SharedUtil::ToVal(std::forward<T>(val), mempool), mempool), ...);
    }
    template<typename... Ts>
    size_t TryGetMany(const rapidjson::SizeType offset, Ts&... val) const
    {
        return InnerTryGetMany(offset, std::make_index_sequence<sizeof...(Ts)>(), val...);
    }
    template<typename T>
    bool TryGet(const rapidjson::SizeType index, T& val) const
    {
        return GetIf(static_cast<const Child*>(this)->ValRef(), index, val);
    }
    template<typename T>
    T Get(const rapidjson::SizeType index, T val = {}) const
    {
        TryGet<T>(name, val);
        return val;
    }
    template<typename T, typename = std::enable_if_t<IsModifiable>>
    bool TryGet(const rapidjson::SizeType index, T& val)
    {
        return GetIf(static_cast<Child*>(this)->ValRef(), index, val);
    }
    template<typename T, typename = std::enable_if_t<IsModifiable>>
    T Get(const rapidjson::SizeType index, T val = {})
    {
        TryGet<T>(name, val);
        return val;
    }
    JObjectRef<true> GetObject(const rapidjson::SizeType index) const;
    JArrayRef<true> GetArray(const rapidjson::SizeType index) const;
    template<typename = std::enable_if_t<IsModifiable>>
    JObjectRef<false> GetObject(const rapidjson::SizeType index);
    template<typename = std::enable_if_t<IsModifiable>>
    JArrayRef<false> GetArray(const rapidjson::SizeType index);
public:
    JMemberIterator<true> begin() const
    {
        return JMemberIterator<true>(static_cast<const Child*>(this)->MemPool, static_cast<const Child*>(this)->ValRef().Begin());
    }
    JMemberIterator<true> end() const
    {
        return JMemberIterator<true>(static_cast<const Child*>(this)->MemPool, static_cast<const Child*>(this)->ValRef().End());
    }
};

class JArray : public JDoc, public JArrayLike<JArray, true>
{
    friend class DocumentHandle;
    template<typename T, bool M> friend class JArrayLike;
    template<typename T, bool M> friend class JObjectLike;
protected:
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
        JArrayLike<JArray, true>::Push(std::forward<T>(val)...);
        return *this;
    }
    using JArrayLike<JArray, true>::TryGet;
    using JArrayLike<JArray, true>::TryGetMany;
    using JArrayLike<JArray, true>::Get;
    using JArrayLike<JArray, true>::GetObject;
    using JArrayLike<JArray, true>::GetArray;
};

template<bool IsConst>
class JArrayRef : public JDocRef<IsConst>, public JArrayLike<JArrayRef<IsConst>, !IsConst>
{
    friend struct SharedUtil;
    template<typename T, bool M> friend class JArrayLike;
    template<typename T, bool M> friend class JObjectLike;
protected:
    JArrayRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDocRef<IsConst>(mempool, nullptr) {}
public:
    /*explicit JArrayRef(JDocRef<IsConst>&& doc) : JDocRef(doc.MemPool, doc.Val)
    {
        if (!doc.Val.IsArray())
            COMMON_THROW(BaseException, L"value is not an array");
    }*/
    template<typename = std::enable_if_t<!IsConst>>
    JArrayRef(JArray& jarray) : JDocRef<IsConst>(jarray) {}
    template<typename = std::enable_if_t<IsConst>>
    JArrayRef(const JArray& jarray) : JDocRef<IsConst>(jarray) {}
    template<typename... T>
    JArrayRef<IsConst>& Push(T&&... val)
    {
        JArrayLike<JArrayRef<IsConst>, !IsConst>::Push(std::forward<T>(val)...);
        return *this;
    }
    using JArrayLike<JArrayRef<IsConst>, !IsConst>::TryGet;
    using JArrayLike<JArrayRef<IsConst>, !IsConst>::TryGetMany;
    using JArrayLike<JArrayRef<IsConst>, !IsConst>::Get;
    using JArrayLike<JArrayRef<IsConst>, !IsConst>::GetObject;
    using JArrayLike<JArrayRef<IsConst>, !IsConst>::GetArray;
};

template<typename Child, bool IsModifiable>
class JObjectLike
{
private:
    template<typename V, typename T>
    static forceinline bool GetIf(V& valref, const std::string_view& name, T& val)
    {
        if (auto it = valref.FindMember(name.data()); it != valref.MemberEnd())
            return SharedUtil::FromVal(it->value, val);
        return false;
    }
protected:
    template<typename T, typename U>
    forceinline void Add(T&& name, U&& val)
    {
        auto& mempool = static_cast<Child*>(this)->GetMemPool();
        auto key = SharedUtil::ToJString(std::forward<T>(name), mempool);
        auto value = SharedUtil::ToVal(std::forward<U>(val), mempool);
        static_cast<Child*>(this)->ValRef().AddMember(key, value, mempool);
    }
    template<typename T>
    bool TryGet(const std::string_view& name, T& val) const
    {
        return GetIf(static_cast<const Child*>(this)->ValRef(), name, val);
    }
    template<typename T>
    T Get(const std::string_view& name, T val = {}) const
    {
        TryGet<T>(name, val);
        return val;
    }
    template<typename T, typename = std::enable_if_t<IsModifiable>>
    bool TryGet(const std::string_view& name, T& val)
    {
        return GetIf(static_cast<Child*>(this)->ValRef(), name, val);
    }
    template<typename T, typename = std::enable_if_t<IsModifiable>>
    T Get(const std::string_view& name, T val = {})
    {
        TryGet<T>(name, val);
        return val;
    }
    JObjectRef<true> GetObject(const std::string_view& name) const;
    JArrayRef<true> GetArray(const std::string_view& name) const;
    template<typename = std::enable_if_t<IsModifiable>>
    JObjectRef<false> GetObject(const std::string_view& name);
    template<typename = std::enable_if_t<IsModifiable>>
    JArrayRef<false> GetArray(const std::string_view& name);
};

class JObject : public JDoc, public JObjectLike<JObject, true>
{
    friend class DocumentHandle;
    template<typename T, bool M> friend class JArrayLike;
    template<typename T, bool M> friend class JObjectLike;
protected:
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
        JObjectLike<JObject, true>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    using JObjectLike<JObject, true>::TryGet;
    using JObjectLike<JObject, true>::Get;
    using JObjectLike<JObject, true>::GetObject;
    using JObjectLike<JObject, true>::GetArray;
};

template<bool IsConst>
class JObjectRef : public JDocRef<IsConst>, public JObjectLike<JObjectRef<IsConst>, !IsConst>
{
    friend struct SharedUtil;
    template<typename T, bool M> friend class JArrayLike;
    template<typename T, bool M> friend class JObjectLike;
protected:
    JObjectRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDocRef<IsConst>(mempool, nullptr) {}
public:
    explicit JObjectRef(const JDocRef<IsConst>& doc) : JDocRef<IsConst>(doc)
    {
        if (!doc.ValRef().IsObject())
            COMMON_THROW(BaseException, L"value is not an object");
    }
    template<typename = std::enable_if_t<!IsConst>>
    JObjectRef(JObject& jobject) : JDocRef<IsConst>(jobject) {}
    template<typename = std::enable_if_t<IsConst>>
    JObjectRef(const JObject& jobject) : JDocRef<IsConst>(jobject) {}
    template<typename T, typename U>
    JObjectRef<IsConst>& Add(T&& name, U&& val)
    {
        JObjectLike<JObjectRef<IsConst>>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    using JObjectLike<JObjectRef<IsConst>, !IsConst>::TryGet;
    using JObjectLike<JObjectRef<IsConst>, !IsConst>::Get;
    using JObjectLike<JObjectRef<IsConst>, !IsConst>::GetObject;
    using JObjectLike<JObjectRef<IsConst>, !IsConst>::GetArray;
};


template<typename Child, bool IsModifiable>
forceinline JObjectRef<true> JArrayLike<Child, IsModifiable>::GetObject(const rapidjson::SizeType index) const
{
    JObjectRef<true> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JObjectRef<true>>(index, val);
    return val;
}
template<typename Child, bool IsModifiable>
forceinline JArrayRef<true> JArrayLike<Child, IsModifiable>::GetArray(const rapidjson::SizeType index) const
{
    JArrayRef<true> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JArrayRef<true>>(index, val);
    return val;
}
template<typename Child, bool IsModifiable>
template<typename>
forceinline JObjectRef<false> JArrayLike<Child, IsModifiable>::GetObject(const rapidjson::SizeType index)
{
    JObjectRef<false> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JObjectRef<false>>(index, val);
    return val;
}
template<typename Child, bool IsModifiable>
template<typename>
forceinline JArrayRef<false> JArrayLike<Child, IsModifiable>::GetArray(const rapidjson::SizeType index)
{
    JArrayRef<false> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JArrayRef<false>>(index, val);
    return val;
}

template<typename Child, bool IsModifiable>
forceinline JObjectRef<true> JObjectLike<Child, IsModifiable>::GetObject(const std::string_view& name) const
{
    JObjectRef<true> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JObjectRef<true>>(name, val);
    return val;
}
template<typename Child, bool IsModifiable>
forceinline JArrayRef<true> JObjectLike<Child, IsModifiable>::GetArray(const std::string_view& name) const
{
    JArrayRef<true> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JArrayRef<true>>(name, val);
    return val;
}
template<typename Child, bool IsModifiable>
template<typename>
forceinline JObjectRef<false> JObjectLike<Child, IsModifiable>::GetObject(const std::string_view& name)
{
    JObjectRef<false> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JObjectRef<false>>(name, val);
    return val;
}
template<typename Child, bool IsModifiable>
template<typename>
forceinline JArrayRef<false> JObjectLike<Child, IsModifiable>::GetArray(const std::string_view& name)
{
    JArrayRef<false> val(static_cast<const Child*>(this)->MemPool);
    TryGet<JArrayRef<false>>(name, val);
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


