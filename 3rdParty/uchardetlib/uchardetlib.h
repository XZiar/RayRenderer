#pragma once

#ifndef UCHARDETLIB_H_
#define UCHARDETLIB_H_

#include "common/CommonRely.hpp"
#include "common/StrCharset.hpp"
#include <vector>
#include <string>
#include <memory>

namespace uchdet
{

class CharsetDetector : public common::NonCopyable
{
private:
    intptr_t Pointer;
public:
    CharsetDetector();
    ~CharsetDetector();
    CharsetDetector(CharsetDetector&& other) noexcept : Pointer(other.Pointer) 
    {
        other.Pointer = reinterpret_cast<intptr_t>(nullptr);
    }
    CharsetDetector& operator=(CharsetDetector&& other) noexcept
    {
        std::swap(Pointer, other.Pointer);
        return *this;
    }
    bool HandleData(const void *data, const size_t len) const;
    void Reset() const;
    void EndData() const;
    std::string GetEncoding() const;
    common::str::Charset DetectEncoding() const
    {
        return common::str::toCharset(GetEncoding());
    }
};

std::string getEncoding(const void *data, const size_t len);

template<typename T>
common::str::Charset detectEncoding(const std::basic_string<T>& str)
{
    return common::str::toCharset(getEncoding(str.c_str(), str.length() * sizeof(T)));
}
template<typename T, typename A>
common::str::Charset detectEncoding(const std::vector<T, A>& str)
{
    return common::str::toCharset(getEncoding(str.data(), str.size() * sizeof(T)));
}


}

#endif