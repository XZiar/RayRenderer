#include "pch.h"


namespace copytest
{
static std::mt19937& GetRanEng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
template<typename T>
static std::vector<T> InitRandom(const uint32_t size);
template<typename Src, typename Dst>
static std::vector<Dst> InitConvertData();


struct CopyTestHelper
{
    static uint32_t GetARand();
    static const std::vector<uint32_t>& GetCopyRanges();
    template<typename T>
    static const std::vector<T>& GetRandomSources()
    {
        static auto rans = InitRandom<T>(16384);
        return rans;
    }
    template<typename Src, typename Dst>
    static const std::vector<Dst>& GetConvertData()
    {
        static auto data = InitConvertData<Src, Dst>();
        return data;
    }
};


template<typename T>
static std::vector<T> InitRandom(const uint32_t size)
{
    std::uniform_int_distribution<T> dis(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    auto& eng = GetRanEng();

    std::vector<T> ret;
    ret.reserve(size);
    for (uint32_t n = 0; n < size; ++n)
        ret.push_back(static_cast<T>(dis(eng)));
    return ret;
}
template<typename Src, typename Dst>
static std::vector<Dst> InitConvertData()
{
    const auto& src = CopyTestHelper::GetRandomSources<Src>();
    std::vector<Dst> ret;
    ret.reserve(src.size());
    for (const auto i : src)
        ret.push_back(static_cast<Dst>(i));
    return ret;
}


uint32_t CopyTestHelper::GetARand()
{
    return GetRanEng()();
}
const std::vector<uint32_t>& CopyTestHelper::GetCopyRanges()
{
    static std::vector<uint32_t> Sizes =
    {
        4800u, 4097u, 4096u, 4095u,
        256u,  260u,  252u,
        64u,   67u,   61u,
        32u,   34u,   30u,
        16u,   17u,   15u,
        9u,    8u,    7u,
        4u,    3u,    2u,    1u,    0u
    };
    return Sizes;
}

template<>
const std::vector<uint32_t>& CopyTestHelper::GetConvertData<uint32_t, uint32_t>()
{
    return GetRandomSources<uint32_t>();
}
template<>
const std::vector<uint16_t>& CopyTestHelper::GetConvertData<uint16_t, uint16_t>()
{
    return GetRandomSources<uint16_t>();
}

template
const std::vector<uint32_t>& CopyTestHelper::GetRandomSources<uint32_t>();
template
const std::vector<uint16_t>& CopyTestHelper::GetRandomSources<uint16_t>();

template
const std::vector<uint16_t>& CopyTestHelper::GetConvertData<uint32_t, uint16_t>();
template
const std::vector<uint8_t >& CopyTestHelper::GetConvertData<uint32_t, uint8_t >();
template
const std::vector<uint8_t >& CopyTestHelper::GetConvertData<uint16_t, uint8_t >();


}
