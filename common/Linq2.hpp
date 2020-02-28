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
#include <tuple>

namespace common
{
namespace linq
{

template<typename T>
class Enumerable;

namespace detail
{
class EnumerableChecker;

struct EnumerableEnd
{};


struct EnumerableObject
{
private:
    template<typename T>
    using GetEnumeratorTyper = decltype(std::declval<T&>().GetEnumerator());
public:
    template<typename T>
    static constexpr void Check()
    {
        static_assert(common::is_detected_v<GetEnumeratorTyper, T>, "EnumerableObject should has a GetEnumerator method");
    }
};


template<typename T>
struct NumericRangeSource
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Need numeric type");
    using OutType = T;
    static constexpr bool InvolveCache      = false;
    static constexpr bool IsCountable       = true;
    static constexpr bool CanSkipMultiple   = true;
    
    T Current;
    T Step;
    T End;

    constexpr NumericRangeSource(T current, T step, T end) noexcept : Current(current), Step(step), End(end) {}
    [[nodiscard]] constexpr T GetCurrent() const noexcept { return Current; }
    constexpr void MoveNext() noexcept { Current += Step; }
    [[nodiscard]] constexpr bool IsEnd() const noexcept
    { 
        return Step < 0 ? Current <= End : Current >= End;
    }
    [[nodiscard]] constexpr size_t Count() const noexcept
    { 
        return IsEnd() ? 0 : static_cast<size_t>((End - Current) / Step);
    }
    constexpr void MoveMultiple(const size_t count) noexcept 
    { 
        Current += static_cast<T>(count * Step);
    }
};


template<typename T>
struct RepeatSource
{
    static_assert(std::is_copy_constructible_v<T>, "Need copyable type");
    using OutType = T;
    static constexpr bool InvolveCache      = false;
    static constexpr bool IsCountable       = true;
    static constexpr bool CanSkipMultiple   = true;

    T Val;
    size_t Avaliable;

    constexpr RepeatSource(T val, const size_t count) : Val(std::move(val)), Avaliable(count) {}
    [[nodiscard]] constexpr T GetCurrent() const { return Val; }
    constexpr void MoveNext() noexcept { Avaliable--; }
    [[nodiscard]] constexpr bool IsEnd() const noexcept { return Avaliable == 0; }
    [[nodiscard]] constexpr size_t Count() const noexcept { return Avaliable; }
    constexpr void MoveMultiple(const size_t count) noexcept { Avaliable -= count; }
};


template<typename T>
struct PassthroughGenerator
{
    template<typename U>
    [[nodiscard]] T operator()(U& eng) const noexcept { return static_cast<T>(eng()); }
};

template<typename T, typename U>
struct RandomSource
{
    using OutType = decltype(std::declval<U&>()(std::declval<T&>()));
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = true;
    static constexpr bool CanSkipMultiple   = true;

    mutable T Engine;
    mutable U Generator;

    constexpr RandomSource(T&& eng, U&& gen) : Engine(std::move(eng)), Generator(std::move(gen)) {}
    [[nodiscard]] OutType GetCurrent() const { return Generator(Engine); }
    constexpr void MoveNext() noexcept { }
    [[nodiscard]] constexpr bool IsEnd() const noexcept { return false; }
    [[nodiscard]] constexpr size_t Count() const noexcept { return SIZE_MAX; }
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

    [[nodiscard]] constexpr T& Obj() { return *Object; }
    [[nodiscard]] constexpr const T& Obj() const { return *Object; }
};

template<typename TB, typename TE>
struct IteratorSource
{
    using OutType = decltype(*std::declval<TB&>());
    static constexpr bool InvolveCache      = false;
    static constexpr bool IsCountable       = false;
    static constexpr bool CanSkipMultiple   = true;

    TB Begin;
    TE End;

    constexpr IteratorSource(TB begin, TE end) : Begin(std::move(begin)), End(std::move(end)) {}
    [[nodiscard]] constexpr OutType GetCurrent() const { return *Begin; }
    constexpr void MoveNext() { ++Begin; }
    [[nodiscard]] constexpr bool IsEnd() const { return !(Begin != End); }
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
    static constexpr bool InvolveCache      = false;
    static constexpr bool IsCountable       = false;
    static constexpr bool CanSkipMultiple   = true;

    template<typename U>
    constexpr ContainedIteratorSource(U&& obj) :
        ObjCache<T>(std::move(obj)), BaseType(std::begin(this->Obj()), std::end(this->Obj())) {}
    using BaseType::GetCurrent;
    using BaseType::MoveNext;
    using BaseType::IsEnd;
    using BaseType::MoveMultiple;
};


template<typename P>
struct LimitSource
{
private:
    P Prev;
    size_t Avaliable;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    static constexpr bool InvolveCache      = P::InvolveCache;
    static constexpr bool IsCountable       = P::IsCountable;
    static constexpr bool CanSkipMultiple   = P::CanSkipMultiple;

    constexpr LimitSource(P&& prev, const size_t n) : Prev(std::move(prev)), Avaliable(n)
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return Prev.GetCurrent();
    }
    constexpr void MoveNext()
    {
        if (Avaliable != 0)
        {
            Avaliable--; 
            Prev.MoveNext();
        }
    }
    [[nodiscard]] constexpr bool IsEnd() const
    { 
        return Avaliable == 0 || Prev.IsEnd();
    }
    [[nodiscard]] constexpr size_t Count() const
    {
        if constexpr (P::IsCountable)
            return std::min(Avaliable, Prev.Count());
        else
            static_assert(!common::AlwaysTrue<P>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (P::CanSkipMultiple)
            Prev.MoveMultiple(count), Avaliable -= count;
        else
            static_assert(!common::AlwaysTrue<P>(), "Not movemultiple");
    }
};


template<typename P, bool ShouldCache>
struct FilteredSourceBase;
template<typename P>
struct FilteredSourceBase<P, false>
{
protected:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    
    P Prev;

    constexpr FilteredSourceBase(P&& prev) : Prev(std::move(prev)) { }

    template<typename Filter>
    constexpr void LoopUntilSatisfy_(Filter& filter)
    {
        while (!this->Prev.IsEnd())
        {
            if (filter(this->Prev.GetCurrent()))
                return;
            this->Prev.MoveNext();
        }
    }

    [[nodiscard]] constexpr bool IsEnd() const
    {
        return Prev.IsEnd();
    }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return Prev.GetCurrent();
    }
};
template<typename P>
struct FilteredSourceBase<P, true>
{
protected:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    using CacheType = std::conditional_t<std::is_reference_v<InType>,
        std::reference_wrapper<std::remove_reference_t<InType>>,
        PlainInType>;

    P Prev;
    mutable std::optional<CacheType> Temp;

    constexpr FilteredSourceBase(P&& prev) : Prev(std::move(prev)) { }

    template<typename Filter>
    constexpr void LoopUntilSatisfy_(Filter& filter)
    {
        using ConstArg = std::add_lvalue_reference_t<std::add_const_t<PlainInType>>;
        using NonConstArg = std::add_lvalue_reference_t<PlainInType>;
        using ArgType = std::conditional_t<std::is_invocable_v<Filter, ConstArg>, ConstArg, NonConstArg>;
        
        while (!this->Prev.IsEnd())
        {
            InType obj = this->Prev.GetCurrent();
            if (filter(static_cast<ArgType>(obj)))
            {
                Temp.emplace(std::move(obj));
                return;
            }
            this->Prev.MoveNext();
        }
    }

    [[nodiscard]] constexpr bool IsEnd() const
    {
        return Prev.IsEnd();
    }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return *std::move(Temp);
    }
};


template<typename P, typename... Filters>
struct FilteredManySource;

template<typename P, typename Filter>
struct FilteredSource : public FilteredSourceBase<P, P::InvolveCache>
{
    template<typename, typename...> friend struct FilteredManySource;
private:
    using BaseType = FilteredSourceBase<P, P::InvolveCache>;
    Filter Func;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    using PrevType = P;
    static constexpr bool InvolveCache = P::InvolveCache;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = false;

    constexpr FilteredSource(P&& prev, Filter&& filter) : 
        BaseType(std::move(prev)), Func(std::forward<Filter>(filter))
    {
        this->LoopUntilSatisfy_(Func);
    }
    constexpr void MoveNext()
    {
        this->Prev.MoveNext();
        this->LoopUntilSatisfy_(Func);
    }

    using BaseType::IsEnd;
    using BaseType::GetCurrent;
};


template<typename PlainInType, typename... Filters>
struct FilteredManyFiler
{
    using ConstArg = std::add_lvalue_reference_t<std::add_const_t<PlainInType>>;
    using NonConstArg = std::add_lvalue_reference_t<PlainInType>;
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(Filters)>{};
    template<size_t... I>
    [[nodiscard]] static constexpr bool CheckSupportConst(std::index_sequence<I...>)
    {
        using Tuple = std::tuple<Filters...>;
        return (std::is_invocable_v<std::tuple_element_t<I, Tuple>, ConstArg> && ...);
    }
    static constexpr bool IsAllConst = CheckSupportConst(Indexes);
    using ArgType = std::conditional_t<IsAllConst, ConstArg, NonConstArg>;

    constexpr FilteredManyFiler(std::tuple<Filters...>&& filter) : Funcs(std::move(filter))
    { }

    template<size_t... I>
    [[nodiscard]] constexpr bool FilterAll(ArgType obj, std::index_sequence<I...>)
    {
        return (std::get<I>(Funcs)(obj) && ...);
    }

    [[nodiscard]] constexpr bool CheckForLast(ArgType obj)
    {
        auto& filter = std::get<sizeof...(Filters) - 1>(Funcs);
        return filter(obj);
    }

    [[nodiscard]] constexpr bool operator()(ArgType obj)
    {
        return FilterAll(obj, Indexes);
    }

    std::tuple<Filters...> Funcs;
};

template<typename P, typename... Filters>
struct FilteredManySource : public FilteredSourceBase<P, P::InvolveCache>
{
    template<typename, typename...> friend struct FilteredManySource;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = InType;
    static constexpr bool InvolveCache = P::InvolveCache;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = false;
private:
    struct DummyTag {};
    using BaseType = FilteredSourceBase<P, P::InvolveCache>;

    FilteredManyFiler<PlainInType, Filters...> Func;

    template<typename T>
    constexpr FilteredManySource(DummyTag, T&& prev, std::tuple<Filters...>&& filter) :
        BaseType(std::move(prev)), Func(std::move(filter))
    {
        if (!IsEnd())
        {
            if constexpr (P::InvolveCache)
            {
                if (Func.CheckForLast(*this->Temp))
                    return;
                else
                    this->Temp.reset();
            }
            else
            {
                if (Func.CheckForLast(this->Prev.GetCurrent()))
                    return;
            }
            MoveNext();
        }
    }
public:
    template<typename T, typename F>
    constexpr FilteredManySource(FilteredSource<P, T>&& prev, F&& filter) :
        FilteredManySource(DummyTag{}, std::move(prev), 
            std::tuple<Filters...>(std::move(prev.Func), std::forward<F>(filter)))
    {
        static_assert(std::is_same_v<std::tuple<Filters...>, std::tuple<T, F>>);
    }
    template<typename... Ts, typename F>
    constexpr FilteredManySource(FilteredManySource<P, Ts...>&& prev, F&& filter) :
        FilteredManySource(DummyTag{}, std::move(prev),
            std::tuple<Filters...>(std::tuple_cat(std::move(prev.Func.Funcs), std::make_tuple(std::forward<F>(filter)))))
    {
        static_assert(std::is_same_v<std::tuple<Filters...>, std::tuple<Ts..., F>>);
    }

    constexpr void MoveNext()
    {
        this->Prev.MoveNext();
        this->LoopUntilSatisfy_(Func);
    }

    using BaseType::IsEnd;
    using BaseType::GetCurrent;
};
template<typename P, typename T, typename F>
FilteredManySource(FilteredSource<P, T>&&, F&&) -> FilteredManySource<P, T, F>;
template<typename P, typename... Ts, typename F>
FilteredManySource(FilteredManySource<P, Ts...>&&, F&&) -> FilteredManySource<P, Ts..., F>;


template<typename P>
struct NestedSource
{
private:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
protected:
    P Prev;
    constexpr NestedSource(P&& prev) : Prev(std::move(prev)) { }

    [[nodiscard]] constexpr size_t Count() const
    {
        if constexpr (P::IsCountable)
            return Prev.Count();
        else
            static_assert(!common::AlwaysTrue<P>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (P::CanSkipMultiple)
            Prev.MoveMultiple(count);
        else
            static_assert(!common::AlwaysTrue<P>(), "Not movemultiple");
    }
};


template<typename P, typename Mapper, bool NeedCache>
struct MappedSource : public NestedSource<P>
{
private:
    using BaseType = NestedSource<P>;
    mutable Mapper Func;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = std::invoke_result_t<Mapper, InType>;
    static constexpr bool InvolveCache      = P::InvolveCache || NeedCache;
    static constexpr bool IsCountable       = P::IsCountable;
    static constexpr bool CanSkipMultiple   = P::CanSkipMultiple;

    constexpr MappedSource(P&& prev, Mapper&& mapper) : BaseType(std::move(prev)), Func(std::move(mapper))
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return Func(this->Prev.GetCurrent());
    }
    constexpr void MoveNext()
    {
        this->Prev.MoveNext();
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return this->Prev.IsEnd();
    }

    using BaseType::Count;
    using BaseType::MoveMultiple;
};


template<typename P, typename Mapper>
struct FlatMappedSource
{
private:
    P Prev;
    mutable Mapper Func;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using MidType = typename std::invoke_result_t<Mapper, InType>::ProviderType;
    using OutType = typename MidType::OutType;
    static constexpr bool InvolveCache      = MidType::InvolveCache;
    static constexpr bool IsCountable       = false;
    static constexpr bool CanSkipMultiple   = false;


    constexpr FlatMappedSource(P&& prev, Mapper&& mapper) : Prev(std::move(prev)), Func(std::move(mapper))
    {
        LoadNextBatch();
    }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return Middle->GetCurrent();
    }
    constexpr void MoveNext()
    {
        Middle->MoveNext();
        if (Middle->IsEnd())
        {
            Middle.reset();
            LoadNextBatch();
        }
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return !Middle.has_value();
    }
private:
    mutable std::optional<MidType> Middle;
    void LoadNextBatch()
    {
        while (!Prev.IsEnd())
        {
            auto tmp = Func(Prev.GetCurrent()).Provider;
            Prev.MoveNext();
            if (!tmp.IsEnd())
            {
                Middle.emplace(std::move(tmp));
                break;
            }
        }
    }
};


struct TupledSourceHelper
{
    template<typename OutType, typename Tuple, size_t... I>
    [[nodiscard]] constexpr static auto GetCurrent(Tuple&& t, std::index_sequence<I...>)
    {
        return OutType(std::get<I>(std::forward<Tuple>(t)).GetCurrent()...);
    }
    template<typename Tuple, size_t... I>
    constexpr static void MoveNext(Tuple&& t, std::index_sequence<I...>)
    {
        (std::get<I>(std::forward<Tuple>(t)).MoveNext(), ...);
    }
    template<typename Tuple, size_t... I>
    [[nodiscard]] constexpr static bool IsEnd(Tuple&& t, std::index_sequence<I...>)
    {
        return (std::get<I>(std::forward<Tuple>(t)).IsEnd() || ...);
    }
    template<typename... Ns>
    [[nodiscard]] constexpr static size_t Count2(const size_t t, const Ns... ns)
    {
        if constexpr (sizeof...(Ns) == 0)
            return t;
        else
            return std::min(t, Count2(ns...));
    }
    template<typename Tuple, size_t... I>
    [[nodiscard]] constexpr static size_t Count(Tuple&& t, std::index_sequence<I...>)
    {
        return Count2(std::get<I>(std::forward<Tuple>(t)).Count()...);
    }
    template<typename Tuple, size_t... I>
    constexpr static void MoveMultiple(const size_t count, Tuple&& t, std::index_sequence<I...>)
    {
        (std::get<I>(std::forward<Tuple>(t)).MoveMultiple(count), ...);
    }
    /*template<typename T>
    struct CheckRef
    {
        using Type = std::conditional_t<std::is_reference_v<T>,
            std::reference_wrapper<std::remove_reference_t<T>>,
            std::remove_cv_t<T>>;
    };
    template<typename T> using ProxyType = typename CheckRef<typename T::OutType>::Type;*/
};

template<typename... Ps>
struct TupledSource
{
private:
    std::tuple<Ps...> Prevs;
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(Ps)>{};
public:
    using InTypes = std::tuple<typename Ps::OutType...>; // tuple support reference
    using OutType = InTypes;
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = (... && Ps::IsCountable);
    static constexpr bool CanSkipMultiple   = (... && Ps::CanSkipMultiple);

    constexpr TupledSource(Ps&&... prevs) : Prevs(std::forward<Ps>(prevs)...)
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return TupledSourceHelper::GetCurrent<OutType>(Prevs, Indexes);
    }
    constexpr void MoveNext()
    {
        TupledSourceHelper::MoveNext(Prevs, Indexes);
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return TupledSourceHelper::IsEnd(Prevs, Indexes);
    }
    [[nodiscard]] constexpr size_t Count() const
    {
        if constexpr (IsCountable)
            return TupledSourceHelper::Count(Prevs, Indexes);
        else
            static_assert(!common::AlwaysTrue<InTypes>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (CanSkipMultiple)
            TupledSourceHelper::MoveMultiple(count, Prevs, Indexes);
        else
            static_assert(!common::AlwaysTrue<InTypes>(), "Not movemultiple");
    }
};


template<typename P1, typename P2>
struct PairedSource
{
private:
    P1 Prev1;
    P2 Prev2;
public:
    using InType1 = typename P1::OutType;
    using InType2 = typename P2::OutType;
    using OutType = std::pair<InType1, InType2>; // pair support reference
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = P1::IsCountable && P2::IsCountable;
    static constexpr bool CanSkipMultiple   = P1::CanSkipMultiple && P2::CanSkipMultiple;

    constexpr PairedSource(P1&& prev1, P2&& prev2) : Prev1(std::move(prev1)), Prev2(std::move(prev2))
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return OutType{ Prev1.GetCurrent(), Prev2.GetCurrent() };
    }
    constexpr void MoveNext()
    {
        Prev1.MoveNext();
        Prev2.MoveNext();
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return Prev1.IsEnd() || Prev2.IsEnd();
    }
    [[nodiscard]] constexpr size_t Count() const
    {
        if constexpr (IsCountable)
            return std::min(Prev1.Count(), Prev2.Count());
        else
            static_assert(!common::AlwaysTrue<P1>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (CanSkipMultiple)
            Prev1.MoveMultiple(count), Prev2.MoveMultiple(count);
        else
            static_assert(!common::AlwaysTrue<P1>(), "Not movemultiple");
    }
};


struct ConcatedSourceHelper
{
    template<typename T1, typename T2>
    struct EqualChecker
    {
        static_assert(std::is_same_v<T1, T2>, "no common type deducted");
        //static_assert(std::is_reference_v<T1> || !std::is_const_v<T1> || !KeepConst, "no common type deducted");
        using Type = T1;
    };
    template<typename T1, typename T2, bool KeepConst>
    struct ConstChecker
    {
        static constexpr bool NeedConst = (std::is_const_v<T1> || std::is_const_v<T2>) && KeepConst;
        using Type_ = typename EqualChecker<std::remove_const_t<T1>, std::remove_const_t<T2>>::Type;
        using Type = std::conditional_t<NeedConst,
            std::add_const_t<Type_>,
            Type_>;
    };
    template<typename T1, typename T2, bool KeepConst>
    struct PointerChecker
    {
        static_assert(std::is_pointer_v<T1> == std::is_pointer_v<T2>, "no common type deducted(pointer/non-pointer)");
        static constexpr bool IsPointer = std::is_pointer_v<T1>;
        static constexpr bool NeedKeepConst = IsPointer || KeepConst;
        using Type_ = typename ConstChecker<std::remove_pointer_t<T1>, std::remove_pointer_t<T2>, NeedKeepConst>::Type;
        using Type = std::conditional_t<IsPointer,
            std::add_pointer_t<Type_>,
            Type_>;
    };
    template<typename T1, typename T2>
    struct RefChecker
    {
        static constexpr bool IsRef = std::is_reference_v<T1> && std::is_reference_v<T2>;
        using Type_ = typename PointerChecker<std::remove_reference_t<T1>, std::remove_reference_t<T2>, IsRef>::Type;
        using Type = std::conditional_t<IsRef,
            std::add_lvalue_reference_t<Type_>,
            Type_>;
    };
    template<typename T1, typename T2>
    using Type = typename RefChecker<T1, T2>::Type;
    template<typename T1, typename T2>
    using ConvCheckType = std::conditional_t<std::is_convertible_v<T1, T2>, T2, T1>;
    template<typename T1, typename T2>
    using Type2 = std::conditional_t<std::is_reference_v<ConvCheckType<T1, T2>>, 
        ConvCheckType<T1, T2>, std::remove_const_t<ConvCheckType<T1, T2>>>;
};
template<typename P1, typename P2>
struct ConcatedSource
{
private:
    P1 Prev1;
    P2 Prev2;
    bool IsSrc2;
public:
    using InType1 = typename P1::OutType;
    using InType2 = typename P2::OutType;
    using OutType = ConcatedSourceHelper::Type<InType1, InType2>;
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = P1::IsCountable && P2::IsCountable;
    static constexpr bool CanSkipMultiple   = P1::CanSkipMultiple && P2::CanSkipMultiple && IsCountable;

    constexpr ConcatedSource(P1&& prev1, P2&& prev2) : 
        Prev1(std::move(prev1)), Prev2(std::move(prev2)), IsSrc2(Prev1.IsEnd())
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return IsSrc2 ? Prev2.GetCurrent() : Prev1.GetCurrent();
    }
    constexpr void MoveNext()
    {
        IsSrc2 ? Prev2.MoveNext() : Prev1.MoveNext();
        if (!IsSrc2)
            IsSrc2 = Prev1.IsEnd();
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return IsSrc2 ? Prev2.IsEnd() : Prev1.IsEnd();
    }
    [[nodiscard]] constexpr size_t Count() const
    {
        if constexpr (IsCountable)
            return Prev1.Count() + Prev2.Count();
        else
            static_assert(!common::AlwaysTrue<P1>(), "Not countable");
    }
    constexpr void MoveMultiple(const size_t count)
    {
        if constexpr (CanSkipMultiple)
        {
            const auto count1 = std::min(Prev1.Count(), count);
            Prev1.MoveMultiple(count1);
            Prev2.MoveMultiple(count - count1);
        }
        else
            static_assert(!common::AlwaysTrue<P1>(), "Not movemultiple");
    }
};


template<typename P, typename T>
struct CastedSource : public NestedSource<P>
{
private:
    using BaseType = NestedSource<P>;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = T;
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = P::IsCountable;
    static constexpr bool CanSkipMultiple   = P::CanSkipMultiple;

    constexpr CastedSource(P&& prev) : BaseType(std::move(prev))
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return static_cast<OutType>(this->Prev.GetCurrent());
    }
    constexpr void MoveNext()
    {
        this->Prev.MoveNext();
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return this->Prev.IsEnd();
    }

    using BaseType::Count;
    using BaseType::MoveMultiple;
};


template<typename P, typename T>
struct CastCtorSource : public NestedSource<P>
{
private:
    using BaseType = NestedSource<P>;
public:
    using InType = typename P::OutType;
    using PlainInType = std::remove_cv_t<InType>;
    using OutType = T;
    static constexpr bool InvolveCache      = true;
    static constexpr bool IsCountable       = P::IsCountable;
    static constexpr bool CanSkipMultiple   = P::CanSkipMultiple;

    constexpr CastCtorSource(P&& prev) : BaseType(std::move(prev))
    { }
    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        return OutType(this->Prev.GetCurrent());
    }
    constexpr void MoveNext()
    {
        this->Prev.MoveNext();
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return this->Prev.IsEnd();
    }

    using BaseType::Count;
    using BaseType::MoveMultiple;
};


}


template<typename T>
class Enumerable;
template<typename T>
inline constexpr Enumerable<T> ToEnumerable(T&& source);

template<typename T>
class Enumerable
{
    friend class detail::EnumerableChecker;
    template<typename> friend class Enumerable;
    template<typename, typename> friend struct detail::FlatMappedSource;
public:
    using ProviderType = T;
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
        using iterator_category = std::input_iterator_tag;
        using value_type = EleType;
        using difference_type = std::ptrdiff_t;
        using pointer = std::add_pointer_t<EleType>;
        using reference = EleType;

        Enumerable<T>* Source;
        constexpr EnumerableIterator(Enumerable<T>* source) noexcept : Source(source) {}
        constexpr decltype(auto) operator*() const
        {
            return Source->Provider.GetCurrent();
        }
        constexpr bool operator!=(const detail::EnumerableEnd&) const
        {
            return !Source->Provider.IsEnd();
        }
        constexpr bool operator==(const detail::EnumerableEnd&) const
        {
            return Source->Provider.IsEnd();
        }
        constexpr EnumerableIterator& operator++()
        {
            Source->Provider.MoveNext();
            return *this;
        }
        constexpr EnumerableIterator& operator+=(size_t n)
        {
            if (T::IsCountable && T::CanSkipMultiple)
                Source->Skip(n);
            return *this;
        }
    };
public:
    constexpr EnumerableIterator begin() noexcept
    {
        return this;
    }
    constexpr EnumerableIterator begin() const noexcept
    {
        return this;
    }
    constexpr detail::EnumerableEnd end() noexcept
    {
        return {};
    }
    constexpr detail::EnumerableEnd end() const noexcept
    {
        return {};
    }

    [[nodiscard]] constexpr Enumerable<T>& Skip(size_t n)
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

    [[nodiscard]] constexpr Enumerable<detail::LimitSource<T>> 
        Take(const size_t n)
    {
        return detail::LimitSource<T>(std::move(Provider), n);
    }

    template<bool ForceCache, typename Mapper>
    [[nodiscard]] constexpr Enumerable<detail::MappedSource<T, common::remove_cvref_t<Mapper>, ForceCache>> 
        Select(Mapper&& mapper)
    {
        static_assert(std::is_invocable_v<Mapper, EleType>, "mapper does not accept EleType");
        return detail::MappedSource<T, common::remove_cvref_t<Mapper>, ForceCache>(std::move(Provider), std::forward<Mapper>(mapper));
    }

    template<typename Mapper>
    [[nodiscard]] constexpr decltype(auto) 
        Select(Mapper&& mapper)
    {
        static_assert(std::is_invocable_v<Mapper, EleType>, "mapper does not accept EleType");
        using OutType = std::invoke_result_t<Mapper, EleType>;
        constexpr bool ShouldCache = !std::is_lvalue_reference_v<OutType>;
        return Select<ShouldCache>(std::forward<Mapper>(mapper));
    }

    template<typename Mapper>
    [[nodiscard]] constexpr Enumerable<detail::FlatMappedSource<T, common::remove_cvref_t<Mapper>>> 
        SelectMany(Mapper&& mapper)
    {
        static_assert(std::is_invocable_v<Mapper, EleType>, "mapper does not accept EleType");
        using MidType = std::invoke_result_t<Mapper, EleType>;
        static_assert(common::is_specialization<MidType, Enumerable>::value, "mapper does not return an Enumerable");
        return detail::FlatMappedSource<T, common::remove_cvref_t<Mapper>>(std::move(Provider), std::forward<Mapper>(mapper));
    }

    template<typename Filter>
    [[nodiscard]] constexpr decltype(auto)
        Where(Filter&& filter)
    {
        static_assert(std::is_invocable_r_v<bool, Filter, EleType>
            || std::is_invocable_r_v<bool, Filter, std::add_lvalue_reference_t<PlainEleType>>,
            "filter should accept EleType and return bool");
        if constexpr (common::is_specialization<T, detail::FilteredManySource>::value 
            || common::is_specialization<T, detail::FilteredSource>::value)
        {
            return ToEnumerable(
                detail::FilteredManySource(std::move(Provider), std::forward<Filter>(filter))
            );
        }
        else
        {
            return ToEnumerable(
                detail::FilteredSource<T, common::remove_cvref_t<Filter>>
                (std::move(Provider), std::forward<Filter>(filter))
            );
        }
    }

    template<typename Other>
    [[nodiscard]] constexpr Enumerable<detail::PairedSource<T, Other>> 
        Pair(Enumerable<Other>&& other)
    {
        return detail::PairedSource<T, Other>(std::move(Provider), std::move(other.Provider));
    }

    template<typename... Others>
    [[nodiscard]] constexpr Enumerable<detail::TupledSource<T, Others...>> 
        Pairs(Enumerable<Others>&&... others)
    {
        return detail::TupledSource<T, Others...>(std::move(Provider), std::move(others.Provider)...);
    }

    template<typename Other>
    [[nodiscard]] constexpr Enumerable<detail::ConcatedSource<T, Other>> 
        Concat(Enumerable<Other>&& other)
    {
        return detail::ConcatedSource<T, Other>(std::move(Provider), std::move(other.Provider));
    }

    template<typename DstType>
    [[nodiscard]] constexpr auto Cast()
    {
        if constexpr (std::is_convertible_v<EleType, DstType>)
            return ToEnumerable(detail::CastedSource<T, DstType>(std::move(Provider)));
        else if constexpr (std::is_constructible_v<DstType, EleType>)
            return ToEnumerable(detail::CastCtorSource<T, DstType>(std::move(Provider)));
        else
            static_assert(!common::AlwaysTrue<DstType>(), "cannot convert to or construct to the target type");
    }

    template<typename Func>
    [[nodiscard]] Enumerable<detail::ContainedIteratorSource<std::vector<PlainEleType>>> 
        OrderBy(Func&& comparator = {})
    {
        static_assert(std::is_invocable_r_v<bool, Func, const PlainEleType&, const PlainEleType&>, 
            "sort need a comparator that accepts two element and returns bool");
        auto tmp = ToVector();
        std::sort(tmp.begin(), tmp.end(), comparator);
        return detail::ContainedIteratorSource<std::vector<PlainEleType>>(std::move(tmp));
    }

    [[nodiscard]] constexpr size_t Count()
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
        [[maybe_unused]] size_t idx = 0;
        while (!Provider.IsEnd())
        {
            if constexpr(std::is_invocable_v<Func, EleType>)
                func(Provider.GetCurrent());
            else if constexpr (std::is_invocable_v<Func, EleType, size_t>)
                func(Provider.GetCurrent(), idx++);
            else
                static_assert(!AlwaysTrue<Func>, "foreach function should accept element, and maybe index");
            Provider.MoveNext();
        }
    }

    template<typename U, typename Func>
    [[nodiscard]] constexpr U Reduce(Func&& func, U data = {})
    {
        [[maybe_unused]] size_t idx = 0;
        while (!Provider.IsEnd())
        {
            if constexpr (std::is_invocable_v<U, Func, const U&, EleType, size_t>)
                data = func(data, Provider.GetCurrent(), idx++);
            else if constexpr (std::is_invocable_v<Func, U&, EleType, size_t>)
                func(data, Provider.GetCurrent(), idx++);
            else if constexpr (std::is_invocable_r_v<U, Func, const U&, EleType>)
                data = func(data, Provider.GetCurrent());
            else if constexpr (std::is_invocable_v<Func, U&, EleType>)
                func(data, Provider.GetCurrent());
            else
                static_assert(!AlwaysTrue<U>, "reduce function should accept target and element, and maybe index");
            Provider.MoveNext();
        }
        return data;
    }

    template<typename U = EleType>
    [[nodiscard]] constexpr U Sum(U data = {})
    {
        return Reduce([](U& ret, const auto& item) { ret += item; }, data);
    }

    [[nodiscard]] constexpr std::optional<PlainEleType> TryGetFirst() const
    {
        return Provider.IsEnd() ? std::optional<PlainEleType>{} : Provider.GetCurrent();
    }

    [[nodiscard]] constexpr bool Empty() const
    {
        return Provider.IsEnd();
    }

    template<typename U>
    [[nodiscard]] constexpr bool Contains(const U& obj)
    {
        while (!Provider.IsEnd())
        {
            if constexpr (common::is_equal_comparable_v<U, EleType>)
            {
                if (obj == Provider.GetCurrent())
                    return true;
            }
            else if constexpr (common::is_notequal_comparable_v<U, EleType>)
            {
                if (!(obj != Provider.GetCurrent()))
                    return true;
            }
            else
                static_assert(!common::AlwaysTrue<U>(), "Type U is not eq/ne comparable with EleType");
            Provider.MoveNext();
        }
        return false;
    }
    template<typename Func>
    [[nodiscard]] constexpr bool ContainsIf(Func&& judger)
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
    [[nodiscard]] constexpr bool All(const U& obj)
    {
        while (!Provider.IsEnd())
        {
            if constexpr (common::is_equal_comparable_v<U, EleType>)
            {
                if (!(obj == Provider.GetCurrent()))
                    return false;
            }
            else if constexpr (common::is_notequal_comparable_v<U, EleType>)
            {
                if (obj != Provider.GetCurrent())
                    return false;
            }
            else
                static_assert(!common::AlwaysTrue<U>(), "Type U is not eq/ne comparable with EleType");
            Provider.MoveNext();
        }
        return true;
    }
    template<typename Func>
    [[nodiscard]] constexpr bool AllIf(Func&& judger)
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

    [[nodiscard]] std::vector<PlainEleType> ToVector()
    {
        std::vector<PlainEleType> ret;
        if constexpr (T::IsCountable)
            ret.reserve(Provider.Count());
        while (!Provider.IsEnd())
        {
            ret.emplace_back(Provider.GetCurrent());
            Provider.MoveNext();
        }
        return ret;
    }
    [[nodiscard]] std::list<PlainEleType> ToList()
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
    [[nodiscard]] std::set<PlainEleType, Comparator> ToOrderSet()
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
    [[nodiscard]] std::unordered_set<PlainEleType, Hasher> ToHashSet()
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
    [[nodiscard]] auto ToAnySet()
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
    [[nodiscard]] auto ToAnyMap(KeyF&& keyMapper, ValF&& valMapper)
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
[[nodiscard]] inline constexpr Enumerable<T> ToEnumerable(T&& source)
{
    return Enumerable<T>(std::forward<T>(source));
}


template<typename T>
[[nodiscard]] inline constexpr decltype(auto) FromEnumerableObject(T&& source)
{
    detail::EnumerableObject::Check<T>();
    return ToEnumerable(source.GetEnumerator());
}


template<typename T>
[[nodiscard]] inline constexpr Enumerable<detail::NumericRangeSource<T>>
FromRange(const T begin, const T end, const T step = static_cast<T>(1))
{
    return Enumerable(detail::NumericRangeSource<T>(begin, step, end));
}


template<typename T>
[[nodiscard]] inline constexpr Enumerable<detail::RepeatSource<common::remove_cvref_t<T>>>
FromRepeat(T&& val, const size_t count)
{
    return Enumerable(detail::RepeatSource<common::remove_cvref_t<T>>(std::forward<T>(val), count));
}


template<typename T>
[[nodiscard]] inline constexpr auto
FromIterable(T& container)
{
    return Enumerable(detail::IteratorSource(std::begin(container), std::end(container)));
}


template<typename T>
[[nodiscard]] inline constexpr auto
FromContainer(T&& container)
{
    static_assert(std::is_rvalue_reference_v<decltype(container)>, "Only accept rvalue of container");
    return Enumerable(detail::ContainedIteratorSource<T>(std::move(container)));
}


template<typename... Args, typename Eng = std::mt19937, 
    typename Gen = detail::PassthroughGenerator<decltype(std::declval<Eng&>()())>>
[[nodiscard]] inline constexpr Enumerable<detail::RandomSource<Eng, Gen>>
FromRandom(Args... args)
{
    return Enumerable(detail::RandomSource<Eng, Gen>(Eng(), Gen(std::forward<Args>(args)...)));
}

template<typename Eng = std::mt19937,
    typename Gen = detail::PassthroughGenerator<decltype(std::declval<Eng&>()())>>
[[nodiscard]] inline constexpr Enumerable<detail::RandomSource<Eng, Gen>>
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
    [[nodiscard]] static constexpr auto& GetProvider(T& e)
    {
        return e.Provider;
    }
};

}


}
}