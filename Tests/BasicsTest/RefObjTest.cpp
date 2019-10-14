#include "pch.h"
#include "common/RefObject.hpp"


namespace refobjtest
{

struct AData
{
    inline static size_t Alive = 0;
    int32_t X;
    AData(int32_t d) : X(d) { Alive++; }
    virtual ~AData() { Alive--; }
};

struct BData : public AData
{
    inline static size_t Alive = 0;
    int32_t Y;
    BData(int32_t d) : AData(d), Y(d) { Alive++; }
    virtual ~BData() override { Alive--; }
};

struct A : public common::RefObject<AData>
{                                                    
    INIT_REFOBJ(A, AData)
    A(int32_t data) : common::RefObject<AData>(Create(data))
    { }
    using common::RefObject<AData>::RefObject;
    int32_t GetIt() const
    {
        return Self().X;
    }
    A Child(int32_t data)
    {
        return CreateWith<AData>(data);
    }
};

struct B : public common::RefObject<BData>
{
    INIT_REFOBJ(B, BData)
    B(int32_t data) : common::RefObject<BData>(Create(data))
    { }
    using common::RefObject<BData>::RefObject;
    int32_t GetIt() const
    {
        return Self().Y;
    }

};


TEST(RefObj, Init)
{
    A a(1);
    EXPECT_EQ(a.GetIt(), 1);
    EXPECT_EQ(AData::Alive, 1);
    B b(2);
    EXPECT_EQ(b.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 2);
    EXPECT_EQ(BData::Alive, 1);
    auto c = a.Child(3);
    EXPECT_EQ(c.GetIt(), 3);
    EXPECT_EQ(AData::Alive, 3);
    EXPECT_EQ(BData::Alive, 1);
}

TEST(RefObj, Assign)
{
    A a(1);
    B b(2);
    EXPECT_EQ(a.GetIt(), 1);
    EXPECT_EQ(b.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 2);
    EXPECT_EQ(BData::Alive, 1);
    a = b;
    EXPECT_EQ(a.GetIt(), 2);
    EXPECT_EQ(b.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 1);
    EXPECT_EQ(BData::Alive, 1);
}

TEST(RefObj, Move)
{
    A a(1);
    B b(2);
    EXPECT_EQ(a.GetIt(), 1);
    EXPECT_EQ(b.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 2);
    EXPECT_EQ(BData::Alive, 1);
    a = std::move(b);
    EXPECT_EQ(a.GetIt(), 2);
    EXPECT_FALSE(b);
    EXPECT_EQ(AData::Alive, 1);
    EXPECT_EQ(BData::Alive, 1);
}

TEST(RefObj, RefLink)
{
    A a1(1);
    A a2(2);
    auto a3 = a1.Child(3);
    EXPECT_EQ(a1.GetIt(), 1);
    EXPECT_EQ(a2.GetIt(), 2);
    EXPECT_EQ(a3.GetIt(), 3);
    EXPECT_EQ(AData::Alive, 3);
    a1 = std::move(a2);
    EXPECT_EQ(a1.GetIt(), 2);
    EXPECT_FALSE(a2);
    EXPECT_EQ(a3.GetIt(), 3);
    EXPECT_EQ(AData::Alive, 3);
    a3 = a1;
    EXPECT_EQ(a1.GetIt(), 2);
    EXPECT_FALSE(a2);
    EXPECT_EQ(a3.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 1);
}

TEST(RefObj, RefCast)
{
    A a(1);
    B b(2);
    a = std::move(b);
    EXPECT_EQ(a.GetIt(), 2);
    EXPECT_FALSE(b);
    EXPECT_EQ(AData::Alive, 1);
    EXPECT_EQ(BData::Alive, 1);
    b = common::RefCast<B>(a);
    EXPECT_EQ(a.GetIt(), 2);
    EXPECT_EQ(b.GetIt(), 2);
    EXPECT_EQ(AData::Alive, 1);
    EXPECT_EQ(BData::Alive, 1);
}


}
