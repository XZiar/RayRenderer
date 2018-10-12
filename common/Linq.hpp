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

template<typename Child, typename EleType>
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
    template<typename T>
    static const auto& KeyMapper(const T& data)
    {
        if constexpr (common::is_specialization<T, std::pair>::value)
            return data.first;
        else if constexpr (common::is_specialization<T, std::tuple>::value)
            return std::get<0>(data);
        else
            static_assert(common::AlwaysTrue<T>());
    }
    template<typename T>
    static const auto& ValueMapper(const T& data)
    {
        if constexpr (common::is_specialization<T, std::pair>::value)
            return data.second;
        else if constexpr (common::is_specialization<T, std::tuple>::value)
            return std::get<1>(data);
        else
            static_assert(common::AlwaysTrue<T>());
    }
};


struct EnumerableEnd
{};
template<typename SourceType>
struct EnumerableIterator
{
    SourceType& Source;
    EnumerableIterator(SourceType& source) : Source(source) {}
    decltype(auto) operator*() const
    {
        return Source.GetCurrent();
    }
    bool operator!=(const EnumerableEnd&)
    {
        return !Source.IsEnd();
    }
    bool operator==(const EnumerableEnd&)
    {
        return Source.IsEnd();
    }
    EnumerableIterator<SourceType> operator++()
    {
        Source.MoveNext();
        return *this;
    }
};


template<typename ElementType>
struct _AnyEnumerable
{
    virtual ~_AnyEnumerable() {};
    virtual ElementType GetCurrent() const = 0;
    virtual void MoveNext() = 0;
    virtual bool IsEnd() const = 0;
};


}

template<typename ElementType>
class AnyEnumerable : public std::unique_ptr<detail::_AnyEnumerable<ElementType>>
{
public:
    using std::unique_ptr<detail::_AnyEnumerable<ElementType>>::unique_ptr;
    AnyEnumerable(detail::_AnyEnumerable<ElementType>* ptr) : std::unique_ptr<detail::_AnyEnumerable<ElementType>>(ptr) {}
    detail::EnumerableIterator<detail::_AnyEnumerable<ElementType>> begin()
    {
        return this->operator*();
    }
    detail::EnumerableIterator<detail::_AnyEnumerable<ElementType>> begin() const
    {
        return this->operator*();
    }
    detail::EnumerableEnd end()
    {
        return {};
    }
    detail::EnumerableEnd end() const
    {
        return {};
    }
};


namespace detail
{

template<typename T, typename EleType>
struct _AnyEnumerableImpl : public _AnyEnumerable<EleType>
{
private:
    T Source;
public:
    _AnyEnumerableImpl(T&& source) : Source(source) {}
    virtual ~_AnyEnumerableImpl() {}
    virtual EleType GetCurrent() const override { return Source.GetCurrent(); }
    virtual void MoveNext() override { return Source.MoveNext(); }
    virtual bool IsEnd() const override { return Source.IsEnd(); }
};

template<typename Child, typename EleType>
struct Enumerable
{
public:
    using RawEleType = std::remove_cv_t<std::remove_reference_t<EleType>>;
    static constexpr bool IsKVPair = common::is_specialization<RawEleType, std::pair>::value;
    static constexpr bool CheckKVTuple() 
    {
        if constexpr (common::is_specialization<RawEleType, std::tuple>::value) 
            return std::tuple_size_v<RawEleType> == 2;
        return false;
    }
    static constexpr bool IsKVTuple = CheckKVTuple();

    AnyEnumerable<EleType> ToAnyEnumerable() 
    {
        return new _AnyEnumerableImpl<Child, EleType>(std::move(*static_cast<Child*>(this)));
    }
    EnumerableIterator<Child> begin()
    {
        return *static_cast<Child*>(this);
    }
    EnumerableIterator<Child> begin() const
    {
        return *static_cast<const Child*>(this);
    }
    EnumerableEnd end()
    {
        return {};
    }
    EnumerableEnd end() const
    {
        return {};
    }

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
    std::vector<RawEleType> ToVector()
    {
        Child* self = static_cast<Child*>(this);
        std::vector<RawEleType> ret;
        while (!self->IsEnd())
        {
            ret.push_back(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    std::list<RawEleType> ToList()
    {
        Child* self = static_cast<Child*>(this);
        std::list<RawEleType> ret;
        while (!self->IsEnd())
        {
            ret.push_back(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<typename Comparator = std::less<RawEleType>>
    std::set<RawEleType, Comparator> ToSet()
    {
        Child* self = static_cast<Child*>(this);
        std::set<RawEleType, Comparator> ret;
        while (!self->IsEnd())
        {
            ret.insert(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<typename Hasher = std::hash<RawEleType>>
    std::unordered_set<RawEleType, Hasher> ToHashSet()
    {
        Child* self = static_cast<Child*>(this);
        std::unordered_set<RawEleType, Hasher> ret;
        while (!self->IsEnd())
        {
            ret.insert(self->GetCurrent());
            self->MoveNext();
        }
        return ret;
    }
    template<bool ShouldReplace = true, typename Map = void, typename KeyF, typename ValF>
    Map ToMap(Map themap, KeyF&& keyMapper = EnumerableHelper::KeyMapper, ValF&& valMapper = EnumerableHelper::ValueMapper)
    {
        Child* self = static_cast<Child*>(this);
        while (!self->IsEnd())
        {
            const auto& tmp = self->GetCurrent();
            if constexpr (ShouldReplace)
                themap.insert_or_assign(keyMapper(tmp), valMapper(tmp));
            else
                themap.emplace(keyMapper(tmp), valMapper(tmp));
            self->MoveNext();
        }
        return themap;
    }
    template<bool ShouldReplace = true, typename Comp = void>
    auto AsMap()
    {
        static_assert(IsKVPair || IsKVTuple, "Element should be pair or tuple of 2");
        using Comparator = std::conditional_t<std::is_same_v<void, Comp>, std::less<typename MapElement<RawEleType>::KeyType>, Comp>;
        std::map<typename MapElement<RawEleType>::KeyType, typename MapElement<RawEleType>::ValType, Comparator> ret;
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
    auto AsHashMap()
    {
        static_assert(IsKVPair || IsKVTuple, "Element should be pair or tuple of 2");
        using Hasher = std::conditional_t<std::is_same_v<void, Hash>, std::hash<typename MapElement<RawEleType>::KeyType>, Hash>;
        std::unordered_map<typename MapElement<RawEleType>::KeyType, typename MapElement<RawEleType>::ValType, Hasher> ret;
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
        static_assert(std::is_invocable_v<Func, T&, const RawEleType&>, "reduce function should accept target and element");
        while (!self->IsEnd())
        {
            if constexpr (std::is_invocable_r_v<T, Func, const T&, const RawEleType&>)
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
        static_assert(std::is_invocable_r_v<bool, Func, RawEleType>, "judger should accept element and return bool");
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
        static_assert(std::is_invocable_r_v<bool, Func, RawEleType>, "judger should accept element and return bool");
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
    template<typename Func>
    auto SortBy(Func&& comparator = {});
    template<typename Other>
    auto Concat(Other&& other);
    template<typename Other>
    auto Pair(Other&& other);
    template<typename Type>
    auto Cast();
    constexpr bool Empty() const 
    { 
        return static_cast<Child*>(this)->IsEnd();
    }
    constexpr std::optional<RawEleType> TryGetFirst() const
    {
        const Child* self = static_cast<const Child*>(this);
        return self->IsEnd() ? std::optional<RawEleType>{} : self->GetCurrent();
    }
};


template<typename Source, typename Func>
struct MappedSource : public Enumerable<MappedSource<Source, Func>, std::invoke_result_t<Func, decltype(std::declval<Source&>().GetCurrent())>>
{
    Source Src;
    Func Mapper;
    MappedSource(Source&& source, Func&& mapper) : Src(source), Mapper(mapper) {}
    decltype(auto) GetCurrent() const { return Mapper(Src.GetCurrent()); }
    void MoveNext() { Src.MoveNext(); }
    bool IsEnd() const { return Src.IsEnd(); }
};

template<typename Child, typename EleType>
template<typename Func>
auto Enumerable<Child, EleType>::Select(Func&& mapper)
{
    static_assert(std::is_invocable_v<Func, RawEleType>, "mapper does not accept target element");
    Child* self = static_cast<Child*>(this);
    return MappedSource<Child, Func>(std::move(*self), std::forward<Func>(mapper));
}


template<typename Src, typename Func, typename MiddleType>
struct MultiMappedSource : public Enumerable<MultiMappedSource<Src, Func, MiddleType>, decltype(std::declval<MiddleType&>().GetCurrent())>
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
    decltype(auto) GetCurrent() const { return (*Middle).GetCurrent(); }
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

template<typename Child, typename EleType>
template<typename Func>
auto Enumerable<Child, EleType>::SelectMany(Func&& mapper)
{
    static_assert(std::is_invocable_v<Func, RawEleType>, "mapper does not accept target element");
    using MiddleType = decltype(mapper(std::declval<RawEleType>()));
    static_assert(EnumerableHelper::IsEnumerable<MiddleType>(), "mapper should return an enumerable");
    return MultiMappedSource<Child, Func, MiddleType>(std::move(*static_cast<Child*>(this)), std::forward<Func>(mapper));
}


template<typename Source, typename Func>
struct FilteredSource : public Enumerable<FilteredSource<Source, Func>, decltype(std::declval<Source&>().GetCurrent())>
{
    Source Src;
    Func Filter;
    FilteredSource(Source&& source, Func&& filter) : Src(source), Filter(filter) 
    {
        while (!Src.IsEnd() && !Filter(Src.GetCurrent()))
            Src.MoveNext();
    }
    decltype(auto) GetCurrent() const { return Src.GetCurrent(); }
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

template<typename Child, typename EleType>
template<typename Func>
auto Enumerable<Child, EleType>::Where(Func&& filter)
{
    static_assert(std::is_invocable_r_v<bool, Func, RawEleType>, "filter should accept target element and return bool");
    Child* self = static_cast<Child*>(this);
    return FilteredSource<Child, Func>(std::move(*self), std::forward<Func>(filter));
}


template<typename Src1, typename Src2>
struct ConcatedSource : public Enumerable<ConcatedSource<Src1, Src2>, decltype(std::declval<Src1&>().GetCurrent())>
{
    Src1 Source1;
    Src2 Source2;
    bool IsSrc2;
    ConcatedSource(Src1&& src1, Src2&& src2) : Source1(src1), Source2(src2), IsSrc2(Source1.IsEnd()) { }
    decltype(auto) GetCurrent() const { return IsSrc2 ? Source2.GetCurrent() : Source1.GetCurrent(); }
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

template<typename Child, typename EleType>
template<typename Other>
auto Enumerable<Child, EleType>::Concat(Other&& other)
{
    static_assert(std::is_base_of_v<Enumerable<Other, EleType>, Other>, "enumerable should be joined with an enumerable of the same type");
    return ConcatedSource<Child, Other>(std::move(*static_cast<Child*>(this)), std::move(other));
}


template<typename Src1, typename Src2>
struct PairedSource : public Enumerable<PairedSource<Src1, Src2>, std::pair<typename Src1::RawEleType, typename Src2::RawEleType>>
{
    Src1 Source1;
    Src2 Source2;
    PairedSource(Src1&& src1, Src2&& src2) : Source1(src1), Source2(src2) { }
    decltype(auto) GetCurrent() const { return std::pair<typename Src1::RawEleType, typename Src2::RawEleType>(Source1.GetCurrent(), Source2.GetCurrent()); }
    void MoveNext()
    {
        Source1.MoveNext();
        Source2.MoveNext();
    }
    bool IsEnd() const { return Source1.IsEnd() || Source2.IsEnd(); }
};

template<typename Child, typename EleType>
template<typename Other>
auto Enumerable<Child, EleType>::Pair(Other&& other)
{
    static_assert(EnumerableHelper::IsEnumerable<Other>(), "enumerable should be paired with an enumerable");
    return PairedSource<Child, Other>(std::move(*static_cast<Child*>(this)), std::move(other));
}


template<typename Src, typename DstType>
struct CastedSource : public Enumerable<CastedSource<Src, DstType>, DstType>
{
    Src Source;
    CastedSource(Src&& source) : Source(source) {}
    DstType GetCurrent() const { return static_cast<DstType>(Source.GetCurrent()); }
    void MoveNext() { Source.MoveNext(); }
    bool IsEnd() const { return Source.IsEnd(); }
};

template<typename Src, typename DstType>
struct CastCtorSource : public Enumerable<CastCtorSource<Src, DstType>, DstType>
{
    Src Source;
    CastCtorSource(Src&& source) : Source(source) {}
    DstType GetCurrent() const { return DstType(Source.GetCurrent()); }
    void MoveNext() { Source.MoveNext(); }
    bool IsEnd() const { return Source.IsEnd(); }
};

template<typename Child, typename EleType>
template<typename Type>
auto Enumerable<Child, EleType>::Cast()
{
    if constexpr (std::is_convertible_v<EleType, Type>)
        return CastedSource<Child, Type>(std::move(*static_cast<Child*>(this)));
    else if constexpr (std::is_constructible_v<Type, EleType>)
        return CastCtorSource<Child, Type>(std::move(*static_cast<Child*>(this)));
    else
        static_assert(common::AlwaysTrue<Type>(), "cannot convert to or construct to the target type");
}


template<typename T>
struct IteratorSource : public Enumerable<IteratorSource<T>, decltype(*std::declval<decltype(std::begin(std::declval<T&>()))>())>
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
    constexpr bool IsEnd() const { return !(Current != End); }
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


template<typename Child, typename EleType>
template<typename Func>
auto Enumerable<Child, EleType>::SortBy(Func&& comparator)
{
    static_assert(std::is_invocable_r_v<bool, Func, const RawEleType&, const RawEleType&>, "sort need a comparator that accept two element and return bool");
    auto tmp = ToVector();
    std::sort(tmp.begin(), tmp.end(), comparator);
    return IteratorSource(std::move(tmp));
}

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