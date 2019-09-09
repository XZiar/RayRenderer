#pragma once


#include "Stream.hpp"
#include "AlignedContainer.hpp"
#include <variant>

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
        return GetSize() - CurrentPos();
    };
    virtual size_t ReadMany(const size_t want, const size_t perSize, void* ptr) override
    {
        const size_t len = want * perSize;
        const auto avaliable = std::min(len, TotalSize - CurPos);
        memcpy_s(ptr, want, Ptr + CurPos, avaliable);
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
        isSuccess = CurPos >= TotalSize;
        if (isSuccess)
            return Ptr[CurPos++];
        else
            return std::byte(0xff);
    }
    forceinline virtual std::byte ReadByteME() override
    {
        if (CurPos >= TotalSize)
            return Ptr[CurPos++];
        else
            COMMON_THROW(BaseException, u"reach end of memoryinputstream");
    }
};


namespace detail
{

template<typename T>
class ContainerHolder
{
private:
    std::variant<T, const T*> Container;

    const T* GetContainer() const
    {
        if (std::holds_alternative<T>(Container))
            return std::get_if<T>(&Container);
        else
            return std::get<const T*>(Container);
    }
protected:
    ContainerHolder(const T& container) : Container(&container) {}
    ContainerHolder(T&& container) : Container(std::move(container)) {}
    ContainerHolder(ContainerHolder<T>&& other) : Container(std::move(other.container)) {}
public:
    const auto* GetPtr() const
    {
        if constexpr (std::is_base_of_v<common::AlignedBuffer, T>)
            return GetContainer()->GetRawPtr();
        else
            return GetContainer()->data();
    }
    size_t GetCount() const
    {
        if constexpr (std::is_base_of_v<common::AlignedBuffer, T>)
            return GetContainer()->GetSize();
        else
            return GetContainer()->size();
    }
};
}


template<typename T>
class ContainerInputStream : private detail::ContainerHolder<T>, public MemoryInputStream
{
public:
    ContainerInputStream(const T& container) 
        : detail::ContainerHolder<T>(container),
        MemoryInputStream(this->GetPtr(), this->GetCount()) {}
    ContainerInputStream(T&& container)
        : detail::ContainerHolder<T>(std::move(container)),
        MemoryInputStream(this->GetPtr(), this->GetCount()) {}
    ContainerInputStream(ContainerInputStream<T>&& other)
        : detail::ContainerHolder<T>(std::move(other)),
        MemoryInputStream(std::move(other)) {}
    virtual ~ContainerInputStream() override {}
};


}

}

