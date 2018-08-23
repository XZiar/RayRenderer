
#include "common/SIMD128.hpp"
#include "common/SIMD256.hpp"

#include "fmt/format.h"
#include "fmt/utfext.h"
#include "common/ColorConsole.inl"
#include <tuple>
#include <array>

template<class... Args>
static std::u16string_view ToU16Str(const std::u16string_view& formater, Args&&... args)
{
    static fmt::basic_memory_buffer<char16_t> buffer;
    buffer.resize(0);
    fmt::format_to(buffer, formater, std::forward<Args>(args)...);
    return std::u16string_view(buffer.data(), buffer.size());
}
static std::u16string_view ToU16Str(const std::u16string_view& content)
{
    return content;
}

enum class LogType { Error, Success, Info, Verbose };
static std::u16string ToColor(const LogType type)
{
    using namespace common::console;
    switch (type)
    {
    case LogType::Error:    return ConsoleHelper::GetColorStr(ConsoleColor::BrightRed);
    case LogType::Success:  return ConsoleHelper::GetColorStr(ConsoleColor::BrightGreen);
    default:
    case LogType::Info:     return ConsoleHelper::GetColorStr(ConsoleColor::BrightWhite);
    case LogType::Verbose:  return ConsoleHelper::GetColorStr(ConsoleColor::BrightCyan);
    }
}
static const common::console::ConsoleHelper CONHELPER;
template<class... Args>
static void Log(const LogType type, const std::u16string_view& formater, Args&&... args)
{
    const auto txt = ToColor(type).append(ToU16Str(formater, std::forward<Args>(args)...)).append(u"\x1b[39m\n");
    CONHELPER.Print(txt);
}


template<size_t N>
std::string GeneratePosString(const std::array<uint8_t, N>& poses)
{
    std::string ret;
    for (size_t i = 0; i < N; ++i)
        ret.append(std::to_string(poses[i])).push_back(',');
    ret.pop_back();
    return ret;
}

template<typename T>
std::string GenerateValString(const T& data)
{
    std::string ret;
    for (size_t i = 0; i < T::Count; ++i)
        ret.append(std::to_string(data.Val[i])).push_back(',');
    ret.pop_back();
    return ret;
}

template<typename T, size_t... Indexes, size_t N>
bool RealCheck(const T& data, std::index_sequence<Indexes...>, const std::array<uint8_t, N>& poses)
{
    return (... && (data.Val[Indexes] == poses[Indexes]));
}

template<typename T, uint32_t PosCount, uint8_t PosRange, uint64_t N, uint8_t... Poses>
bool GenerateShuffle(const T& data)
{
    if constexpr (PosCount == 0)
    {
        const auto output = data.Shuffle< Poses... >();
        const auto poses = std::array<uint8_t, sizeof...(Poses)>{ Poses... };
        const bool ret = RealCheck(output, std::make_index_sequence<sizeof...(Poses)>{}, poses);
        //if (!ret)
            Log(ret ? LogType::Verbose : LogType::Error, u"at poses [{}], get val [{}]", GeneratePosString(poses), GenerateValString<T>(output));
        return ret;
    }
    else
    {
        return GenerateShuffle<T, PosCount - 1, PosRange, N / PosRange, Poses..., (uint8_t)(N%PosRange)>(data);
    }
}

template<typename T, uint8_t PosCount, uint8_t IdxRange, size_t Idx>
bool TestShuffle(const T& data)
{
    if constexpr (Idx == 0)
        return GenerateShuffle<T, PosCount, IdxRange, Idx>(data);
    else
        return GenerateShuffle<T, PosCount, IdxRange, Idx>(data) && TestShuffle<T, PosCount, IdxRange, Idx-1>(data);
}


int main()
{
    using namespace common::simd;
    Log(LogType::Info, u"begin shuffle test on F64x4\n");
    {
        F64x4 data(0, 1, 2, 3);
        const auto ret = TestShuffle<F64x4, 4, 4, 4 * 4 * 4 * 4 - 1>(data);
        if (ret)
            Log(LogType::Success, u"Test passes.\n");
        else
            Log(LogType::Error, u"Test failed.\n");
    }
    if constexpr (false)
    {
        Log(LogType::Info, u"begin shuffle test on F32x8\n");
        F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
        const auto ret = TestShuffle<F32x8, 8, 8, 8 * 8 * 8 * 8 * 8 * 8 * 8 * 8 - 1>(data);
        if (ret)
            Log(LogType::Success, u"Test passes.\n");
        else
            Log(LogType::Error, u"Test failed.\n");
    }
#if defined(COMPILER_MSVC) && COMPILER_MSVC
    getchar();
#endif
}