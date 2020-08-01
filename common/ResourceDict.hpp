#include "CommonRely.hpp"
#include <deque>
#include <tuple>
#include <any>
#include <string_view>
#include <string>


namespace common
{

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
        bool Compare(const char* str, const size_t len) const noexcept
        {
            if (IsEmpty()) return false;
            if (GetLen() != len) return false;
            return std::char_traits<char>::compare(Key, str, len) == 0;
        }
        void ResetStr()
        {
            if ((Len & SizeTag) == 0) // allocated
                delete Key;
            Key = nullptr;
            Len = SIZE_MAX;
        }
        void SetStr(const char* str, const size_t len, const bool shouldCopy)
        {
            ResetStr();
            if (shouldCopy)
            {
                Len = len;
                auto ptr = new char[len];
                memcpy_s(ptr, len, str, len);
                Key = ptr;
            }
            else
            {
                Len = SizeTag | len;
                Key = str;
            }
        }
    };
    static constexpr auto kkk = sizeof(RawItem);
    std::deque<RawItem> Items;
    template<typename T>
    bool Add_(const char* str, const size_t len, const bool shouldCopy, T&& val)
    {
        for (auto& item : Items)
        {
            if (item.Compare(str, len))
            {
                item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
                return false;
            }
        }
        for (auto& item : Items)
        {
            if (item.IsEmpty())
            {
                item.SetStr(str, len, shouldCopy);
                item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
                return true;
            }
        }
        auto& item = Items.emplace_back();
        item.SetStr(str, len, shouldCopy);
        item.Val.emplace<std::decay_t<T>>(std::forward<T>(val));
        return true;
    }
public:
    ~ResourceDict()
    {
        for (auto& item : Items)
        {
            if (!item.IsEmpty())
            {
                item.ResetStr();
            }
        }
    }
    template<typename T>
    bool Add(const std::string& key, T&& val)
    {
        return Add_(key.c_str(), key.size(), true, std::forward<T>(val));
    }
    template<typename T>
    bool Add(const std::string_view& key, T&& val)
    {
        return Add_(key.data(), key.size(), false, std::forward<T>(val));
    }
    template<typename T>
    bool Add(const char* key, T&& val)
    {
        return Add_(key, std::char_traits<char>::length(key), true, std::forward<T>(val));
    }
    template<typename T, size_t N>
    bool Add(const char(&key)[N], T&& val)
    {
        return Add_(key, N - 1, false, std::forward<T>(val));
    }
    bool Remove(const std::string_view& key)
    {
        for (auto& item : Items)
        {
            if (item.Compare(key.data(), key.size()))
            {
                item.ResetStr();
                item.Val.reset();
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
    const std::any* QueryItem(const std::string_view& key) const noexcept
    {
        for (auto& item : Items)
        {
            if (item.Compare(key.data(), key.size()))
            {
                return &item.Val;
            }
        }
        return nullptr;
    }
    template<typename T>
    const T* QueryItem(const std::string_view& key) const noexcept
    {
        for (auto& item : Items)
        {
            if (item.Compare(key.data(), key.size()))
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
