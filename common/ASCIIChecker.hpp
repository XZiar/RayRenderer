#include "CommonRely.hpp"

namespace common
{

#if COMMON_COMPILER_MSVC && _MSC_VER <= 1914
#   pragma message("ASCIIChecker may not be supported due to lack of constexpr support before VS 15.7")
#endif

template<size_t N>
struct ASCIICheckerNBit
{
#if COMMON_OSBIT == 32
    using EleType = uint32_t;
#elif COMMON_OSBIT == 64
    using EleType = uint64_t;
#endif
    static_assert(8 % N == 0, "bit N should be the common factor of 8");
protected:
    static constexpr uint8_t EleBits = sizeof(EleType) * 8;
    static constexpr EleType KeepMask = (0x1u << N) - 1;
    static constexpr uint32_t RangeMask = ~127u;
    std::array<EleType, 128 * N / EleBits> LUT = { 0 };
    constexpr ASCIICheckerNBit() noexcept {}
public:
    template<typename T, typename U>
    constexpr ASCIICheckerNBit(const T defVal, const U& mappings) noexcept
    {
        static_assert(std::is_same_v<std::decay_t<typename U::value_type>, std::pair<char, T>>);
        EleType baseVal = 0;
        for (size_t i = 0; i < EleBits; i += N)
            baseVal = (baseVal << N) | (static_cast<EleType>(defVal) & KeepMask);
        for (auto& ele : LUT)
            ele = baseVal;
        static_assert(std::is_unsigned_v<T>, "val should be unsigned");
        for (const auto& [ch, val] : mappings)
        {
            if (ch >= 0) // char is signed
            {
                auto& ele = LUT[ch * N / EleBits];
                const auto shiftBits = (ch * N) % EleBits;
                const auto cleanMask = ~(KeepMask << shiftBits);
                ele &= cleanMask;
                const auto obj = (static_cast<EleType>(val) & KeepMask) << shiftBits;
                ele |= obj;
            }
        }
    }
    template<typename T>
    [[nodiscard]] constexpr EleType operator()(const T ch, EleType defVal) const noexcept
    {
        if (const auto ch32 = static_cast<uint32_t>(ch); ch32 & RangeMask)
            // ch < 0 || ch >= 128
            return defVal;
        else
        {
            const auto ele = LUT[ch32 * N / EleBits];
            const auto ret = ele >> ((ch32 * N) % EleBits);
            return ret & KeepMask;
        }
    }
};

template<bool Result = true>
struct ASCIIChecker : ASCIICheckerNBit<1>
{
    constexpr ASCIIChecker(const std::string_view str) noexcept : ASCIICheckerNBit()
    {
        for (auto& ele : LUT)
            ele = Result ? std::numeric_limits<EleType>::min() : std::numeric_limits<EleType>::max();
        for (const auto ch : str)
        {
            if (ch >= 0) // char is signed
            {
                auto& ele = LUT[ch / EleBits];
                const auto obj = KeepMask << (ch % EleBits);
                if constexpr (Result)
                    ele |= obj;
                else
                    ele &= (~obj);
            }
        }
    }
    template<typename T>
    [[nodiscard]] constexpr bool operator()(const T ch) const noexcept
    {
        if (const auto ch32 = static_cast<uint32_t>(ch); ch32 & RangeMask)
            // ch < 0 || ch >= 128
            return !Result;
        else
        {
            const auto ele = LUT[ch32 / EleBits];
            const auto bit = KeepMask << (ch32 % EleBits);
            return ele & bit;
        }
    }
};

}