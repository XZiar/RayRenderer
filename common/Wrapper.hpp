#pragma once

#include "CommonRely.hpp"
#include <type_traits>
#include <utility>
#include <memory>
#include <string>


namespace common
{

template<class T>
class Wrapper : public std::shared_ptr<T>
{
private:
    using base_type = std::shared_ptr<T>;
    constexpr static auto is_def_con_able = std::is_default_constructible_v<T>;
    constexpr static auto is_self_share = std::is_base_of_v<std::enable_shared_from_this<T>, T>;
public:
    using inner_type = T;
    using weak_type = std::weak_ptr<T>;

    constexpr Wrapper() noexcept { }
    template<class = typename std::enable_if_t<is_def_con_able>>
    Wrapper(std::in_place_t) : base_type(std::make_shared<T>())
    { }

    template<class U, class = typename std::enable_if<std::is_convertible_v<U*, T*>>::type>
    Wrapper(const Wrapper<U>& other) noexcept : base_type(std::static_pointer_cast<T>(other))
    { }
    template<class U, class = typename std::enable_if<std::is_convertible_v<U*, T*>>::type>
    Wrapper(Wrapper<U>&& other) noexcept : base_type(static_cast<Wrapper<T>&&>(other))
    { }

    Wrapper(const base_type& other) noexcept : base_type(other)
    { }
    Wrapper(base_type&& other) noexcept : base_type(other)
    { }

    template<class = typename std::enable_if<is_self_share>::type>
    Wrapper(T * const src) noexcept : base_type(src->shared_from_this())
    { }
    template<class = typename std::enable_if<!is_self_share>::type>
    explicit Wrapper(const T * const src) noexcept : base_type(src)
    { }
    explicit Wrapper(T * const src) noexcept : base_type(src)
    { }

    template<typename Arg, typename = typename std::enable_if<!std::is_base_of_v<Wrapper<T>, std::remove_reference_t<Arg>>>::type>
    explicit Wrapper(Arg&& arg) : base_type(std::make_shared<T>(std::forward<Arg>(arg)))
    { }
    template<typename Arg, typename... Args, typename = typename std::enable_if_t<sizeof...(Args) != 0>>
    explicit Wrapper(Arg&& arg, Args&&... args) : base_type(std::make_shared<T>(std::forward<Arg>(arg), std::forward<Args>(args)...))
    { }

    template<class... Args>
    void reset(Args&&... args)
    {
        *this = Wrapper<T>(std::forward<Args>(args)...);
    }
    template<class = typename std::enable_if<is_def_con_able>::type>
    void reset()
    {
        *this = Wrapper<T>(std::in_place);
    }
    void release()
    {
        base_type::reset();
    }

    weak_type weakRef() const noexcept
    {
        return weak_type(*this);
    }
    template<class U>
    Wrapper<U> cast_static() const noexcept
    {
        return Wrapper<U>(std::static_pointer_cast<U>(*this));
    }
    template<class U, class = typename std::enable_if<std::is_convertible_v<U*, T*> || std::is_convertible_v<T*, U*>>::type>
    Wrapper<U> cast_dynamic() const noexcept
    {
        return Wrapper<U>(std::dynamic_pointer_cast<U>(*this));
    }

    Wrapper(const Wrapper<T>& other) noexcept = default;
    Wrapper(Wrapper<T>&& other) noexcept = default;
    Wrapper& operator=(const Wrapper<T>& other) noexcept = default;
    Wrapper& operator=(Wrapper<T>&& other) noexcept = default;
};

template<typename T>
struct SharedPtrHelper
{
private:
    template<typename Real>
    static std::true_type is_derived_from_sharedptr_impl(const std::shared_ptr<Real>*);
    static std::false_type is_derived_from_sharedptr_impl(const void*);
public:
    static constexpr bool IsSharedPtr = decltype(is_derived_from_sharedptr_impl(std::declval<T*>()))::value;
};


}