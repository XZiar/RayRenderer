#include "pch.h"
#include "common/AlignedBuffer.hpp"

using common::AlignedBuffer;


TEST(AlignBuf, Alignment)
{
    {
        AlignedBuffer buf(16384, 128);
        EXPECT_TRUE(buf.GetRawPtr() != nullptr);
        EXPECT_EQ(buf.GetSize(), 16384u);
        EXPECT_EQ(buf.GetAlignment(), 128u);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(buf.GetRawPtr()) % 128, 0u);
    }
    {
        AlignedBuffer buf(4399, 512);
        EXPECT_TRUE(buf.GetRawPtr() != nullptr);
        EXPECT_EQ(buf.GetSize(), 4399u);
        EXPECT_EQ(buf.GetAlignment(), 512u);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(buf.GetRawPtr()) % 512, 0u);
    }
}


TEST(AlignBuf, CopyMove)
{
    AlignedBuffer buf(16384, 128);
    const auto ptr = buf.GetRawPtr();
    AlignedBuffer buf1 = buf;
    EXPECT_TRUE(buf1.GetRawPtr() != buf.GetRawPtr());
    EXPECT_EQ(buf1.GetSize(), buf.GetSize());
    EXPECT_EQ(buf1.GetAlignment(), buf.GetAlignment());
    buf1 = std::move(buf);
    EXPECT_TRUE(buf1.GetRawPtr() == ptr);
}


TEST(AlignBuf, SubBuf)
{
    AlignedBuffer buf(16384, 128);
    {
        AlignedBuffer buf1 = buf.CreateSubBuffer(0);
        EXPECT_TRUE(buf1.GetRawPtr() == buf.GetRawPtr());
        EXPECT_EQ(buf1.GetSize(), buf.GetSize());
        EXPECT_EQ(buf1.GetAlignment(), buf.GetAlignment());
    }
    {
        AlignedBuffer buf1 = buf.CreateSubBuffer(16);
        EXPECT_TRUE(buf1.GetRawPtr() == buf.GetRawPtr() + 16);
        EXPECT_EQ(buf1.GetSize(), buf.GetSize() - 16);
        EXPECT_EQ(buf1.GetAlignment(), 16u);
    }
    {
        AlignedBuffer buf1 = buf.CreateSubBuffer(16);
        AlignedBuffer buf2 = buf1.CreateSubBuffer(3);
        EXPECT_TRUE(buf2.GetRawPtr() == buf1.GetRawPtr() + 3);
        EXPECT_EQ(buf2.GetSize(), buf1.GetSize() - 3);
        EXPECT_EQ(buf2.GetAlignment(), 1u);
    }
}


struct MemInfo : public AlignedBuffer::ExternBufInfo
{
    static inline size_t Alive = 0;
    size_t Size;
    std::byte* Ptr;
    [[nodiscard]] size_t GetSize() const noexcept override
    {
        return Size;
    }
    [[nodiscard]] std::byte* GetPtr() const noexcept override
    {
        return Ptr;
    }
    MemInfo(const size_t size) noexcept : Size(size), Ptr(reinterpret_cast<std::byte*>(malloc(Size)))
    {
        Alive++;
    }
    ~MemInfo() override 
    {
        Alive--;
        free(Ptr);
    }
};

TEST(AlignBuf, ExtBuf)
{
    MemInfo::Alive = 0;
    auto info = std::make_unique<MemInfo>(1023);
    EXPECT_EQ(MemInfo::Alive, 1u);
    auto ptr = info->GetPtr();
    auto buf1 = AlignedBuffer::CreateBuffer(std::move(info));
    EXPECT_EQ(MemInfo::Alive, 1u);
    EXPECT_EQ(buf1.GetRawPtr(), ptr);
    EXPECT_EQ(buf1.GetSize(), 1023u);
    //EXPECT_EQ(buf1.GetAlignment(), sizeof(std::max_align_t));
    auto buf2 = buf1.CreateSubBuffer(16); // subbuf only add reference
    EXPECT_EQ(MemInfo::Alive, 1u);
    auto buf3 = buf1; // copy will not create original extbuf
    EXPECT_EQ(MemInfo::Alive, 1u);
    auto buf4 = std::move(buf1); // transfer buf1's ownership to buf4
    EXPECT_EQ(MemInfo::Alive, 1u);
    buf4 = AlignedBuffer(14); // clear buf4, buf2 remains the extbuf
    EXPECT_EQ(MemInfo::Alive, 1u);
    buf2 = buf3; // release buf2, release extbuf
    EXPECT_EQ(MemInfo::Alive, 0u);
}
