#pragma once
#include "ResourcePackagerRely.h"

namespace xziar::respak
{

namespace detail
{
class RESPAKAPI Serializable
{
public:
    virtual ~Serializable() {}
    virtual rapidjson::Value Serialize() const = 0;
};
}

class RESPAKAPI SerializeUtil
{
public:
    SerializeUtil();
    ~SerializeUtil();
    void AddObject(const string& name, const detail::Serializable& object);
    string TestSHA2(const void* data, const size_t size);
};

}