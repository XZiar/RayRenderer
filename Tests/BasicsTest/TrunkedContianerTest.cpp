#include "pch.h"
#include "common/TrunckedContainer.hpp"


template<typename T>
struct TCon : public common::container::TrunckedContainer<T>
{
    using Trunk = typename common::container::TrunckedContainer<T>::Trunk;
    using value_type = T;
    //using const_iterator = typename common::container::TrunckedContainer<T>::Iterator<true>;
    using common::container::TrunckedContainer<T>::Trunks;
    using common::container::TrunckedContainer<T>::DefaultTrunkLength;

    TCon() : common::container::TrunckedContainer<T>(3) {}
};

#define CHECK_TRUNK(trunk, off, ava) do { EXPECT_EQ(trunk.Offset, off); EXPECT_EQ(trunk.Avaliable, ava); } while(0)

#define CHECK_ALL(tcon, ref, cnt) EXPECT_THAT(tcon, testing::ElementsAreArray(ref.data(), cnt))

TEST(TrunckContainer, AllocOne)
{
    constexpr std::array ref{ 0, 1, 2, 3 };
    TCon<int32_t> container;
    ASSERT_EQ(container.Trunks.size(), 0u);

    container.AllocOne() = 0;
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 1u, 2u);
    CHECK_ALL(container, ref, 1);

    container.AllocOne() = 1;
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 2u, 1u);
    CHECK_ALL(container, ref, 2);

    container.AllocOne() = 2;
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 3u, 0u);
    CHECK_ALL(container, ref, 3);

    container.AllocOne() = 3;
    ASSERT_EQ(container.Trunks.size(), 2u);
    CHECK_TRUNK(container.Trunks[0], 3u, 0u);
    CHECK_TRUNK(container.Trunks[1], 1u, 2u);
    CHECK_ALL(container, ref, 4);
}


template<size_t N>
forceinline auto AllocArray(TCon<int32_t>& tcon, const std::array<int32_t, N>& data, const size_t align = 1)
{
    const auto space = tcon.Alloc(N, align);
    std::copy_n(data.begin(), N, space.begin());
    return space;
}

TEST(TrunckContainer, AllocMany)
{
    constexpr std::array ref1{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    TCon<int32_t> container;
    ASSERT_EQ(container.Trunks.size(), 0u);

    AllocArray<1>(container, { 0 });
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 1u, 2u);
    CHECK_ALL(container, ref1, 1);

    AllocArray<2>(container, { 1, 2 });
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 3u, 0u);
    CHECK_ALL(container, ref1, 3);

    AllocArray<3>(container, { 3, 4, 5 });
    ASSERT_EQ(container.Trunks.size(), 2u);
    CHECK_TRUNK(container.Trunks[0], 3u, 0u);
    CHECK_TRUNK(container.Trunks[1], 3u, 0u);
    CHECK_ALL(container, ref1, 6);

    AllocArray<4>(container, { 6, 7, 8, 9 });
    ASSERT_EQ(container.Trunks.size(), 3u);
    CHECK_TRUNK(container.Trunks[0], 3u, 0u);
    CHECK_TRUNK(container.Trunks[1], 3u, 0u);
    CHECK_TRUNK(container.Trunks[2], 4u, 2u);
    CHECK_ALL(container, ref1, 10);

    {
        std::array<int32_t, 25> tmp;
        tmp.fill(1);
        AllocArray(container, tmp);
    }
    ASSERT_EQ(container.Trunks.size(), 4u);
    CHECK_TRUNK(container.Trunks[0],  3u, 0u);
    CHECK_TRUNK(container.Trunks[1],  3u, 0u);
    CHECK_TRUNK(container.Trunks[2],  4u, 2u);
    CHECK_TRUNK(container.Trunks[3], 25u, 0u);
    {
        std::array<int, 10 + 25> ref;
        auto it = std::copy_n(ref1.begin(), 10, ref.begin());
        std::fill_n(it, 25, 1);
        EXPECT_THAT(container, testing::ElementsAreArray(ref));
    }

    AllocArray<1>(container, { 10 });
    ASSERT_EQ(container.Trunks.size(), 4u);
    CHECK_TRUNK(container.Trunks[0],  3u, 0u);
    CHECK_TRUNK(container.Trunks[1],  3u, 0u);
    CHECK_TRUNK(container.Trunks[2],  5u, 1u);
    CHECK_TRUNK(container.Trunks[3], 25u, 0u);
    {
        std::array<int, 11 + 25> ref;
        auto it = std::copy_n(ref1.begin(), 11, ref.begin());
        std::fill_n(it, 25, 1);
        EXPECT_THAT(container, testing::ElementsAreArray(ref));
    }

    container.AllocOne() = 11;
    ASSERT_EQ(container.Trunks.size(), 4u);
    CHECK_TRUNK(container.Trunks[0],  3u, 0u);
    CHECK_TRUNK(container.Trunks[1],  3u, 0u);
    CHECK_TRUNK(container.Trunks[2],  6u, 0u);
    CHECK_TRUNK(container.Trunks[3], 25u, 0u);
    {
        std::array<int, 12 + 25> ref;
        auto it = std::copy_n(ref1.begin(), 12, ref.begin());
        std::fill_n(it, 25, 1);
        EXPECT_THAT(container, testing::ElementsAreArray(ref));
    }
}

TEST(TrunckContainer, DeAlloc)
{
    TCon<int32_t> container;

    const auto space1 = AllocArray<2>(container, { 0, 1 });
    const auto space2 = AllocArray<3>(container, { 2, 3, 4 });
    ASSERT_EQ(space1.size(), 2u);
    ASSERT_EQ(space2.size(), 3u);
    ASSERT_EQ(container.Trunks.size(), 2u);
    CHECK_TRUNK(container.Trunks[0], 2u, 1u);
    CHECK_TRUNK(container.Trunks[1], 3u, 0u);
    EXPECT_THAT(container, testing::ElementsAre(0, 1, 2, 3, 4));

    EXPECT_FALSE(container.TryDealloc(space1[0]));
    ASSERT_EQ(container.Trunks.size(), 2u);
    CHECK_TRUNK(container.Trunks[0], 2u, 1u);
    CHECK_TRUNK(container.Trunks[1], 3u, 0u);
    EXPECT_THAT(container, testing::ElementsAre(0, 1, 2, 3, 4));

    EXPECT_TRUE(container.TryDealloc(space1[1]));
    ASSERT_EQ(container.Trunks.size(), 2u);
    CHECK_TRUNK(container.Trunks[0], 1u, 2u);
    CHECK_TRUNK(container.Trunks[1], 3u, 0u);
    EXPECT_THAT(container, testing::ElementsAre(0, 2, 3, 4));

    EXPECT_TRUE(container.TryDealloc(space2));
    ASSERT_EQ(container.Trunks.size(), 1u);
    CHECK_TRUNK(container.Trunks[0], 1u, 2u);
    EXPECT_THAT(container, testing::ElementsAre(0));

    EXPECT_TRUE(container.TryDealloc(space1[0]));
    ASSERT_EQ(container.Trunks.size(), 0u);

}
