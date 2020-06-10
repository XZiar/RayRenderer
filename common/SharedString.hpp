#pragma once
#include "CommonRely.hpp"
#include "AlignedBase.hpp"
#include <atomic>
#include <string_view>

namespace common
{

template<typename Char>
class SharedString
{
private:
    static constexpr size_t Offset = sizeof(std::atomic_uint32_t);
    std::basic_string_view<Char> StrView;
    [[nodiscard]] std::atomic_uint32_t* GetCountor() noexcept
    {
        const auto strptr = reinterpret_cast<uintptr_t>(StrView.data());
        if (strptr)
            return reinterpret_cast<std::atomic_uint32_t*>(strptr - Offset);
        else
            return nullptr;
    }
    void Increase() noexcept
    {
        auto ptrcnt = GetCountor();
        if (ptrcnt)
            (*ptrcnt)++;
    }
    void Decrease() noexcept
    {
        auto ptrcnt = GetCountor();
        if (ptrcnt && (*ptrcnt)-- == 1)
            free(ptrcnt);
    }
public:
    using value_type = const Char;
    constexpr SharedString() noexcept { }
    SharedString(const Char* const str, const size_t length) noexcept
    {
        if (length > 0 && str != nullptr)
        {
            if (uint8_t* ptr = (uint8_t*)malloc(Offset + sizeof(Char) * length); ptr)
            {
                new (ptr)std::atomic_uint32_t(1);
                Char* const ptrText = reinterpret_cast<Char*>(ptr + Offset);
                memcpy_s(ptrText, sizeof(Char) * length, str, sizeof(Char) * length);
                StrView = std::basic_string_view<Char>(ptrText, length);
                return;
            }
        }
    }
    SharedString(const Char* const str) noexcept : SharedString(str, std::char_traits<Char>::length(str)) {}
    SharedString(const std::basic_string<Char>& str) noexcept : SharedString(str.data(), str.size()) {}
    SharedString(const std::basic_string_view<Char>& sv) noexcept : SharedString(sv.data(), sv.size()) {}
    SharedString(const SharedString<Char>& other) noexcept : StrView(other.StrView)
    {
        Increase();
    }
    SharedString(SharedString<Char>&& other) noexcept : StrView(other.StrView)
    {
        other.StrView = {};
    }
    ~SharedString()
    {
        Decrease();
    }
    SharedString& operator=(const SharedString<Char>& other) noexcept
    {
        Decrease();
        StrView = other.StrView;
        Increase();
        return *this;
    }
    SharedString& operator=(SharedString<Char>&& other) noexcept
    {
        if (this != &other)
        {
            Decrease();
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
};


}

