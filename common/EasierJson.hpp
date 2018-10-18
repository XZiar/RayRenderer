#pragma once
#include "common/CommonRely.hpp"
#include "common/FileEx.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include "3rdParty/rapidjson/document.h"
#include "3rdParty/rapidjson/encodings.h"
#include "3rdParty/rapidjson/stringbuffer.h"
#include "3rdParty/rapidjson/writer.h"
#include "3rdParty/rapidjson/prettywriter.h"
#include "3rdParty/rapidjson/pointer.h"

namespace xziar::ejson
{
using common::NonCopyable;
using common::BaseException;
using std::string;
using std::string_view;

template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
class JComplexType;
class JDoc;
class JNull;
class JObject;
class JArray;
template<bool IsConst>
class JDocRef;
template<bool IsConst>
class JObjectRef;
template<bool IsConst>
class JArrayRef;
template<typename Child, bool IsConst>
class JArrayLike;
template<typename Child, bool IsConst>
class JObjectLike;

namespace detail
{
template<bool IsConst>
class JArrayIterator;
template<bool IsConst>
class JObjectIterator;
}

class DocumentHandle
{
    template<typename, typename, typename, bool> friend class JComplexType;
protected:
    std::shared_ptr<rapidjson::MemoryPoolAllocator<>> MemPool;
public:
    DocumentHandle() : MemPool(std::make_shared<rapidjson::MemoryPoolAllocator<>>()) {}
    DocumentHandle(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : MemPool(mempool) {}

    rapidjson::MemoryPoolAllocator<>& GetMemPool() { return *MemPool; }
    template<typename T>
    JObject NewObject(const T& data);
    template<typename T>
    JArray NewArray(const T& data);
    JObject NewObject();
    JArray NewArray();
};

struct SharedUtil
{
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
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
        }
        return jstr;
    }
    template<typename T>
    static rapidjson::Value ToVal(T&& val, [[maybe_unused]] rapidjson::MemoryPoolAllocator<>& mempool)
    {
        using PlainType = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (common::is_specialization<PlainType, std::optional>::value)
        {
            if (val.has_value())
                return ToVal(val.value(), mempool);
            else
                return rapidjson::Value(rapidjson::kNullType);
        }
        else if constexpr(IsString<PlainType>())
            return ToJString(std::forward<T>(val), mempool);
        else if constexpr(std::is_convertible_v<T, rapidjson::Value>)
            return static_cast<rapidjson::Value>(val);
        else if constexpr(std::is_constructible_v<rapidjson::Value, T>)
            return rapidjson::Value(std::forward<T>(val));
        else
        {
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
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
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
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
struct JNode
{
    friend struct JNodeHash;
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
    bool IsNull() const 
    {
        const auto valptr = &ValRef();
        return (valptr == nullptr) || valptr->IsNull();
    }
    template<typename T>
    bool operator==(const JNode<T>& other) const { return &ValRef() == &other.ValRef(); }
};

struct JNodeHash : public std::hash<const rapidjson::Value*>
{
public:
    template<typename Child>
    constexpr size_t operator()(const JNode<Child>& node) const noexcept
    {
        return std::hash<const rapidjson::Value*>::operator()(&node.ValRef());
    }
};

template<typename Child, bool IsConst>
struct JPointerSupport
{
    JDocRef<IsConst> GetFromPath(const string_view& path);
    JDocRef<true> GetFromPath(const string_view& path) const;
    template<typename = std::enable_if_t<!IsConst>>
    JObjectRef<false> GetOrCreateObjectFromPath(const string_view& path);
    template<typename = std::enable_if_t<!IsConst>>
    JArrayRef<false> GetOrCreateArrayFromPath(const string_view& path);
};

class JDoc : public NonCopyable, public DocumentHandle, public JNode<JDoc>, public JPointerSupport<JDoc, false>
{
    friend struct JNode<JDoc>;
    friend struct JPointerSupport<JDoc, false>;
    friend class JArray;
    friend class JObject;
    template<bool> friend class JDocRef;
protected:
    rapidjson::Value Val;
    JDoc(const rapidjson::Type type) : DocumentHandle(), Val(type) {}
    JDoc(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const rapidjson::Type type) : DocumentHandle(mempool), Val(type) {}
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
class JDocRef : public DocumentHandle, public JNode<JDocRef<IsConst>>, public JPointerSupport<JDocRef<IsConst>, false>
{
    template<bool> friend class JDocRef;
    template<typename> friend struct JNode;
    template<typename, bool> friend struct JPointerSupport;
    template<typename, bool> friend class JObjectLike;
    template<bool> friend class detail::JObjectIterator;
protected:
    using InnerValType = std::conditional_t<IsConst, const rapidjson::Value*, rapidjson::Value*>;
    InnerValType Val;
    JDocRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val) : DocumentHandle(mempool), Val(val) {}
    template<bool R = !IsConst>
    std::enable_if_t<R, rapidjson::Value&> GetValRef() { return *Val; }
    const rapidjson::Value& GetValRef() const { return *Val; }
public:
    template<bool OtherConst, typename = std::enable_if_t<IsConst || (IsConst == OtherConst)>>
    explicit JDocRef(const JDocRef<OtherConst>& doc) : DocumentHandle(doc.MemPool), Val(doc.Val) {}
    //template<bool R = !IsConst, typename = std::enable_if_t<R>>
    explicit JDocRef(JDoc& doc) : DocumentHandle(doc.MemPool), Val(&doc.Val) {}
    template<bool R = IsConst, typename = std::enable_if_t<R>>
    explicit JDocRef(const JDoc& doc) : DocumentHandle(doc.MemPool), Val(&doc.Val) {}
    template<bool R = !IsConst, typename = std::enable_if_t<R>>
    explicit operator rapidjson::Value() { return std::move(Val); }
    template<typename T>
    T AsValue(T val = {}) const
    {
        SharedUtil::FromVal(*Val, val);
        return val;
    }
};
template<typename Child, bool IsConst>
forceinline JDocRef<IsConst> JPointerSupport<Child, IsConst>::GetFromPath(const string_view& path)
{
    using ChildType = std::conditional_t<IsConst, const Child*, Child*>;
    const auto self = static_cast<ChildType>(this);
    size_t dummy = 0;
    const auto valptr = rapidjson::Pointer(path.data(), path.size()).Get(self->GetValRef(), &dummy);
    return JDocRef<IsConst>(self->MemPool, valptr);
}
template<typename Child, bool IsConst>
forceinline JDocRef<true> JPointerSupport<Child, IsConst>::GetFromPath(const string_view& path) const
{
    const auto self = static_cast<const Child*>(this);
    const auto valptr = rapidjson::Pointer(path.data(), path.size()).Get(self->GetValRef());
    return JDocRef<true>(self->MemPool, valptr);
}

class JNull : public JDoc
{
private:
    using JDoc::GetMemPool;
    using JDoc::NewArray;
    using JDoc::NewObject;
public:
    JNull() : JDoc({}, rapidjson::kNullType) {}
};

template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
class JComplexType
{
protected:
    const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& InnerMemPool() const
    {
        return static_cast<const DocumentHandle*>(static_cast<const ValHolder*>(this))->MemPool;
    }
public:
    template<typename T>
    bool TryGet(KeyType key, T& val) const
    {
        return KeyChecker::GetIf(static_cast<const ValHolder*>(this)->ValRef(), key, val);
    }
    template<typename T>
    T Get(KeyType key, T val = {}) const
    {
        TryGet<T>(key, val);
        return val;
    }
    JObjectRef<true> GetObject(KeyType key) const;
    JArrayRef<true> GetArray(KeyType key) const;

    template<typename T, bool R = !IsConst, typename = std::enable_if_t<R>>
    bool TryGet(KeyType key, T& val)
    {
        return KeyChecker::GetIf(static_cast<ValHolder*>(this)->ValRef(), key, val);
    }
    template<typename T, bool R = !IsConst, typename = std::enable_if_t<R>>
    T Get(KeyType key, T val = {})
    {
        TryGet<T>(key, val);
        return val;
    }
    template<bool R = !IsConst>
    std::enable_if_t<R, JObjectRef<false>> GetObject(KeyType key);
    template<bool R = !IsConst>
    std::enable_if_t<R, JArrayRef<false>> GetArray(KeyType key);
};

namespace detail
{
template<bool IsConst>
class JArrayIterator : protected JDocRef<IsConst>
{
    template<typename, bool> friend class JArrayLike;
private:
public: // gcc&clang need constructor to be public, although I've already make it friend to JObjectLike
    JArrayIterator(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, typename JDocRef<IsConst>::InnerValType val) : JDocRef<IsConst>(mempool, val) {}
    JArrayIterator<IsConst>& operator++()
    {
        this->Val++; return *this;
    }
    template<bool B>
    bool operator!=(const JArrayIterator<B>& other) const { return this->Val != other.Val; }
    JDocRef<IsConst> operator*() const
    {
        return *this;
    }
};

template<bool IsConst>
class JObjectIterator : protected DocumentHandle
{
    template<typename, bool> friend class JObjectLike;
private:
    using InnerValType = rapidjson::GenericMemberIterator<IsConst, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>>;
    InnerValType InnerIterator;
public: // gcc&clang need constructor to be public, although I've already make it friend to JObjectLike
    JObjectIterator(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val) : DocumentHandle(mempool), InnerIterator(val) {}
    JObjectIterator<IsConst>& operator++()
    {
        InnerIterator++; return *this;
    }
    template<bool B>
    bool operator!=(const JObjectIterator<B>& other) const { return InnerIterator != other.InnerIterator; }
    std::pair<std::string_view, JDocRef<IsConst>> operator*() const
    {
        std::string_view name(InnerIterator->name.GetString(), InnerIterator->name.GetStringLength());
        JDocRef<IsConst> doc(MemPool, &InnerIterator->value);
        return { name, doc };
    }
};
}

template<typename Child, bool IsConst>
class JArrayLike : public JComplexType<const rapidjson::SizeType, JArrayLike<Child, IsConst>, Child, IsConst>
{
    template<typename, typename, typename, bool>friend class JComplexType;
    using Parent = JComplexType<const rapidjson::SizeType, JArrayLike<Child, IsConst>, Child, IsConst>;
private:

    template<typename V, typename T>
    static forceinline bool GetIf(V& valref, const rapidjson::SizeType index, T& val)
    {
        if (index < valref.Size())
            return SharedUtil::FromVal(valref[index], val);
        return false;
    }
    template<typename... Ts, size_t... Indexes>
    size_t InnerTryGetMany(const rapidjson::SizeType offset, std::index_sequence<Indexes...>, Ts&... val) const
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
public:
    template<typename... Ts>
    size_t TryGetMany(const rapidjson::SizeType offset, Ts&... val) const
    {
        return InnerTryGetMany(offset, std::make_index_sequence<sizeof...(Ts)>(), val...);
    }
    detail::JArrayIterator<true> begin() const
    {
        return detail::JArrayIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().Begin());
    }
    detail::JArrayIterator<true> end() const
    {
        return detail::JArrayIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().End());
    }
    detail::JArrayIterator<IsConst> begin()
    {
        return detail::JArrayIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().Begin());
    }
    detail::JArrayIterator<IsConst> end()
    {
        return detail::JArrayIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().End());
    }
};

template<typename Child, bool IsConst>
class JObjectLike : public JComplexType<const std::string_view&, JObjectLike<Child, IsConst>, Child, IsConst>
{
    template<typename, typename, typename, bool>friend class JComplexType;
    using Parent = JComplexType<const std::string_view&, JObjectLike<Child, IsConst>, Child, IsConst>;
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
        auto& mempool = *Parent::InnerMemPool();
        auto key = SharedUtil::ToJString(std::forward<T>(name), mempool);
        auto value = SharedUtil::ToVal(std::forward<U>(val), mempool);
        static_cast<Child*>(this)->ValRef().AddMember(key, value, mempool);
    }
public:
    detail::JObjectIterator<true> begin() const
    {
        return detail::JObjectIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().MemberBegin());
    }
    detail::JObjectIterator<true> end() const
    {
        return detail::JObjectIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().MemberEnd());
    }
    detail::JObjectIterator<IsConst> begin()
    {
        return detail::JObjectIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().MemberBegin());
    }
    detail::JObjectIterator<IsConst> end()
    {
        return detail::JObjectIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().MemberEnd());
    }
};

class JArray : public JDoc, public JArrayLike<JArray, false>
{
    friend class DocumentHandle;
protected:
    JArray(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDoc(mempool, rapidjson::kArrayType) {}
    template<typename T>
    JArray(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const T& data) : JArray(mempool)
    {
        for (const auto& val : data)
            JArrayLike<JArray, false>::Push(val);
    }
public:
    JArray() : JDoc(rapidjson::kArrayType) {}
    explicit JArray(JDoc&& doc) : JArray(doc.MemPool)
    {
        if (doc.Val.IsArray())
            Val = std::move(doc.Val);
    }
    template<typename T>
    explicit JArray(JDoc&& doc, const T& data) : JArray(doc.MemPool, data) {}
    template<typename... T>
    JArray& Push(T&&... val)
    {
        JArrayLike<JArray, false>::Push(std::forward<T>(val)...);
        return *this;
    }
};

template<bool IsConst>
class JArrayRef : public JDocRef<IsConst>, public JArrayLike<JArrayRef<IsConst>, IsConst>
{
    friend struct SharedUtil;
    template<typename, typename, typename, bool>friend class JComplexType;
    template<typename, bool> friend struct JPointerSupport;
protected:
    JArrayRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, typename JDocRef<IsConst>::InnerValType val = nullptr) 
        : JDocRef<IsConst>(mempool, val) {}
public:
    JArrayRef() : JDocRef<IsConst>() {}
    explicit JArrayRef(const JDocRef<IsConst>& doc) : JDocRef<IsConst>(doc)
    {
        if (!doc.ValRef().IsArray())
            this->Val = nullptr;
    }
    explicit JArrayRef(const JDoc& doc) : JDocRef<IsConst>(doc)
    {
        if (!doc.ValRef().IsArray())
            this->Val = nullptr;
    }
    template<bool R = IsConst, typename = std::enable_if_t<R>>
    JArrayRef(const JArray& jarray) : JDocRef<IsConst>(jarray) {}
    //template<bool R = !IsConst, typename = std::enable_if_t<R>>
    JArrayRef(JArray& jarray) : JDocRef<IsConst>(jarray) {}
    template<typename... T>
    JArrayRef<IsConst>& Push(T&&... val)
    {
        JArrayLike<JArrayRef<IsConst>, IsConst>::Push(std::forward<T>(val)...);
        return *this;
    }
};

class JObject : public JDoc, public JObjectLike<JObject, false>
{
    friend class DocumentHandle;
protected:
    JObject(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool) : JDoc(mempool, rapidjson::kObjectType) {}
    template<typename T>
    JObject(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, const T& datamap) : JObject(mempool)
    {
        for (const auto&[name, val] : datamap)
            JObjectLike<JObject, false>::Add(name, val);
    }
public:
    JObject() : JDoc(rapidjson::kObjectType) {}
    explicit JObject(JDoc&& doc) : JObject(doc.MemPool)
    {
        if (doc.Val.IsObject())
            Val = std::move(doc.Val);
    }
    template<typename T>
    explicit JObject(JDoc&& doc, const T& datamap) : JObject(doc.MemPool, datamap) {}
    template<typename T, typename U>
    JObject& Add(T&& name, U&& val)
    {
        JObjectLike<JObject, false>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
};

template<bool IsConst>
class JObjectRef : public JDocRef<IsConst>, public JObjectLike<JObjectRef<IsConst>, IsConst>
{
    friend struct SharedUtil;
    template<typename, typename, typename, bool> friend class JComplexType;
    template<typename, bool> friend struct JPointerSupport;
    template<bool> friend class JObjectRef;
protected:
    JObjectRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, typename JDocRef<IsConst>::InnerValType val = nullptr) 
        : JDocRef<IsConst>(mempool, val) {}
public:
    template<bool OtherConst, typename = std::enable_if_t<IsConst || (IsConst == OtherConst)>>
    JObjectRef(const JObjectRef<OtherConst>& doc) : JDocRef<IsConst>(doc.MemPool, doc.Val) {}
    template<bool OtherConst, typename = std::enable_if_t<IsConst || (IsConst == OtherConst)>>
    explicit JObjectRef(const JDocRef<OtherConst>& doc) : JDocRef<IsConst>(doc)
    {
        if (!doc.ValRef().IsObject())
            this->Val = nullptr;
    }
    explicit JObjectRef(const JDoc& doc) : JDocRef<IsConst>(doc)
    {
        if (!doc.ValRef().IsObject())
            this->Val = nullptr;
    }
    JObjectRef(const JNull& doc) : JDocRef<IsConst>(doc) {}
    template<bool R = IsConst, typename = std::enable_if_t<R>>
    JObjectRef(const JObject& jobject) : JDocRef<IsConst>(jobject) {}
    //template<bool R = !IsConst, typename = std::enable_if_t<R>>
    JObjectRef(JObject& jobject) : JDocRef<IsConst>(jobject) {}
    template<typename T, typename U>
    JObjectRef<IsConst>& Add(T&& name, U&& val)
    {
        JObjectLike<JObjectRef<IsConst>, IsConst>::Add(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
};


template<typename Child, bool IsConst>
template<typename>
forceinline JObjectRef<false> JPointerSupport<Child, IsConst>::GetOrCreateObjectFromPath(const string_view& path)
{
    const auto self = static_cast<Child*>(this);
    bool isExist;
    const auto valptr = &rapidjson::Pointer(path.data(), path.size())
        .Create(self->GetValRef(), self->GetMemPool(), &isExist);
    if (!isExist) valptr->SetObject();
    return JObjectRef<false>(self->MemPool, valptr);
}
template<typename Child, bool IsConst>
template<typename>
forceinline JArrayRef<false> JPointerSupport<Child, IsConst>::GetOrCreateArrayFromPath(const string_view& path)
{
    const auto self = static_cast<Child*>(this);
    bool isExist;
    const auto valptr = &rapidjson::Pointer(path.data(), path.size())
        .Create(self->GetValRef(), self->GetMemPool(), &isExist);
    if (!isExist) valptr->SetArray();
    return JArrayRef<false>(self->MemPool, valptr);
}

template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
forceinline JObjectRef<true> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetObject(KeyType key) const
{
    JObjectRef<true> val(InnerMemPool());
    TryGet<JObjectRef<true>>(key, val);
    return val;
}
template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
forceinline JArrayRef<true> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetArray(KeyType key) const
{
    JArrayRef<true> val(InnerMemPool());
    TryGet<JArrayRef<true>>(key, val);
    return val;
}

template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
template<bool R>
forceinline std::enable_if_t<R, JObjectRef<false>> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetObject(KeyType key)
{
    JObjectRef<false> val(InnerMemPool());
    TryGet<JObjectRef<false>>(key, val);
    return val;
}
template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
template<bool R>
forceinline std::enable_if_t<R, JArrayRef<false>> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetArray(KeyType key)
{
    JArrayRef<false> val(InnerMemPool());
    TryGet<JArrayRef<false>>(key, val);
    return val;
}


template<typename T>
forceinline JObject DocumentHandle::NewObject(const T& data)
{
    return JObject(MemPool, data);
}
template<typename T>
forceinline JArray DocumentHandle::NewArray(const T& data)
{
    return JArray(MemPool, data);
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
#define EJSON_GET_MEMBER(jobject, field) jobject.TryGet(u8"" #field, field)


