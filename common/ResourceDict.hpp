#pragma once
#include "CommonRely.hpp"
#include "TrunckedContainer.hpp"
#include <tuple>
#include <any>
#include <string_view>
#include <string>


namespace common::container
{

namespace detail
{
template<typename T> struct RDStrHelper
{
    static_assert(!AlwaysTrue<T>, "need char string-like");
};
template<> struct RDStrHelper<::std::string>
{
    const std::string_view Str;
    const bool NeedCopy;
    RDStrHelper(const std::string& str) noexcept : Str{ str }, NeedCopy(true) { }
};
template<> struct RDStrHelper<::std::string_view>
{
    const std::string_view Str;
    const bool NeedCopy;
    constexpr RDStrHelper(const std::string_view str) noexcept : Str{ str }, NeedCopy(false) { }
};
struct RDStrHelperRaw
{
    const std::string_view Str;
    const bool NeedCopy;
    constexpr RDStrHelperRaw(const char* str) noexcept : Str{ str }, NeedCopy(true) { }
};
template<typename T> struct RDStrHelper<T*> : public RDStrHelperRaw
{
    static_assert(std::is_same_v<std::remove_const_t<T>, char>, "need char pointer");
};
template<size_t N> struct RDStrHelper<char[N]>
{
    const std::string_view Str;
    const bool NeedCopy;
    constexpr RDStrHelper(const char(&str)[N]) noexcept : Str{ str }, NeedCopy(false) { }
};
}

class ResourceDict
{
private:
    static constexpr size_t SizeTag  = size_t(0b1) << (sizeof(size_t) * 8 - 1);
    static constexpr size_t SizeMask = ~SizeTag;
    
protected:
    struct RawItem
    {
        const char* Key = nullptr;
        size_t Len = SIZE_MAX;
        std::any Val;
        constexpr bool IsEmpty() const noexcept { return Len == SIZE_MAX; }
        constexpr size_t GetLen() const noexcept { return Len & SizeMask; }
        constexpr bool operator==(const std::string_view str) const noexcept
        {
            if (IsEmpty()) return false;
            if (GetLen() != str.size()) return false;
            return std::char_traits<char>::compare(Key, str.data(), str.size()) == 0;
        }
        void ResetStr()
        {
            if ((Len & SizeTag) == 0) // allocated
                delete Key;
            Key = nullptr;
            Len = SIZE_MAX;
        }
        void SetStr(const std::string_view str, const bool shouldCopy)
        {
            ResetStr();
            if (shouldCopy)
            {
                Len = str.size();
                auto ptr = new char[str.size()];
                memcpy_s(ptr, str.size(), str.data(), str.size());
                Key = ptr;
            }
            else
            {
                Len = SizeTag | str.size();
                Key = str.data();
            }
        }
    };
    TrunckedContainer<RawItem> Items{ 8 };
    size_t UsedSlot = 0;
public:
    ~ResourceDict()
    {
        for (auto& item : Items)
        {
            if (item.IsEmpty())
            {
                item.ResetStr();
                item.Val.reset();
            }
        }
    }
    template<typename T, typename S>
    bool Add(const S& key_, T&& val)
    {
        const detail::RDStrHelper<S> key(key_);
        for (auto& item : Items)
        {
            if (item == key.Str)
            {
                item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
                return false;
            }
        }
        for (auto& item : Items)
        {
            if (item.IsEmpty())
            {
                item.SetStr(key.Str, key.NeedCopy);
                item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
                UsedSlot++;
                return true;
            }
        }
        auto& item = Items.AllocOne();
        new (&item) RawItem();
        item.SetStr(key.Str, key.NeedCopy);
        item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
        UsedSlot++;
        return true;
    }
    bool Remove(const std::string_view& key)
    {
        for (auto& item : Items)
        {
            if (item == key)
            {
                item.ResetStr();
                item.Val.reset();
                UsedSlot--;
                [[maybe_unused]] const auto dummy = Items.TryDealloc(item);
                return true;
            }
        }
        return false;
    }
    struct Item
    {
        std::string_view Key;
        const std::any* Val;
        constexpr explicit operator bool() const noexcept
        {
            return Key.size() != SIZE_MAX;
        }
    };
    const std::any* QueryItem(const std::string_view key) const noexcept
    {
        for (auto& item : Items)
        {
            if (item == key)
            {
                return &item.Val;
            }
        }
        return nullptr;
    }
    template<typename T>
    const T* QueryItem(const std::string_view key) const noexcept
    {
        for (auto& item : Items)
        {
            if (item == key)
            {
                if (item.Val.type() == typeid(T))
                    return std::any_cast<T>(&item.Val);
                else
                    return nullptr;
            }
        }
        return nullptr;
    }

};

}
