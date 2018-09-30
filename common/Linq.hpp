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

namespace common
{
namespace linq
{

namespace detail
{


template<typename T>
struct MapElement;
template<typename K, typename V> struct MapElement<std::pair<K, V>>
{
    using KeyType = K;
    using ValType = V;
};
template<typename K, typename V> struct MapElement<std::tuple<K, V>>
{
    using KeyType = K;
    using ValType = V;
};

template<typename Child, typename ElementType>
struct Enumerable;

struct EnumerableHelper
{
    template<typename T, typename E>
    static std::true_type IsEnumerable(const Enumerable<T, E>*);
    static std::false_type IsEnumerable(const void*);
    template<typename T>
    static constexpr bool IsEnumerable()
    {
        return decltype(IsEnumerable(std::declval<const T*>()))::value;
    }
};


template<typename Child, typename ElementType>
struct Enumerable
{
    using EleType = ElementType;
    static constexpr bool IsKVPair = common::is_specialization<ElementType, std::pair>::value;
    static constexpr bool CheckKVTuple() 
    {
        if constexpr (common::is_specialization<ElementType, std::tuple>::value) return std::tuple_size_v<ElementType> == 2;
        return false;
    }
    static constexpr bool IsKVTuple = CheckKVTuple();
    Child& Skip(size_t size) 
    { 
        Child* self = static_cast<Child*>(this);
        while (size && !self->IsEnd())
        {
            self->MoveNext();
            size--;
        }
        return *self;
    }
    std::vector<ElementType> ToVector()
    {
        Child* self = static_cast<Child*>(this);
        std::vector<ElementType> ret;
        while (!self->IsEnd())
        {
            ret.push_back(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    std::list<ElementType> ToList()
    {
        Child* self = static_cast<Child*>(this);
        std::list<ElementType> ret;
        while (!self->IsEnd())
        {
            ret.push_back(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<typename Comparator = std::less<ElementType>>
    std::set<ElementType, Comparator> ToSet()
    {
        Child* self = static_cast<Child*>(this);
        std::set<ElementType, Comparator> ret;
        while (!self->IsEnd())
        {
            ret.insert(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<typename Hasher = std::hash<ElementType>>
    std::unordered_set<ElementType, Hasher> ToHashSet()
    {
        Child* self = static_cast<Child*>(this);
        std::unordered_set<ElementType, Hasher> ret;
        while (!self->IsEnd())
        {
            ret.insert(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<bool ShouldReplace = true, typename Comp = void>
    auto ToMap()
    {
        static_assert(IsKVPair || IsKVTuple, "Element should be pair or tuple of 2");
        using Comparator = std::conditional_t<std::is_same_v<void, Comp>, std::less<typename MapElement<ElementType>::KeyType>, Comp>;
        std::map<typename MapElement<ElementType>::KeyType, typename MapElement<ElementType>::ValType, Comparator> ret;
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            const auto& tmp = self->GetCurrent();
            if constexpr (IsKVPair)
            {
                if constexpr (ShouldReplace)
                    ret.insert_or_assign(tmp.first, tmp.second);
                else
                    ret.insert(tmp);
            }
            else
            {
                if constexpr (ShouldReplace)
                    ret.insert_or_assign(std::get<0>(tmp), std::get<1>(tmp));
                else
                    ret.try_emplace(std::get<0>(tmp), std::get<1>(tmp));
            }
            self->MoveNext();
        }
        return ret;
    }
    template<bool ShouldReplace = true, typename Hash = void>
    auto ToHashMap()
    {
        static_assert(IsKVPair || IsKVTuple, "Element should be pair or tuple of 2");
        using Hasher = std::conditional_t<std::is_same_v<void, Hash>, std::hash<typename MapElement<ElementType>::KeyType>, Hash>;
        std::unordered_map<typename MapElement<ElementType>::KeyType, typename MapElement<ElementType>::ValType, Hasher> ret;
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            const auto& tmp = self->GetCurrent();
            if constexpr (IsKVPair)
            {
                if constexpr (ShouldReplace)
                    ret.insert_or_assign(tmp.first, tmp.second);
                else
                    ret.insert(tmp);
            }
            else
            {
                if constexpr (ShouldReplace)
                    ret.insert_or_assign(std::get<0>(tmp), std::get<1>(tmp));
                else
                    ret.try_emplace(std::get<0>(tmp), std::get<1>(tmp));
            }
            self->MoveNext();
        }
        return ret;
    }
    template<typename Func>
    void ForEach(Func&& func)
    {
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            func(self->GetCurrent());
            self->MoveNext();
        }
    }
    template<typename T, typename Func>
    T Reduce(Func&& func, T target = {})
    {
        Child* self = static_cast<Child*>(this);
        static_assert(std::is_invocable_v<Func, T&, const ElementType&>, "reduce function should accept target and element");
        while (!self->IsEnd())
        {
            if constexpr (std::is_invocable_r_v<T, Func, const T&, const ElementType&>)
                target = func(target, self->GetCurrent());
            else
                func(target, self->GetCurrent());
            self->MoveNext();
        }
        return target;
    }
    template<typename T>
    T Sum(T target = {})
    {
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            target += self->GetCurrent();
            self->MoveNext();
        }
        return target;
    }
    template<typename T>
    bool Contains(const T& obj)
    {
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            if (obj == self->GetCurrent())
                return true;
            self->MoveNext();
        }
        return false;
    }
    template<typename Func>
    bool ContainsIf(Func&& judger)
    {
        static_assert(std::is_invocable_r_v<bool, Func, ElementType>, "judger should accept element and return bool");
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            if (judger(self->GetCurrent()))
                return true;
            self->MoveNext();
        }
        return false;
    }
    template<typename T>
    bool All(const T& obj)
    {
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            if (obj != self->GetCurrent())
                return false;
            self->MoveNext();
        }
        return true;
    }
    template<typename Func>
    bool AllIf(Func&& judger)
    {
        static_assert(std::is_invocable_r_v<bool, Func, ElementType>, "judger should accept element and return bool");
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            if (!judger(self->GetCurrent()))
                return false;
            self->MoveNext();
        }
        return true;
    }
    template<typename Func>
    auto Select(Func&& mapper);
    template<typename Func>
    auto SelectMany(Func&& mapper);
    template<typename Func>
    auto Where(Func&& filter);
    template<typename Other>
    auto Concat(Other&& other);
    template<typename Other>
    auto Pair(Other&& other);
    template<typename Type>
    auto Cast();
    constexpr bool Empty() const { return static_cast<Child*>(this)->IsEnd(); }
};


template<typename Source, typename Func, typename ElementType>
struct MappedSource : public Enumerable<MappedSource<Source, Func, ElementType>, ElementType>
{
    Source Src;
    Func Mapper;
    MappedSource(Source&& source, Func&& mapper) : Src(source), Mapper(mapper) {}
    ElementType GetCurrent() const { return Mapper(Src.GetCurrent()); }
    void MoveNext() { Src.MoveNext(); }
    bool IsEnd() const { return Src.IsEnd(); }
};

template<typename Child, typename ElementType>
template<typename Func>
auto Enumerable<Child, ElementType>::Select(Func&& mapper)
{
    static_assert(std::is_invocable_v<Func, ElementType>, "mapper does not accept target element");
    using MappedType = decltype(mapper(std::declval<ElementType>()));
    Child* self = static_cast<Child*>(this);
    return MappedSource<Child, Func, MappedType>(std::move(*self), std::forward<Func>(mapper));
}


template<typename Src, typename Func, typename MiddleType, typename ElementType>
struct MultiMappedSource : public Enumerable<MultiMappedSource<Src, Func, MiddleType, ElementType>, ElementType>
{
    Src Source; // Source is at next element of Middle
    Func Mapper;
    std::optional<MiddleType> Middle; // won't keep ended state
    MultiMappedSource(Src&& source, Func&& mapper) : Source(source), Mapper(mapper)
    {
        while (!Source.IsEnd())
        {
            auto tmp = Mapper(Source.GetCurrent());
            Source.MoveNext();
            if (!tmp.IsEnd()) // only set Middle when is not ended
            {
                Middle.emplace(std::move(tmp));
                return;
            }
        }
    }
    ElementType GetCurrent() const { return (*Middle).GetCurrent(); }
    void MoveNext() 
    {
        if (!Middle.has_value()) // has end of source
            return;
        auto& middle = Middle.value();
        middle.MoveNext();
        if (!middle.IsEnd()) // successfully move to next
            return;
        // need to update middle
        Middle.reset();
        while (!Source.IsEnd())
        {
            auto tmp = Mapper(Source.GetCurrent());
            Source.MoveNext();
            if (!tmp.IsEnd()) // only set Middle when is not ended
            {
                Middle.emplace(std::move(tmp));
                return;
            }
        }
    }
    bool IsEnd() const { return Source.IsEnd() && !Middle.has_value(); }
};

template<typename Child, typename ElementType>
template<typename Func>
auto Enumerable<Child, ElementType>::SelectMany(Func&& mapper)
{
    static_assert(std::is_invocable_v<Func, ElementType>, "mapper does not accept target element");
    using MiddleType = decltype(mapper(std::declval<ElementType>()));
    using MappedType = decltype(std::declval<MiddleType&>().GetCurrent());
    static_assert(std::is_base_of_v<Enumerable<MiddleType, MappedType>, MiddleType>, "mapper should return an enumerable");
    return MultiMappedSource<Child, Func, MiddleType, MappedType>(std::move(*static_cast<Child*>(this)), std::forward<Func>(mapper));
}


template<typename Source, typename Func, typename ElementType>
struct FilteredSource : public Enumerable<FilteredSource<Source, Func, ElementType>, ElementType>
{
    Source Src;
    Func Filter;
    FilteredSource(Source&& source, Func&& filter) : Src(source), Filter(filter) 
    {
        while (!Src.IsEnd() && !Filter(Src.GetCurrent()))
            Src.MoveNext();
    }
    ElementType GetCurrent() const { return Src.GetCurrent(); }
    void MoveNext() 
    {
        if (Src.IsEnd())
            return;
        while (true)
        {
            Src.MoveNext();
            if (Src.IsEnd())
                break;
            if (Filter(Src.GetCurrent()))
                break;
        }
    }
    bool IsEnd() const { return Src.IsEnd(); }
};

template<typename Child, typename ElementType>
template<typename Func>
auto Enumerable<Child, ElementType>::Where(Func&& filter)
{
    static_assert(std::is_invocable_r_v<bool, Func, ElementType>, "filter should accept target element and return bool");
    Child* self = static_cast<Child*>(this);
    return FilteredSource<Child, Func, ElementType>(std::move(*self), std::forward<Func>(filter));
}


template<typename Src1, typename Src2, typename ElementType>
struct ConcatedSource : public Enumerable<ConcatedSource<Src1, Src2, ElementType>, ElementType>
{
    Src1 Source1;
    Src2 Source2;
    bool IsSrc2;
    ConcatedSource(Src1&& src1, Src2&& src2) : Source1(src1), Source2(src2), IsSrc2(Source1.IsEnd()) { }
    ElementType GetCurrent() const { return IsSrc2 ? Source2.GetCurrent() : Source1.GetCurrent(); }
    void MoveNext()
    {
        if (!IsSrc2)
        {
            if (!Source1.IsEnd())
            {
                Source1.MoveNext(); 
                IsSrc2 = Source1.IsEnd();
                return;
            }
            IsSrc2 = true;
        }
        if (!Source2.IsEnd())
            Source2.MoveNext();
    }
    bool IsEnd() const { return Source1.IsEnd() && Source2.IsEnd(); }
};

template<typename Child, typename ElementType>
template<typename Other>
auto Enumerable<Child, ElementType>::Concat(Other&& other)
{
    static_assert(std::is_base_of_v<Enumerable<Other, ElementType>, Other>, "enumerable should be joined with an enumerable of the same type");
    return ConcatedSource<Child, Other, ElementType>(std::move(*static_cast<Child*>(this)), std::move(other));
}


template<typename Src1, typename Src2, typename ElementType>
struct PairedSource : public Enumerable<PairedSource<Src1, Src2, ElementType>, ElementType>
{
    Src1 Source1;
    Src2 Source2;
    PairedSource(Src1&& src1, Src2&& src2) : Source1(src1), Source2(src2) { }
    ElementType GetCurrent() const { return std::pair(Source1.GetCurrent(), Source2.GetCurrent()); }
    void MoveNext()
    {
        Source1.MoveNext();
        Source2.MoveNext();
    }
    bool IsEnd() const { return Source1.IsEnd() || Source2.IsEnd(); }
};

template<typename Child, typename ElementType>
template<typename Other>
auto Enumerable<Child, ElementType>::Pair(Other&& other)
{
    static_assert(EnumerableHelper::IsEnumerable<Other>(), "enumerable should be paired with an enumerable");
    return PairedSource<Child, Other, std::pair<ElementType, typename Other::EleType>>(std::move(*static_cast<Child*>(this)), std::move(other));
}


template<typename Src, typename ElementType>
struct CastedSource : public Enumerable<CastedSource<Src, ElementType>, ElementType>
{
    Src Source;
    CastedSource(Src&& source) : Source(source) {}
    ElementType GetCurrent() const { return static_cast<ElementType>(Source.GetCurrent()); }
    void MoveNext() { Source.MoveNext(); }
    bool IsEnd() const { return Source.IsEnd(); }
};

template<typename Child, typename ElementType>
template<typename Type>
auto Enumerable<Child, ElementType>::Cast()
{
    static_assert(std::is_convertible_v<ElementType, Type>, "cannot convert current type to the target type");
    return CastedSource<Child, Type>(std::move(*static_cast<Child*>(this)));
}


template<typename T>
struct IteratorSource : public Enumerable<IteratorSource<T>, std::remove_cv_t<std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>>>
{
    using TB = decltype(std::begin(std::declval<T&>()));
    using TE = decltype(std::end(std::declval<T&>()));
    std::optional<T> Source;
    TB Current;
    TE End;
    constexpr IteratorSource(T&& source) : Source(std::move(source)), Current(std::begin(*Source)), End(std::end(*Source)) {}
    constexpr IteratorSource(T& source) : Source(), Current(std::begin(source)), End(std::end(source)) {}
    constexpr decltype(*std::declval<TB>()) GetCurrent() const { return *Current; }
    constexpr void MoveNext() { ++Current; }
    constexpr bool IsEnd() const { return Current == End; }
    constexpr bool Empty() const { return IsEnd(); }
};

template<typename T>
struct NumericRangeSource : public Enumerable<NumericRangeSource<T>, T>
{
    T Current;
    const T Step;
    T End;
    constexpr NumericRangeSource(T current, T step, T end) : Current(current), Step(step), End(end) {}
    constexpr T GetCurrent() const { return Current; }
    constexpr void MoveNext() { Current += Step; }
    constexpr bool IsEnd() const { return Step < 0 ? Current <= End : Current >= End; }
    constexpr bool Empty() const { return IsEnd(); }
};

template<typename T>
struct RepeatSource : public Enumerable<RepeatSource<T>, T>
{
    const T Val;
    size_t Count;
    constexpr RepeatSource(T val, size_t count) : Val(val), Count(count) {}
    constexpr T GetCurrent() const { return Val; }
    constexpr void MoveNext() { if (Count > 0) Count--; }
    constexpr bool IsEnd() const { return Count == 0; }
    constexpr bool Empty() const { return IsEnd(); }
};

}

struct Linq
{
    // Code from https://stackoverflow.com/questions/13830158/check-if-a-variable-is-iterable/29634934#29634934
    template <typename T>
    static auto IsIterable(int) -> decltype (
        std::begin(std::declval<T&>()) != std::end(std::declval<T&>()), // begin/end and operator !=
        void(), // Handle evil operator ,
        ++std::declval<decltype(std::begin(std::declval<T&>()))&>(), // operator ++
        void(*std::begin(std::declval<T&>())), // operator*
        std::true_type{});
    template <typename T>
    static std::false_type IsIterable(...);

    template<typename T>
    static decltype(auto) FromIterable(T&& source)
    {
        using CVType = std::remove_reference_t<decltype(source)>;
        using RawType = std::remove_cv_t<CVType>;
        static_assert(decltype(IsIterable<CVType>(0))::value, "only Iterable type supported");

        if constexpr (std::is_rvalue_reference_v<decltype(source)>)
            return detail::IteratorSource<RawType>(std::move(source));
        else if constexpr (std::is_const_v<CVType>)
            return detail::IteratorSource<const RawType>(source);
        else
            return detail::IteratorSource<RawType>(source);
    }
    template<typename T>
    static constexpr detail::NumericRangeSource<T> FromRange(const T begin, const T end, const T step = 1)
    {
        return detail::NumericRangeSource<T>(begin, step, end);
    }
    template<typename T>
    static constexpr detail::RepeatSource<T> Repeat(const T val, const size_t count)
    {
        static_assert(std::is_copy_constructible_v<T>, "type should be able to copy-construct");
        return detail::RepeatSource<T>(val, count);
    }
};

}
}