#pragma once
#include "SIMDRely.h"

#include "common/SIMD128.hpp"
#include "common/SIMD256.hpp"
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


template<size_t N>
std::string GeneratePosString(const uint8_t(&poses)[N])
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
bool RealCheck(const T& data, std::index_sequence<Indexes...>, const uint8_t(&poses)[N])
{
    return (... && (data.Val[Indexes] == poses[Indexes]));
}

template<typename T, uint32_t PosCount, uint8_t PosRange, uint64_t N, uint8_t... Poses>
bool GenerateShuffle(const T& data)
{
    if constexpr (PosCount == 0)
    {
        const auto output = data.template Shuffle<Poses...>();
        const uint8_t poses[sizeof...(Poses)]{ Poses... };
        //const auto poses = std::array<uint8_t, sizeof...(Poses)>{ Poses... };
        const bool ret = RealCheck(output, std::make_index_sequence<sizeof...(Poses)>{}, poses);
        if (!ret)
            Log(ret ? LogType::Verbose : LogType::Error, u"at poses [{}], get val [{}]", GeneratePosString(poses), GenerateValString<T>(output));
        return ret;
    }
    else
    {
        return GenerateShuffle<T, PosCount - 1, PosRange, N / PosRange, Poses..., (uint8_t)(N%PosRange)>(data);
    }
}

template<typename T, uint8_t PosCount, uint8_t IdxRange, size_t IdxBegin, size_t IdxEnd>
bool TestShuffle(const T& data)
{
    if constexpr (IdxBegin == IdxEnd)
        return GenerateShuffle<T, PosCount, IdxRange, IdxBegin>(data);
    else
        return TestShuffle<T, PosCount, IdxRange, IdxBegin, IdxBegin + (IdxEnd - IdxBegin) / 2>(data)
        && TestShuffle<T, PosCount, IdxRange, IdxBegin + (IdxEnd - IdxBegin) / 2 + 1, IdxEnd>(data);
}

template<typename T, uint32_t PosCount, uint8_t PosRange, typename... Poses>
bool GenerateShuffleVar(const T& data, [[maybe_unused]]const uint64_t N, Poses... poses)
{
    if constexpr (PosCount == 0)
    {
        const auto output = data.Shuffle(poses...);
        const uint8_t posArray[sizeof...(Poses)]{ poses... };
        //const auto posArray = std::array<uint8_t, sizeof...(Poses)>{ poses... };
        const bool ret = RealCheck(output, std::make_index_sequence<sizeof...(Poses)>{}, posArray);
        if (!ret)
            Log(ret ? LogType::Verbose : LogType::Error, u"at poses [{}], get val [{}]", GeneratePosString(posArray), GenerateValString<T>(output));
        return ret;
    }
    else
    {
        return GenerateShuffleVar<T, PosCount - 1, PosRange>(data, N / PosRange, poses..., (uint8_t)(N%PosRange));
    }
}

template<typename T, uint8_t PosCount, uint8_t IdxRange>
bool TestShuffleVar(const T& data, uint64_t idxBegin, uint64_t idxEnd)
{
    if (idxBegin == idxEnd)
        return GenerateShuffleVar<T, PosCount, IdxRange>(data, idxBegin);
    else
        return TestShuffleVar<T, PosCount, IdxRange>(data, idxBegin, idxBegin + (idxEnd - idxBegin) / 2)
        && TestShuffleVar<T, PosCount, IdxRange>(data, idxBegin + (idxEnd - idxBegin) / 2 + 1, idxEnd);
}
