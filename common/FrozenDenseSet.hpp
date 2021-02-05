#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <algorithm>


namespace common::container
{


template<typename T, typename Compare = std::less<>>
class FrozenDenseSet
{
private:
    std::vector<T> Data;
public:
    FrozenDenseSet() noexcept {}
    template<typename T1, typename Alloc>
    FrozenDenseSet(const std::set<T1, Compare, Alloc>& data)
    {
        Data.reserve(data.size());
        for (const auto& dat : data)
            Data.emplace_back(dat);
    }
    template<typename T1, typename Alloc>
    FrozenDenseSet(const std::vector<T1, Alloc>& data)
    {
        Data.resize(data.size());
        std::partial_sort_copy(data.cbegin(), data.cend(), Data.begin(), Data.end(), Compare());
    }
    FrozenDenseSet(std::vector<T>&& data) : Data(std::move(data))
    {
        std::sort(Data.begin(), Data.end(), Compare());
    }
    forceinline decltype(auto) begin() const noexcept { return Data.cbegin(); }
    forceinline decltype(auto) end()   const noexcept { return Data.cend(); }
    forceinline size_t         Size()  const noexcept { return Data.size(); }

    constexpr const std::vector<T>& RawData() const noexcept { return Data; }
    template<typename E>
    constexpr bool Has(E&& element) const
    {
        return std::binary_search(Data.cbegin(), Data.cend(), element, Compare());
    }
    template<typename E>
    constexpr const T* Find(E&& element) const
    {
        const auto ret = std::lower_bound(Data.cbegin(), Data.cend(), element, Compare());
        if (ret != Data.cend() && !Compare()(element, *ret))
            return &*ret;
        else
            return nullptr;
    }
};





template<typename Ch, typename Hasher = DJBHash>
class FrozenDenseStringSetBase
{
protected:
    struct NoOpTag {};
    using SVType = std::basic_string_view<Ch>;
    struct Piece
    {
        uint64_t Hash;
        uint32_t Offset, Size;
        constexpr Piece(uint64_t hash, uint32_t offset, uint32_t size) noexcept :
            Hash(hash), Offset(offset), Size(size) { }
        constexpr bool operator<(const Piece& other) const noexcept
        {
            return Hash < other.Hash;
        }
        constexpr bool operator<(const uint64_t hash) const noexcept
        {
            return Hash < hash;
        }
    };
    std::vector<Ch> Pool;
    std::vector<Piece> Pieces;

    forceinline constexpr bool InnerSearch(SVType element, uint64_t hash) const noexcept
    {
        for (const auto piece : Pieces)
        {
            if (piece.Hash == hash)
            {
                const SVType target(Pool.data() + piece.Offset, piece.Size);
                if (element == target)
                    return true;
            }
        }
        return false;
    }

    template<typename C, typename F>
    forceinline void FillFrom(const C& data, F func = NoOpTag{})
    {
        Expects(Pieces.empty());
        Expects(Pool.empty());
        Pieces.reserve(data.size());
        size_t total = 0;
        for (const SVType dat : data)
        {
            total += dat.size();
        }
        Expects(total < UINT32_MAX);
        Pool.reserve(total);
        uint32_t offset = 0;
        for (const SVType dat : data)
        {
            if (dat.empty()) continue;
            const auto hash = Hasher()(dat);
            if (InnerSearch(dat, hash)) continue;
            const auto size = static_cast<uint32_t>(dat.size());
            Pool.insert(Pool.end(), dat.cbegin(), dat.cend());
            Pieces.emplace_back(hash, offset, size);
            if constexpr (!std::is_same_v<F, NoOpTag>)
            {
                static_assert(std::is_invocable_v<F, SVType, uint32_t>, "need accept SVType and offset");
                func(dat, offset);
            }
            offset += size;
        }
        std::sort(Pieces.begin(), Pieces.end());
    }
public:
    forceinline size_t Size() const noexcept { return Pieces.size(); }
    template<typename E>
    constexpr SVType Find(E&& element) const noexcept
    {
        using TmpType = std::conditional_t<std::is_base_of_v<str::PreHashed<Hasher>, std::decay_t<E>>,
            std::decay_t<E>, SVType>;
        uint64_t hash = 0;
        const TmpType tmp(std::forward<E>(element));
        if constexpr (std::is_base_of_v<str::PreHashed<Hasher>, TmpType>)
        {
            hash = element.Hash;
        }
        else
        {
            static_assert(std::is_constructible_v<SVType, const E&>, "element should be able to construct string_view");
            hash = Hasher()(tmp);
        }
        
        for (auto it = std::lower_bound(Pieces.cbegin(), Pieces.cend(), hash); 
            it != Pieces.cend() && it->Hash == hash; ++it)
        {
            const SVType target(Pool.data() + it->Offset, it->Size);
            if (tmp == target)
                return target;
        }
        return {};
    }
    template<typename E>
    constexpr bool Has(E&& element) const noexcept
    {
        return !Find(std::forward<E>(element)).empty();
    }
};

template<typename Ch, typename Hasher = DJBHash, typename Compare = std::less<>>
class FrozenDenseStringSet : public FrozenDenseStringSetBase<Ch, Hasher>
{
private:
    using SVType = typename FrozenDenseStringSetBase<Ch>::SVType;
    std::vector<std::pair<uint32_t, uint32_t>> OrderedView;
    class OrderedIterator
    {
        using HostType = FrozenDenseStringSet<Ch, Hasher, Compare>;
        const HostType* Host;
        size_t Idx;
    public:
        using value_type = SVType;
        constexpr OrderedIterator(const HostType* host, size_t idx) :
            Host(host), Idx(idx) { }
        constexpr bool operator==(const OrderedIterator& other) const noexcept
        {
            return Host == other.Host && Idx == other.Idx;
        }
        constexpr bool operator!=(const OrderedIterator& other) const noexcept
        {
            return Host != other.Host || Idx != other.Idx;
        }
        constexpr value_type operator*() const noexcept
        {
            return (*Host)[Idx];
            /*const auto [offset, size] = Host->OrderedView[Idx];
            return { Host->Pool.data() + offset, size };*/
        }
        constexpr OrderedIterator& operator++()
        {
            Idx++;
            return *this;
        }
        constexpr OrderedIterator& operator+=(size_t n)
        {
            Idx += n;
            return *this;
        }
    };
public:
    FrozenDenseStringSet() noexcept {}
    template<typename T, typename Alloc>
    FrozenDenseStringSet(const std::set<T, Compare, Alloc>& data)
    {
        static_assert(std::is_constructible_v<SVType, const T&>, "element should be able to construct string_view");
        this->FillFrom(data, [&](SVType str, uint32_t offset) 
            {
                OrderedView.emplace_back(offset, static_cast<uint32_t>(str.size()));
            });
    }
    template<typename C>
    FrozenDenseStringSet(const C& data)
    {
        using T = typename C::value_type;
        static_assert(std::is_constructible_v<SVType, const T&>, "element should be able to construct string_view");
        std::vector<SVType> tmp;
        tmp.reserve(data.size());
        for (const auto& dat : data)
            tmp.emplace_back(dat);
        std::sort(tmp.begin(), tmp.end(), Compare());
        this->FillFrom(tmp, [&](SVType str, uint32_t offset)
            {
                OrderedView.emplace_back(offset, static_cast<uint32_t>(str.size()));
            });
    }
    template<typename E>
    constexpr size_t GetIndex(E&& element) const noexcept
    {
        const auto target = this->Find(std::forward<E>(element));
        if (target.empty()) return SIZE_MAX;
        const auto offset = target.data() - this->Pool.data();
        for (size_t i = 0; i < OrderedView.size(); ++i)
            if (OrderedView[i].first == offset)
                return i;
        Expects(false);
        return SIZE_MAX;
    }
    constexpr SVType operator[](size_t idx) const noexcept
    {
        const auto [offset, size] = OrderedView[idx];
        return { this->Pool.data() + offset, size }; 
    }
    forceinline OrderedIterator begin() const noexcept { return { this, 0 }; }
    forceinline OrderedIterator end()   const noexcept { return { this, this->Pieces.size() }; }
};

template<typename Ch, typename Hasher>
class FrozenDenseStringSet<Ch, Hasher, void> : public FrozenDenseStringSetBase<Ch, Hasher>
{
private:
    using SVType = typename FrozenDenseStringSetBase<Ch>::SVType;
    class Iterator
    {
        using HostType = FrozenDenseStringSet<Ch, Hasher, void>;
        const HostType* Host;
        size_t Idx;
    public:
        using value_type = SVType;
        constexpr Iterator(const HostType* host, size_t idx) :
            Host(host), Idx(idx) { }
        constexpr bool operator==(const Iterator& other) const noexcept
        {
            return Host == other.Host && Idx == other.Idx;
        }
        constexpr bool operator!=(const Iterator& other) const noexcept
        {
            return Host != other.Host || Idx != other.Idx;
        }
        constexpr value_type operator*() const noexcept
        {
            const auto& piece = Host->Pieces[Idx];
            return { Host->Pool.data() + piece.Offset, piece.Size };
        }
        constexpr Iterator& operator++()
        {
            Idx++;
            return *this;
        }
        constexpr Iterator& operator+=(size_t n)
        {
            Idx += n;
            return *this;
        }
    };
public:
    FrozenDenseStringSet() noexcept {}
    template<typename C>
    FrozenDenseStringSet(const C& data)
    {
        using T = typename C::value_type;
        static_assert(std::is_constructible_v<SVType, const T&>, "element should be able to construct string_view");
        this->FillFrom(data);
    }

    forceinline Iterator begin() const noexcept { return { this, 0 }; }
    forceinline Iterator end()   const noexcept { return { this, this->Pieces.size() }; }
};


}

