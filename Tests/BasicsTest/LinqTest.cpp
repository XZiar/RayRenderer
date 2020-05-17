#include "pch.h"
#include "common/Linq2.hpp"
#include "common/MemoryStream.hpp"

using namespace common::linq;
using common::linq::detail::EnumerableChecker;


#define SrcType(e) decltype(e)::ProviderType

template<typename E>
constexpr bool GetIsRef(const E&)
{
    return std::is_reference_v<typename E::ProviderType::OutType>;
}

TEST(Linq, Compiling)
{
    std::vector<std::unique_ptr<int>> data;
    auto e1 = FromIterable(data);
    //non-copyable
    EXPECT_EQ(SrcType(e1)::InvolveCache, false);
    EXPECT_EQ(GetIsRef(e1), true);

    auto e2 = e1.Select([](const auto& i) -> auto& { return i; });
    //ref-type
    EXPECT_EQ(SrcType(e2)::InvolveCache, false);
    EXPECT_EQ(GetIsRef(e2), true);

    auto e3 = e2.Select([](const auto& i) { return *i; });
    //value-type
    EXPECT_EQ(SrcType(e3)::InvolveCache, true);
    EXPECT_EQ(GetIsRef(e3), false);

    auto e4 = e3.Select<false>([](const auto& i) { return i; });
    //force-non-cache
    EXPECT_EQ(SrcType(e4)::InvolveCache, true);
    EXPECT_EQ(GetIsRef(e4), false);

    auto e5 = e4.Select([&](const auto& i) -> auto& { return data[i]; });
    //ref-type
    EXPECT_EQ(SrcType(e5)::InvolveCache, true);
    EXPECT_EQ(GetIsRef(e5), true);

    auto e6 = e5.Select<true>([&](const auto& i) -> auto& { return i; });
    //force-cache on reference
    EXPECT_EQ(SrcType(e6)::InvolveCache, true);
    EXPECT_EQ(GetIsRef(e6), true);
}

TEST(Linq, Range)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, 1).ToVector(), 
            testing::ElementsAre(0, 1, 2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, -1).ToVector(),
            testing::ElementsAre());
    }
    {
        EXPECT_THAT(FromRange<int32_t>(5, 1, -1).ToVector(), 
            testing::ElementsAre(5, 4, 3, 2));
    }
    {
        EXPECT_THAT(FromRange<float>(1.0f, 3.0f, 0.5f).ToVector(), 
            testing::ElementsAre(1.0f, 1.5f, 2.0f, 2.5f));
    }
}

TEST(Linq, TryGetFirst)
{
    EXPECT_FALSE(FromRange<int32_t>(0, 0, 1).TryGetFirst().has_value());
    EXPECT_EQ(FromRange<int32_t>(0, 1, 1).TryGetFirst().value(), 0);
}

TEST(Linq, Skip)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, 1).Skip(0).ToVector(), 
            testing::ElementsAre(0, 1, 2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, 1).Skip(1).ToVector(), 
            testing::ElementsAre(1, 2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, 1).Skip(2).ToVector(), 
            testing::ElementsAre(2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 3, 1).Skip(5).ToVector(), 
            testing::ElementsAre());
    }
}

TEST(Linq, Take)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 5, 1).Take(3).Skip(0).ToVector(),
            testing::ElementsAre(0, 1, 2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 5, 1).Take(3).Skip(1).ToVector(),
            testing::ElementsAre(1, 2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 5, 1).Take(3).Skip(2).ToVector(),
            testing::ElementsAre(2));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 5, 1).Take(3).Skip(5).ToVector(),
            testing::ElementsAre());
    }
}

TEST(Linq, Where)
{
    EXPECT_THAT(
        FromRange<int32_t>(0, 5, 1).Where([](const auto i) { return i % 2 == 0; }).ToVector(),
        testing::ElementsAre(0, 2, 4));
}

TEST(Linq, MultiWhere)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 5, 1)
            .Where([](const auto i) { return i % 2 == 0; })
            .Where([](const auto i) { return i > 3; })
            .ToVector(),
            testing::ElementsAre(4));
    }
    {
        int32_t src[] = { 0,1,2,3,4,5 };
        EXPECT_THAT(FromIterable(src)
            .Select([](auto& i) { i++; return i; })
            .Where([](const auto i) { return i % 2 == 0; })
            .Where([](const auto i) { return i > 3; })
            .ToVector(),
            testing::ElementsAre(4, 6));
        EXPECT_THAT(src, testing::ElementsAre(1, 2, 3, 4, 5, 6));
    }
    {
        int32_t src[] = { 0,1,2,3,4,5 };
        EXPECT_THAT(FromIterable(src)
            .Select([](auto& i) { i++; return i; })
            .Where([](const auto i) { return i % 2 == 0; })
            .Where([](const auto i) { return i > 3; })
            .Where([](const auto i) { return i > 4; })
            .ToVector(),
            testing::ElementsAre(6));
        EXPECT_THAT(src, testing::ElementsAre(1, 2, 3, 4, 5, 6));
    }
}

TEST(Linq, Relation)
{
    {
        EXPECT_TRUE(FromRange<int32_t>(0, 5, 1)
            .Where([](const auto i) { return i % 2 == 0; })
            .AllIf([](const auto i) { return i % 2 == 0; }));
    }
    {
        EXPECT_FALSE(FromRange<int32_t>(0, 5, 1)
            .Where([](const auto i) { return i % 2 == 0; })
            .ContainsIf([](const auto i) { return i % 2 != 0; }));
    }
    {
        EXPECT_FALSE(FromRange<int32_t>(0, 5, 1)
            .Where([](const auto i) { return i % 2 == 0; })
            .Contains(1));
    }
    {
        EXPECT_TRUE(FromRange<int32_t>(0, 5, 1)
            .Select([](const auto i) { return i - i; })
            .All(0));
    }
}

TEST(Linq, OrderBy)
{
    {
        EXPECT_THAT(FromRange<int32_t>(5, 0, -1)
            .OrderBy<std::less<>>().ToVector(),
            testing::ElementsAre(1, 2, 3, 4, 5));
    }
}

TEST(Linq, Random)
{
    {
        const auto ret = FromRandom().Take(15)
            .OrderBy<std::less<>>().ToVector();
        EXPECT_TRUE(std::is_sorted(ret.cbegin(), ret.cend()));
    }
    {
        const auto ret = FromRandomSource(std::mt19937_64(std::random_device()())).Take(15)
            .OrderBy<std::less<>>().ToVector();
        EXPECT_TRUE(std::is_sorted(ret.cbegin(), ret.cend()));
    }
}

struct Repeater
{
    static inline size_t CopyConstCnt = 0;
    static inline size_t MoveConstCnt = 0;
    static inline size_t CopyAssignCnt = 0;
    static inline size_t MoveAssignCnt = 0;
    static void Reset() noexcept
    {
        CopyConstCnt = MoveConstCnt = CopyAssignCnt = MoveAssignCnt = 0;
    }

    constexpr Repeater() noexcept {}
    Repeater(const Repeater&) noexcept { CopyConstCnt++; }
    Repeater(Repeater&&) noexcept { MoveConstCnt++; }
    Repeater& operator=(const Repeater&) noexcept { CopyAssignCnt++; return *this; }
    Repeater& operator=(Repeater&&) noexcept { MoveAssignCnt++; return *this; }
};

TEST(Linq, Repeat)
{
    {
        const auto ret = FromRepeat<Repeater>({}, 3).ToVector();
        EXPECT_EQ(Repeater::CopyConstCnt, 3u);
        Repeater::Reset();
    }
    {
        const auto ret = FromRepeat<Repeater>({}, 10).Skip(5).ToVector();
        EXPECT_EQ(Repeater::CopyConstCnt, 5u);
        Repeater::Reset();
    }
}

TEST(Linq, MapCache)
{
    size_t selectCnt = 0, whereCnt = 0;
    auto ret1 = FromRange<int16_t>(0, 12, 1)
        .Select([&](const auto i) { selectCnt++; return i; })
        .Where([&](const auto) { whereCnt++; return true; })
        .ToVector();
    EXPECT_EQ(selectCnt, whereCnt);

    selectCnt = whereCnt = 0;
    auto ret2 = FromRange<int16_t>(0, 12, 1)
        .Select<false>([&](const auto i) { selectCnt++; return i; })
        .Where([&](const auto) { whereCnt++; return true; })
        .ToVector();
    EXPECT_GT(selectCnt, whereCnt);
    EXPECT_EQ(selectCnt, whereCnt * 2);

    EXPECT_THAT(ret1, testing::ContainerEq(ret2));
}

TEST(Linq, FlatMap)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .SelectMany([](const auto) { return FromRange(0, 1, 1); })
            .ToVector(),
            testing::ElementsAre(0, 0, 0, 0));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .SelectMany([](const auto i) { return FromRange(0, i, 1); })
            .ToVector(),
            testing::ElementsAre(0, 0, 1, 0, 1, 2));
    }
    {
        size_t selectCnt = 0, whereCnt = 0;
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .SelectMany([&](const auto i) { selectCnt++; return FromRange(0, i, 1); })
            .Where([&](const auto i) { whereCnt++; return i > 0; })
            .ToVector(),
            testing::ElementsAre(1, 1, 2));
        EXPECT_EQ(selectCnt, 4u);
        EXPECT_EQ(whereCnt, 6u);
    }
}

TEST(Linq, ModifySource)
{
    std::vector<int32_t> data{ 1,2,3 };
    auto ret = FromIterable(data)
        .Select([](auto& i) { return ++i; })
        .ToVector();
    EXPECT_THAT(data, testing::ElementsAre(2, 3, 4));
    EXPECT_THAT(data, testing::ContainerEq(ret));
}

TEST(Linq, ModifyNonCopyable)
{
    std::vector<std::unique_ptr<int>> data(5);
    const auto ret = FromIterable(data)
        .Select([](auto& ptr) -> auto& { ptr = std::make_unique<int>(0); return ptr; })
        .Reduce([](bool ret, const auto& ptr) { return ret && (bool)ptr; }, true);
    EXPECT_TRUE(ret);
    EXPECT_THAT(data, testing::Each(testing::Pointee(0)));
}

TEST(Linq, CacheNonCopyable)
{
    std::vector<std::unique_ptr<int>> data(5);
    FromIterable(data)
        .ForEach([](auto& ptr, size_t idx) { ptr = std::make_unique<int>(static_cast<int>(idx)); });
    const auto ret = FromIterable(data)
        .Where([](const auto& ptr) { return *ptr != 0; })
        .Reduce([](bool ret, const auto& ptr) { return ret && (bool)ptr && *ptr; }, true);
    EXPECT_TRUE(ret);
    const auto ret2 = FromIterable(data)
        .Select([](const auto& ptr) { return *ptr; })
        .ToVector();
    EXPECT_THAT(ret2, testing::ElementsAre(0, 1, 2, 3, 4));
}

TEST(Linq, Pair)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .Pair(FromRange<int32_t>(1, 4, 1))
            .ToVector(),
            testing::ElementsAre(std::pair{ 0,1 }, std::pair{ 1,2 }, std::pair{ 2,3 }));
    } 
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .Pair(FromRange<int32_t>(1, 4, 1))
            .Skip(2)
            .ToVector(),
            testing::ElementsAre(std::pair{ 2,3 }));
    }
    {
        std::vector<int32_t> data1{ 1,2,3 };
        std::vector<int32_t> data2{ 3,2,1 };
        auto ret = FromIterable(data1).Pair(FromIterable(data2))
            .Select([](const auto& p) { return (p.first++) + (p.second++); })
            .ToVector();
        EXPECT_THAT(ret, testing::ElementsAre(4, 4, 4));
        EXPECT_THAT(data1, testing::ElementsAre(2, 3, 4));
        EXPECT_THAT(data2, testing::ElementsAre(4, 3, 2));
    }
}

TEST(Linq, Cast)
{
    {
        EXPECT_THAT(FromRange<float>(0, 2, 0.5f)
            .Cast<uint32_t>()
            .ToVector(),
            testing::ElementsAre(0, 0, 1, 1));
    }
    {
        std::string src = "here";
        std::string_view sv = src;
        const auto ret = FromRepeat(sv, 2)
            .Cast<std::string>()
            .ToVector();
        src[2] = 'a', src[3] = 'r';
        EXPECT_THAT(ret, testing::ElementsAre("here", "here"));
    }
}

TEST(Linq, ConcatTypeDeduct)
{
    {
        using kk = detail::ConcatedSourceHelper::Type<const int&, const int&>;
        static_assert(std::is_same_v<kk, const int&>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<const int&, int&>;
        static_assert(std::is_same_v<kk, const int&>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<const int&, const int>;
        static_assert(std::is_same_v<kk, int>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<int&, const int>;
        static_assert(std::is_same_v<kk, int>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<const int&, int>;
        static_assert(std::is_same_v<kk, int>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<const int*, const int*>;
        static_assert(std::is_same_v<kk, const int*>, "xxx");
    }
    {
        using kk = detail::ConcatedSourceHelper::Type<const int*, int*>;
        static_assert(std::is_same_v<kk, const int*>, "xxx");
    }
}

TEST(Linq, Concat)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 2, 1)
            .Concat(FromRange<int32_t>(1, 2, 1))
            .ToVector(),
            testing::ElementsAre(0, 1, 1));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 0, 1)
            .Concat(FromRange<int32_t>(1, 4, 1))
            .ToVector(),
            testing::ElementsAre(1, 2, 3));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(1, 4, 1)
            .Concat(FromRange<int32_t>(1, 0, 1))
            .ToVector(),
            testing::ElementsAre(1, 2, 3));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(1, 0, 1)
            .Concat(FromRange<int32_t>(1, 0, 1))
            .ToVector(),
            testing::ElementsAre());
    }
    {
        std::vector<int32_t> data1{ 1,2,3 };
        std::vector<int32_t> data2{ 3,2,1 };
        auto ret = FromIterable(data1).Concat(FromIterable(data2))
            .Select([](auto& p) { return p++; })
            .ToVector();
        EXPECT_THAT(ret, testing::ElementsAre(1, 2, 3, 3, 2, 1));
        EXPECT_THAT(data1, testing::ElementsAre(2, 3, 4));
        EXPECT_THAT(data2, testing::ElementsAre(4, 3, 2));
    }
    {
        std::vector<int32_t> data1{ 1,2,3 };
        const std::vector<int32_t> data2{ 3,2,1 };
        auto ret = FromIterable(data1).Concat(FromIterable(data2))
            .Select([](const auto& p) { return p; })
            .ToVector();
        EXPECT_THAT(ret, testing::ElementsAre(1, 2, 3, 3, 2, 1));
        EXPECT_THAT(data1, testing::ElementsAre(1, 2, 3));
        EXPECT_THAT(data2, testing::ElementsAre(3, 2, 1));
    }
}

TEST(Linq, ConcatSkip)
{
    {
        EXPECT_THAT(FromRange<int32_t>(1, 4, 1)
            .Concat(FromRange<int32_t>(1, 0, 1))
            .Skip(2)
            .ToVector(),
            testing::ElementsAre(3));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(1, 4, 1)
            .Concat(FromRange<int32_t>(1, 0, 1))
            .Skip(4)
            .ToVector(),
            testing::ElementsAre());
    }
    {
        std::vector<int32_t> data1{ 1,2,3 };
        std::vector<int32_t> data2{ 3,2,1 };
        auto ret = FromIterable(data1).Concat(FromIterable(data2))
            .Skip(4)
            .ToVector();
        EXPECT_THAT(ret, testing::ElementsAre(2, 1));
    }
}

TEST(Linq, Pairs)
{
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .Pairs(FromRange<int32_t>(1, 4, 1))
            .ToVector(),
            testing::ElementsAre(std::tuple{ 0,1 }, std::tuple{ 1,2 }, std::tuple{ 2,3 }));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .Pairs(FromRange<int32_t>(1, 4, 1), FromRange<int32_t>(1, 4, 1))
            .ToVector(),
            testing::ElementsAre(std::tuple{ 0,1,1 }, std::tuple{ 1,2,2 }, std::tuple{ 2,3,3 }));
    }
    {
        EXPECT_THAT(FromRange<int32_t>(0, 4, 1)
            .Pairs(FromRange<int32_t>(1, 4, 1), FromRange<int32_t>(1, 4, 1))
            .Skip(2)
            .ToVector(),
            testing::ElementsAre(std::tuple{ 2,3,3 }));
    }
    {
        EXPECT_EQ(FromRange<int32_t>(0, 4, 1)
            .Pairs(FromRange<int32_t>(1, 4, 1))
            .Count(), 3u);
    }
    {
        std::vector<int32_t> data1{ 1,2,3 };
        std::vector<int32_t> data2{ 3,2,1 };
        auto ret = FromIterable(data1).Pairs(FromIterable(data2))
            .Select([](const auto& p) { return (std::get<0>(p)++) + (std::get<1>(p)++); })
            .ToVector();
        EXPECT_THAT(ret, testing::ElementsAre(4, 4, 4));
        EXPECT_THAT(data1, testing::ElementsAre(2, 3, 4));
        EXPECT_THAT(data2, testing::ElementsAre(4, 3, 2));
    }
}

TEST(Linq, StreamLinq)
{
    std::vector<int32_t> dat = { 0,1,2,3,4 };
    common::io::MemoryInputStream memStream(common::to_span(dat));
    const auto ret1 = ToEnumerable(memStream.GetEnumerator<int32_t>())
        .Skip(2)
        .ToVector();
    EXPECT_THAT(ret1, testing::ElementsAre(2, 3, 4));
    memStream.SetPos(sizeof(int32_t));
    common::io::InputStream& inStream = memStream;
    const auto ret2 = ToEnumerable(inStream.GetEnumerator<int32_t>())
        .Skip(2)
        .ToVector();
    EXPECT_THAT(ret2, testing::ElementsAre(3, 4));
}

TEST(Linq, Basic)
{
    size_t selectCnt = 0, whereCnt = 0;
    auto ret = FromRange<int16_t>(0, 12, 1)
        .Select([&](const auto i) { selectCnt++; return i * 2; })
        .Take(10).Skip(3)
        .Where([&](const auto i) { whereCnt++; return i % 3 == 0; })
        .ToVector();
    EXPECT_THAT(ret, testing::ElementsAre(6, 12, 18));
}

