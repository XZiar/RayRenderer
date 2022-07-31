#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef SYSCOMMON_EXPORT
#   define SYSCOMMONAPI _declspec(dllexport)
# else
#   define SYSCOMMONAPI _declspec(dllimport)
# endif
#else
# define SYSCOMMONAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <string_view>


namespace common
{

#if COMMON_OS_WIN
[[nodiscard]] SYSCOMMONAPI uint32_t GetWinBuildNumber() noexcept;
#endif

enum class CommonColor : uint8_t
{
    Black = 0, Red = 1, Green = 2, Yellow = 3, Blue = 4, Magenta = 5, Cyan = 6, White = 7,
    BrightBlack = 8, BrightRed = 9, BrightGreen = 10, BrightYellow = 11, BrightBlue = 12, BrightMagenta = 13, BrightCyan = 14, BrightWhite = 15
};
MAKE_ENUM_BITFIELD(CommonColor)
[[nodiscard]] SYSCOMMONAPI std::string_view GetColorName(CommonColor color) noexcept;

struct ScreenColor
{
    enum class ColorType { Default = 0, Bit8, Bit24, Common };
    uint8_t Value[3];
    ColorType Type : 4;
    bool IsBackground : 2;
    bool IsUnchanged : 2;
    constexpr ScreenColor(bool isBackground) noexcept :
        Value{ 0,0,0 }, Type(ColorType::Default), IsBackground(isBackground), IsUnchanged(false) { }
    constexpr ScreenColor(bool isBackground, uint8_t color) noexcept :
        Value{ color,0,0 }, Type(ColorType::Bit8), IsBackground(isBackground), IsUnchanged(false) { }
    constexpr ScreenColor(bool isBackground, std::array<uint8_t, 3> color) noexcept :
        Value{ color[0], color[1], color[2] }, Type(ColorType::Bit24), IsBackground(isBackground), IsUnchanged(false) { }
    constexpr ScreenColor(bool isBackground, uint8_t red, uint8_t green, uint8_t blue) noexcept :
        Value{ red, green, blue }, Type(ColorType::Bit24), IsBackground(isBackground), IsUnchanged(false) { }
    constexpr ScreenColor(bool isBackground, CommonColor color) noexcept :
        Value{ static_cast<uint8_t>(color), 0, 0 }, Type(ColorType::Common), IsBackground(isBackground), IsUnchanged(false) { }
    constexpr bool operator==(const ScreenColor& other) const noexcept
    {
        if (Type == other.Type && IsBackground == other.IsBackground)
        {
            switch (Type)
            {
            case ColorType::Default: return true;
            case ColorType::Bit8:
            case ColorType::Common: return Value[0] == other.Value[0];
            case ColorType::Bit24: return Value[0] == other.Value[0] && Value[1] == other.Value[1] && Value[2] == other.Value[2];
            default: return false;
            }
        }
        return false;
    }
    constexpr bool operator!=(const ScreenColor& other) const noexcept
    {
        return !operator==(other);
    }
};


[[nodiscard]] SYSCOMMONAPI bool CheckCPUFeature(str::HashedStrView<char> feature) noexcept;
[[nodiscard]] SYSCOMMONAPI span<const std::string_view> GetCPUFeatures() noexcept;

class FastPathBase
{
public:
    class PathInfo
    {
        friend FastPathBase;
        class MethodInfo
        {
            friend PathInfo;
            friend FastPathBase;
            void* FuncPtr;
        public:
            str::HashedStrView<char> MethodName;
            MethodInfo(std::string_view name, void* ptr) noexcept : FuncPtr(ptr), MethodName(name) { }
        };
        void*& (*Access)(FastPathBase&) noexcept = nullptr;
    public:
        str::HashedStrView<char> FuncName;
        std::vector<MethodInfo> Variants;
        PathInfo(std::string_view name, void*& (*access)(FastPathBase&) noexcept) noexcept : Access(access), FuncName(name) { }
    };
    using VarItem = std::pair<std::string_view, std::string_view>;
    COMMON_NO_COPY(FastPathBase)
    COMMON_NO_MOVE(FastPathBase)
    [[nodiscard]] virtual bool IsComplete() const noexcept = 0;
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
protected:
    FastPathBase() noexcept {}
    SYSCOMMONAPI void Init(common::span<const PathInfo> info, common::span<const VarItem> requests) noexcept;
private:
    std::vector<VarItem> VariantMap;
};
namespace fastpath
{
struct PathHack;
}
template<typename T>
class RuntimeFastPath : public FastPathBase
{
private:
    using FastPathBase::Init;
protected:
    void Init(common::span<const VarItem> requests) noexcept { Init(T::GetSupportMap(), requests); }
};


namespace str::detail
{
template <typename T>
inline common::span<const std::byte> ToByteSpan(T&& arg)
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_span_v<T>)
        return common::as_bytes(arg);
    else if constexpr (common::has_valuetype_v<U>)
        return ToByteSpan(common::span<std::add_const_t<typename U::value_type>>(arg));
    else if constexpr (std::is_convertible_v<T, common::span<const std::byte>>)
        return arg;
    else if constexpr (std::is_pointer_v<U>)
        return ToByteSpan(std::basic_string_view(arg));
    else if constexpr (std::is_array_v<U>)
        return ToByteSpan(std::basic_string_view(arg));
    else
        static_assert(!common::AlwaysTrue<T>, "unsupported");
}
template <typename T>
inline constexpr common::str::Encoding GetDefaultEncoding() noexcept
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_span_v<T>)
        return common::str::DefaultEncoding<typename T::value_type>;
    else if constexpr (common::has_valuetype_v<U>)
        return common::str::DefaultEncoding<typename U::value_type>;
    else if constexpr (std::is_pointer_v<U>)
        return common::str::DefaultEncoding<std::remove_pointer_t<U>>;
    else if constexpr (std::is_array_v<U>)
        return common::str::DefaultEncoding<std::remove_extent_t<U>>;
    else if constexpr (std::is_convertible_v<T, common::span<const std::byte>>)
        return common::str::DefaultEncoding<std::byte>;
    else
        return common::str::DefaultEncoding<T>;
}
}

}