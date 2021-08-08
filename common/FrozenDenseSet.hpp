#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include "StringPool.hpp"
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





template<typename Ch>
class FrozenDenseStringSetBase
{
protected:
    struct NoOpTag {};
    using SVType = std::basic_string_view<Ch>;
    StringPool<Ch> Pool;
    std::vector<HashedStringPiece<Ch>> Pieces;

    forceinline constexpr bool InnerSearch(SVType element, uint64_t hash) const noexcept
    {
        // reverse check to maximize collision
        for (size_t idx = Pieces.size(); idx-- > 0;)
        {
            const auto& piece = Pieces[idx];
            if (piece.GetHash() == hash)
            {
                if (Pool.GetStringView(piece) == element)
                {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename C, typename F>
    forceinline void FillFrom(const C& data, F beforeSort = NoOpTag{})
    {
        Expects(Pieces.empty());
        Expects(Pool.IsEmpty());
        Pieces.reserve(data.size());
        size_t total = 0;
        for (const SVType& dat : data)
        {
            total += dat.size();
        }
        Expects(total < UINT32_MAX);
        Pool.Reserve(total);
        for (const SVType& dat : data)
        {
            if (dat.empty()) continue;
            const auto hash = DJBHash::HashC(dat);
            if (!InnerSearch(dat, hash))
            {
                Pieces.emplace_back(Pool.AllocateString(dat), hash);
            }
        }
        if constexpr (!std::is_same_v<F, NoOpTag>)
        {
            beforeSort();
        }
        std::sort(Pieces.begin(), Pieces.end());
    }
public:
    forceinline size_t Size() const noexcept { return Pieces.size(); }
    template<typename E>
    constexpr SVType Find(E&& element) const noexcept
    {
        using TmpType = std::conditional_t<std::is_base_of_v<str::PreHashed<DJBHash>, std::decay_t<E>>,
            std::decay_t<E>, SVType>;
        uint64_t hash = 0;
        const TmpType tmp(std::forward<E>(element));
        if constexpr (std::is_base_of_v<str::PreHashed<DJBHash>, TmpType>)
        {
            hash = element.Hash;
        }
        else
        {
            static_assert(std::is_constructible_v<SVType, const E&>, "element should be able to construct string_view");
            hash = DJBHash::HashC(tmp);
        }
        
        for (auto it = std::lower_bound(Pieces.cbegin(), Pieces.cend(), hash); 
            it != Pieces.cend() && it->GetHash() == hash; ++it)
        {
            if (const auto target = Pool.GetStringView(*it); tmp == target)
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

template<typename Ch, bool SortStr = true, typename Compare = std::less<>>
class FrozenDenseStringSet : public FrozenDenseStringSetBase<Ch>
{
private:
    using SVType = typename FrozenDenseStringSetBase<Ch>::SVType;
    std::vector<StringPiece<Ch>> OrderedView;
    class OrderedIterator
    {
        using HostType = FrozenDenseStringSet<Ch, SortStr, Compare>;
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
        this->FillFrom(data, [&]() 
            {
                this->OrderedView.assign(this->Pieces.cbegin(), this->Pieces.cend());
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
        if constexpr (SortStr)
            std::sort(tmp.begin(), tmp.end(), Compare());
        this->FillFrom(tmp, [&]()
            {
                this->OrderedView.assign(this->Pieces.cbegin(), this->Pieces.cend());
            });
    }
    template<typename E>
    constexpr size_t GetIndex(E&& element) const noexcept
    {
        const auto target = this->Find(std::forward<E>(element));
        if (target.empty()) return SIZE_MAX;
        for (size_t i = 0; i < OrderedView.size(); ++i)
        {
            if (const auto tmp = this->Pool.GetStringView(OrderedView[i]); tmp.data() == target.data())
                return i;
        }
        Expects(false);
        return SIZE_MAX;
    }
    constexpr SVType operator[](size_t idx) const noexcept
    {
        return this->Pool.GetStringView(OrderedView[idx]);
    }
    forceinline OrderedIterator begin() const noexcept { return { this, 0 }; }
    forceinline OrderedIterator end()   const noexcept { return { this, this->Pieces.size() }; }
};

//template<typename Ch>
//class FrozenDenseStringSet<Ch, void> : public FrozenDenseStringSetBase<Ch>
//{
//private:
//    using SVType = typename FrozenDenseStringSetBase<Ch>::SVType;
//    class Iterator
//    {
//        using HostType = FrozenDenseStringSet<Ch, Hasher, void>;
//        const HostType* Host;
//        size_t Idx;
//    public:
//        using value_type = SVType;
//        constexpr Iterator(const HostType* host, size_t idx) :
//            Host(host), Idx(idx) { }
//        constexpr bool operator==(const Iterator& other) const noexcept
//        {
//            return Host == other.Host && Idx == other.Idx;
//        }
//        constexpr bool operator!=(const Iterator& other) const noexcept
//        {
//            return Host != other.Host || Idx != other.Idx;
//        }
//        constexpr value_type operator*() const noexcept
//        {
//            const auto& piece = Host->Pieces[Idx];
//            return { Host->Pool.data() + piece.Offset, piece.Size };
//        }
//        constexpr Iterator& operator++()
//        {
//            Idx++;
//            return *this;
//        }
//        constexpr Iterator& operator+=(size_t n)
//        {
//            Idx += n;
//            return *this;
//        }
//    };
//public:
//    FrozenDenseStringSet() noexcept {}
//    template<typename C>
//    FrozenDenseStringSet(const C& data)
//    {
//        using T = typename C::value_type;
//        static_assert(std::is_constructible_v<SVType, const T&>, "element should be able to construct string_view");
//        this->FillFrom(data);
//    }
//
//    forceinline Iterator begin() const noexcept { return { this, 0 }; }
//    forceinline Iterator end()   const noexcept { return { this, this->Pieces.size() }; }
//};


}

