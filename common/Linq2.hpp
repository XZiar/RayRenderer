#pragma once

#include "CommonRely.hpp"
#include <iterator>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>

namespace common
{
namespace linq
{


namespace detail
{
class EnumerableChecker;

struct EnumerableEnd
{};

template<typename T>
struct NumericRangeSource
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Need numeric type");
    using OutType = T;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = false;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;
    
    T Current;
    T Step;
    T End;

    constexpr NumericRangeSource(T current, T step, T end) noexcept : Current(current), Step(step), End(end) {}
    constexpr T GetCurrent() const noexcept { return Current; }
    constexpr void MoveNext() noexcept { Current += Step; }
    constexpr bool IsEnd() const noexcept { return Step < 0 ? Current <= End : Current >= End; }

    constexpr size_t Count() const noexcept { return static_cast<size_t>((End - Current) / Step); }
    constexpr void MoveMultiple(const size_t count) noexcept { Current += static_cast<T>(count * Step); }
};


template<typename T>
struct RepeatSource
{
    static_assert(std::is_copy_constructible_v<T>, "Need copyable type");
    using OutType = T;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = false;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;

    T Val;
    size_t Avaliable;

    constexpr RepeatSource(T val, const size_t count) : Val(std::move(val)), Avaliable(count) {}
    constexpr T GetCurrent() const { return Val; }
    constexpr void MoveNext() noexcept { Avaliable--; }
    constexpr bool IsEnd() const noexcept { return Avaliable == 0; }
    constexpr size_t Count() const noexcept { return Avaliable; }
    constexpr void MoveMultiple(const size_t count) noexcept { Avaliable -= count; }
};


template<typename T>
struct PassthroughGenerator
{
    template<typename U>
    T operator()(U& eng) const noexcept { return static_cast<T>(eng()); }
};

template<typename T, typename U>
struct RandomSource
{
    using OutType = decltype(std::declval<U&>()(std::declval<T&>()));
    static constexpr bool ShouldCache = true;
    static constexpr bool InvolveCache = true;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;

    mutable T Engine;
    mutable U Generator;

    constexpr RandomSource(T&& eng, U&& gen) : Engine(std::move(eng)), Generator(std::move(gen)) {}
    OutType GetCurrent() const { return Generator(Engine); }
    constexpr void MoveNext() noexcept { }
    constexpr bool IsEnd() const noexcept { return false; }
    constexpr size_t Count() const noexcept { return SIZE_MAX; }
    void MoveMultiple(size_t count) noexcept 
    { 
        Engine.discard(count);
    }
};


template<typename T>
struct ObjCache
{
private:
    std::unique_ptr<T> Object;
protected:
    template<typename U>
    ObjCache(U&& obj) : Object(std::make_unique<T>(obj)) {}

    constexpr T& Obj() { return *Object; }
    constexpr const T& Obj() const { return *Object; }
};

template<typename TB, typename TE>
struct IteratorSource
{
    using OutType = decltype(*std::declval<TB&>());
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = false;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = true;

    TB Begin;
    TE End;

    constexpr IteratorSource(TB begin, TE end) : Begin(std::move(begin)), End(std::move(end)) {}
    constexpr OutType GetCurrent() const { return *Begin; }
    constexpr void MoveNext() { Begin++; }
    constexpr bool IsEnd() const { return !(Begin != End); }
    void MoveMultiple(size_t count) noexcept
    {
        std::advance(Begin, count);
    }
};


template<typename T>
struct ContainedIteratorSource : public ObjCache<T>, 
    public IteratorSource<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>
{
private:
    using BaseType = IteratorSource<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>;
public:
    using OutType = typename BaseType::OutType;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = false;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = true;

    template<typename U>
    constexpr ContainedIteratorSource(U&& obj) :
        ObjCache<T>(std::move(obj)), BaseType(std::begin(this->Obj()), std::end(this->Obj())) {}
    using BaseType::GetCurrent;
    using BaseType::MoveNext;
    using BaseType::IsEnd;
    using BaseType::MoveMultiple;
};


template<typename P>
struct NestedSource
{
private:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
protected:
    P Prev;
    constexpr NestedSource(P&& prev) : Prev(std::move(prev)) { }

    constexpr InType GetCurrentFromPrev() const
    {
        return Prev.GetCurrent();
    }
    constexpr void MoveNextFromPrev()
    {
        Prev.MoveNext();
    }
    constexpr size_t Count() const
    {
        if constexpr (P::IsCountable)
            return Prev.Count();
        else
            static_assert(common::AlwaysTrue<P>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (P::CanSkipMultiple)
            return Prev.MoveMultiple(count);
        else
            static_assert(common::AlwaysTrue<P>(), "Not movemultiple");
    }
};

template<typename P>
struct NestedCacheSource : NestedSource<P>
{
private:
    using InType = typename P::OutType;
    //using PlainInType = std::remove_cv_t<InType>;
    using PlainInType = std::conditional_t<std::is_reference_v<InType>,
        std::reference_wrapper<std::remove_reference_t<InType>>,
        std::remove_cv_t<InType>>;

protected:
    constexpr NestedCacheSource(P&& prev) : NestedSource<P>(std::move(prev)) { }

    constexpr std::add_lvalue_reference_t<PlainInType> GetCurrentRefFromPrev() const
    {
        if (!Temp)
            Temp.emplace(this->Prev.GetCurrent());
        return *Temp;
    }
    constexpr std::add_lvalue_reference_t<std::add_const_t<PlainInType>> GetCurrentConstRefFromPrev() const
    {
        return GetCurrentRefFromPrev();
    }
    constexpr InType GetCurrentFromPrev() const
    {
        if (Temp)
            return *std::move(Temp);
        else
            return this->Prev.GetCurrent();
    }
    constexpr void MoveNextFromPrev()
    {
        Temp.reset();
        this->Prev.MoveNext();
    }
private:
    mutable std::optional<PlainInType> Temp;
};


template<typename P>
struct LimitSource : public NestedSource<P>
{
private:
    using BaseType = NestedSource<P>;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = P::InvolveCache || ShouldCache;
    static constexpr bool IsCountable = P::IsCountable;
    static constexpr bool CanSkipMultiple = P::CanSkipMultiple;

    size_t Avaliable;

    constexpr LimitSource(P&& prev, const size_t n) : BaseType(std::move(prev)), Avaliable(n)
    { }
    constexpr OutType GetCurrent() const
    {
        return this->GetCurrentFromPrev();
    }
    constexpr void MoveNext()
    {
        if (Avaliable != 0)
        {
            Avaliable--; 
            this->MoveNextFromPrev();
        }
    }
    constexpr bool IsEnd() const 
    { 
        return Avaliable == 0 || this->Prev.IsEnd();
    }
    
    constexpr size_t Count() const
    {
        return std::min(Avaliable, BaseType::Count());
    }
    constexpr void MoveMultiple(const size_t count)
    {
        BaseType::MoveMultiple(count);
        Avaliable -= count;
    }
};


template<typename P, typename Filter>
struct FilteredSource : std::conditional_t<P::InvolveCache, NestedCacheSource<P>, NestedSource<P>>
{
private:
    using BaseType = std::conditional_t<P::InvolveCache, NestedCacheSource<P>, NestedSource<P>>;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = P::InvolveCache || ShouldCache;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = false;

    Filter Func;

    constexpr FilteredSource(P&& prev, Filter&& filter) : BaseType(std::move(prev)), Func(std::move(filter))
    {
        while (!this->Prev.IsEnd() && !CheckCurrent())
        {
            this->MoveNextFromPrev();
        }
    }
    constexpr OutType GetCurrent() const
    {
        return this->GetCurrentFromPrev();
    }
    constexpr void MoveNext()
    {
        do
        {
            this->MoveNextFromPrev();
        } while (!this->Prev.IsEnd() && !CheckCurrent());
    }
    constexpr bool IsEnd() const
    {
        return this->Prev.IsEnd();
    }
private:
    static constexpr bool AcceptConstRef = std::is_invocable_v<Filter, std::add_lvalue_reference_t<std::add_const_t<PlainInType>>>;
    constexpr bool CheckCurrent() const
    {
        //static_assert(AcceptConstRef || P::InvolveCache, "uncache value should not be filtered as non-const ref");
        if constexpr (P::InvolveCache)
        {
            if constexpr(AcceptConstRef)
                return Func(this->GetCurrentConstRefFromPrev());
            else
                return Func(this->GetCurrentRefFromPrev());
        }
        else
            return Func(this->GetCurrentFromPrev());
    }
};


template<typename P, typename Mapper, bool NeedCache>
struct MappedSource : public NestedSource<P>
{
private:
    using BaseType = NestedSource<P>;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = std::invoke_result_t<Mapper, InType>;
    static constexpr bool ShouldCache = NeedCache;
    static constexpr bool InvolveCache = P::InvolveCache || ShouldCache;
    static constexpr bool IsCountable = P::IsCountable;
    static constexpr bool CanSkipMultiple = P::CanSkipMultiple;

    Mapper Func;

    constexpr MappedSource(P&& prev, Mapper&& mapper) : BaseType(std::move(prev)), Func(std::move(mapper))
    { }
    constexpr OutType GetCurrent() const
    {
        return Func(this->GetCurrentFromPrev());
    }
    constexpr void MoveNext()
    {
        this->MoveNextFromPrev();
    }
    constexpr bool IsEnd() const
    {
        return this->Prev.IsEnd();
    }

    using BaseType::Count;
    using BaseType::MoveMultiple;
};


}


template<typename T>
class Enumerable
{
    friend class detail::EnumerableChecker;
public:
    using EleType = typename T::OutType;
    static_assert(!std::is_rvalue_reference_v<EleType>, "Output should never be r-value reference");
    using PlainEleType = common::remove_cvref_t<EleType>;
    //using HoldType = std::conditional_t<std::is_lvalue_reference_v<EleType>,
    //    std::reference_wrapper<std::remove_reference_t<EleType>>,
    //    PlainEleType>;

    static constexpr bool IsRefType = std::is_lvalue_reference_v<EleType>;

    constexpr Enumerable(T provider) : Provider(std::move(provider))
    { }
    Enumerable(const Enumerable&) = delete;
    constexpr Enumerable(Enumerable&&) = default;
    Enumerable& operator=(const Enumerable&) = delete;
    constexpr Enumerable& operator=(Enumerable&&) = default;

private:
    struct EnumerableIterator
    {
        Enumerable<T>& Source;
        constexpr EnumerableIterator(Enumerable<T>* source) : Source(*source) {}
        decltype(auto) operator*() const
        {
            return Source.Provider.GetCurrent();
        }
        bool operator!=(const detail::EnumerableEnd&)
        {
            return !Source.Provider.IsEnd();
        }
        bool operator==(const detail::EnumerableEnd&)
        {
            return Source.Provider.IsEnd();
        }
        EnumerableIterator& operator++()
        {
            Source.Provider.MoveNext();
            return *this;
        }
    };
public:
    EnumerableIterator begin()
    {
        return this;
    }
    EnumerableIterator begin() const
    {
        return this;
    }
    constexpr detail::EnumerableEnd end()
    {
        return {};
    }
    constexpr detail::EnumerableEnd end() const
    {
        return {};
    }

    constexpr Enumerable<T>& Skip(size_t n)
    {
        if constexpr (T::CanSkipMultiple && T::IsCountable)
        {
            n = std::min(Provider.Count(), n);
            Provider.MoveMultiple(n);
        }
        else
        {
            while (!Provider.IsEnd() && n--)
                Provider.MoveNext();
        }
        return *this;
    }
    constexpr Enumerable<detail::LimitSource<T>> Take(const size_t n)
    {
        return detail::LimitSource<T>(std::move(Provider), n);
    }
    template<bool ForceCache, typename Mapper>
    constexpr Enumerable<detail::MappedSource<T, common::remove_cvref_t<Mapper>, ForceCache>> Select(Mapper&& mapper)
    {
        using OutType = std::invoke_result_t<Mapper, EleType>;
        return detail::MappedSource<T, common::remove_cvref_t<Mapper>, ForceCache>(std::move(Provider), std::forward<Mapper>(mapper));
    }
    template<typename Mapper>
    constexpr decltype(auto) Select(Mapper&& mapper)
    {
        static_assert(std::is_invocable_v<Mapper, EleType>, "mapper does not accept EleType");
        using OutType = std::invoke_result_t<Mapper, EleType>;
        constexpr bool ShouldCache = !std::is_lvalue_reference_v<OutType>;
        return Select<ShouldCache>(std::forward<Mapper>(mapper));
    }
    template<typename Filter>
    constexpr Enumerable<detail::FilteredSource<T, common::remove_cvref_t<Filter>>> Where(Filter&& filter)
    {
        return detail::FilteredSource<T, common::remove_cvref_t<Filter>>(std::move(Provider), std::forward<Filter>(filter));
    }

    template<typename Func>
    Enumerable<detail::ContainedIteratorSource<std::vector<PlainEleType>>> OrderBy(Func&& comparator = {})
    {
        static_assert(std::is_invocable_r_v<bool, Func, const PlainEleType&, const PlainEleType&>, 
            "sort need a comparator that accepts two element and returns bool");
        auto tmp = ToVector();
        std::sort(tmp.begin(), tmp.end(), comparator);
        return detail::ContainedIteratorSource<std::vector<PlainEleType>>(std::move(tmp));
    }

    constexpr size_t Count()
    {
        if constexpr (T::IsCountable)
            return Provider.Count();
        else
        {
            size_t cnt = 0;
            while (!Provider.IsEnd())
            {
                cnt++;
                Provider.MoveNext();
            }
            return cnt;
        }
    }

    template<typename Func>
    constexpr void ForEach(Func&& func)
    {
        static_assert(std::is_invocable_v<Func, EleType>, "foreach function should accept element");
        while (!Provider.IsEnd())
        {
            func(Provider.GetCurrent());
            Provider.MoveNext();
        }
    }
    template<typename U, typename Func>
    constexpr U Reduce(Func&& func, U data = {})
    {
        static_assert(std::is_invocable_v<Func, U&, EleType>, "reduce function should accept target and element");
        while (!Provider.IsEnd())
        {
            if constexpr (std::is_invocable_r_v<U, Func, const U&, EleType>)
                data = func(data, Provider.GetCurrent());
            else
                func(data, Provider.GetCurrent());
            Provider.MoveNext();
        }
        return data;
    }
    template<typename U = EleType>
    constexpr U Sum(U data = {})
    {
        return Reduce([](U& ret, const auto& item) { ret += item; }, data);
    }
    template<typename U>
    constexpr bool Contains(const U& obj)
    {
        while (!Provider.IsEnd())
        {
            if constexpr (common::is_equal_comparable<U, EleType>::value)
            {
                if (obj == Provider.GetCurrent())
                    return true;
            }
            else if constexpr (common::is_notequal_comparable<U, EleType>::value)
            {
                if (!(obj != Provider.GetCurrent()))
                    return true;
            }
            else
                static_assert(common::AlwaysTrue<U>(), "Type U is not eq/ne comparable with EleType");
            Provider.MoveNext();
        }
        return false;
    }
    template<typename Func>
    constexpr bool ContainsIf(Func&& judger)
    {
        static_assert(std::is_invocable_r_v<bool, Func, EleType>, "judger should accept element and return bool");
        while (!Provider.IsEnd())
        {
            if (judger(Provider.GetCurrent()))
                return true;
            Provider.MoveNext();
        }
        return false;
    }
    template<typename U>
    constexpr bool All(const U& obj)
    {
        while (!Provider.IsEnd())
        {
            if constexpr (common::is_equal_comparable<U, EleType>::value)
            {
                if (!(obj == Provider.GetCurrent()))
                    return false;
            }
            else if constexpr (common::is_notequal_comparable<U, EleType>::value)
            {
                if (obj != Provider.GetCurrent())
                    return false;
            }
            else
                static_assert(common::AlwaysTrue<U>(), "Type U is not eq/ne comparable with EleType");
            Provider.MoveNext();
        }
        return true;
    }
    template<typename Func>
    constexpr bool AllIf(Func&& judger)
    {
        static_assert(std::is_invocable_r_v<bool, Func, EleType>, "judger should accept element and return bool");
        while (!Provider.IsEnd())
        {
            if (!judger(Provider.GetCurrent()))
                return false;
            Provider.MoveNext();
        }
        return true;
    }

    std::vector<PlainEleType> ToVector()
    {
        std::vector<PlainEleType> ret;
        while (!Provider.IsEnd())
        {
            ret.emplace_back(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    std::list<PlainEleType> ToList()
    {
        std::list<PlainEleType> ret;
        while (!Provider.IsEnd())
        {
            ret.emplace_back(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    template<typename Comparator = std::less<PlainEleType>>
    std::set<PlainEleType, Comparator> ToOrderSet()
    {
        std::set<PlainEleType, Comparator> ret;
        while (!Provider.IsEnd())
        {
            ret.emplace(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    template<typename Hasher = std::hash<PlainEleType>>
    std::unordered_set<PlainEleType, Hasher> ToHashSet()
    {
        std::unordered_set<PlainEleType, Hasher> ret;
        while (!Provider.IsEnd())
        {
            ret.emplace(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    template<template<typename> typename Set>
    auto ToAnySet()
    {
        Set<PlainEleType> ret;
        while (!Provider.IsEnd())
        {
            ret.emplace(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    template<bool ShouldReplace = true, typename Map, typename KeyF, typename ValF>
    void IntoMap(Map& themap, KeyF&& keyMapper, ValF&& valMapper)
    {
        while (!Provider.IsEnd())
        {
            EleType tmp = Provider.GetCurrent();
            if constexpr (ShouldReplace)
                themap.insert_or_assign(keyMapper(tmp), valMapper(tmp));
            else
                themap.emplace(keyMapper(tmp), valMapper(tmp));
            Provider.MoveNext();
        }
    }
    template<template<typename, typename> typename Map, bool ShouldReplace = true, typename KeyF, typename ValF>
    auto ToAnyMap(KeyF&& keyMapper, ValF&& valMapper)
    {
        using KeyType = std::invoke_result_t<KeyF, EleType>;
        using ValType = std::invoke_result_t<ValF, EleType>;
        Map<KeyType, ValType> themap;
        IntoMap<ShouldReplace>(themap, std::forward<KeyF>(keyMapper), std::forward<ValF>(valMapper));
        return themap;
    }
private:
    T Provider;
};



template<typename T>
inline constexpr Enumerable<detail::NumericRangeSource<T>>
FromRange(const T begin, const T end, const T step = static_cast<T>(1))
{
    return Enumerable(detail::NumericRangeSource<T>(begin, step, end));
}


template<typename T>
inline constexpr Enumerable<detail::RepeatSource<T>>
FromRepeat(T&& val, const size_t count)
{
    return Enumerable(detail::RepeatSource<common::remove_cvref_t<T>>(std::forward<T>(val), count));
}


template<typename T>
inline constexpr auto
FromIterable(T& container)
{
    return Enumerable(detail::IteratorSource(std::begin(container), std::end(container)));
}


template<typename T>
inline constexpr auto
FromContainer(T&& container)
{
    static_assert(std::is_rvalue_reference_v<decltype(container)>, "Only accept rvalue of container");
    return Enumerable(detail::ContainedIteratorSource<T>(std::move(container)));
}


template<typename... Args, typename Eng = std::mt19937, 
    typename Gen = detail::PassthroughGenerator<decltype(std::declval<Eng&>()())>>
inline constexpr Enumerable<detail::RandomSource<Eng, Gen>>
FromRandom(Args... args)
{
    return Enumerable(detail::RandomSource<Eng, Gen>(Eng(), Gen(std::forward<Args>(args)...)));
}

template<typename Eng = std::mt19937,
    typename Gen = detail::PassthroughGenerator<decltype(std::declval<Eng&>()())>>
inline constexpr Enumerable<detail::RandomSource<Eng, Gen>>
FromRandomSource(Eng&& eng = {}, Gen&& gen = {})
{
    return Enumerable(detail::RandomSource<Eng, Gen>(std::move(eng), std::move(gen)));
}

namespace detail
{

class EnumerableChecker
{
public:
    template<typename T>
    static constexpr auto& GetProvider(T& e)
    {
        return e.Provider;
    }
    
    template<typename T> using ProviderType = decltype(std::declval<T&>().Provider);
};

}


}
}