#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef WDHOST_EXPORT
#   define WDHOSTAPI _declspec(dllexport)
# else
#   define WDHOSTAPI _declspec(dllimport)
# endif
#else
# define WDHOSTAPI [[gnu::visibility("default")]]
#endif

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/StringPool.hpp"
#include <string>
#include <functional>
#include <memory>
#include <atomic>

namespace xziar::gui
{

class FileList
{
    // Block: Offset(2) Index(1) Length(1) String(N) '\0'(1) Index(1) Length(1)
    struct BlockTail;
    struct BlockHead
    {
        uint32_t Offset = 0;
        uint16_t Index = 0;
        uint16_t Length = 0;
        [[nodiscard]] BlockTail* Tail() noexcept { return reinterpret_cast<BlockTail*>(reinterpret_cast<char16_t*>(this + 1) + Length); }
        [[nodiscard]] const BlockTail* Tail() const noexcept { return reinterpret_cast<const BlockTail*>(reinterpret_cast<const char16_t*>(this + 1) + Length); }
        [[nodiscard]] std::u16string_view Text() const noexcept { return { reinterpret_cast<const char16_t*>(this + 1), Length }; }
    };
    struct BlockTail
    {
        char16_t Null = u'\0';
        uint16_t Index = 0;
        uint16_t Length = 0;
        [[nodiscard]] BlockHead* Head() noexcept { return reinterpret_cast<BlockHead*>(reinterpret_cast<char16_t*>(this) - Length) - 1; }
        [[nodiscard]] const BlockHead* Head() const noexcept { return reinterpret_cast<const BlockHead*>(reinterpret_cast<const char16_t*>(this) - Length) - 1; }
        [[nodiscard]] BlockHead* Next() noexcept { return reinterpret_cast<BlockHead*>(this + 1); }
        [[nodiscard]] const BlockHead* Next() const noexcept { return reinterpret_cast<const BlockHead*>(this + 1); }
    };
    static inline constexpr std::u16string_view BlockHeadStr = { u"\0\0\0\0", 4 };
    static inline constexpr std::u16string_view BlockTailStr = { u"\0\0\0", 3 };
    static_assert(sizeof(BlockHead) == BlockHeadStr.size() * sizeof(char16_t));
    static_assert(sizeof(BlockTail) == BlockTailStr.size() * sizeof(char16_t));

    struct Iterator
    {
        friend FileList;
    private:
        const common::StringPool<char16_t>& Pool;
        size_t Offset;
        constexpr Iterator(const common::StringPool<char16_t>& pool, size_t offset) noexcept : Pool(pool), Offset(offset) {}
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::u16string_view;
        [[nodiscard]] value_type operator*() const noexcept
        {
            const auto& head = *reinterpret_cast<const BlockHead*>(&Pool.GetAllStr()[Offset]);
#if CM_DEBUG
            Ensures(head.Index < static_cast<uint16_t>(Pool.GetAllStr()[0]));
            Ensures(head.Index == head.Tail()->Index);
#endif
            return head.Text();
        }
        [[nodiscard]] constexpr bool operator!=(const Iterator& other) const noexcept { return &Pool != &other.Pool || Offset != other.Offset; }
        [[nodiscard]] constexpr bool operator==(const Iterator& other) const noexcept { return &Pool == &other.Pool && Offset == other.Offset; }
        Iterator& operator++() noexcept 
        {
            Expects(Offset * sizeof(char16_t) < Pool.GetAllStr().size() * sizeof(char16_t) - sizeof(BlockHead) - sizeof(BlockTail));
            const auto& head = *reinterpret_cast<const BlockHead*>(&Pool.GetAllStr()[Offset]);
            const auto next = head.Tail()->Next();
            Offset = reinterpret_cast<const char16_t*>(next) - Pool.GetAllStr().data();
            return *this;
        }
    };
    common::StringPool<char16_t> Pool;
public:
    FileList() noexcept 
    {
        Pool.AllocateString({ u"\0", 1 });
    }
    
    bool AppendFile(std::u16string_view name) noexcept
    {
        IF_UNLIKELY(name.size() >= UINT16_MAX)
            return false;
        IF_UNLIKELY(static_cast<uint16_t>(Pool.ExposeData()[0]) == UINT16_MAX)
            return false;
#if CM_DEBUG
        if (const auto data = Pool.ExposeData(); data.size() > 1)
        {
            const auto lastTail = reinterpret_cast<const BlockTail*>(reinterpret_cast<const std::byte*>(data.data() + data.size()) - sizeof(BlockTail));
            IF_UNLIKELY(reinterpret_cast<const char16_t*>(lastTail) < &data[1])
                return false;
            IF_UNLIKELY(lastTail->Null != u'\0')
                return false;
            IF_UNLIKELY(static_cast<uint16_t>(data[0]) != lastTail->Index + 1)
                return false;
            const auto lastHead = lastTail->Head();
            IF_UNLIKELY(reinterpret_cast<const char16_t*>(lastHead) < &data[1])
                return false;
            const auto lastHeadIdx = reinterpret_cast<const char16_t*>(lastHead) - data.data();
            IF_UNLIKELY(lastHead->Index != lastTail->Index || lastHead->Length != lastTail->Length)
                return false;
            IF_UNLIKELY(lastHead->Offset != lastHeadIdx)
                return false;
        }
#endif
        const auto piece = Pool.AllocateConcatString(BlockHeadStr, name, BlockTailStr);
        const auto data = Pool.ExposeData();

        auto& idx = *reinterpret_cast<uint16_t*>(&data[0]);
        //data[0] = static_cast<uint32_t>(data[0]) + 1;
        auto& head = *reinterpret_cast<BlockHead*>(&data[piece.GetOffset()]);
        head.Offset = static_cast<uint32_t>(piece.GetOffset()), head.Index = idx, head.Length = static_cast<char16_t>(name.size());
        auto& tail = *head.Tail();
        tail.Null = u'\0', tail.Index = idx, tail.Length = static_cast<char16_t>(name.size());
        idx++;
        return true;
    }

    size_t size() const noexcept { return static_cast<uint32_t>(Pool.GetAllStr()[0]); }
    Iterator begin() const noexcept { return { Pool, 1 }; }
    Iterator end() const noexcept { return { Pool, Pool.GetAllStr().size() }; }
};


}