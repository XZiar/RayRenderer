#pragma once


#include "Stream.hpp"
#include "AlignedBuffer.hpp"
#include <variant>
#include <tuple>

namespace common
{

namespace io
{

class MemoryInputStream : public RandomInputStream
{
private:
    const std::byte* Ptr;
    size_t TotalSize, CurPos = 0;
public:
    template<typename T>
    MemoryInputStream(const span<T> srcSpan) noexcept : 
        Ptr(common::as_bytes(srcSpan).data()), TotalSize(srcSpan.size_bytes()) { }
    constexpr MemoryInputStream(MemoryInputStream&& stream) noexcept :
        Ptr(stream.Ptr), TotalSize(stream.TotalSize), CurPos(stream.CurPos)
    {
        stream.Ptr = nullptr; stream.TotalSize = stream.CurPos = 0;
    }
    virtual ~MemoryInputStream() override {}

    [[nodiscard]] constexpr std::pair<const std::byte*, size_t> ExposeAvaliable() const
    {
        return { Ptr + CurPos, TotalSize - CurPos };
    }

    //==========RandomStream=========//

    [[nodiscard]] virtual size_t GetSize() noexcept override
    {
        return TotalSize;
    }
    [[nodiscard]] virtual size_t CurrentPos() const noexcept override
    {
        return CurPos;
    }
    virtual bool SetPos(const size_t pos) noexcept override
    {
        if (pos >= TotalSize)
            return false;
        CurPos = pos;
        return true;
    }

    //==========InputStream=========//

    [[nodiscard]] virtual size_t AvaliableSpace() noexcept override
    {
        return TotalSize - CurPos;
    };
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) noexcept override
    {
        const size_t len = want * perSize;
        const auto avaliable = std::min(len, TotalSize - CurPos);
        memcpy_s(ptr, len, Ptr + CurPos, avaliable);
        CurPos += avaliable;
        return avaliable / perSize;
    }
    virtual bool Skip(const size_t len) noexcept override
    {
        return SetPos(CurPos + len);
    }
    [[nodiscard]] virtual bool IsEnd() noexcept override
    {
        return CurPos >= TotalSize;
    }

    [[nodiscard]] forceinline virtual std::byte ReadByteNE(bool& isSuccess) noexcept override
    {
        isSuccess = CurPos < TotalSize;
        if (isSuccess)
            return Ptr[CurPos++];
        else
            return std::byte(0xff);
    }
    [[nodiscard]] forceinline virtual std::byte ReadByteME() override
    {
        if (CurPos < TotalSize)
            return Ptr[CurPos++];
        else
            COMMON_THROW(BaseException, u"reach end of memoryinputstream");
    }
};


class MemoryOutputStream : public RandomOutputStream
{
private:
    std::byte* Ptr;
    size_t TotalSize, CurPos = 0;
    bool IsGrowable;
protected:
    [[nodiscard]] virtual common::span<std::byte> Grow(const size_t) { return common::span<std::byte>(Ptr, TotalSize); }
public:
    template<typename T>
    MemoryOutputStream(const span<T> srcSpan, const bool isGrowable = false) noexcept :
        Ptr(common::as_writable_bytes(srcSpan).data()), TotalSize(srcSpan.size_bytes()), IsGrowable(isGrowable) { }
    constexpr MemoryOutputStream(MemoryOutputStream&& stream) noexcept :
        Ptr(stream.Ptr), TotalSize(stream.TotalSize), CurPos(stream.CurPos), IsGrowable(stream.IsGrowable)
    {
        stream.Ptr = nullptr; stream.TotalSize = stream.CurPos = 0;
    }
    virtual ~MemoryOutputStream() override {}

    //==========RandomStream=========//

    [[nodiscard]] virtual size_t GetSize() noexcept override
    {
        return TotalSize;
    }
    [[nodiscard]] virtual size_t CurrentPos() const noexcept override
    {
        return CurPos;
    }
    virtual bool SetPos(const size_t pos) noexcept override
    {
        if (pos >= TotalSize)
            return false;
        CurPos = pos;
        return true;
    }

    //==========OutputStream=========//

    [[nodiscard]] virtual size_t AcceptableSpace() noexcept override
    {
        return IsGrowable ? SIZE_MAX : TotalSize - CurPos;
    };
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override
    {
        const size_t len = want * perSize;
        size_t acceptable = TotalSize - CurPos;
        if (acceptable < len)
        {
            const auto newspan = Grow(len - acceptable);
            Ptr = newspan.data(), TotalSize = newspan.size();
            acceptable = TotalSize - CurPos;
        }
        const auto avaliable = std::min(len, acceptable);
        memcpy_s(Ptr + CurPos, avaliable, ptr, avaliable);
        CurPos += avaliable;
        return avaliable / perSize;
    }
};


namespace detail
{

template<typename T>
class ContainerHolder
{
private:
    std::variant<T, T*> Container;
    using Helper = common::container::ContiguousHelper<std::remove_cv_t<T>>;
    static_assert(Helper::IsContiguous, "need a container that satisfies contiguous container concept");
protected:
    using EleType = std::conditional_t<std::is_const_v<T>, std::add_const_t<typename Helper::EleType>, typename Helper::EleType>;
    ContainerHolder(T& container) : Container(&container) {}
    constexpr ContainerHolder(T&& container) noexcept : Container(std::move(container)) {}
    constexpr ContainerHolder(ContainerHolder<T>&& other) noexcept : Container(std::move(other.container)) {}
    [[nodiscard]] constexpr T* GetContainer() noexcept
    {
        if (std::holds_alternative<T>(Container))
            return std::get_if<T>(&Container);
        else
            return std::get<T*>(Container);
    }
public:
    [[nodiscard]] constexpr static size_t GetElementSize() noexcept
    {
        return Helper::EleSize;
    }
    [[nodiscard]] constexpr auto GetSpan() noexcept
    {
        return common::span<EleType>(Helper::Data(*GetContainer()), Helper::Count(*GetContainer()));
    }
};
}


template<typename T>
class ContainerInputStream : private detail::ContainerHolder<const T>, public MemoryInputStream
{
public:
    ContainerInputStream(const T& container)  noexcept
        : detail::ContainerHolder<const T>(container),
        MemoryInputStream(this->GetSpan()) {}
    ContainerInputStream(T&& container) noexcept
        : detail::ContainerHolder<const T>(std::move(container)),
        MemoryInputStream(this->GetSpan()) {}
    ContainerInputStream(ContainerInputStream<T>&& other) noexcept
        : detail::ContainerHolder<const T>(std::move(other)),
        MemoryInputStream(std::move(other)) {}
    virtual ~ContainerInputStream() override {}
};


template<typename T>
class ContainerOutputStream : private detail::ContainerHolder<T>, public MemoryOutputStream
{
private:
    static constexpr bool IsConst = std::is_const_v<T> || std::is_base_of_v<common::AlignedBuffer, std::remove_cv_t<T>>;
protected:
    [[nodiscard]] virtual common::span<std::byte> Grow([[maybe_unused]] const size_t size) noexcept(IsConst) override
    {
        if constexpr (IsConst)
            return MemoryOutputStream::Grow(size);
        else
        {
            constexpr auto eleSize = detail::ContainerHolder<T>::GetElementSize();
            auto* container = this->GetContainer();
            const auto newSize = GetSize() + size + eleSize - 1;
            const auto newCount = newSize / eleSize;
            container->resize(newCount);
            return common::as_writable_bytes(this->GetSpan());
        }
    }
public:
    ContainerOutputStream(T& container) noexcept
        : detail::ContainerHolder<T>(container),
        MemoryOutputStream(this->GetSpan(), !IsConst) {}
    ContainerOutputStream(ContainerInputStream<T>&& other) noexcept
        : detail::ContainerHolder<T>(std::move(other)),
        MemoryOutputStream(std::move(other)) {}
    virtual ~ContainerOutputStream() override {}
};


}

}

