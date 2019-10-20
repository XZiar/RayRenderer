#pragma once


#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "ContainerHelper.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <array>

namespace common
{

namespace io
{


namespace detail
{
template<typename T>
struct InputStreamEnumerateSource;
template<typename T>
struct RandomInputStreamEnumerateSource;
}


class InputStream : public NonCopyable
{
private:
    template<size_t Size>
    [[nodiscard]] forceinline size_t CalcCount(const size_t offset, const size_t count, size_t want)
    {
        if (offset >= count)
            return 0;
        want = std::min(want, count - offset);
        const auto avaliable = AvaliableSpace();
        want = std::min(avaliable / Size, want);
        return want;
    }
    template<size_t Size, typename C>
    [[nodiscard]] forceinline size_t ReadInto_(C& output, const size_t offset, size_t want)
    {
        if (want == 0)
            return 0;
        return ReadMany(want, Size, &output[offset]);
    }
    [[nodiscard]] forceinline virtual std::byte ReadByteNE(bool& isSuccess)
    {
        std::byte data{ 0xff };
        isSuccess = ReadMany(1, 1, &data) == 1;
        return data;
    }
    [[nodiscard]] forceinline virtual std::byte ReadByteME()
    {
        std::byte data{ 0xff };
        if (ReadMany(1, 1, &data) != 1)
            COMMON_THROW(BaseException, u"read from stream failed");
        else
            return data;
    }
protected:
    [[nodiscard]] virtual size_t AvaliableSpace() { return SIZE_MAX; };
public:
    virtual size_t ReadMany(const size_t want, const size_t perSize, void * ptr) = 0;
    virtual bool Skip(const size_t len) = 0;
    [[nodiscard]] virtual bool IsEnd() { return AvaliableSpace() == 0; }
    virtual ~InputStream() {}

    virtual bool Read(const size_t len, void* ptr) 
    { 
        return ReadMany(len, 1, ptr) == len;
    }
    template<typename T>
    bool Read(T& output)
    {
        return Read(sizeof(T), &output);
    }
    template<typename T = std::byte>
    [[nodiscard]] T ReadByteNE() noexcept // without checking
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        bool isSuccess = true;
        return static_cast<T>(ReadByteNE(isSuccess));
    }
    template<typename T = std::byte>
    [[nodiscard]] T ReadByte()
    {
        static_assert(sizeof(T) == 1, "only 1-byte length type allowed");
        return static_cast<T>(ReadByteME());
    }

    template<typename T, size_t N>
    forceinline size_t ReadInto(T(&output)[N], const size_t offset = 0, size_t count = N)
    {
        constexpr size_t Size = sizeof(T);
        const auto want = CalcCount<Size>(offset, N, count);
        return ReadInto_<Size>(output, offset, want);
    }
    template<typename T, size_t N>
    forceinline size_t ReadInto(std::array<T, N>& output, const size_t offset = 0, size_t count = N)
    {
        constexpr size_t Size = sizeof(T);
        const auto want = CalcCount<Size>(offset, N, count);
        return ReadInto_<Size>(output, offset, want);
    }

    template<class T>
    size_t ReadInto(T& output, size_t count)
    {
        constexpr size_t Size = sizeof(typename T::value_type);
        const auto want = CalcCount<Size>(0, SIZE_MAX, count);
        output.resize(want);
        return ReadInto_<Size>(output, 0, want);
    }

    template<typename T>
    [[nodiscard]] detail::InputStreamEnumerateSource<T> GetEnumerator();
};


class OutputStream : public NonCopyable
{
private:
    template<size_t Size>
    [[nodiscard]] forceinline size_t CalcCount(const size_t offset, const size_t count, size_t want)
    {
        if (offset >= count)
            return 0;
        want = std::min(want, count - offset);
        const auto acceptable = AcceptableSpace();
        want = std::min(acceptable / Size, want);
        return want;
    }
protected:
    virtual size_t AcceptableSpace() { return SIZE_MAX; };
public:
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void * ptr) = 0;
    virtual void Flush() {}
    virtual ~OutputStream() {}

    virtual bool Write(const size_t len, const void* ptr)
    {
        return WriteMany(len, 1, ptr) == len;
    }
    template<typename T>
    bool Write(const T& output)
    {
        return Write(sizeof(T), &output);
    }

    template<class T>
    size_t WriteFrom(const T& input, size_t offset = 0, size_t count = SIZE_MAX)
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "Only accept contiguous type");
        constexpr size_t EleSize = Helper::EleSize;
        const auto want = CalcCount<EleSize>(offset, Helper::Count(input), count);
        if (want == 0)
            return 0;
        return WriteMany(want, EleSize, Helper::Data(input) + offset);
    }
};


class RandomStream : public NonCopyable
{
public:
    [[nodiscard]] virtual size_t GetSize() = 0;
    [[nodiscard]] virtual size_t CurrentPos() const = 0;
    virtual bool SetPos(const size_t offset) = 0;
};


class RandomInputStream  : public RandomStream, public InputStream
{
public:
    template<typename T>
    [[nodiscard]] detail::RandomInputStreamEnumerateSource<T> GetEnumerator();
};
class RandomOutputStream : public RandomStream, public OutputStream
{
};


namespace detail
{
template<typename T>
struct InputStreamEnumerateSource
{
    static_assert(std::is_trivially_copyable_v<T>, "element type should be trivially copyable");
    InputStream& Stream;
    mutable bool HasReachNext = false;
    using OutType = T;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = true;
    static constexpr bool IsCountable = false;
    static constexpr bool CanSkipMultiple = true;

    InputStreamEnumerateSource(InputStream& stream)
        : Stream(stream) { }
    InputStreamEnumerateSource(InputStreamEnumerateSource&& other)
        : Stream(other.Stream) { }

    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        T dat;
        Stream.Read(dat);
        HasReachNext = true;
        return dat;
    }
    constexpr void MoveNext()
    {
        if (!HasReachNext)
            Stream.Skip(sizeof(T));
        HasReachNext = false;
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return Stream.IsEnd();
    }

    constexpr void MoveMultiple(const size_t count) noexcept
    {
        Stream.Skip(sizeof(T) * count);
    }
};


template<typename T>
struct RandomInputStreamEnumerateSource
{
    static_assert(std::is_trivially_copyable_v<T>, "element type should be trivially copyable");
    RandomInputStream& Stream;
    mutable bool HasReachNext = false;
    using OutType = T;
    static constexpr bool ShouldCache = false;
    static constexpr bool InvolveCache = true;
    static constexpr bool IsCountable = true;
    static constexpr bool CanSkipMultiple = true;

    RandomInputStreamEnumerateSource(RandomInputStream& stream)
        : Stream(stream) { }
    RandomInputStreamEnumerateSource(RandomInputStreamEnumerateSource&& other)
        : Stream(other.Stream) { }

    [[nodiscard]] constexpr OutType GetCurrent() const
    {
        T dat;
        Stream.Read(dat);
        HasReachNext = true;
        return dat;
    }
    constexpr void MoveNext()
    {
        if (!HasReachNext)
            Stream.Skip(sizeof(T));
        HasReachNext = false;
    }
    [[nodiscard]] constexpr bool IsEnd() const
    {
        return Stream.IsEnd();
    }

    [[nodiscard]] constexpr size_t Count() const noexcept
    {
        return Stream.GetSize() - Stream.CurrentPos();
    }
    constexpr void MoveMultiple(const size_t count) noexcept
    {
        const auto cnt = (HasReachNext && count > 1) ? count - 1 : count;
        Stream.Skip(sizeof(T) * cnt);
        HasReachNext = false;
    }
};
}


template<typename T>
detail::InputStreamEnumerateSource<T> InputStream::GetEnumerator()
{
    return detail::InputStreamEnumerateSource<T>(*this);
}
template<typename T>
detail::RandomInputStreamEnumerateSource<T> RandomInputStream::GetEnumerator()
{
    return detail::RandomInputStreamEnumerateSource<T>(*this);
}


class BufferedRandomInputStream : public RandomInputStream
{
protected:
    std::unique_ptr<RandomInputStream> BackStream;
    AlignedBuffer Buffer;
    size_t BufBegin, BufPos = 0, BufLen = 0;
    forceinline void CheckStreamValid()
    {
        if (!BackStream)
            COMMON_THROW(BaseException, u"operate a released buffered stream");
    }
    forceinline void LoadBuffer()
    {
        BufPos = 0;
        BufLen = BackStream->ReadMany(Buffer.GetSize(), 1, Buffer.GetRawPtr());
    }
    forceinline void LoadBuffer(const size_t pos)
    {
        BufBegin = pos;
        LoadBuffer();
    }
    forceinline void FlushPos()
    {
        BufBegin += BufPos;
        BackStream->SetPos(BufBegin);
        BufPos = BufLen = 0;
    }
    [[nodiscard]] forceinline size_t CurrentPos_() const
    {
        return BufBegin + BufPos;
    }
    //copy as many as possible
    [[nodiscard]] forceinline size_t UseBuffer(void* ptr, const size_t want)
    {
        const auto bufBytes = std::min(BufLen - BufPos, want);
        memcpy_s(ptr, want, Buffer.GetRawPtr() + BufPos, bufBytes);
        BufPos += bufBytes;
        return bufBytes;
    }
public:
    BufferedRandomInputStream(std::unique_ptr<RandomInputStream>&& stream, const size_t bufSize)
        : BackStream(std::move(stream)), 
          Buffer(bufSize), BufBegin(BackStream->CurrentPos())
    {
        LoadBuffer();
    }
    template<typename T>
    BufferedRandomInputStream(T&& stream, const size_t bufSize)
        : BackStream(std::make_unique<T>(std::move(stream))),
          Buffer(bufSize), BufBegin(BackStream->CurrentPos())
    {
        static_assert(std::is_base_of_v<RandomInputStream, T>, "T should derive from RandomInputStream");
        LoadBuffer();
    }
    virtual ~BufferedRandomInputStream() override {}

    [[nodiscard]] std::unique_ptr<RandomInputStream> Release()
    {
        FlushPos();
        return std::move(BackStream);
    }

    [[nodiscard]] std::pair<const std::byte*, size_t> ExposeAvaliable() const
    {
        return { Buffer.GetRawPtr() + BufPos, BufLen - BufPos };
    }
    void LoadNext()
    {
        LoadBuffer(BufBegin + BufLen);
    }

    //==========RandomStream=========//

    [[nodiscard]] virtual size_t GetSize() override
    {
        return BackStream->GetSize();
    }
    [[nodiscard]] virtual size_t CurrentPos() const override
    {
        return CurrentPos_();
    }
    virtual bool SetPos(const size_t pos) override
    {
        if (pos >= BufBegin)
        {
            const auto dist = pos - BufBegin;
            if (dist < BufLen)
            {
                BufPos = dist;
                return true;
            }
        }
        //need reload
        if (!BackStream->SetPos(pos))
        {
            BufBegin = pos, BufPos = BufLen = 0;
            return false;
        }
        LoadBuffer(pos);
        return true;
    }

    //==========InputStream=========//

    [[nodiscard]] virtual size_t AvaliableSpace() override
    {
        return BackStream->GetSize() - CurrentPos_();
    };
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override
    {
        const size_t len = want * perSize;
        const auto bufBytes = UseBuffer(ptr, len);

        const auto stillNeed = len - bufBytes;
        if (stillNeed == 0)
            return want;

        size_t extraBytes = 0;
        if (stillNeed >= Buffer.GetSize())
        { // force bypass buffer
            extraBytes = BackStream->ReadMany(stillNeed, 1, reinterpret_cast<uint8_t*>(ptr) + bufBytes);
            LoadBuffer(BackStream->CurrentPos());
        }
        else
        { // 0 < stillNeed < BufSize
            LoadBuffer(BufBegin + BufLen);
            extraBytes = UseBuffer(reinterpret_cast<uint8_t*>(ptr) + bufBytes, stillNeed);
        }
        return (bufBytes + extraBytes) / perSize;
    }
    virtual bool Skip(const size_t len) override
    {
        return SetPos(CurrentPos_() + len);
    }
    [[nodiscard]] virtual bool IsEnd() override
    { 
        return (BufPos >= BufLen) && BackStream->IsEnd();
    }

    [[nodiscard]] forceinline virtual std::byte ReadByteNE(bool& isSuccess) override
    {
        if (BufPos >= BufLen)
            LoadBuffer();
        isSuccess = BufLen > 0;
        if (isSuccess)
            return Buffer.GetRawPtr()[BufPos++];
        else
            return std::byte(0xff);
    }
    [[nodiscard]] forceinline virtual std::byte ReadByteME() override
    {
        if (BufPos >= BufLen)
            LoadBuffer();
        if (BufLen > 0)
            return Buffer.GetRawPtr()[BufPos++];
        else
            COMMON_THROW(BaseException, u"reach end of inputstream");
    }

};


}

}

