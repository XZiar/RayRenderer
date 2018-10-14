#pragma once
#include "CommonRely.hpp"
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
    std::atomic_uint32_t* GetCountor() noexcept
    {
        const auto strptr = reinterpret_cast<const uint8_t*>(StrView.data());
        if (strptr)
            return reinterpret_cast<std::atomic_uint32_t*>(const_cast<uint8_t*>(strptr - Offset));
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
            free_align(ptrcnt);
    }
public:
    SharedString(const Char* const str, const size_t length)
    {
        if (length == 0)
        {
            StrView = std::basic_string_view<Char>(nullptr, 0);
        }
        else
        {
            uint8_t* ptr = (uint8_t*)malloc_align(Offset + sizeof(Char)*length, 64);
            if (!ptr)
                throw std::bad_alloc();
            new (ptr)std::atomic_uint32_t(1);
            Char* const ptrText = reinterpret_cast<Char*>(ptr + Offset);
            memcpy_s(ptrText, sizeof(Char)*length, str, sizeof(Char)*length);
            StrView = std::basic_string_view<Char>(ptrText, length);
        }
    }
    SharedString(const Char* const str) : SharedString(str, std::char_traits<Char>::length(str)) {}
    SharedString(const std::basic_string<Char>& str) : SharedString(str.data(), str.size()) {}
    SharedString(const std::basic_string_view<Char>& sv) : SharedString(sv.data(), sv.size()) {}
    SharedString(const SharedString<Char>& other) noexcept : StrView(other.StrView)
    {
        Increase();
    }
    SharedString(SharedString<Char>&& other) noexcept : StrView(other.StrView)
    {
        other.StrView = std::basic_string_view<Char>(nullptr, 0);
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
            other.StrView = std::basic_string_view<Char>(nullptr, 0);
        }
        return *this;
    }
    operator const std::basic_string_view<Char>&() const noexcept
    {
        return StrView;
    }
};


}

