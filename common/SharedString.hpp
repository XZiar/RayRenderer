#pragma once
#include "CommonRely.hpp"
#include <atomic>
#include <string_view>

namespace common
{

template<typename Char>
class SharedString : public NonMovable
{
private:
    static constexpr size_t Offset = sizeof(std::atomic_uint32_t);
    std::basic_string_view<Char> StrView;
    std::atomic_uint32_t& GetCountor()
    {
        return *reinterpret_cast<std::atomic_uint32_t*>(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(StrView.data()) - Offset));
    }
    void Increase()
    {
        GetCountor()++;
    }
    bool Decrease()
    {
        return GetCountor()-- == 1;
    }
public:
    SharedString(const Char* const str, const size_t length)
    {
        uint8_t* ptr = (uint8_t*)malloc_align(Offset + sizeof(Char)*length, 64);
        if (!ptr)
            throw std::bad_alloc();
        new (ptr)std::atomic_uint32_t(1);
        Char* const ptrText = reinterpret_cast<Char*>(ptr + Offset);
        memcpy_s(ptrText, sizeof(Char)*length, str, sizeof(Char)*length);
        StrView = std::basic_string_view<Char>(ptrText, length);
    }
    SharedString(const std::basic_string<Char>& str) : SharedString(str.data(), str.size()) {}
    SharedString(const std::basic_string_view<Char>& sv) : SharedString(sv.data(), sv.size()) {}
    SharedString(const SharedString<Char>& other)
    {
        StrView = other.StrView;
        Increase();
    }
    ~SharedString()
    {
        if (Decrease())
            free_align(&GetCountor());
    }
    SharedString& operator=(const SharedString<Char>& other) = delete;
    operator const std::basic_string_view<Char>&() const
    {
        return StrView;
    }
};


}

