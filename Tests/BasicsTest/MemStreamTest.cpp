#include "pch.h"
#include "common/MemoryStream.hpp"


using namespace common::io;


TEST(MemStream, ReadByte)
{
    {
        std::vector<uint8_t> dat = { 0,1,2 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 3);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 0 });
        EXPECT_FALSE(memStream.IsEnd());
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 1 });
        EXPECT_FALSE(memStream.IsEnd());
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 2 });
        EXPECT_TRUE(memStream.IsEnd());
    }
    {
        std::vector<uint16_t> dat = { 0x0102, 0x4567 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 4);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ IsLittleEndian ? 0x02 : 0x01 });
        EXPECT_FALSE(memStream.IsEnd());
        EXPECT_EQ(memStream.ReadByte(), std::byte{ IsLittleEndian ? 0x01 : 0x02 });
        EXPECT_FALSE(memStream.IsEnd());
        EXPECT_EQ(memStream.ReadByte(), std::byte{ IsLittleEndian ? 0x67 : 0x45 });
        EXPECT_FALSE(memStream.IsEnd());
        EXPECT_EQ(memStream.ReadByte(), std::byte{ IsLittleEndian ? 0x45 : 0x67 });
        EXPECT_TRUE(memStream.IsEnd());
    }
}


TEST(MemStream, Read)
{
    {
        std::vector<uint8_t> dat = { 0,1,2 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 3);
        std::byte ret1;
        std::uint16_t ret2;

        EXPECT_TRUE(memStream.Read(ret1));
        EXPECT_EQ(ret1, std::byte{ 0 });
        EXPECT_FALSE(memStream.IsEnd());

        EXPECT_TRUE(memStream.Read(ret2));
        EXPECT_EQ(ret2, std::uint16_t{ IsLittleEndian ? 0x0201 : 0x0102 });
        EXPECT_TRUE(memStream.IsEnd());
    }
    {
        std::vector<uint16_t> dat = { 0x0102, 0x4567 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 4);
        std::byte ret1;
        std::uint16_t ret2;

        EXPECT_TRUE(memStream.Read(ret1));
        EXPECT_EQ(ret1, std::byte{ IsLittleEndian ? 0x02 : 0x01 });
        EXPECT_FALSE(memStream.IsEnd());

        EXPECT_TRUE(memStream.Read(ret2));
        EXPECT_EQ(ret2, std::uint16_t{ IsLittleEndian ? 0x6701 : 0x0245 });
        EXPECT_FALSE(memStream.IsEnd());

        EXPECT_TRUE(memStream.Read(ret1));
        EXPECT_EQ(ret1, std::byte{ IsLittleEndian ? 0x45 : 0x67 });
        EXPECT_TRUE(memStream.IsEnd());
    }
}


TEST(MemStream, SetPos)
{
    {
        std::vector<uint8_t> dat = { 0,1,2,3,4,5,6,7,8,9 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 10);
        EXPECT_EQ(memStream.CurrentPos(), 0);

        EXPECT_EQ(memStream.ReadByte(), std::byte{ 0 });
        EXPECT_EQ(memStream.CurrentPos(), 1);

        EXPECT_TRUE(memStream.SetPos(9));
        EXPECT_EQ(memStream.CurrentPos(), 9);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 9 });
        EXPECT_EQ(memStream.CurrentPos(), 10);
        EXPECT_TRUE(memStream.IsEnd());

        EXPECT_TRUE(memStream.SetPos(5));
        EXPECT_EQ(memStream.CurrentPos(), 5);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 5 });
        EXPECT_EQ(memStream.CurrentPos(), 6);

        EXPECT_FALSE(memStream.SetPos(10));
        EXPECT_EQ(memStream.CurrentPos(), 6);
    }
}


TEST(MemStream, Skip)
{
    {
        std::vector<uint8_t> dat = { 0,1,2,3,4,5,6,7,8,9 };
        MemoryInputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 10);
        EXPECT_EQ(memStream.CurrentPos(), 0);

        EXPECT_TRUE(memStream.Skip(5));
        EXPECT_EQ(memStream.CurrentPos(), 5);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 5 });
        EXPECT_EQ(memStream.CurrentPos(), 6);

        EXPECT_TRUE(memStream.Skip(3));
        EXPECT_EQ(memStream.CurrentPos(), 9);
        EXPECT_EQ(memStream.ReadByte(), std::byte{ 9 });
        EXPECT_EQ(memStream.CurrentPos(), 10);
        EXPECT_TRUE(memStream.IsEnd());

        EXPECT_FALSE(memStream.Skip(2));
        EXPECT_EQ(memStream.CurrentPos(), 10);
    }
}


TEST(MemStream, ReadInto)
{
    std::vector<uint8_t> dat = { 0,1,2,3,4,5 };
    MemoryInputStream memStream(common::to_span(dat));
    EXPECT_EQ(memStream.GetSize(), 6);

    std::vector<uint8_t> ret;
    EXPECT_EQ(memStream.ReadTo(ret, dat.size()), dat.size());
    EXPECT_EQ(ret.size(), dat.size());
    EXPECT_THAT(ret, testing::ContainerEq(ret));
    EXPECT_TRUE(memStream.IsEnd());

    EXPECT_TRUE(memStream.SetPos(4));
    EXPECT_EQ(memStream.CurrentPos(), 4);
    EXPECT_EQ(memStream.ReadTo(ret, 10), 2);
    EXPECT_EQ(ret.size(), 2);
    EXPECT_THAT(ret, testing::ElementsAre(uint8_t(4), uint8_t(5)));
    EXPECT_TRUE(memStream.IsEnd());

    std::array<uint8_t, 4> ret2{ 10,9,8,7 };
    EXPECT_TRUE(memStream.SetPos(4));
    EXPECT_EQ(memStream.CurrentPos(), 4);
    EXPECT_EQ(memStream.ReadInto(ret2, 1), 2);
    EXPECT_THAT(ret2, testing::ElementsAreArray(std::array<uint8_t, 4>{10, 4, 5, 7}));
    EXPECT_TRUE(memStream.IsEnd());

    EXPECT_TRUE(memStream.SetPos(4));
    EXPECT_EQ(memStream.CurrentPos(), 4);
    EXPECT_THAT(memStream.ReadToVector<uint8_t>(), testing::ElementsAre(uint8_t(4), uint8_t(5)));

}


TEST(MemStream, Container)
{
    {
        std::vector<uint8_t> dat = { 0,1,2,3,4,5,6,7,8,9 };
        ContainerInputStream contStream(dat);
        EXPECT_EQ(contStream.GetSize(), 10);
        EXPECT_EQ(contStream.CurrentPos(), 0);

        EXPECT_TRUE(contStream.Skip(5));
        EXPECT_EQ(contStream.CurrentPos(), 5);
        EXPECT_EQ(contStream.ReadByte(), std::byte{ 5 });
        EXPECT_EQ(contStream.CurrentPos(), 6);
    }
    {
        ContainerInputStream contStream(std::vector<uint8_t>{ 0,1,2,3,4,5,6,7,8,9 });
        EXPECT_EQ(contStream.GetSize(), 10);
        EXPECT_EQ(contStream.CurrentPos(), 0);

        EXPECT_TRUE(contStream.Skip(5));
        EXPECT_EQ(contStream.CurrentPos(), 5);
        EXPECT_EQ(contStream.ReadByte(), std::byte{ 5 });
        EXPECT_EQ(contStream.CurrentPos(), 6);
    }
}


TEST(MemStream, Write)
{
    {
        std::vector<uint8_t> dat(4, 0);
        EXPECT_THAT(dat, testing::Each(uint8_t(0)));
        MemoryOutputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), 4);
        
        uint8_t b1 = 0x1;
        EXPECT_TRUE(memStream.Write(b1));
        EXPECT_EQ(dat[0], b1);
        EXPECT_EQ(memStream.CurrentPos(), 1);

        uint16_t b2 = 0x0203;
        EXPECT_TRUE(memStream.Write(b2));
        EXPECT_EQ(dat[1], IsLittleEndian ? 0x03 : 0x02);
        EXPECT_EQ(dat[2], IsLittleEndian ? 0x02 : 0x03);
        EXPECT_EQ(memStream.CurrentPos(), 3);
    }
    {
        auto& eng = GetRanEng();
        std::vector<uint32_t> src;
        src.reserve(1025);
        for (uint32_t n = 0; n < 1025; ++n)
            src.push_back(eng());

        std::vector<uint32_t> dat(src.size(), 0);
        EXPECT_THAT(dat, testing::Each(0u));
        MemoryOutputStream memStream(common::to_span(dat));
        EXPECT_EQ(memStream.GetSize(), src.size() * sizeof(uint32_t));

        EXPECT_EQ(memStream.WriteFrom(src), src.size());
        EXPECT_THAT(dat, testing::ContainerEq(src));
    }
}


TEST(MemStream, ContainerWrite)
{
    std::vector<uint32_t> src;
    src.reserve(16);
    for (uint32_t n = 0; n < 16; ++n)
        src.push_back(n);
    {
        std::vector<uint32_t> ref;
        std::vector<uint32_t> dat;
        ContainerOutputStream contStream(dat);
        EXPECT_EQ(contStream.GetSize(), 0);

        EXPECT_EQ(contStream.WriteFrom(src), src.size());
        ref.insert(ref.end(), src.cbegin(), src.cend());
        EXPECT_THAT(dat, testing::ContainerEq(ref));

        EXPECT_EQ(contStream.WriteFrom(src, 10), src.size() - 10);
        ref.insert(ref.end(), src.cbegin() + 10, src.cend());
        EXPECT_THAT(dat, testing::ContainerEq(ref));

        EXPECT_TRUE(contStream.SetPos(9 * sizeof(uint32_t)));
        EXPECT_EQ(contStream.CurrentPos(), 9 * sizeof(uint32_t));
        EXPECT_EQ(contStream.WriteFrom(src, 11, 3), 3);
        std::copy(src.cbegin() + 11, src.cbegin() + 11 + 3, ref.begin() + 9);
        EXPECT_THAT(dat, testing::ContainerEq(ref));

        EXPECT_TRUE(contStream.SetPos(11 * sizeof(uint32_t)));
        EXPECT_EQ(contStream.CurrentPos(), 11 * sizeof(uint32_t));
        EXPECT_EQ(contStream.WriteFrom(src, 12, 10), 16 - 12);
        std::copy(src.cbegin() + 12, src.cend(), ref.begin() + 11);
        EXPECT_THAT(dat, testing::ContainerEq(ref));
    }
}


