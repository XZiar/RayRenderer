#include "pch.h"
#include "common/Linq2.hpp"

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
    EXPECT_EQ(SrcType(e1)::ShouldCache, false);
    EXPECT_EQ(GetIsRef(e1), true);

    auto e2 = e1.Select([](const auto& i) -> auto& { return i; });
    //ref-type
    EXPECT_EQ(SrcType(e2)::ShouldCache, false);
    EXPECT_EQ(GetIsRef(e2), true);

    auto e3 = e2.Select([](const auto& i) { return *i; });
    //value-type
    EXPECT_EQ(SrcType(e3)::ShouldCache, true);
    EXPECT_EQ(GetIsRef(e3), false);

    auto e4 = e3.Select<false>([](const auto& i) { return i; });
    //force-non-cache
    EXPECT_EQ(SrcType(e4)::ShouldCache, false);
    EXPECT_EQ(GetIsRef(e4), false);

    auto e5 = e4.Select([&](const auto& i) -> auto& { return data[i]; });
    //ref-type
    EXPECT_EQ(SrcType(e5)::ShouldCache, false);
    EXPECT_EQ(SrcType(e5)::InvolveCache, true);
    EXPECT_EQ(GetIsRef(e5), true);

    auto e6 = e5.Select<true>([&](const auto& i) -> auto& { return i; });
    //force-cache on reference
    EXPECT_EQ(SrcType(e6)::ShouldCache, true);
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
    static constexpr void Reset() noexcept
    {
        CopyConstCnt = MoveConstCnt = CopyAssignCnt = MoveAssignCnt = 0;
    }

    constexpr Repeater() noexcept {}
    constexpr Repeater(const Repeater&) noexcept { CopyConstCnt++; }
    constexpr Repeater(Repeater&&) noexcept { MoveConstCnt++; }
    Repeater& operator=(const Repeater&) noexcept { CopyAssignCnt++; return *this; }
    Repeater& operator=(Repeater&&) noexcept { MoveAssignCnt++; return *this; }
};

TEST(Linq, Repeat)
{
    {
        const auto ret = FromRepeat<Repeater>({}, 3).ToVector();
        EXPECT_EQ(Repeater::CopyConstCnt, 3);
        Repeater::Reset();
    }
    {
        const auto ret = FromRepeat<Repeater>({}, 10).Skip(5).ToVector();
        EXPECT_EQ(Repeater::CopyConstCnt, 5);
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
        .Reduce([](bool ret, const auto& item) { return ret && (bool)item; }, true);
    EXPECT_TRUE(ret);
    EXPECT_THAT(data, testing::Each(testing::Pointee(0)));
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

