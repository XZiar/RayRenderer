#pragma once


#include "Stream.hpp"
#include "AlignedContainer.hpp"
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
    MemoryInputStream(const T* ptr, const size_t count) noexcept :
        Ptr(reinterpret_cast<const std::byte*>(ptr)), TotalSize(count * sizeof(T)) {}
    MemoryInputStream(MemoryInputStream&& stream) noexcept :
        Ptr(stream.Ptr), TotalSize(stream.TotalSize), CurPos(stream.CurPos)
    {
        stream.Ptr = nullptr; stream.TotalSize = stream.CurPos = 0;
    }
    virtual ~MemoryInputStream() override {}

    std::pair<const std::byte*, size_t> ExposeAvaliable() const
    {
        return { Ptr + CurPos, TotalSize - CurPos };
    }

    //==========RandomStream=========//

    virtual size_t GetSize() override
    {
        return TotalSize;
    }
    virtual size_t CurrentPos() const override
    {
        return CurPos;
    }
    virtual bool SetPos(const size_t pos) override
    {
        if (pos >= TotalSize)
            return false;
        CurPos = pos;
        return true;
    }

    //==========InputStream=========//

    virtual size_t AvaliableSpace() override
    {
        return TotalSize - CurPos;
    };
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override
    {
        const size_t len = want * perSize;
        const auto avaliable = std::min(len, TotalSize - CurPos);
        memcpy_s(ptr, len, Ptr + CurPos, avaliable);
        CurPos += avaliable;
        return avaliable / perSize;
    }
    virtual bool Skip(const size_t len) override
    {
        return SetPos(CurPos + len);
    }
    virtual bool IsEnd() override
    {
        return CurPos >= TotalSize;
    }

    forceinline virtual std::byte ReadByteNE(bool& isSuccess) override
    {
        isSuccess = CurPos < TotalSize;
        if (isSuccess)
            return Ptr[CurPos++];
        else
            return std::byte(0xff);
    }
    forceinline virtual std::byte ReadByteME() override
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
protected:
    virtual std::pair<std::byte*, size_t> Grow([[maybe_unused]] const size_t size) { return { Ptr, TotalSize }; }
public:
    template<typename T>
    MemoryOutputStream(T* ptr, const size_t count) noexcept :
        Ptr(reinterpret_cast<std::byte*>(ptr)), TotalSize(count * sizeof(T)) {}
    MemoryOutputStream(MemoryOutputStream&& stream) noexcept :
        Ptr(stream.Ptr), TotalSize(stream.TotalSize), CurPos(stream.CurPos)
    {
        stream.Ptr = nullptr; stream.TotalSize = stream.CurPos = 0;
    }
    virtual ~MemoryOutputStream() override {}

    //==========RandomStream=========//

    virtual size_t GetSize() override
    {
        return TotalSize;
    }
    virtual size_t CurrentPos() const override
    {
        return CurPos;
    }
    virtual bool SetPos(const size_t pos) override
    {
        if (pos >= TotalSize)
            return false;
        CurPos = pos;
        return true;
    }

    //==========OutputStream=========//

    virtual size_t AcceptableSpace() override
    {
        return TotalSize - CurPos;
    };
    virtual size_t WriteMany(const size_t want, const size_t perSize, const void* ptr) override
    {
        const size_t len = want * perSize;
        size_t acceptable = TotalSize - CurPos;
        if (acceptable < len)
        {
            std::tie(Ptr, TotalSize) = Grow(len - acceptable);
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
    ContainerHolder(T& container) : Container(&container) {}
    ContainerHolder(T&& container) : Container(std::move(container)) {}
    ContainerHolder(ContainerHolder<T>&& other) : Container(std::move(other.container)) {}
    T* GetContainer()
    {
        if (std::holds_alternative<T>(Container))
            return std::get_if<T>(&Container);
        else
            return std::get<T*>(Container);
    }
public:
    constexpr static size_t GetElementSize()
    {
        return Helper::EleSize;
    }
    auto* GetPtr()
    {
        return Helper::Data(*GetContainer());
    }
    size_t GetCount()
    {
        return Helper::Count(*GetContainer());
    }
};
}


template<typename T>
class ContainerInputStream : private detail::ContainerHolder<const T>, public MemoryInputStream
{
public:
    ContainerInputStream(const T& container) 
        : detail::ContainerHolder<const T>(container),
        MemoryInputStream(this->GetPtr(), this->GetCount()) {}
    ContainerInputStream(T&& container)
        : detail::ContainerHolder<const T>(std::move(container)),
        MemoryInputStream(this->GetPtr(), this->GetCount()) {}
    ContainerInputStream(ContainerInputStream<T>&& other)
        : detail::ContainerHolder<const T>(std::move(other)),
        MemoryInputStream(std::move(other)) {}
    virtual ~ContainerInputStream() override {}
};


template<typename T>
class ContainerOutputStream : private detail::ContainerHolder<T>, public MemoryOutputStream
{
protected:
    virtual std::pair<std::byte*, size_t> Grow([[maybe_unused]] const size_t size) override
    {
        if constexpr (std::is_base_of_v<common::AlignedBuffer, std::remove_cv_t<T>>)
            return MemoryOutputStream::Grow(size);
        else
        {
            constexpr auto eleSize = detail::ContainerHolder<T>::GetElementSize();
            auto* container = this->GetContainer();
            const auto newSize = GetSize() + size + eleSize - 1;
            const auto newCount = newSize / eleSize;
            container->resize(newCount);
            return { reinterpret_cast<std::byte*>(this->GetPtr()), newCount * eleSize };
        }
    }
public:
    ContainerOutputStream(T& container)
        : detail::ContainerHolder<T>(container),
        MemoryOutputStream(this->GetPtr(), this->GetCount()) {}
    ContainerOutputStream(ContainerInputStream<T>&& other)
        : detail::ContainerHolder<T>(std::move(other)),
        MemoryOutputStream(std::move(other)) {}
    virtual ~ContainerOutputStream() override {}
};


}

}

