#pragma once
#include "CommonRely.hpp"
#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:5054)
#endif
#include "3rdParty/rapidjson/document.h"
#if COMPILER_MSVC
#   pragma warning(pop)
#endif
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




/* u8 stringview extra support */

#if defined(__cpp_lib_string_view)
#   if COMPILER_CLANG
#       define U8STR_CONSTEXPR 
#   else
#       define U8STR_CONSTEXPR constexpr
#   endif
#endif
class u8StrView
{
private:
    const uintptr_t Ptr;
    const size_t Size;
public:
    constexpr u8StrView(const u8StrView& other) noexcept : Ptr(other.Ptr), Size(other.Size) {}
    u8StrView(u8StrView&&) noexcept = delete;
    u8StrView& operator=(const u8StrView&) = delete;
    u8StrView& operator=(u8StrView&&) = delete;
    constexpr size_t Length() const noexcept { return Size; }

    U8STR_CONSTEXPR u8StrView(const std::string_view& sv) noexcept :
        Ptr((uintptr_t)(sv.data())), Size(sv.length()) { }
    template<size_t N> U8STR_CONSTEXPR u8StrView(const char(&str)[N]) noexcept :
        Ptr((uintptr_t)(str)), Size(std::char_traits<char>::length(str)) { }

    [[nodiscard]] U8STR_CONSTEXPR const char* CharData() const noexcept { return (const char*)(Ptr); }
    [[nodiscard]] U8STR_CONSTEXPR operator std::string_view() const noexcept { return { CharData(), Length() }; }

#if defined(__cpp_char8_t) && defined(__cpp_lib_char8_t)
    U8STR_CONSTEXPR u8StrView(const std::u8string_view& sv) noexcept :
        Ptr((uintptr_t)(sv.data())), Size(sv.length()) { }
    template<size_t N> U8STR_CONSTEXPR u8StrView(const char8_t(&str)[N]) noexcept :
        Ptr((uintptr_t)(str)), Size(std::char_traits<char8_t>::length(str)) { }

    [[nodiscard]] U8STR_CONSTEXPR const char8_t* U8Data() const noexcept { return (const char8_t*)(Ptr); }
    [[nodiscard]] U8STR_CONSTEXPR operator std::u8string_view() const noexcept { return { U8Data(), Length() }; }
#endif
};
#undef U8STR_CONSTEXPR




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
template<bool IsConst>
class JArrayEnumerableSource;
template<bool IsConst>
class JObjectEnumerableSource;
}

// mempool holder, provide node creation
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
    [[nodiscard]] JObject NewObject(const T& data);
    template<typename T>
    [[nodiscard]] JArray NewArray(const T& data);
    [[nodiscard]] JObject NewObject();
    [[nodiscard]] JArray NewArray();
};

struct SharedUtil
{
    template<typename T>
    constexpr static bool IsString()
    {
        using PlainType = common::remove_cvref_t<T>;
        if constexpr (std::is_same_v<std::string, PlainType> || std::is_same_v<std::string_view, PlainType>)
            return true;
        if constexpr (std::is_convertible_v<const T&, std::string_view> || std::is_convertible_v<const T&, std::string>)
            return true;
#if defined(__cpp_char8_t) && defined(__cpp_lib_char8_t)
        if constexpr (std::is_same_v<std::u8string, PlainType> || std::is_same_v<std::u8string_view, PlainType>)
            return true;
        if constexpr (std::is_convertible_v<const T&, std::u8string_view> || std::is_convertible_v<const T&, std::u8string>)
            return true;
#endif
        return false;
    }
};

struct JsonConvertor
{
public:
    template<typename T>
    static rapidjson::Value ToJString(const T& str, [[maybe_unused]] rapidjson::MemoryPoolAllocator<>& mempool)
    {
        using PlainType = common::remove_cvref_t<T>;
        rapidjson::Value jstr;
        if constexpr (std::is_same_v<u8StrView, PlainType>)
            jstr.SetString(rapidjson::StringRef(str.CharData(), static_cast<uint32_t>(str.Length())));
        else if constexpr(std::is_same_v<string, PlainType>)
            jstr.SetString(str.data(), static_cast<uint32_t>(str.size()), mempool);
        else if constexpr(std::is_same_v<string_view, PlainType> || std::is_convertible_v<const T&, string_view>)
        {
            const string_view strsv(str);
            jstr.SetString(rapidjson::StringRef(strsv.data(), strsv.size()));
        }
        else if constexpr(std::is_convertible_v<const T&, string>)
        {
            const string str2(str);
            jstr.SetString(str2.data(), static_cast<uint32_t>(str2.size()), mempool);
        }
#if defined(__cpp_char8_t) && defined(__cpp_lib_char8_t)
        else if constexpr (std::is_same_v<std::u8string, PlainType>)
            jstr.SetString(reinterpret_cast<const char*>(str.data()), static_cast<uint32_t>(str.size()), mempool);
        else if constexpr (std::is_same_v<std::u8string_view, PlainType> || std::is_convertible_v<const T&, std::u8string_view>)
        {
            const std::u8string_view strsv(str);
            jstr.SetString(rapidjson::StringRef(reinterpret_cast<const char*>(strsv.data()), strsv.size()));
        }
        else if constexpr (std::is_convertible_v<const T&, std::u8string>)
        {
            const std::u8string str2(str);
            jstr.SetString(reinterpret_cast<const char*>(str2.data()), static_cast<uint32_t>(str2.size()), mempool);
        }
#endif
        else
        {
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
        }
        return jstr;
    }

#define EJSONCOV_TOVAL_BEGIN template<typename T> \
    static rapidjson::Value ToVal(T&& val, [[maybe_unused]] xziar::ejson::DocumentHandle& handle) \
    { \
        using PlainType = common::remove_cvref_t<T>;
#define EJSONCOV_TOVAL_END }

#define EJSONCOV_FROMVAL template<typename V, typename T> \
    static bool FromVal(V& value, T& val)


    EJSONCOV_TOVAL_BEGIN
    {
        if constexpr (common::is_specialization<PlainType, std::optional>::value)
        {
            if (val.has_value())
                return ToVal(val.value(), handle);
            else
                return rapidjson::Value(rapidjson::kNullType);
        }
        else if constexpr (SharedUtil::IsString<PlainType>())
            return ToJString(std::forward<T>(val), handle.GetMemPool());
        else if constexpr (std::is_convertible_v<T, rapidjson::Value>)
            return static_cast<rapidjson::Value>(val);
        else if constexpr (std::is_constructible_v<rapidjson::Value, T>)
            return rapidjson::Value(std::forward<T>(val));
        else
        {
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
            //return {};
        }
    }
    EJSONCOV_TOVAL_END

    EJSONCOV_FROMVAL
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
class WriteStreamWrapper
{
private:
    common::io::OutputStream& Backend;
public:
    using Ch = char;
    WriteStreamWrapper(common::io::OutputStream& backend) : Backend(backend) { }
    char Peek() const { assert(false); return '\0'; }
    char Take() { assert(false); return '\0'; }
    size_t Tell() const { assert(false); return 0; }
    char* PutBegin() { assert(false); return nullptr; }
    void Put(char c)
    {
        Backend.Write(c);
    }
    void Flush()
    {
        Backend.Flush();
    }
    size_t PutEnd(char*) { assert(false); return 0; }
};
class ReadStreamWrapper
{
private:
    common::io::RandomInputStream& Backend;
public:
    using Ch = char;
    ReadStreamWrapper(common::io::RandomInputStream& backend) : Backend(backend) { }
    char Peek() const
    {
        const auto pos = Backend.CurrentPos();
        const char data = Take();
        Backend.SetPos(pos);
        return data;
    }
    char Take() const
    {
        return Backend.ReadByteNE<char>();
    }
    size_t Tell() const
    {
        return Backend.CurrentPos();
    }
    char* PutBegin() { assert(false); return nullptr; }
    void Put(char) { assert(false); }
    void Flush() { assert(false); }
    size_t PutEnd(char*) { assert(false); return 0; }
};
}

// Basic JSON node, wrapping a rapidjson::Value
template<typename Child>
struct JNode
{
    friend struct JNodeHash;
public:
    [[nodiscard]] rapidjson::Value& ValRef()
    {
        return static_cast<Child*>(this)->GetValRef();
    }
    [[nodiscard]] const rapidjson::Value& ValRef() const
    {
        return static_cast<const Child*>(this)->GetValRef();
    }
    [[nodiscard]] string Stringify(const bool pretty = false) const
    {
        rapidjson::StringBuffer strBuf;
        if (pretty)
        {
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strBuf);
            ValRef().Accept(writer);
        }
        else
        {
            rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
            ValRef().Accept(writer);
        }
        return strBuf.GetString();
    }
    void Stringify(common::io::OutputStream& writeBackend, const bool pretty = false) const
    {
        detail::WriteStreamWrapper streamer(writeBackend);
        if (pretty)
        {
            rapidjson::PrettyWriter<detail::WriteStreamWrapper> writer(streamer);
            ValRef().Accept(writer);
        }
        else
        {
            rapidjson::Writer<detail::WriteStreamWrapper> writer(streamer);
            ValRef().Accept(writer);
        }
    }
    [[nodiscard]] bool IsNull() const
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
    [[nodiscard]] JDocRef<IsConst> GetFromPath(const string_view& path);
    [[nodiscard]] JDocRef<true> GetFromPath(const string_view& path) const;
    template<typename = std::enable_if_t<!IsConst>>
    [[nodiscard]] JObjectRef<false> GetOrCreateObjectFromPath(const string_view& path);
    template<typename = std::enable_if_t<!IsConst>>
    [[nodiscard]] JArrayRef<false> GetOrCreateArrayFromPath(const string_view& path);
};

// JSON Document
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
    [[nodiscard]] rapidjson::Value& GetValRef() { return Val; }
    [[nodiscard]] const rapidjson::Value& GetValRef() const { return Val; }
public:
    [[nodiscard]] explicit operator rapidjson::Value() { return std::move(Val); }
    [[nodiscard]] static JDoc Parse(const string_view& json)
    {
        JDoc doc(rapidjson::kNullType);
        rapidjson::Document rawdoc(doc.MemPool.get());
        rawdoc.Parse(json.data(), json.size());
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
    template<bool> friend class detail::JObjectEnumerableSource;
protected:
    using InnerValType = std::conditional_t<IsConst, const rapidjson::Value*, rapidjson::Value*>;
    InnerValType Val;
    JDocRef(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val) : DocumentHandle(mempool), Val(val) {}
    template<bool R = !IsConst>
    [[nodiscard]] std::enable_if_t<R, rapidjson::Value&> GetValRef() { return *Val; }
    [[nodiscard]] const rapidjson::Value& GetValRef() const { return *Val; }
public:
    template<bool OtherConst, typename = std::enable_if_t<IsConst || (IsConst == OtherConst)>>
    explicit JDocRef(const JDocRef<OtherConst>& doc) : DocumentHandle(doc.MemPool), Val(doc.Val) {}
    //template<bool R = !IsConst, typename = std::enable_if_t<R>>
    explicit JDocRef(JDoc& doc) : DocumentHandle(doc.MemPool), Val(&doc.Val) {}
    template<bool R = IsConst, typename = std::enable_if_t<R>>
    explicit JDocRef(const JDoc& doc) : DocumentHandle(doc.MemPool), Val(&doc.Val) {}
    template<bool R = !IsConst, typename = std::enable_if_t<R>>
    [[nodiscard]] explicit operator rapidjson::Value() { return std::move(*Val); }
    template<typename T, typename Convertor = JsonConvertor>
    [[nodiscard]] T AsValue(T val = {}) const
    {
        Convertor::FromVal(*Val, val);
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
    [[nodiscard]] const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& InnerMemPool() const
    {
        return static_cast<const DocumentHandle*>(static_cast<const ValHolder*>(this))->MemPool;
    }
public:
    template<typename Convertor, typename T>
    bool TryGet(KeyType key, T & val) const
    {
        return KeyChecker::template GetIf<Convertor>(static_cast<const ValHolder*>(this)->ValRef(), key, val);
    }
    template<typename T>
    bool TryGet(KeyType key, T& val) const
    {
        return TryGet<JsonConvertor>(key, val);
    }
    template<typename T, typename Convertor = JsonConvertor>
    [[nodiscard]] T Get(KeyType key, T val = {}) const
    {
        TryGet<Convertor>(key, val);
        return val;
    }
    [[nodiscard]] JObjectRef<true> GetObject(KeyType key) const;
    [[nodiscard]] JArrayRef<true> GetArray(KeyType key) const;

    template<typename Convertor, typename T, bool R = !IsConst, typename = std::enable_if_t<R>>
    bool TryGet(KeyType key, T& val)
    {
        return KeyChecker::template GetIf<Convertor>(static_cast<ValHolder*>(this)->ValRef(), key, val);
    }
    template<typename T, bool R = !IsConst, typename = std::enable_if_t<R>>
    bool TryGet(KeyType key, T& val)
    {
        return TryGet<JsonConvertor>(key, val);
    }
    template<typename T, typename Convertor = JsonConvertor, bool R = !IsConst, typename = std::enable_if_t<R>>
    [[nodiscard]] T Get(KeyType key, T val = {})
    {
        TryGet<Convertor>(key, val);
        return val;
    }
    template<bool R = !IsConst>
    [[nodiscard]] std::enable_if_t<R, JObjectRef<false>> GetObject(KeyType key);
    template<bool R = !IsConst>
    [[nodiscard]] std::enable_if_t<R, JArrayRef<false>> GetArray(KeyType key);
};

namespace detail
{
template<bool IsConst>
class JArrayIterator : protected JDocRef<IsConst>
{
    template<typename, bool> friend class JArrayLike;
private:
public: // gcc&clang need constructor to be public, although I've already make it friend to JObjectLike
    using iterator_category = std::forward_iterator_tag;
    using value_type = JDocRef<IsConst>;
    using difference_type = std::ptrdiff_t;

    JArrayIterator(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, typename JDocRef<IsConst>::InnerValType val) 
        : JDocRef<IsConst>(mempool, val) {}
    JArrayIterator<IsConst>& operator++()
    {
        this->Val++; return *this;
    }
    JArrayIterator<IsConst> operator++(int)
    {
        JArrayIterator<IsConst> ret(this->MemPool, this->Val);
        this->Val++; return ret;
    }
    template<bool B>
    bool operator!=(const JArrayIterator<B>& other) const { return this->Val != other.Val; }
    JDocRef<IsConst> operator*() const
    {
        return *this;
    }
};
template<bool IsConst>
class JArrayEnumerableSource : protected JDocRef<IsConst>
{
    template<typename, bool> friend class JArrayLike;
private:
    using InnerValType = typename JDocRef<IsConst>::InnerValType;
    size_t Index = 0;
public:
    using OutType = JDocRef<IsConst>;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = true;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;

    JArrayEnumerableSource(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val)
        : JDocRef<IsConst>(mempool, val) {}

    [[nodiscard]] constexpr OutType GetCurrent() const noexcept
    { 
        return JDocRef<IsConst>(this->MemPool, this->GetValRef()[Index]);
    }
    constexpr void MoveNext() noexcept { Index++; }
    [[nodiscard]] constexpr bool IsEnd() const noexcept
    {
        return Index >= this->GetValRef().Size();
    }

    [[nodiscard]] constexpr size_t Count() const noexcept
    {
        const auto size = this->GetValRef().Size();
        return size > Index ? size - Index : 0;
    }
    constexpr void MoveMultiple(const size_t count) noexcept
    {
        Index += count;
    }
};

template<bool IsConst>
class JObjectIterator
{
    template<typename, bool> friend class JObjectLike;
private:
    using InnerValType = rapidjson::GenericMemberIterator<IsConst, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>>;
    const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>* MemPool = nullptr;
    InnerValType InnerIterator;
public: // gcc&clang need constructor to be public, although I've already make it friend to JObjectLike
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<std::string_view, JDocRef<IsConst>>;
    using difference_type = std::ptrdiff_t;

    JObjectIterator(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>* mempool, InnerValType val) 
        : MemPool(mempool), InnerIterator(val) {}
    JObjectIterator<IsConst>& operator++()
    {
        InnerIterator++; return *this;
    }
    JObjectIterator<IsConst> operator++(int)
    {
        JObjectIterator<IsConst> ret(MemPool, InnerIterator);
        InnerIterator++; 
        return ret;
    }
    template<bool B>
    bool operator!=(const JObjectIterator<B>& other) const { return InnerIterator != other.InnerIterator; }
    std::pair<std::string_view, JDocRef<IsConst>> operator*() const
    {
        std::string_view name(InnerIterator->name.GetString(), InnerIterator->name.GetStringLength());
        JDocRef<IsConst> doc(*MemPool, &InnerIterator->value);
        return { name, doc };
    }
};
template<bool IsConst>
class JObjectEnumerableSource : protected JDocRef<IsConst>
{
    template<typename, bool> friend class JArrayLike;
private:
    using InnerValType = typename JDocRef<IsConst>::InnerValType;
    using InnerItType = rapidjson::GenericMemberIterator<IsConst, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>>;
    InnerItType InnerIterator;
    size_t Index = 0;
public:
    using OutType = std::pair<std::string_view, JDocRef<IsConst>>;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = true;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;

    JObjectEnumerableSource(const std::shared_ptr<rapidjson::MemoryPoolAllocator<>>& mempool, InnerValType val)
        : JDocRef<IsConst>(mempool, val), InnerIterator(this->GetValRef().MemberBegin()) {}

    [[nodiscard]] constexpr OutType GetCurrent() const noexcept
    {
        std::string_view name(InnerIterator->name.GetString(), InnerIterator->name.GetStringLength());
        JDocRef<IsConst> doc(this->MemPool, &InnerIterator->value);
        return { name, doc };
    }
    constexpr void MoveNext() noexcept 
    {
        Index++; ++InnerIterator;
    }
    [[nodiscard]] constexpr bool IsEnd() const noexcept
    {
        return Index >= this->GetValRef().MemberCount();
    }

    [[nodiscard]] constexpr size_t Count() const noexcept
    {
        const auto size = this->GetValRef().MemberCount();
        return size > Index ? size - Index : 0;
    }
    constexpr void MoveMultiple(const size_t count) noexcept
    {
        Index += count;
        InnerIterator += count;
    }
};
}

template<typename Child, bool IsConst>
class JArrayLike : public JComplexType<const rapidjson::SizeType, JArrayLike<Child, IsConst>, Child, IsConst>
{
    template<typename, typename, typename, bool>friend class JComplexType;
    using Parent = JComplexType<const rapidjson::SizeType, JArrayLike<Child, IsConst>, Child, IsConst>;
private:

    template<typename Convertor, typename V, typename T>
    static forceinline bool GetIf(V& valref, const rapidjson::SizeType index, T& val)
    {
        if (index < valref.Size())
            return Convertor::FromVal(valref[index], val);
        return false;
    }
    template<typename Convertor, typename... Ts, size_t... Indexes>
    [[nodiscard]] size_t InnerTryGetMany(const rapidjson::SizeType offset, std::index_sequence<Indexes...>, Ts&... val) const
    {
        const auto& valref = static_cast<const Child*>(this)->ValRef();
        return (0 + ... + GetIf<Convertor>(valref, static_cast<rapidjson::SizeType>(Indexes + offset), val));
    }
protected:
    template<typename Convertor, typename... T>
    forceinline void Push(T&&... val)
    {
        auto& self = *static_cast<Child*>(this);
        auto& mempool = self.GetMemPool();
        auto& valref = self.ValRef();
        (valref.PushBack(Convertor::ToVal(std::forward<T>(val), self), mempool), ...);
    }
public:
    template<typename Convertor, typename... Ts>
    size_t TryGetMany(const rapidjson::SizeType offset, Ts&... val) const
    {
        return InnerTryGetMany<Convertor>(offset, std::make_index_sequence<sizeof...(Ts)>(), val...);
    }
    template<typename... Ts>
    size_t TryGetMany(const rapidjson::SizeType offset, Ts&... val) const
    {
        return TryGetMany<JsonConvertor>(offset, val...);
    }

    [[nodiscard]] size_t Size() const noexcept
    {
        const auto valref = &static_cast<const Child*>(this)->ValRef();
        return valref == nullptr ? 0 : valref->Size();
    }

    [[nodiscard]] detail::JArrayIterator<true> begin() const
    {
        return detail::JArrayIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().Begin());
    }
    [[nodiscard]] detail::JArrayIterator<true> end() const
    {
        return detail::JArrayIterator<true>(Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().End());
    }
    [[nodiscard]] detail::JArrayEnumerableSource<true> GetEnumerator() const
    {
        return detail::JArrayEnumerableSource<true>(
            Parent::InnerMemPool(),
            &static_cast<const Child*>(this)->ValRef()
            );
    }
    [[nodiscard]] detail::JArrayIterator<IsConst> begin()
    {
        return detail::JArrayIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().Begin());
    }
    [[nodiscard]] detail::JArrayIterator<IsConst> end()
    {
        return detail::JArrayIterator<IsConst>(Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().End());
    }
    [[nodiscard]] detail::JArrayEnumerableSource<IsConst> GetEnumerator()
    {
        return detail::JArrayEnumerableSource<IsConst>(
            Parent::InnerMemPool(), 
            &static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef()
            );
    }
};

template<typename Child, bool IsConst>
class JObjectLike : public JComplexType<const u8StrView&, JObjectLike<Child, IsConst>, Child, IsConst>
{
    template<typename, typename, typename, bool>friend class JComplexType;
    using Parent = JComplexType<const u8StrView&, JObjectLike<Child, IsConst>, Child, IsConst>;
private:

    template<typename Convertor, typename V, typename T>
    static forceinline bool GetIf(V& valref, const u8StrView& name, T& val)
    {
        if (auto it = valref.FindMember(name.CharData()); it != valref.MemberEnd())
            return Convertor::FromVal(it->value, val);
        return false;
    }
protected:
    template<typename Convertor, typename T, typename U>
    forceinline void Add(T&& name, U&& val)
    {
        auto& self = *static_cast<Child*>(this);
        auto& mempool = *Parent::InnerMemPool();
        auto key = Convertor::ToJString(std::forward<T>(name), mempool);
        auto value = Convertor::ToVal(std::forward<U>(val), self);
        self.ValRef().AddMember(key, value, mempool);
    }
public: 
    [[nodiscard]] size_t Size() const noexcept
    {
        const auto valref = &static_cast<const Child*>(this)->ValRef();
        return valref == nullptr ? 0 : valref->MemberCount();
    }

    [[nodiscard]] detail::JObjectIterator<true> begin() const
    {
        return detail::JObjectIterator<true>(&Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().MemberBegin());
    }
    [[nodiscard]] detail::JObjectIterator<true> end() const
    {
        return detail::JObjectIterator<true>(&Parent::InnerMemPool(), static_cast<const Child*>(this)->ValRef().MemberEnd());
    }
    [[nodiscard]] detail::JObjectEnumerableSource<true> GetEnumerator() const
    {
        return detail::JObjectEnumerableSource<true>(
            Parent::InnerMemPool(),
            &static_cast<const Child*> (this)->ValRef()
            );
    }
    [[nodiscard]] detail::JObjectIterator<IsConst> begin()
    {
        return detail::JObjectIterator<IsConst>(&Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().MemberBegin());
    }
    [[nodiscard]] detail::JObjectIterator<IsConst> end()
    {
        return detail::JObjectIterator<IsConst>(&Parent::InnerMemPool(), static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef().MemberEnd());
    }
    [[nodiscard]] detail::JObjectEnumerableSource<IsConst> GetEnumerator()
    {
        return detail::JObjectEnumerableSource<IsConst>(
            Parent::InnerMemPool(),
            &static_cast<std::conditional_t<IsConst, const Child*, Child*>>(this)->ValRef()
            );
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
            JArrayLike<JArray, false>::Push<JsonConvertor>(val);
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
    template<typename Convertor, typename... T>
    JArray& Push(T&&... val)
    {
        JArrayLike<JArray, false>::Push<Convertor>(std::forward<T>(val)...);
        return *this;
    }
    template<typename... T>
    JArray& Push(T&&... val)
    {
        return Push<JsonConvertor>(std::forward<T>(val)...);
    }
};

template<bool IsConst>
class JArrayRef : public JDocRef<IsConst>, public JArrayLike<JArrayRef<IsConst>, IsConst>
{
    friend struct JsonConvertor;
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
    template<typename Convertor, typename... T>
    JArrayRef<IsConst>& Push(T&&... val)
    {
        JArrayLike<JArrayRef<IsConst>, IsConst>::template Push<Convertor>(std::forward<T>(val)...);
        return *this;
    }
    template<typename... T>
    JArrayRef<IsConst>& Push(T&&... val)
    {
        return Push<JsonConvertor>(std::forward<T>(val)...);
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
            JObjectLike<JObject, false>::Add<JsonConvertor>(name, val);
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
    template<typename Convertor, typename T, typename U>
    JObject& Add(T&& name, U&& val)
    {
        JObjectLike<JObject, false>::Add<Convertor>(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    template<typename T, typename U>
    JObject& Add(T&& name, U&& val)
    {
        return Add<JsonConvertor>(std::forward<T>(name), std::forward<U>(val));
    }
};

template<bool IsConst>
class JObjectRef : public JDocRef<IsConst>, public JObjectLike<JObjectRef<IsConst>, IsConst>
{
    friend struct JsonConvertor;
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
    template<typename Convertor, typename T, typename U>
    JObjectRef<IsConst>& Add(T&& name, U&& val)
    {
        JObjectLike<JObjectRef<IsConst>, IsConst>::template Add<Convertor>(std::forward<T>(name), std::forward<U>(val));
        return *this;
    }
    template<typename T, typename U>
    JObjectRef<IsConst>& Add(T&& name, U&& val)
    {
        return Add<JsonConvertor>(std::forward<T>(name), std::forward<U>(val));
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
    TryGet(key, val);
    return val;
}
template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
forceinline JArrayRef<true> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetArray(KeyType key) const
{
    JArrayRef<true> val(InnerMemPool());
    TryGet(key, val);
    return val;
}

template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
template<bool R>
forceinline std::enable_if_t<R, JObjectRef<false>> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetObject(KeyType key)
{
    JObjectRef<false> val(InnerMemPool());
    TryGet(key, val);
    return val;
}
template<typename KeyType, typename KeyChecker, typename ValHolder, bool IsConst>
template<bool R>
forceinline std::enable_if_t<R, JArrayRef<false>> JComplexType<KeyType, KeyChecker, ValHolder, IsConst>::GetArray(KeyType key)
{
    JArrayRef<false> val(InnerMemPool());
    TryGet(key, val);
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


#define EJ_FIELD(field)                         u8 ## #field, field

