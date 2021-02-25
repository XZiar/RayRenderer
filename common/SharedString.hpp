#pragma once
#include "CommonRely.hpp"
#include "RefHolder.hpp"
#include <string_view>

namespace common
{

template<typename Char>
class COMMON_EMPTY_BASES SharedString : public FixedLenRefHolder<SharedString<Char>, Char>
{
    friend RefHolder<SharedString<Char>>;
    friend FixedLenRefHolder<SharedString<Char>, Char>;
private:
    std::basic_string_view<Char> StrView;
    [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
    {
        return reinterpret_cast<uintptr_t>(StrView.data());
    }
    forceinline void Destruct() noexcept { }
public:
    using value_type = const Char;
    constexpr SharedString() noexcept { }
    SharedString(const Char* const str, const size_t length) noexcept
    {
        if (length > 0 && str != nullptr)
        {
            const auto space = FixedLenRefHolder<SharedString<Char>, Char>::Allocate(length);
            if (space.size() > 0)
            {
                memcpy_s(space.data(), sizeof(Char) * space.size(), str, sizeof(Char) * length);
                StrView = std::basic_string_view<Char>(space.data(), space.size());
            }
        }
    }
    SharedString(const Char* const str) noexcept : SharedString(str, std::char_traits<Char>::length(str)) {}
    SharedString(const std::basic_string<Char>& str) noexcept : SharedString(str.data(), str.size()) {}
    SharedString(const std::basic_string_view<Char>& sv) noexcept : SharedString(sv.data(), sv.size()) {}
    SharedString(const SharedString<Char>& other) noexcept : StrView(other.StrView)
    {
        this->Increase();
    }
    SharedString(SharedString<Char>&& other) noexcept : StrView(other.StrView)
    {
        other.StrView = {};
    }
    ~SharedString()
    {
        this->Decrease();
    }
    SharedString& operator=(const SharedString<Char>& other) noexcept
    {
        this->Decrease();
        StrView = other.StrView;
        this->Increase();
        return *this;
    }
    SharedString& operator=(SharedString<Char>&& other) noexcept
    {
        if (this != &other)
        {
            this->Decrease();
            StrView = other.StrView;
            other.StrView = {};
        }
        return *this;
    }
    constexpr operator std::basic_string_view<Char>() const noexcept
    {
        return StrView;
    }
    constexpr operator common::span<const Char>() const noexcept
    {
        return common::span<const Char>(StrView.data(), StrView.size());
    }

    decltype(auto) begin() const
    {
        return StrView.begin();
    }
    decltype(auto) end() const
    {
        return StrView.end();
    }
    decltype(auto) data() const
    {
        return StrView.data();
    }
    decltype(auto) size() const
    {
        return StrView.size();
    }

    template<typename T>
    static std::basic_string_view<Char> PlacedCreate(const T& obj)
    {
        SharedString str(obj);
        str.Increase();
        return str;
    }
    static void PlacedIncrease(std::basic_string_view<Char>& obj)
    {
        SharedString& str = *reinterpret_cast<common::SharedString<char32_t>*>(&obj);
        str.Increase();
    }
    static void PlacedDecrease(std::basic_string_view<Char>& obj)
    {
        SharedString& str = *reinterpret_cast<common::SharedString<char32_t>*>(&obj);
        str.Decrease();
    }
};


}

